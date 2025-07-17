#include "OptimizedGeometryManager.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <regex>

OptimizedGeometryManager::OptimizedGeometryManager() 
    : m_wireframeMode(false), m_shadingMode(true), m_showEdges(false), 
      m_showNormals(false), m_antiAliasing(true), m_lodEnabled(true),
      m_lodRoughMode(false), m_lodRoughDeflection(0.1), m_lodFineDeflection(0.01),
      m_lodTransitionTime(300), m_geometryCache(std::make_unique<OptimizedGeometryCache>(500)) {
    
    // Initialize mesh parameters
    m_meshParams.deflection = 0.01;
    m_meshParams.angularDeflection = 0.1;
    m_meshParams.relativeDeflection = true;
}

OptimizedGeometryManager::~OptimizedGeometryManager() = default;

void OptimizedGeometryManager::addGeometry(std::shared_ptr<OCCGeometry> geometry) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    if (!geometry || geometry->getName().empty()) {
        return;
    }
    
    // Add to map for O(1) lookup
    m_geometryMap[geometry->getName()] = geometry;
    
    // Add to list for index-based access
    m_geometryList.push_back(geometry);
    
    // Apply current settings to new geometry
    applyDisplayModeToGeometry(geometry);
    applyMeshSettingsToGeometry(geometry);
    applyLODSettingsToGeometry(geometry);
}

void OptimizedGeometryManager::removeGeometry(const std::string& name) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    auto it = m_geometryMap.find(name);
    if (it != m_geometryMap.end()) {
        auto geometry = it->second;
        m_geometryMap.erase(it);
        
        // Remove from list
        auto listIt = std::find(m_geometryList.begin(), m_geometryList.end(), geometry);
        if (listIt != m_geometryList.end()) {
            m_geometryList.erase(listIt);
        }
        
        // Remove from selection
        m_selectedGeometries.erase(name);
    }
}

void OptimizedGeometryManager::removeGeometry(std::shared_ptr<OCCGeometry> geometry) {
    if (geometry) {
        removeGeometry(geometry->getName());
    }
}

void OptimizedGeometryManager::clearAll() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_geometryMap.clear();
    m_geometryList.clear();
    m_selectedGeometries.clear();
}

std::shared_ptr<OCCGeometry> OptimizedGeometryManager::findGeometry(const std::string& name) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    m_lookupCount++;
    auto it = m_geometryMap.find(name);
    if (it != m_geometryMap.end()) {
        updateStats(true);
        return it->second;
    }
    
    updateStats(false);
    return nullptr;
}

std::shared_ptr<OCCGeometry> OptimizedGeometryManager::findGeometryByIndex(size_t index) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    if (index < m_geometryList.size()) {
        return m_geometryList[index];
    }
    return nullptr;
}

void OptimizedGeometryManager::addGeometries(const std::vector<std::shared_ptr<OCCGeometry>>& geometries) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    for (const auto& geometry : geometries) {
        if (geometry && !geometry->getName().empty()) {
            m_geometryMap[geometry->getName()] = geometry;
            m_geometryList.push_back(geometry);
            
            // Apply current settings
            applyDisplayModeToGeometry(geometry);
            applyMeshSettingsToGeometry(geometry);
            applyLODSettingsToGeometry(geometry);
        }
    }
}

void OptimizedGeometryManager::removeGeometries(const std::vector<std::string>& names) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    for (const auto& name : names) {
        auto it = m_geometryMap.find(name);
        if (it != m_geometryMap.end()) {
            auto geometry = it->second;
            m_geometryMap.erase(it);
            
            // Remove from list
            auto listIt = std::find(m_geometryList.begin(), m_geometryList.end(), geometry);
            if (listIt != m_geometryList.end()) {
                m_geometryList.erase(listIt);
            }
            
            // Remove from selection
            m_selectedGeometries.erase(name);
        }
    }
}

std::vector<std::shared_ptr<OCCGeometry>> OptimizedGeometryManager::findGeometries(
    const std::vector<std::string>& names) const {
    
    std::vector<std::shared_ptr<OCCGeometry>> result;
    result.reserve(names.size());
    
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    for (const auto& name : names) {
        auto it = m_geometryMap.find(name);
        if (it != m_geometryMap.end()) {
            result.push_back(it->second);
        }
    }
    
    return result;
}

void OptimizedGeometryManager::selectGeometry(const std::string& name) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    auto it = m_geometryMap.find(name);
    if (it != m_geometryMap.end()) {
        m_selectedGeometries[name] = it->second;
    }
}

