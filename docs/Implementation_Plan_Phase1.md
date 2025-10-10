# Phase 1 Implementation Plan - Week by Week

## 总览

**目标**: 实施3个高优先级优化，快速见效
**周期**: 5周
**预期提升**: 边显示性能80-90%，参数调整效率60-80%，边点数减少40-60%

---

## Week 1: 边几何缓存（EdgeGeometryCache）

### 🎯 目标
- 实现边几何缓存机制
- 集成到EdgeExtractor
- 验证性能提升

### 📋 任务清单

#### Day 1-2: 创建缓存类
- [ ] 创建 `include/edges/EdgeGeometryCache.h`
- [ ] 创建 `src/opencascade/edges/EdgeGeometryCache.cpp`
- [ ] 添加到CMakeLists.txt
- [ ] 编译验证

#### Day 3-4: 集成到EdgeExtractor
- [ ] 修改 `extractOriginalEdges()` 使用缓存
- [ ] 修改 `extractFeatureEdges()` 使用缓存
- [ ] 修改 `extractMeshEdges()` 使用缓存
- [ ] 添加缓存失效逻辑

#### Day 5: 测试和优化
- [ ] 创建性能测试用例
- [ ] 测试小/中/大模型
- [ ] 记录性能数据
- [ ] 调整缓存策略

### ✅ 成功标准
- 第二次边显示切换时间 < 20ms
- 缓存命中率 > 80%
- 无内存泄漏
- 日志显示缓存统计信息

### 📊 性能指标
```
测试模型：小零件（100 edges）
- 首次提取：150ms
- 缓存命中：<20ms（目标）
- 内存增加：<5MB

测试模型：中型装配（1000 edges）
- 首次提取：800ms
- 缓存命中：<50ms（目标）
- 内存增加：<20MB
```

---

## Week 2-3: 智能网格参数推荐（MeshParameterAdvisor）

### 🎯 目标
- 分析几何复杂度
- 自动推荐最佳参数
- 预估三角形数量

### 📋 任务清单

#### Week 2 Day 1-3: 核心分析功能
- [ ] 创建 `include/viewer/MeshParameterAdvisor.h`
- [ ] 实现 `analyzeShape()` - 几何复杂度分析
- [ ] 实现 `recommendParameters()` - 参数推荐
- [ ] 实现 `estimateTriangleCount()` - 三角形预估
- [ ] 单元测试验证

#### Week 2 Day 4-5: 质量预设
- [ ] 实现5档质量预设（Draft/Low/Medium/High/VeryHigh）
- [ ] 测试不同尺寸模型的推荐参数
- [ ] 验证预估准确度

#### Week 3 Day 1-3: UI集成
- [ ] 在MeshQualityDialog添加"自动推荐"按钮
- [ ] 添加三角形数量预估显示
- [ ] 添加质量预设下拉菜单
- [ ] 实现参数应用逻辑

#### Week 3 Day 4-5: 测试和调优
- [ ] 测试各种类型几何（简单/复杂/装配体）
- [ ] 收集推荐参数效果数据
- [ ] 调整推荐算法
- [ ] 编写用户文档

### ✅ 成功标准
- 推荐参数的网格质量评分 > 0.7
- 三角形预估误差 < 30%
- 用户参数调整次数减少 60%
- UI响应流畅

### 📊 测试用例
```
测试1：小零件（10mm bbox）
- 推荐deflection：0.001
- 预估三角形：~5000
- 实际三角形：4500-6000（可接受）

测试2：中型零件（100mm bbox）
- 推荐deflection：0.01
- 预估三角形：~50000
- 实际三角形：40000-65000（可接受）

测试3：大型装配体（1000mm bbox）
- 推荐deflection：0.1
- 预估三角形：~100000
- 实际三角形：80000-130000（可接受）
```

---

## Week 4: 自适应边采样

