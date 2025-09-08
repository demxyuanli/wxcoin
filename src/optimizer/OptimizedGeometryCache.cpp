#include "optimizer/OptimizedGeometryCache.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <OpenCASCADE/BRepPrimAPI_MakeBox.hxx>
#include <OpenCASCADE/BRepPrimAPI_MakeSphere.hxx>
#include <OpenCASCADE/BRepPrimAPI_MakeCylinder.hxx>
#include <OpenCASCADE/BRepPrimAPI_MakeCone.hxx>
#include <OpenCASCADE/BRepPrimAPI_MakeTorus.hxx>
#include <OpenCASCADE/gp_Trsf.hxx>
#include <OpenCASCADE/BRepBuilderAPI_Transform.hxx>
#include <OpenCASCADE/BRepAlgoAPI_Fuse.hxx>
#include <OpenCASCADE/BRepAlgoAPI_Common.hxx>
#include <OpenCASCADE/BRepAlgoAPI_Cut.hxx>

// GeometryThreadPool implementation
GeometryThreadPool::GeometryThreadPool(size_t threadCount) : m_stop(false) {
    for (size_t i = 0; i < threadCount; ++i) {
        m_workers.emplace_back(&GeometryThreadPool::workerFunction, this);
    }
}

GeometryThreadPool::~GeometryThreadPool() {
    shutdown();
}

void GeometryThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_condition.notify_all();
    
    for (auto& worker : m_workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void GeometryThreadPool::workerFunction() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [this] { return m_stop || !m_tasks.empty(); });
            
            if (m_stop && m_tasks.empty()) {
                return;
            }
            
            task = std::move(m_tasks.front());
            m_tasks.pop();
        }
        
        task();
    }
}

// OptimizedGeometryCache implementation
OptimizedGeometryCache::OptimizedGeometryCache(size_t maxCacheSize) 
    : m_maxCacheSize(maxCacheSize), m_threadPool(std::thread::hardware_concurrency()) {
}


std::shared_ptr<CachedGeometry> OptimizedGeometryCache::getGeometry(const GeometryKey& key) const {
    std::shared_lock<std::shared_mutex> lock(m_cacheMutex);
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        it->second->accessCount++;
        it->second->lastAccessTime = static_cast<uint32_t>(std::time(nullptr));
        updateStats(true);
        return it->second;
    }
    updateStats(false);
    return nullptr;
}

void OptimizedGeometryCache::removeGeometry(const GeometryKey& key) {
    std::unique_lock<std::shared_mutex> lock(m_cacheMutex);
    m_cache.erase(key);
}

void OptimizedGeometryCache::clearCache() {
    std::unique_lock<std::shared_mutex> lock(m_cacheMutex);
    m_cache.clear();
}

void OptimizedGeometryCache::cleanupOldEntries(std::chrono::seconds maxAge) {
    auto now = std::chrono::steady_clock::now();
    std::unique_lock<std::shared_mutex> lock(m_cacheMutex);
    
    auto it = m_cache.begin();
    while (it != m_cache.end()) {
        if (now - it->second->timestamp > maxAge) {
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
}

std::string OptimizedGeometryCache::getCacheStats() const {
    std::ostringstream oss;
    oss << "Geometry Cache Statistics:\n";
    oss << "  Cache size: " << m_cache.size() << "/" << m_maxCacheSize << "\n";
    oss << "  Cache hits: " << m_cacheHits.load() << "\n";
    oss << "  Cache misses: " << m_cacheMisses.load() << "\n";
    oss << "  Total computations: " << m_computations.load() << "\n";
    
    if (m_cacheHits.load() + m_cacheMisses.load() > 0) {
        double hitRate = static_cast<double>(m_cacheHits.load()) / 
                        (m_cacheHits.load() + m_cacheMisses.load()) * 100.0;
        oss << "  Hit rate: " << std::fixed << std::setprecision(2) << hitRate << "%\n";
    }
    
    // Memory usage estimation
    size_t totalMemory = 0;
    for (const auto& [key, geometry] : m_cache) {
        totalMemory += geometry->mesh.vertices.size() * sizeof(gp_Pnt);
        totalMemory += geometry->mesh.triangles.size() * sizeof(int);
    }
    
    oss << "  Estimated memory usage: " << (totalMemory / 1024 / 1024) << " MB\n";
    
    return oss.str();
}

void OptimizedGeometryCache::precomputeGeometries(const std::vector<GeometryKey>& commonKeys) {
    std::vector<std::future<void>> futures;
    
    for (const auto& key : commonKeys) {
        auto future = m_threadPool.enqueue([this, key]() {
            // Precompute common geometries in background
            if (key.type == "box") {
                double width = key.params[0];
                double height = key.params[1];
                double depth = key.params[2];
                
                BRepPrimAPI_MakeBox boxMaker(width, height, depth);
                TopoDS_Shape shape = boxMaker.Shape();
                
                // Convert to mesh
                OCCMeshConverter converter;
                auto mesh = converter.convertToMesh(shape, OCCMeshConverter::MeshParameters{});
                
                // Store in cache
                std::unique_lock<std::shared_mutex> lock(m_cacheMutex);
                auto cachedGeometry = std::make_shared<CachedGeometry>(shape, mesh);
                m_cache[key] = cachedGeometry;
            }
        });
        
        futures.push_back(std::move(future));
    }
    
    // Wait for all precomputations to complete
    for (auto& future : futures) {
        future.wait();
    }
}

void OptimizedGeometryCache::evictLeastUsed() {
    if (m_cache.size() <= m_maxCacheSize) {
        return;
    }
    
    // Find least used entry
    auto leastUsed = m_cache.begin();
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it->second->accessCount < leastUsed->second->accessCount) {
            leastUsed = it;
        }
    }
    
    // Remove least used entry
    m_cache.erase(leastUsed);
}