void OptimizedGeometryManager::deselectGeometry(const std::string& name) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_selectedGeometries.erase(name);
}

void OptimizedGeometryManager::selectAll() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_selectedGeometries = m_geometryMap;
}

void OptimizedGeometryManager::deselectAll() {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_selectedGeometries.clear();
}

std::vector<std::shared_ptr<OCCGeometry>> OptimizedGeometryManager::getSelectedGeometries() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    std::vector<std::shared_ptr<OCCGeometry>> result;
    result.reserve(m_selectedGeometries.size());
    
    for (const auto& [name, geometry] : m_selectedGeometries) {
        result.push_back(geometry);
    }
    
    return result;
}

void OptimizedGeometryManager::setGeometryVisible(const std::string& name, bool visible) {
    auto geometry = findGeometry(name);
    if (geometry) {
        geometry->setVisible(visible);
    }
}

void OptimizedGeometryManager::setAllVisible(bool visible) {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    for (auto& [name, geometry] : m_geometryMap) {
        geometry->setVisible(visible);
    }
}

void OptimizedGeometryManager::hideAll() {
    setAllVisible(false);
}

void OptimizedGeometryManager::showAll() {
    setAllVisible(true);
}

void OptimizedGeometryManager::setGeometryColor(const std::string& name, const Quantity_Color& color) {
    auto geometry = findGeometry(name);
    if (geometry) {
        geometry->setColor(color);
    }
}

void OptimizedGeometryManager::setGeometryTransparency(const std::string& name, double transparency) {
    auto geometry = findGeometry(name);
    if (geometry) {
        geometry->setTransparency(transparency);
    }
}

void OptimizedGeometryManager::setAllColor(const Quantity_Color& color) {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    for (auto& [name, geometry] : m_geometryMap) {
        geometry->setColor(color);
    }
}

void OptimizedGeometryManager::fitAll() {
    // Implementation would fit all geometries in view
}

void OptimizedGeometryManager::fitGeometry(const std::string& name) {
    auto geometry = findGeometry(name);
    if (geometry) {
        // Implementation would fit specific geometry in view
    }
}

void OptimizedGeometryManager::setWireframeMode(bool wireframe) {
    m_wireframeMode = wireframe;
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    for (auto& [name, geometry] : m_geometryMap) {
        applyDisplayModeToGeometry(geometry);
    }
}

void OptimizedGeometryManager::setShadingMode(bool shaded) {
    m_shadingMode = shaded;
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    for (auto& [name, geometry] : m_geometryMap) {
        applyDisplayModeToGeometry(geometry);
    }
}

void OptimizedGeometryManager::setShowEdges(bool showEdges) {
    m_showEdges = showEdges;
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    for (auto& [name, geometry] : m_geometryMap) {
        applyDisplayModeToGeometry(geometry);
    }
}

void OptimizedGeometryManager::setShowNormals(bool showNormals) {
    m_showNormals = showNormals;
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    for (auto& [name, geometry] : m_geometryMap) {
        applyDisplayModeToGeometry(geometry);
    }
}

void OptimizedGeometryManager::setMeshDeflection(double deflection, bool remesh) {
    m_meshParams.deflection = deflection;
    
    if (remesh) {
        remeshAllGeometries();
    }
}

double OptimizedGeometryManager::getMeshDeflection() const {
    return m_meshParams.deflection;
}

void OptimizedGeometryManager::remeshAllGeometries() {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    for (auto& [name, geometry] : m_geometryMap) {
        applyMeshSettingsToGeometry(geometry);
    }
}

void OptimizedGeometryManager::setLODEnabled(bool enabled) {
    m_lodEnabled = enabled;
}

bool OptimizedGeometryManager::isLODEnabled() const {
    return m_lodEnabled;
}

void OptimizedGeometryManager::setLODRoughDeflection(double deflection) {
    m_lodRoughDeflection = deflection;
}

double OptimizedGeometryManager::getLODRoughDeflection() const {
    return m_lodRoughDeflection;
}

void OptimizedGeometryManager::setLODFineDeflection(double deflection) {
    m_lodFineDeflection = deflection;
}

double OptimizedGeometryManager::getLODFineDeflection() const {
    return m_lodFineDeflection;
}

void OptimizedGeometryManager::setLODTransitionTime(int milliseconds) {
    m_lodTransitionTime = milliseconds;
}

int OptimizedGeometryManager::getLODTransitionTime() const {
    return m_lodTransitionTime;
}

