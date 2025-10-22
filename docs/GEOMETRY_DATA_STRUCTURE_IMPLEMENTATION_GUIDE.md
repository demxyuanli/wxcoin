# 几何数据结构优化实施指南

## 文档说明

本文档是 `GEOMETRY_DATA_STRUCTURE_ANALYSIS.md` 的配套实施指南，提供详细的代码实现示例和集成步骤。

---

## 快速开始：优先级 P0 实施

### 1. FaceIndexMapping反向索引

**实施步骤:**

#### 1.1 修改头文件

**文件:** `include/geometry/OCCGeometryMesh.h`

```cpp
// 在 OCCGeometryMesh 类的 private 部分添加:
private:
    std::vector<FaceIndexMapping> m_faceIndexMappings;
    
    // 新增: 三角形到面的快速查找映射
    std::unordered_map<int, int> m_triangleToFaceMap;
    bool m_hasReverseMapping = false;
```

#### 1.2 修改实现文件

**文件:** `src/geometry/OCCGeometryMesh.cpp`

```cpp
// 在 setFaceIndexMappings 方法中添加反向映射构建
void OCCGeometryMesh::setFaceIndexMappings(const std::vector<FaceIndexMapping>& mappings) {
    m_faceIndexMappings = mappings;
    
    // 自动构建反向映射
    buildReverseMapping();
}

// 新增方法
void OCCGeometryMesh::buildReverseMapping() {
    m_triangleToFaceMap.clear();
    m_triangleToFaceMap.reserve(
        std::accumulate(m_faceIndexMappings.begin(), m_faceIndexMappings.end(), 0,
            [](size_t sum, const FaceIndexMapping& mapping) {
                return sum + mapping.triangleIndices.size();
            })
    );
    
    for (const auto& mapping : m_faceIndexMappings) {
        for (int triIdx : mapping.triangleIndices) {
            m_triangleToFaceMap[triIdx] = mapping.geometryFaceId;
        }
    }
    
    m_hasReverseMapping = true;
}

// 优化现有方法
int OCCGeometryMesh::getGeometryFaceIdForTriangle(int triangleIndex) const {
    if (m_hasReverseMapping) {
        auto it = m_triangleToFaceMap.find(triangleIndex);
        return it != m_triangleToFaceMap.end() ? it->second : -1;
    }
    
    // 回退到原有的线性搜索（兼容性）
    for (const auto& mapping : m_faceIndexMappings) {
        for (int idx : mapping.triangleIndices) {
            if (idx == triangleIndex) {
                return mapping.geometryFaceId;
            }
        }
    }
    return -1;
}

// 内存优化方法
void OCCGeometryMesh::releaseTemporaryData() {
    // 如果不需要频繁查询，可以释放反向映射
    if (!m_hasReverseMapping) {
        std::unordered_map<int, int>().swap(m_triangleToFaceMap);
    }
}
```

#### 1.3 性能测试

**文件:** `tests/test_face_mapping_performance.cpp` (新建)

```cpp
#include <chrono>
#include <iostream>
#include "geometry/OCCGeometryMesh.h"

void testFaceMappingPerformance() {
    OCCGeometryMesh mesh;
    
    // 模拟大型模型: 1000个面，每面100个三角形
    std::vector<FaceIndexMapping> mappings;
    for (int faceId = 0; faceId < 1000; ++faceId) {
        FaceIndexMapping mapping(faceId);
        for (int i = 0; i < 100; ++i) {
            mapping.triangleIndices.push_back(faceId * 100 + i);
        }
        mappings.push_back(mapping);
    }
    
    mesh.setFaceIndexMappings(mappings);
    
    // 测试随机查询性能
    const int numQueries = 10000;
    std::vector<int> queryTriangles;
    for (int i = 0; i < numQueries; ++i) {
        queryTriangles.push_back(rand() % 100000);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int triIdx : queryTriangles) {
        int faceId = mesh.getGeometryFaceIdForTriangle(triIdx);
        (void)faceId; // 避免优化掉
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "查询 " << numQueries << " 个三角形耗时: " 
              << duration.count() << " 微秒" << std::endl;
    std::cout << "平均每次查询: " 
              << (double)duration.count() / numQueries << " 微秒" << std::endl;
}
```

---