void OptimizedGeometryCache::updateStats(bool hit) const {
    if (hit) {
        m_cacheHits++;
    } else {
        m_cacheMisses++;
    }
}

// OptimizedShapeBuilder implementation
OptimizedShapeBuilder::OptimizedShapeBuilder() 
    : m_cache(std::make_unique<OptimizedGeometryCache>(1000)) {
}


TopoDS_Shape OptimizedShapeBuilder::createBox(double width, double height, double depth, const gp_Pnt& position) {
    GeometryKey key = createGeometryKey("box", {width, height, depth});
    
    auto cachedGeometry = m_cache->getGeometry(key);
    if (cachedGeometry) {
        return applyTransform(cachedGeometry->shape, position);
    }
    
    // Create new geometry
    BRepPrimAPI_MakeBox boxMaker(width, height, depth);
    TopoDS_Shape shape = boxMaker.Shape();
    
    // Convert to mesh for caching
    OCCMeshConverter converter;
    auto mesh = converter.convertToMesh(shape, OCCMeshConverter::MeshParameters{});
    
    // Cache the geometry
    auto geometry = std::make_pair(shape, mesh);
    m_cache->getOrCreateGeometry(key, OCCMeshConverter::MeshParameters{}, [geometry]() { return geometry; });
    
    return applyTransform(shape, position);
}

TopoDS_Shape OptimizedShapeBuilder::createSphere(double radius, const gp_Pnt& center) {
    GeometryKey key = createGeometryKey("sphere", {radius});
    
    auto cachedGeometry = m_cache->getGeometry(key);
    if (cachedGeometry) {
        return applyTransform(cachedGeometry->shape, center);
    }
    
    BRepPrimAPI_MakeSphere sphereMaker(center, radius);
    TopoDS_Shape shape = sphereMaker.Shape();
    
    OCCMeshConverter converter;
    auto mesh = converter.convertToMesh(shape, OCCMeshConverter::MeshParameters{});
    
    auto geometry = std::make_pair(shape, mesh);
    m_cache->getOrCreateGeometry(key, OCCMeshConverter::MeshParameters{}, [geometry]() { return geometry; });
    
    return shape;
}

TopoDS_Shape OptimizedShapeBuilder::createCylinder(double radius, double height, const gp_Pnt& position) {
    GeometryKey key = createGeometryKey("cylinder", {radius, height});
    
    auto cachedGeometry = m_cache->getGeometry(key);
    if (cachedGeometry) {
        return applyTransform(cachedGeometry->shape, position);
    }
    
    gp_Ax2 axis(position, gp_Dir(0, 0, 1));
    BRepPrimAPI_MakeCylinder cylinderMaker(axis, radius, height);
    TopoDS_Shape shape = cylinderMaker.Shape();
    
    OCCMeshConverter converter;
    auto mesh = converter.convertToMesh(shape, OCCMeshConverter::MeshParameters{});
    
    auto geometry = std::make_pair(shape, mesh);
    m_cache->getOrCreateGeometry(key, OCCMeshConverter::MeshParameters{}, [geometry]() { return geometry; });
    
    return shape;
}

TopoDS_Shape OptimizedShapeBuilder::createCone(double bottomRadius, double topRadius, double height, const gp_Pnt& position) {
    GeometryKey key = createGeometryKey("cone", {bottomRadius, topRadius, height});
    
    auto cachedGeometry = m_cache->getGeometry(key);
    if (cachedGeometry) {
        return applyTransform(cachedGeometry->shape, position);
    }
    
    gp_Ax2 axis(position, gp_Dir(0, 0, 1));
    BRepPrimAPI_MakeCone coneMaker(axis, bottomRadius, topRadius, height);
    TopoDS_Shape shape = coneMaker.Shape();
    
    OCCMeshConverter converter;
    auto mesh = converter.convertToMesh(shape, OCCMeshConverter::MeshParameters{});
    
    auto geometry = std::make_pair(shape, mesh);
    m_cache->getOrCreateGeometry(key, OCCMeshConverter::MeshParameters{}, [geometry]() { return geometry; });
    
    return shape;
}