void OptimizedGeometryManager::setLODMode(bool roughMode) {
    m_lodRoughMode = roughMode;
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    for (auto& [name, geometry] : m_geometryMap) {
        applyLODSettingsToGeometry(geometry);
    }
}

bool OptimizedGeometryManager::isLODRoughMode() const {
    return m_lodRoughMode;
}

void OptimizedGeometryManager::startLODInteraction() {
    // Implementation would start LOD interaction mode
}

std::shared_ptr<OCCGeometry> OptimizedGeometryManager::pickGeometry(int x, int y) {
    // Implementation would pick geometry at screen coordinates
    return nullptr;
}

std::vector<std::shared_ptr<OCCGeometry>> OptimizedGeometryManager::pickGeometries(
    const std::vector<std::pair<int, int>>& points) {
    
    std::vector<std::shared_ptr<OCCGeometry>> result;
    result.reserve(points.size());
    
    for (const auto& [x, y] : points) {
        auto geometry = pickGeometry(x, y);
        if (geometry) {
            result.push_back(geometry);
        }
    }
    
    return result;
}

size_t OptimizedGeometryManager::getGeometryCount() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_geometryMap.size();
}

std::vector<std::string> OptimizedGeometryManager::getAllGeometryNames() const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    std::vector<std::string> names;
    names.reserve(m_geometryMap.size());
    
    for (const auto& [name, geometry] : m_geometryMap) {
        names.push_back(name);
    }
    
    return names;
}

std::string OptimizedGeometryManager::getPerformanceStats() const {
    std::ostringstream oss;
    oss << "Geometry Manager Performance Stats:\n";
    oss << "  Total geometries: " << getGeometryCount() << "\n";
    oss << "  Selected geometries: " << m_selectedGeometries.size() << "\n";
    oss << "  Lookup count: " << m_lookupCount.load() << "\n";
    oss << "  Cache hits: " << m_cacheHits.load() << "\n";
    oss << "  Cache misses: " << m_cacheMisses.load() << "\n";
    
    if (m_lookupCount.load() > 0) {
        double hitRate = static_cast<double>(m_cacheHits.load()) / m_lookupCount.load() * 100.0;
        oss << "  Cache hit rate: " << std::fixed << std::setprecision(2) << hitRate << "%\n";
    }
    
    return oss.str();
}

void OptimizedGeometryManager::clearCache() {
    m_geometryCache->clearCache();
}

void OptimizedGeometryManager::cleanupCache() {
    m_geometryCache->cleanupOldEntries();
}

std::string OptimizedGeometryManager::getCacheStats() const {
    return m_geometryCache->getCacheStats();
}

void OptimizedGeometryManager::lockForWrite() {
    m_mutex.lock();
}

void OptimizedGeometryManager::unlockForWrite() {
    m_mutex.unlock();
}

void OptimizedGeometryManager::lockForRead() const {
    m_mutex.lock_shared();
}

void OptimizedGeometryManager::unlockForRead() const {
    m_mutex.unlock_shared();
}

void OptimizedGeometryManager::updateGeometryList() {
    // This method would update the list when map changes
    // Currently handled inline in add/remove methods
}

void OptimizedGeometryManager::updateSelectionMap() {
    // This method would update selection map
    // Currently handled inline in select/deselect methods
}

void OptimizedGeometryManager::updateStats(bool cacheHit) const {
    if (cacheHit) {
        m_cacheHits++;
    } else {
        m_cacheMisses++;
    }
}

bool OptimizedGeometryManager::isValidGeometry(const std::shared_ptr<OCCGeometry>& geometry) const {
    return geometry && !geometry->getName().empty();
}

void OptimizedGeometryManager::applyDisplayModeToGeometry(std::shared_ptr<OCCGeometry> geometry) {
    if (!geometry) return;
    
    // Apply wireframe mode
    if (m_wireframeMode) {
        geometry->setDisplayMode(DisplayMode::Wireframe);
    } else if (m_shadingMode) {
        geometry->setDisplayMode(DisplayMode::Shaded);
    }
    
    // Apply edge display
    geometry->setShowEdges(m_showEdges);
    
    // Apply normal display
    geometry->setShowNormals(m_showNormals);
}

void OptimizedGeometryManager::applyMeshSettingsToGeometry(std::shared_ptr<OCCGeometry> geometry) {
    if (!geometry) return;
    
    geometry->setMeshDeflection(m_meshParams.deflection);
    geometry->setAngularDeflection(m_meshParams.angularDeflection);
    geometry->setRelativeDeflection(m_meshParams.relativeDeflection);
}