### 🎯 目标
- 根据曲率自适应调整采样密度
- 减少简单边的点数
- 提高复杂曲线质量

### 📋 任务清单

#### Day 1-2: 曲率分析
- [ ] 实现 `analyzeCurvature()` 方法
- [ ] 计算最大曲率
- [ ] 确定曲率阈值

#### Day 3-4: 自适应采样实现
- [ ] 修改EdgeExtractor添加 `adaptiveSampleCurve()`
- [ ] 根据曲率确定采样数
- [ ] 针对不同曲线类型优化
- [ ] 集成到extractOriginalEdges

#### Day 5: 测试和对比
- [ ] 对比自适应vs固定采样
- [ ] 测量点数减少比例
- [ ] 验证视觉质量不降低
- [ ] 性能基准测试

### ✅ 成功标准
- 直线只用2个点
- 简单曲线点数减少40-60%
- 复杂曲线质量提升
- 总体渲染性能提升15-25%

### 📊 采样策略
```
曲率 < 0.01  -> 4 samples  (低曲率)
曲率 < 0.1   -> 8 samples  (中曲率)
曲率 < 1.0   -> 16 samples (高曲率)
曲率 >= 1.0  -> 32 samples (极高曲率)

特殊类型：
- Line: 2 samples（固定）
- Circle/Ellipse: min 16 samples
- BSpline/Bezier: min 20 samples
- 上限：64 samples
```

---

## Week 5: 性能测试和数据收集

### 🎯 目标
- 全面性能测试
- 收集实际数据
- 准备用户反馈

### 📋 任务清单

#### Day 1-2: 自动化测试
- [ ] 创建性能测试套件
- [ ] 准备测试模型集（小/中/大/装配体）
- [ ] 实现性能数据自动收集
- [ ] 生成性能报告

#### Day 3: 内存和稳定性测试
- [ ] 长时间运行测试
- [ ] 内存泄漏检测
- [ ] 缓存清理测试
- [ ] 边界条件测试

#### Day 4: 用户体验测试
- [ ] 邀请内部用户测试
- [ ] 收集使用反馈
- [ ] 记录问题和改进点
- [ ] 准备用户文档

#### Day 5: 总结和规划
- [ ] 整理性能数据
- [ ] 编写Phase 1总结报告
- [ ] 规划Phase 2任务
- [ ] 准备演示材料

### 📊 性能数据收集模板

```markdown
## 测试模型: [模型名称]
- 文件大小: [size]
- 顶点数: [count]
- 面数: [count]
- 边数: [count]

### 优化前
- 边提取时间: [ms]
- 边显示切换: [ms]
- 网格生成时间: [ms]
- 内存占用: [MB]

### 优化后
- 边提取时间: [ms] (改善: [%])
- 边显示切换: [ms] (改善: [%])
- 网格生成时间: [ms] (改善: [%])
- 内存占用: [MB] (改善: [%])

### 边几何缓存
- 缓存命中率: [%]
- 平均缓存响应: [ms]
- 缓存内存: [MB]

### 智能参数推荐
- 推荐deflection: [value]
- 预估三角形: [count]
- 实际三角形: [count]
- 预估误差: [%]

### 自适应采样
- 原始边点数: [count]
- 优化后点数: [count]
- 减少比例: [%]
```

---

## 快速开始指南

### 立即开始第一周任务

#### Step 1: 创建EdgeGeometryCache.h

```bash
# 在项目根目录执行
cd d:\source\repos\wxcoin
```

创建文件并复制以下代码：

**文件**: `include/edges/EdgeGeometryCache.h`