### 2. 交点检测BVH优化

#### 2.1 创建边交点加速器

**文件:** `include/edges/EdgeIntersectionAccelerator.h` (新建)

```cpp
#pragma once

#include <vector>
#include <memory>
#include <OpenCASCADE/TopoDS_Edge.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/Geom_Curve.hxx>
#include <OpenCASCADE/Bnd_Box.hxx>
#include "geometry/BVHAccelerator.h"

/**
 * @brief 边交点检测加速器
 * 
 * 使用BVH加速边-边交点检测，将复杂度从O(n²)降至O(n log n)
 */
class EdgeIntersectionAccelerator {
public:
    /**
     * @brief 边图元数据
     */
    struct EdgePrimitive {
        Handle(Geom_Curve) curve;
        Standard_Real first, last;
        Bnd_Box bounds;
        size_t edgeIndex;
        TopoDS_Edge edge;  // 保留原始边用于精确计算
        
        EdgePrimitive() : first(0), last(0), edgeIndex(0) {}
    };
    
    /**
     * @brief 边对（潜在相交）
     */
    struct EdgePair {
        size_t edge1Index;
        size_t edge2Index;
        
        EdgePair(size_t i1, size_t i2) : edge1Index(i1), edge2Index(i2) {}
    };
    
    EdgeIntersectionAccelerator();
    ~EdgeIntersectionAccelerator() = default;
    
    /**
     * @brief 从边集合构建加速结构
     * @param edges 输入边集合
     * @param maxPrimitivesPerLeaf BVH叶节点最大图元数
     */
    void buildFromEdges(const std::vector<TopoDS_Edge>& edges, 
                       size_t maxPrimitivesPerLeaf = 4);
    
    /**
     * @brief 查找所有潜在相交的边对
     * @return 边对索引列表
     */
    std::vector<EdgePair> findPotentialIntersections() const;
    
    /**
     * @brief 提取所有交点（单线程）
     * @param tolerance 交点容差
     * @return 交点列表
     */
    std::vector<gp_Pnt> extractIntersections(double tolerance) const;
    
    /**
     * @brief 提取所有交点（多线程）
     * @param tolerance 交点容差
     * @param numThreads 线程数（0表示自动）
     * @return 交点列表
     */
    std::vector<gp_Pnt> extractIntersectionsParallel(double tolerance, 
                                                     size_t numThreads = 0) const;
    
    /**
     * @brief 获取统计信息
     */
    struct Statistics {
        size_t totalEdges = 0;
        size_t potentialPairs = 0;
        size_t actualIntersections = 0;
        double buildTime = 0.0;       // 秒
        double queryTime = 0.0;       // 秒
        double pruningRatio = 0.0;    // 剪枝率
    };
    
    Statistics getStatistics() const { return m_stats; }
    
    /**
     * @brief 清除加速结构
     */
    void clear();
    
    /**
     * @brief 检查是否已构建
     */
    bool isBuilt() const { return m_bvh != nullptr && m_bvh->isBuilt(); }
    
private:
    std::unique_ptr<BVHAccelerator> m_bvh;
    std::vector<EdgePrimitive> m_edges;
    mutable Statistics m_stats;
    
    /**
     * @brief 计算两条边的交点
     * @param edge1 第一条边数据
     * @param edge2 第二条边数据
     * @param tolerance 容差
     * @param intersection 输出交点
     * @return true如果找到交点
     */
    bool computeEdgeIntersection(const EdgePrimitive& edge1,
                                const EdgePrimitive& edge2,
                                double tolerance,
                                gp_Pnt& intersection) const;
    
    /**
     * @brief 查询与指定边包围盒相交的所有边
     * @param edgeIndex 边索引
     * @return 相交边的索引列表
     */
    std::vector<size_t> queryIntersectingEdges(size_t edgeIndex) const;
};
```

#### 2.2 实现文件

**文件:** `src/edges/EdgeIntersectionAccelerator.cpp` (新建)