void OptimizedGeometryManager::applyLODSettingsToGeometry(std::shared_ptr<OCCGeometry> geometry) {
    if (!geometry || !m_lodEnabled) return;
    
    if (m_lodRoughMode) {
        geometry->setMeshDeflection(m_lodRoughDeflection);
    } else {
        geometry->setMeshDeflection(m_lodFineDeflection);
    }
}

// GeometryIterator implementation
GeometryIterator::GeometryIterator(const OptimizedGeometryManager& manager) 
    : m_manager(manager), m_currentIndex(0) {
    
    std::shared_lock<std::shared_mutex> lock(m_iteratorMutex);
    m_geometries = m_manager.getAllGeometryNames();
}

bool GeometryIterator::hasNext() const {
    std::shared_lock<std::shared_mutex> lock(m_iteratorMutex);
    return m_currentIndex < m_geometries.size();
}

std::shared_ptr<OCCGeometry> GeometryIterator::next() {
    std::unique_lock<std::shared_mutex> lock(m_iteratorMutex);
    
    if (m_currentIndex < m_geometries.size()) {
        auto geometry = m_manager.findGeometry(m_geometries[m_currentIndex]);
        m_currentIndex++;
        return geometry;
    }
    
    return nullptr;
}

void GeometryIterator::reset() {
    std::unique_lock<std::shared_mutex> lock(m_iteratorMutex);
    m_currentIndex = 0;
}

std::vector<std::shared_ptr<OCCGeometry>> GeometryIterator::getNextBatch(size_t batchSize) {
    std::unique_lock<std::shared_mutex> lock(m_iteratorMutex);
    
    std::vector<std::shared_ptr<OCCGeometry>> batch;
    size_t endIndex = std::min(m_currentIndex + batchSize, m_geometries.size());
    
    for (size_t i = m_currentIndex; i < endIndex; ++i) {
        auto geometry = m_manager.findGeometry(m_geometries[i]);
        if (geometry) {
            batch.push_back(geometry);
        }
    }
    
    m_currentIndex = endIndex;
    return batch;
}

// GeometrySearchEngine implementation
GeometrySearchEngine::GeometrySearchEngine(const OptimizedGeometryManager& manager) 
    : m_manager(manager) {
    buildIndices();
}

std::shared_ptr<OCCGeometry> GeometrySearchEngine::findExact(const std::string& name) const {
    return m_manager.findGeometry(name);
}

std::vector<std::shared_ptr<OCCGeometry>> GeometrySearchEngine::findPattern(const std::string& pattern) const {
    std::vector<std::shared_ptr<OCCGeometry>> result;
    
    auto names = m_manager.getAllGeometryNames();
    for (const auto& name : names) {
        if (matchesPattern(name, pattern)) {
            auto geometry = m_manager.findGeometry(name);
            if (geometry) {
                result.push_back(geometry);
            }
        }
    }
    
    return result;
}

std::vector<std::shared_ptr<OCCGeometry>> GeometrySearchEngine::findByType(const std::string& type) const {
    auto it = m_typeIndex.find(type);
    if (it != m_typeIndex.end()) {
        return it->second;
    }
    return {};
}

std::vector<std::shared_ptr<OCCGeometry>> GeometrySearchEngine::findByPropertyRange(
    const std::string& property, double minValue, double maxValue) const {
    
    // Implementation would search by property range
    return {};
}

std::vector<std::shared_ptr<OCCGeometry>> GeometrySearchEngine::findByBoundingBox(
    const gp_Pnt& minPoint, const gp_Pnt& maxPoint) const {
    
    std::vector<std::shared_ptr<OCCGeometry>> result;
    
    auto names = m_manager.getAllGeometryNames();
    for (const auto& name : names) {
        auto geometry = m_manager.findGeometry(name);
        if (geometry && isInBoundingBox(geometry, minPoint, maxPoint)) {
            result.push_back(geometry);
        }
    }
    
    return result;
}

std::vector<std::shared_ptr<OCCGeometry>> GeometrySearchEngine::findByDistance(
    const gp_Pnt& center, double maxDistance) const {
    
    std::vector<std::shared_ptr<OCCGeometry>> result;
    
    auto names = m_manager.getAllGeometryNames();
    for (const auto& name : names) {
        auto geometry = m_manager.findGeometry(name);
        if (geometry) {
            // Implementation would calculate distance and check against maxDistance
            // For now, return empty result
        }
    }
    
    return result;
}