TopoDS_Shape OptimizedShapeBuilder::createTorus(double majorRadius, double minorRadius, const gp_Pnt& center) {
    GeometryKey key = createGeometryKey("torus", {majorRadius, minorRadius});
    
    auto cachedGeometry = m_cache->getGeometry(key);
    if (cachedGeometry) {
        return applyTransform(cachedGeometry->shape, center);
    }
    
    gp_Ax2 axis(center, gp_Dir(0, 0, 1));
    BRepPrimAPI_MakeTorus torusMaker(axis, majorRadius, minorRadius);
    TopoDS_Shape shape = torusMaker.Shape();
    
    OCCMeshConverter converter;
    auto mesh = converter.convertToMesh(shape, OCCMeshConverter::MeshParameters{});
    
    auto geometry = std::make_pair(shape, mesh);
    m_cache->getOrCreateGeometry(key, OCCMeshConverter::MeshParameters{}, [geometry]() { return geometry; });
    
    return shape;
}

TopoDS_Shape OptimizedShapeBuilder::createExtrusion(const TopoDS_Shape& profile, const gp_Vec& direction) {
    // Extrusion is typically unique, so we don't cache it
    // Implementation would use BRepPrimAPI_MakePrism
    return profile; // Placeholder
}

TopoDS_Shape OptimizedShapeBuilder::createRevolution(const TopoDS_Shape& profile, 
                                                    const gp_Pnt& axisPosition,
                                                    const gp_Dir& axisDirection,
                                                    double angle) {
    // Revolution is typically unique, so we don't cache it
    // Implementation would use BRepPrimAPI_MakeRevol
    return profile; // Placeholder
}

TopoDS_Shape OptimizedShapeBuilder::booleanUnion(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2) {
    BRepAlgoAPI_Fuse fuseMaker(shape1, shape2);
    return fuseMaker.Shape();
}

TopoDS_Shape OptimizedShapeBuilder::booleanIntersection(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2) {
    BRepAlgoAPI_Common commonMaker(shape1, shape2);
    return commonMaker.Shape();
}

TopoDS_Shape OptimizedShapeBuilder::booleanDifference(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2) {
    BRepAlgoAPI_Cut cutMaker(shape1, shape2);
    return cutMaker.Shape();
}

std::vector<TopoDS_Shape> OptimizedShapeBuilder::createMultipleBoxes(
    const std::vector<std::tuple<double, double, double, gp_Pnt>>& params) {
    
    std::vector<TopoDS_Shape> shapes;
    shapes.reserve(params.size());
    
    std::vector<std::future<TopoDS_Shape>> futures;
    
    for (const auto& [width, height, depth, position] : params) {
        auto future = m_cache->m_threadPool.enqueue([this, width, height, depth, position]() {
            return createBox(width, height, depth, position);
        });
        futures.push_back(std::move(future));
    }
    
    for (auto& future : futures) {
        shapes.push_back(future.get());
    }
    
    return shapes;
}

std::vector<TopoDS_Shape> OptimizedShapeBuilder::createMultipleSpheres(
    const std::vector<std::pair<double, gp_Pnt>>& params) {
    
    std::vector<TopoDS_Shape> shapes;
    shapes.reserve(params.size());
    
    std::vector<std::future<TopoDS_Shape>> futures;
    
    for (const auto& [radius, center] : params) {
        auto future = m_cache->m_threadPool.enqueue([this, radius, center]() {
            return createSphere(radius, center);
        });
        futures.push_back(std::move(future));
    }
    
    for (auto& future : futures) {
        shapes.push_back(future.get());
    }
    
    return shapes;
}

void OptimizedShapeBuilder::clearCache() {
    m_cache->clearCache();
}

std::string OptimizedShapeBuilder::getPerformanceStats() const {
    return m_cache->getCacheStats();
}

GeometryKey OptimizedShapeBuilder::createGeometryKey(const std::string& type, const std::vector<double>& params) {
    return GeometryKey(type, params);
}

TopoDS_Shape OptimizedShapeBuilder::applyTransform(const TopoDS_Shape& shape, const gp_Pnt& position) {
    if (position.X() == 0.0 && position.Y() == 0.0 && position.Z() == 0.0) {
        return shape;
    }
    
    gp_Trsf transform;
    transform.SetTranslation(gp_Vec(position.X(), position.Y(), position.Z()));
    
    BRepBuilderAPI_Transform transformMaker(shape, transform);
    return transformMaker.Shape();
} 