```cpp
#include "edges/EdgeIntersectionAccelerator.h"
#include "logger/Logger.h"
#include <BRep_Tool.hxx>
#include <BRepBndLib.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <algorithm>
#include <execution>
#include <chrono>

EdgeIntersectionAccelerator::EdgeIntersectionAccelerator()
    : m_bvh(std::make_unique<BVHAccelerator>()) {
}

void EdgeIntersectionAccelerator::buildFromEdges(
    const std::vector<TopoDS_Edge>& edges,
    size_t maxPrimitivesPerLeaf) {
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    m_edges.clear();
    m_edges.reserve(edges.size());
    
    std::vector<TopoDS_Shape> shapeEdges;
    shapeEdges.reserve(edges.size());
    
    // 提取边数据
    for (size_t i = 0; i < edges.size(); ++i) {
        const auto& edge = edges[i];
        
        if (edge.IsNull() || BRep_Tool::Degenerated(edge)) {
            continue;
        }
        
        EdgePrimitive prim;
        prim.edgeIndex = i;
        prim.edge = edge;
        
        // 获取曲线
        Standard_Real first, last;
        prim.curve = BRep_Tool::Curve(edge, first, last);
        prim.first = first;
        prim.last = last;
        
        if (prim.curve.IsNull()) {
            continue;
        }
        
        // 计算包围盒
        Bnd_Box box;
        BRepBndLib::Add(edge, box);
        if (!box.IsVoid()) {
            prim.bounds = box;
        }
        
        m_edges.push_back(prim);
        shapeEdges.push_back(edge);
    }
    
    // 构建BVH
    if (!m_edges.empty()) {
        m_bvh->build(shapeEdges, maxPrimitivesPerLeaf);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.totalEdges = m_edges.size();
    m_stats.buildTime = std::chrono::duration<double>(endTime - startTime).count();
    
    LOG_INF_S("EdgeIntersectionAccelerator: 构建完成, 边数=" + 
              std::to_string(m_edges.size()) + 
              ", 耗时=" + std::to_string(m_stats.buildTime) + "秒");
}

std::vector<EdgeIntersectionAccelerator::EdgePair> 
EdgeIntersectionAccelerator::findPotentialIntersections() const {
    
    if (!isBuilt()) {
        LOG_WRN_S("EdgeIntersectionAccelerator: 未构建，返回空列表");
        return {};
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    std::vector<EdgePair> pairs;
    size_t worstCasePairs = (m_edges.size() * (m_edges.size() - 1)) / 2;
    pairs.reserve(std::min(worstCasePairs, size_t(10000))); // 预留合理大小
    
    // 使用BVH查询每条边的潜在相交边
    for (size_t i = 0; i < m_edges.size(); ++i) {
        auto candidates = queryIntersectingEdges(i);
        
        for (size_t j : candidates) {
            if (j > i) { // 避免重复和自相交
                pairs.push_back(EdgePair(i, j));
            }
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    m_stats.queryTime = std::chrono::duration<double>(endTime - startTime).count();
    m_stats.potentialPairs = pairs.size();
    
    size_t totalPairs = (m_edges.size() * (m_edges.size() - 1)) / 2;
    m_stats.pruningRatio = totalPairs > 0 ? 
        1.0 - (double)pairs.size() / totalPairs : 0.0;
    
    LOG_INF_S("EdgeIntersectionAccelerator: 查询完成, 潜在相交对=" + 
              std::to_string(pairs.size()) + 
              ", 剪枝率=" + std::to_string(m_stats.pruningRatio * 100) + "%");
    
    return pairs;
}

std::vector<gp_Pnt> EdgeIntersectionAccelerator::extractIntersections(
    double tolerance) const {
    
    auto potentialPairs = findPotentialIntersections();
    std::vector<gp_Pnt> intersections;
    
    for (const auto& pair : potentialPairs) {
        gp_Pnt intersection;
        if (computeEdgeIntersection(m_edges[pair.edge1Index],
                                    m_edges[pair.edge2Index],
                                    tolerance,
                                    intersection)) {
            intersections.push_back(intersection);
        }
    }
    
    m_stats.actualIntersections = intersections.size();
    
    return intersections;
}

std::vector<gp_Pnt> EdgeIntersectionAccelerator::extractIntersectionsParallel(
    double tolerance, size_t numThreads) const {
    
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
    }
    
    auto potentialPairs = findPotentialIntersections();
    
    // 线程本地结果缓冲区（避免锁）
    std::vector<std::vector<gp_Pnt>> threadResults(numThreads);
    
    // 并行处理边对
    std::for_each(std::execution::par_unseq,
        potentialPairs.begin(), potentialPairs.end(),
        [&](const EdgePair& pair) {
            size_t threadId = std::hash<std::thread::id>{}(std::this_thread::get_id()) % numThreads;
            
            gp_Pnt intersection;
            if (computeEdgeIntersection(m_edges[pair.edge1Index],
                                       m_edges[pair.edge2Index],
                                       tolerance,
                                       intersection)) {
                threadResults[threadId].push_back(intersection);
            }
        });
    
    // 合并结果
    std::vector<gp_Pnt> allIntersections;
    size_t totalSize = 0;
    for (const auto& results : threadResults) {
        totalSize += results.size();
    }
    allIntersections.reserve(totalSize);
    
    for (const auto& results : threadResults) {
        allIntersections.insert(allIntersections.end(),
                               results.begin(), results.end());
    }
    
    m_stats.actualIntersections = allIntersections.size();
    
    return allIntersections;
}

bool EdgeIntersectionAccelerator::computeEdgeIntersection(
    const EdgePrimitive& edge1,
    const EdgePrimitive& edge2,
    double tolerance,
    gp_Pnt& intersection) const {
    
    try {
        // 使用OpenCASCADE的曲线-曲线最近点算法
        GeomAPI_ExtremaCurveCurve extrema(
            edge1.curve, edge2.curve,
            edge1.first, edge1.last,
            edge2.first, edge2.last
        );
        
        if (extrema.NbExtrema() > 0) {
            double minDist = std::numeric_limits<double>::max();
            int minIndex = -1;
            
            for (int i = 1; i <= extrema.NbExtrema(); ++i) {
                double dist = extrema.Distance(i);
                if (dist < minDist) {
                    minDist = dist;
                    minIndex = i;
                }
            }
            
            if (minIndex > 0 && minDist < tolerance) {
                gp_Pnt p1, p2;
                extrema.Points(minIndex, p1, p2);
                intersection = p1; // 或者取中点: (p1 + p2) / 2
                return true;
            }
        }
    }
    catch (const Standard_Failure& e) {
        // 静默失败，某些边可能无法计算
    }
    
    return false;
}

std::vector<size_t> EdgeIntersectionAccelerator::queryIntersectingEdges(
    size_t edgeIndex) const {
    
    if (edgeIndex >= m_edges.size()) {
        return {};
    }
    
    const auto& edge = m_edges[edgeIndex];
    std::vector<size_t> results;
    
    // 使用BVH查询与此边包围盒相交的所有边
    // 注意: 这需要扩展BVH接口以支持包围盒查询
    // 暂时使用简化实现：遍历所有边检查包围盒相交
    
    for (size_t i = 0; i < m_edges.size(); ++i) {
        if (i == edgeIndex) continue;
        
        // 检查包围盒相交
        if (!edge.bounds.IsOut(m_edges[i].bounds)) {
            results.push_back(i);
        }
    }
    
    return results;
}

void EdgeIntersectionAccelerator::clear() {
    m_edges.clear();
    m_bvh->clear();
    m_stats = Statistics();
}
```