std::vector<std::shared_ptr<OCCGeometry>> GeometrySearchEngine::advancedSearch(
    const std::unordered_map<std::string, std::string>& criteria) const {
    
    // Implementation would perform advanced search with multiple criteria
    return {};
}

void GeometrySearchEngine::buildIndices() {
    // Build type index
    auto names = m_manager.getAllGeometryNames();
    for (const auto& name : names) {
        auto geometry = m_manager.findGeometry(name);
        if (geometry) {
            // Implementation would extract type and add to index
        }
    }
}

bool GeometrySearchEngine::matchesPattern(const std::string& text, const std::string& pattern) const {
    try {
        std::regex regex(pattern);
        return std::regex_search(text, regex);
    } catch (const std::regex_error&) {
        return false;
    }
}

double GeometrySearchEngine::calculateDistance(const gp_Pnt& p1, const gp_Pnt& p2) const {
    double dx = p2.X() - p1.X();
    double dy = p2.Y() - p1.Y();
    double dz = p2.Z() - p1.Z();
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

bool GeometrySearchEngine::isInBoundingBox(const std::shared_ptr<OCCGeometry>& geometry, 
                                          const gp_Pnt& minPoint, const gp_Pnt& maxPoint) const {
    // Implementation would check if geometry is within bounding box
    return true; // Placeholder
}

// GeometryBatchProcessor implementation
GeometryBatchProcessor::GeometryBatchProcessor(OptimizedGeometryManager& manager) 
    : m_manager(manager) {
}

void GeometryBatchProcessor::batchSetColor(const std::vector<std::string>& names, const Quantity_Color& color) {
    for (const auto& name : names) {
        m_manager.setGeometryColor(name, color);
        updateProgress(0, names.size()); // Placeholder
    }
}

void GeometryBatchProcessor::batchSetTransparency(const std::vector<std::string>& names, double transparency) {
    for (const auto& name : names) {
        m_manager.setGeometryTransparency(name, transparency);
        updateProgress(0, names.size()); // Placeholder
    }
}

void GeometryBatchProcessor::batchSetVisible(const std::vector<std::string>& names, bool visible) {
    for (const auto& name : names) {
        m_manager.setGeometryVisible(name, visible);
        updateProgress(0, names.size()); // Placeholder
    }
}

void GeometryBatchProcessor::batchSetSelected(const std::vector<std::string>& names, bool selected) {
    for (const auto& name : names) {
        if (selected) {
            m_manager.selectGeometry(name);
        } else {
            m_manager.deselectGeometry(name);
        }
        updateProgress(0, names.size()); // Placeholder
    }
}

void GeometryBatchProcessor::batchTranslate(const std::vector<std::string>& names, const gp_Vec& translation) {
    // Implementation would translate geometries
}

void GeometryBatchProcessor::batchRotate(const std::vector<std::string>& names, const gp_Pnt& center, 
                                        const gp_Dir& axis, double angle) {
    // Implementation would rotate geometries
}

void GeometryBatchProcessor::batchScale(const std::vector<std::string>& names, const gp_Pnt& center, double factor) {
    // Implementation would scale geometries
}

void GeometryBatchProcessor::batchRemesh(const std::vector<std::string>& names, 
                                        const OCCMeshConverter::MeshParameters& params) {
    // Implementation would remesh geometries
}

void GeometryBatchProcessor::batchUpdateLOD(const std::vector<std::string>& names, bool roughMode) {
    // Implementation would update LOD for geometries
}

void GeometryBatchProcessor::batchExportToSTL(const std::vector<std::string>& names, const std::string& directory) {
    // Implementation would export geometries to STL
}

void GeometryBatchProcessor::batchExportToOBJ(const std::vector<std::string>& names, const std::string& directory) {
    // Implementation would export geometries to OBJ
}

void GeometryBatchProcessor::setProgressCallback(std::function<void(size_t current, size_t total)> callback) {
    m_progressCallback = std::move(callback);
}

std::string GeometryBatchProcessor::getLastError() const {
    return m_lastError;
}

void GeometryBatchProcessor::updateProgress(size_t current, size_t total) {
    if (m_progressCallback) {
        m_progressCallback(current, total);
    }
}

void GeometryBatchProcessor::setError(const std::string& error) {
    m_lastError = error;
}

bool GeometryBatchProcessor::validateGeometryNames(const std::vector<std::string>& names) const {
    for (const auto& name : names) {
        if (!m_manager.findGeometry(name)) {
            return false;
        }
    }
    return true;
} 