```cpp
#pragma once

#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <functional>
#include <OpenCASCADE/gp_Pnt.hxx>

class EdgeGeometryCache {
public:
    struct CacheEntry {
        std::vector<gp_Pnt> points;
        size_t shapeHash;
        std::chrono::steady_clock::time_point lastAccess;
        
        CacheEntry() : shapeHash(0) {}
    };

    static EdgeGeometryCache& getInstance() {
        static EdgeGeometryCache instance;
        return instance;
    }

    // Get cached result or compute
    std::vector<gp_Pnt> getOrCompute(
        const std::string& key,
        std::function<std::vector<gp_Pnt>()> computeFunc);

    // Cache management
    void invalidate(const std::string& key);
    void clear();
    void evictOldEntries(std::chrono::seconds maxAge = std::chrono::seconds(300));

    // Statistics
    size_t getCacheSize() const { return m_cache.size(); }
    size_t getHitCount() const { return m_hitCount; }
    size_t getMissCount() const { return m_missCount; }
    double getHitRate() const {
        size_t total = m_hitCount + m_missCount;
        return total > 0 ? (100.0 * m_hitCount / total) : 0.0;
    }

private:
    EdgeGeometryCache() : m_hitCount(0), m_missCount(0) {}
    
    std::unordered_map<std::string, CacheEntry> m_cache;
    mutable std::mutex m_mutex;
    size_t m_hitCount;
    size_t m_missCount;
};
```

#### Step 2: 创建EdgeGeometryCache.cpp

**文件**: `src/opencascade/edges/EdgeGeometryCache.cpp`

```cpp
#include "edges/EdgeGeometryCache.h"
#include "logger/Logger.h"
#include <sstream>

std::vector<gp_Pnt> EdgeGeometryCache::getOrCompute(
    const std::string& key,
    std::function<std::vector<gp_Pnt>()> computeFunc)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        // Cache hit
        it->second.lastAccess = std::chrono::steady_clock::now();
        m_hitCount++;
        
        LOG_DBG_S("EdgeCache HIT: " + key + 
                  " (rate: " + std::to_string(getHitRate()) + "%, " +
                  "points: " + std::to_string(it->second.points.size()) + ")");
        
        return it->second.points;
    }

    // Cache miss - compute
    m_missCount++;
    LOG_DBG_S("EdgeCache MISS: " + key + " (computing...)");
    
    auto points = computeFunc();
    
    CacheEntry entry;
    entry.points = points;
    entry.shapeHash = 0;
    entry.lastAccess = std::chrono::steady_clock::now();
    
    m_cache[key] = std::move(entry);
    
    LOG_INF_S("EdgeCache stored: " + key + 
              " (" + std::to_string(points.size()) + " points, " +
              "cache size: " + std::to_string(m_cache.size()) + ")");
    
    return points;
}

void EdgeGeometryCache::invalidate(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(key);
    if (it != m_cache.end()) {
        LOG_DBG_S("EdgeCache invalidated: " + key);
        m_cache.erase(it);
    }
}

void EdgeGeometryCache::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t oldSize = m_cache.size();
    m_cache.clear();
    m_hitCount = 0;
    m_missCount = 0;
    LOG_INF_S("EdgeCache cleared (" + std::to_string(oldSize) + " entries removed)");
}

void EdgeGeometryCache::evictOldEntries(std::chrono::seconds maxAge) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto now = std::chrono::steady_clock::now();
    size_t evicted = 0;
    
    for (auto it = m_cache.begin(); it != m_cache.end();) {
        if (now - it->second.lastAccess > maxAge) {
            LOG_DBG_S("EdgeCache evicting old entry: " + it->first);
            it = m_cache.erase(it);
            evicted++;
        } else {
            ++it;
        }
    }
    
    if (evicted > 0) {
        LOG_INF_S("EdgeCache evicted " + std::to_string(evicted) + 
                  " old entries (remaining: " + std::to_string(m_cache.size()) + ")");
    }
}
```

#### Step 3: 修改CMakeLists.txt

在 `src/opencascade/CMakeLists.txt` 中添加：

```cmake
# Find the line with other edge source files and add:
target_sources(opencascade PRIVATE
    edges/EdgeExtractor.cpp
    edges/EdgeRenderer.cpp
    edges/EdgeGeometryCache.cpp  # <-- ADD THIS LINE
)
```