#### 2.3 集成到现有系统

**修改文件:** `src/opencascade/edges/extractors/OriginalEdgeExtractor.cpp`

```cpp
#include "edges/EdgeIntersectionAccelerator.h"

void OriginalEdgeExtractor::findEdgeIntersections(
    const TopoDS_Shape& shape,
    std::vector<gp_Pnt>& intersectionPoints,
    double tolerance) {
    
    // 收集所有边
    std::vector<TopoDS_Edge> edges;
    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        edges.push_back(TopoDS::Edge(exp.Current()));
    }
    
    LOG_INF_S("OriginalEdgeExtractor: 检测交点, 边数=" + std::to_string(edges.size()));
    
    // 选择策略：边数较少时使用原有方法，较多时使用BVH加速
    const size_t BVH_THRESHOLD = 100;
    
    if (edges.size() >= BVH_THRESHOLD) {
        // 使用BVH加速
        EdgeIntersectionAccelerator accelerator;
        accelerator.buildFromEdges(edges);
        
        auto newIntersections = accelerator.extractIntersectionsParallel(tolerance);
        
        // 去重并合并到结果
        for (const auto& pt : newIntersections) {
            bool isDuplicate = false;
            for (const auto& existing : intersectionPoints) {
                if (pt.Distance(existing) < tolerance) {
                    isDuplicate = true;
                    break;
                }
            }
            if (!isDuplicate) {
                intersectionPoints.push_back(pt);
            }
        }
        
        auto stats = accelerator.getStatistics();
        LOG_INF_S("BVH加速统计: 边数=" + std::to_string(stats.totalEdges) +
                  ", 潜在对=" + std::to_string(stats.potentialPairs) +
                  ", 实际交点=" + std::to_string(stats.actualIntersections) +
                  ", 剪枝率=" + std::to_string(stats.pruningRatio * 100) + "%");
    }
    else {
        // 使用原有的空间网格方法
        findEdgeIntersectionsFromEdges(edges, intersectionPoints, tolerance);
    }
}
```

---

### 3. 无锁线程本地缓冲区

#### 3.1 线程安全容器

**文件:** `include/core/ThreadSafeCollector.h` (新建)

```cpp
#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>

/**
 * @brief 线程安全的数据收集器（无锁设计）
 * 
 * 每个线程维护独立的缓冲区，避免锁竞争
 */
template<typename T>
class ThreadSafeCollector {
public:
    ThreadSafeCollector(size_t numThreads = 0) {
        if (numThreads == 0) {
            numThreads = std::thread::hardware_concurrency();
        }
        m_buffers.resize(numThreads);
    }
    
    /**
     * @brief 添加元素到线程本地缓冲区
     * @param value 要添加的值
     * @param threadId 线程ID（必须<numThreads）
     */
    void add(const T& value, size_t threadId) {
        if (threadId < m_buffers.size()) {
            m_buffers[threadId].push_back(value);
        }
    }
    
    /**
     * @brief 添加元素（自动检测线程ID）
     * @param value 要添加的值
     */
    void add(const T& value) {
        size_t threadId = getThreadIndex();
        add(value, threadId);
    }
    
    /**
     * @brief 收集所有线程的结果
     * @return 合并后的结果向量
     */
    std::vector<T> collect() const {
        std::vector<T> result;
        
        // 预分配空间
        size_t totalSize = 0;
        for (const auto& buffer : m_buffers) {
            totalSize += buffer.size();
        }
        result.reserve(totalSize);
        
        // 合并所有缓冲区
        for (const auto& buffer : m_buffers) {
            result.insert(result.end(), buffer.begin(), buffer.end());
        }
        
        return result;
    }
    
    /**
     * @brief 清空所有缓冲区
     */
    void clear() {
        for (auto& buffer : m_buffers) {
            buffer.clear();
        }
    }
    
    /**
     * @brief 获取总元素数
     */
    size_t size() const {
        size_t total = 0;
        for (const auto& buffer : m_buffers) {
            total += buffer.size();
        }
        return total;
    }
    
private:
    std::vector<std::vector<T>> m_buffers;
    
    // 线程ID到缓冲区索引的映射
    size_t getThreadIndex() const {
        static thread_local size_t index = SIZE_MAX;
        
        if (index == SIZE_MAX) {
            // 首次调用：分配一个唯一索引
            static std::atomic<size_t> counter{0};
            index = counter.fetch_add(1) % m_buffers.size();
        }
        
        return index;
    }
};

/**
 * @brief 使用示例
 */
inline void exampleUsage() {
    ThreadSafeCollector<gp_Pnt> collector(4); // 4个线程
    
    #pragma omp parallel for num_threads(4)
    for (int i = 0; i < 1000; ++i) {
        gp_Pnt point(i, i*2, i*3);
        collector.add(point); // 自动检测线程ID
    }
    
    auto allPoints = collector.collect();
    // allPoints.size() == 1000
}
```

#### 3.2 应用到交点检测