#### Step 4: 集成到EdgeExtractor

在 `src/opencascade/edges/EdgeExtractor.cpp` 顶部添加：

```cpp
#include "edges/EdgeGeometryCache.h"
#include <sstream>
```

然后修改 `extractOriginalEdges` 方法，在方法开始处添加：

```cpp
std::vector<gp_Pnt> EdgeExtractor::extractOriginalEdges(
    const TopoDS_Shape& shape, 
    double samplingDensity, 
    double minLength, 
    bool showLinesOnly,
    std::vector<gp_Pnt>* intersectionPoints)
{
    // Generate cache key
    std::ostringstream keyStream;
    keyStream << "original_" 
              << shape.HashCode(INT_MAX) << "_"
              << samplingDensity << "_"
              << minLength << "_"
              << (showLinesOnly ? "1" : "0");
    std::string cacheKey = keyStream.str();
    
    // Try to get from cache
    auto& cache = EdgeGeometryCache::getInstance();
    
    // Check if we need intersection points (can't cache with intersections)
    if (intersectionPoints == nullptr) {
        return cache.getOrCompute(cacheKey, [&]() {
            return extractOriginalEdgesImpl(shape, samplingDensity, minLength, showLinesOnly, nullptr);
        });
    } else {
        // With intersection points, always compute
        return extractOriginalEdgesImpl(shape, samplingDensity, minLength, showLinesOnly, intersectionPoints);
    }
}

// Rename the existing implementation
std::vector<gp_Pnt> EdgeExtractor::extractOriginalEdgesImpl(
    const TopoDS_Shape& shape, 
    double samplingDensity, 
    double minLength, 
    bool showLinesOnly,
    std::vector<gp_Pnt>* intersectionPoints)
{
    // PUT ALL EXISTING CODE HERE (lines 73-189)
    std::vector<EdgeData> edgeData;
    // ... rest of original code ...
}
```

同时在头文件 `include/edges/EdgeExtractor.h` 中添加私有方法声明：

```cpp
private:
    // ... existing private methods ...
    
    // Implementation without cache
    std::vector<gp_Pnt> extractOriginalEdgesImpl(
        const TopoDS_Shape& shape, 
        double samplingDensity, 
        double minLength, 
        bool showLinesOnly,
        std::vector<gp_Pnt>* intersectionPoints);
```

---

## 验证步骤

### 编译验证

```bash
cmake --build build --config Release
```

### 简单测试

运行程序后：
1. 导入一个STEP文件
2. 启用"显示原始边"
3. 关闭"显示原始边"
4. 再次启用"显示原始边" ← 这里应该很快（缓存命中）
5. 检查日志文件，应该看到：
   ```
   EdgeCache MISS: original_...
   EdgeCache stored: ...
   EdgeCache HIT: original_... (rate: 50%, points: ...)
   ```

---

## 性能对比模板

创建简单的性能记录：

**文件**: `performance_log_week1.txt`

```
=== Week 1 Performance Test ===
Date: [填写日期]
Tester: [你的名字]

Model: small_part.step
Size: 50 KB
Edges: 124

Test 1: First edge display (cache miss)
- Time: _____ ms

Test 2: Second edge display (cache hit)
- Time: _____ ms
- Improvement: _____ %

Test 3: Third edge display (cache hit)
- Time: _____ ms

Cache Statistics:
- Hit rate: _____ %
- Cache size: _____ entries
- Memory usage: _____ MB (estimated)

Notes:
[记录任何观察到的问题或改进点]
```

---

## 需要帮助？

如果遇到问题：
1. 检查编译错误
2. 查看日志输出
3. 确认缓存键生成正确
4. 验证线程安全

准备好开始了吗？我可以帮助您：
- 解决编译问题
- 调试缓存逻辑
- 优化性能
- 准备下一周任务