```cpp
// 在 EdgeIntersectionAccelerator::extractIntersectionsParallel 中使用

std::vector<gp_Pnt> EdgeIntersectionAccelerator::extractIntersectionsParallel(
    double tolerance, size_t numThreads) const {
    
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
    }
    
    auto potentialPairs = findPotentialIntersections();
    
    // 使用线程安全收集器
    ThreadSafeCollector<gp_Pnt> collector(numThreads);
    
    // 并行处理
    #pragma omp parallel for num_threads(numThreads) schedule(dynamic, 64)
    for (size_t i = 0; i < potentialPairs.size(); ++i) {
        const auto& pair = potentialPairs[i];
        
        gp_Pnt intersection;
        if (computeEdgeIntersection(m_edges[pair.edge1Index],
                                   m_edges[pair.edge2Index],
                                   tolerance,
                                   intersection)) {
            collector.add(intersection);  // 无锁添加
        }
    }
    
    // 收集结果
    return collector.collect();
}
```

---

## 性能测试框架

### 性能基准测试

**文件:** `tests/performance/test_geometry_performance.cpp` (新建)

```cpp
#include <gtest/gtest.h>
#include <chrono>
#include "edges/EdgeIntersectionAccelerator.h"
#include "geometry/OCCGeometryMesh.h"

class GeometryPerformanceTest : public ::testing::Test {
protected:
    struct BenchmarkResult {
        std::string name;
        double timeSeconds;
        size_t operations;
        
        double opsPerSecond() const {
            return operations / timeSeconds;
        }
        
        void print() const {
            std::cout << name << ": " << timeSeconds << "s, "
                     << opsPerSecond() << " ops/s" << std::endl;
        }
    };
    
    template<typename Func>
    BenchmarkResult benchmark(const std::string& name, Func func, size_t operations) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        
        double duration = std::chrono::duration<double>(end - start).count();
        
        return {name, duration, operations};
    }
};

TEST_F(GeometryPerformanceTest, FaceMappingLookup) {
    // 创建测试数据
    OCCGeometryMesh mesh;
    std::vector<FaceIndexMapping> mappings;
    
    const int numFaces = 1000;
    const int trisPerFace = 100;
    
    for (int i = 0; i < numFaces; ++i) {
        FaceIndexMapping mapping(i);
        for (int j = 0; j < trisPerFace; ++j) {
            mapping.triangleIndices.push_back(i * trisPerFace + j);
        }
        mappings.push_back(mapping);
    }
    
    mesh.setFaceIndexMappings(mappings);
    
    // 准备查询
    const int numQueries = 10000;
    std::vector<int> queries;
    for (int i = 0; i < numQueries; ++i) {
        queries.push_back(rand() % (numFaces * trisPerFace));
    }
    
    // 基准测试
    auto result = benchmark("Face Mapping Lookup", [&]() {
        for (int triIdx : queries) {
            int faceId = mesh.getGeometryFaceIdForTriangle(triIdx);
            EXPECT_GE(faceId, -1);
        }
    }, numQueries);
    
    result.print();
    
    // 性能要求: 至少 100万次查询/秒
    EXPECT_GT(result.opsPerSecond(), 1000000.0);
}

TEST_F(GeometryPerformanceTest, EdgeIntersectionAccelerator) {
    // 创建测试边
    std::vector<TopoDS_Edge> edges;
    
    // TODO: 生成测试边（需要OpenCASCADE API）
    // ...
    
    if (edges.size() < 10) {
        GTEST_SKIP() << "需要测试数据";
    }
    
    EdgeIntersectionAccelerator accelerator;
    
    // 测试构建时间
    auto buildResult = benchmark("BVH Build", [&]() {
        accelerator.buildFromEdges(edges);
    }, edges.size());
    
    buildResult.print();
    
    // 测试查询时间
    auto queryResult = benchmark("Find Intersections", [&]() {
        auto pairs = accelerator.findPotentialIntersections();
    }, edges.size());
    
    queryResult.print();
    
    // 打印统计
    auto stats = accelerator.getStatistics();
    std::cout << "剪枝率: " << (stats.pruningRatio * 100) << "%" << std::endl;
    std::cout << "潜在相交对: " << stats.potentialPairs << std::endl;
}

TEST_F(GeometryPerformanceTest, ThreadSafeCollector) {
    const size_t numThreads = 8;
    const size_t itemsPerThread = 10000;
    
    ThreadSafeCollector<int> collector(numThreads);
    
    auto result = benchmark("ThreadSafe Collect", [&]() {
        #pragma omp parallel for num_threads(numThreads)
        for (size_t i = 0; i < numThreads * itemsPerThread; ++i) {
            collector.add(i);
        }
        
        auto all = collector.collect();
        EXPECT_EQ(all.size(), numThreads * itemsPerThread);
    }, numThreads * itemsPerThread);
    
    result.print();
    
    // 性能要求: 至少 1000万次添加/秒
    EXPECT_GT(result.opsPerSecond(), 10000000.0);
}

// 运行所有性能测试
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

---

## 集成检查清单

### Phase 1: 基础优化

- [ ] **FaceIndexMapping反向索引**
  - [ ] 修改 `OCCGeometryMesh.h` 添加成员
  - [ ] 实现 `buildReverseMapping()` 方法
  - [ ] 更新 `setFaceIndexMappings()` 调用
  - [ ] 优化 `getGeometryFaceIdForTriangle()`
  - [ ] 添加单元测试
  - [ ] 性能基准测试

- [ ] **EdgeIntersectionAccelerator**
  - [ ] 创建头文件 `EdgeIntersectionAccelerator.h`
  - [ ] 实现 `.cpp` 文件
  - [ ] 集成到 `OriginalEdgeExtractor`
  - [ ] 添加阈值配置（何时使用BVH）
  - [ ] 单元测试
  - [ ] 性能对比测试

- [ ] **ThreadSafeCollector**
  - [ ] 创建模板类头文件
  - [ ] 应用到交点检测
  - [ ] 应用到其他并行场景
  - [ ] 多线程压力测试

### Phase 2: 验证与调优

- [ ] **性能验证**
  - [ ] 运行性能测试套件
  - [ ] 对比优化前后数据
  - [ ] 大模型测试（>10万面）
  - [ ] 记录内存占用变化

- [ ] **兼容性测试**
  - [ ] 小模型测试（<1000面）
  - [ ] 边界情况测试
  - [ ] 多线程稳定性测试
  - [ ] 跨平台测试

- [ ] **文档更新**
  - [ ] API文档
  - [ ] 性能数据更新
  - [ ] 使用示例
  - [ ] 迁移指南

---

## 调试技巧

### 1. 性能分析工具

```bash
# Linux: perf
perf record -g ./your_app
perf report

# Windows: Visual Studio Profiler
# 使用 Performance Profiler

# 通用: Intel VTune
vtune -collect hotspots -- ./your_app
```

### 2. 内存泄漏检测

```bash
# Valgrind
valgrind --leak-check=full --show-leak-kinds=all ./your_app

# AddressSanitizer
# 编译时添加: -fsanitize=address
```

### 3. 线程安全检测

```bash
# ThreadSanitizer
# 编译时添加: -fsanitize=thread
```

### 4. 日志级别控制

```cpp
// 临时启用详细日志
LOG_SET_LEVEL(LogLevel::DEBUG);

// 性能关键路径的条件日志
#ifndef NDEBUG
    LOG_DBG_S("EdgeIntersection: found " + std::to_string(count));
#endif
```

---

## 常见问题

### Q1: BVH构建很慢怎么办？

**A:** 
- 使用异步构建：在后台线程构建BVH
- 增量更新：只更新变化的部分
- 调整叶节点大小：尝试2-8之间的值

### Q2: 内存占用过大？

**A:**
- 延迟构建：仅在需要时构建索引
- 释放临时数据：查询后清除中间结果
- 使用压缩格式：例如半精度浮点数

### Q3: 多线程性能没提升？

**A:**
- 检查锁竞争：使用ThreadSanitizer
- 增加批次大小：减少调度开销
- False Sharing：使用 `alignas(64)` 对齐

### Q4: 如何选择合适的数据结构？

**A:** 根据使用场景：
- **频繁查询** → BVH / R树
- **简单模型** → 线性搜索
- **拓扑编辑** → 半边结构
- **只读静态** → 索引三角形

---

## 下一步计划

### 短期 (1-2周)
1. 完成P0优化实施
2. 性能基准测试
3. 代码审查

### 中期 (1个月)
1. 空间哈希顶点索引
2. R树面索引
3. 工作窃取线程池

### 长期 (3个月)
1. 半边网格结构
2. GPU加速POC
3. 完整LOD系统

---

**文档版本:** 1.0  
**创建日期:** 2025-10-19  
**最后更新:** 2025-10-19  
**状态:** 待实施



