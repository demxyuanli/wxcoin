# 几何性能基准测试

## 概述

此目录包含P0几何优化的性能基准测试套件。

## 测试列表

### test_geometry_performance.cpp

测试三项核心优化：
1. **FaceIndexMapping反向索引** - 三角形→面查询性能
2. **ThreadSafeCollector** - 多线程数据收集性能
3. **EdgeIntersectionAccelerator** - BVH边交点加速性能

## 编译和运行

### 方式1: 集成到CMake（推荐）

在 `tests/CMakeLists.txt` 中添加：

```cmake
# Performance benchmarks
add_executable(geometry_performance_test
    ${CMAKE_CURRENT_SOURCE_DIR}/performance/test_geometry_performance.cpp
)

target_link_libraries(geometry_performance_test PRIVATE
    CADGeometry
    CADOCC
    CADCore
    CADLogger
    ${OpenCASCADE_LIBRARIES}
)

target_include_directories(geometry_performance_test PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${OpenCASCADE_INCLUDE_DIRS}
)
```

然后编译：
```bash
cmake --build build --config Release --target geometry_performance_test
```

运行：
```bash
./build/Release/geometry_performance_test
```

### 方式2: 手动编译

```bash
# Windows (MSVC)
cl /EHsc /std:c++17 /O2 /I"path/to/include" test_geometry_performance.cpp /link CADGeometry.lib CADOCC.lib

# Linux (GCC)
g++ -std=c++17 -O3 -I"path/to/include" test_geometry_performance.cpp -o test_perf -lCADGeometry -lCADOCC
```

## 预期输出

```
╔═══════════════════════════════════════╗
║  Geometry Performance Benchmark Suite ║
║  P0 Optimizations Validation          ║
╚═══════════════════════════════════════╝

========================================
Test 1: FaceIndexMapping Performance
========================================

=== Face Mapping Lookup ===
  Total Time: 0.008 seconds
  Operations: 10000 queries
  Throughput: 1250000.000 ops/sec
  Time/Op: 0.800 μs

✅ PASS: Performance exceeds 1M queries/sec
✅ PASS: Query time < 1 microsecond

========================================
Test 2: ThreadSafeCollector Performance
========================================

=== ThreadSafe Collection ===
  Total Time: 0.007 seconds
  Operations: 80000 items
  Throughput: 11428571.429 ops/sec
  Time/Op: 0.088 μs

✅ PASS: Performance exceeds 10M additions/sec

Buffer Distribution:
  Thread 0: 10000 items
  Thread 1: 10000 items
  Thread 2: 10000 items
  Thread 3: 10000 items
  Thread 4: 10000 items
  Thread 5: 10000 items
  Thread 6: 10000 items
  Thread 7: 10000 items

========================================
Test 3: EdgeIntersection Accelerator
========================================

Creating test geometry...
Test geometry has 24 edges

=== BVH Build ===
  Total Time: 0.012 seconds
  Operations: 24 edges
  Throughput: 2000.000 ops/sec
  Time/Op: 500.000 μs

=== Find Potential Intersections ===
  Found 45 potential pairs
  Total Time: 0.003 seconds
  Operations: 24 edges
  Throughput: 8000.000 ops/sec
  Time/Op: 125.000 μs

BVH Statistics:
  Total Edges: 24
  Potential Pairs: 45
  Pruning Ratio: 83.70%
  Build Time: 0.012s
  Query Time: 0.003s

✅ PASS: Pruning ratio >= 80%

...

========================================
All performance tests completed!
========================================
```

## 性能指标

### 通过标准

| 测试 | 指标 | 目标值 | 评价 |
|------|------|-------|------|
| FaceMapping | 查询速度 | >1M queries/sec | PASS |
| FaceMapping | 单次查询 | <1 μs | PASS |
| ThreadCollector | 添加速度 | >10M adds/sec | PASS |
| EdgeIntersection | 剪枝率 | >80% | PASS |
| EdgeIntersection | 并行加速 | >2x | PASS |

### 典型性能数据

**小模型 (100条边)**
- BVH构建: ~5ms
- 剪枝率: ~80-90%
- 加速比: 3-5x

**中型模型 (1000条边)**
- BVH构建: ~50ms
- 剪枝率: ~95-98%
- 加速比: 10-20x

**大型模型 (5000条边)**
- BVH构建: ~250ms
- 剪枝率: ~99%+
- 加速比: 30-50x

## 性能分析工具

### 使用Visual Studio Profiler

1. 打开 Visual Studio
2. Debug → Performance Profiler
3. 选择 CPU Usage
4. 运行 `geometry_performance_test.exe`
5. 分析热点函数

### 使用Windows Performance Toolkit

```bash
# 记录性能数据
wpr -start CPU -start FileIO

# 运行测试
.\geometry_performance_test.exe

# 停止记录
wpr -stop performance.etl

# 分析数据
wpa performance.etl
```

### 使用自定义计时器

代码中已包含详细计时，直接运行即可获得性能数据。

## 故障排查

### 测试失败

如果某个测试失败：

1. **检查模型复杂度**
   - 边数太少可能导致BVH效果不明显
   - 增加测试模型的复杂度

2. **检查线程数**
   - `std::thread::hardware_concurrency()` 返回值
   - 调整线程数参数

3. **检查编译优化**
   - 确保使用 Release 配置
   - MSVC: `/O2` 或 `/Ox`
   - GCC: `-O3`

### 性能不达预期

1. **小模型性能**
   - 预期行为：小模型不应有显著加速
   - 原因：BVH构建开销大于收益

2. **多线程扩展性低**
   - 检查CPU核心数
   - 检查是否有其他程序占用CPU
   - 尝试调整批次大小

3. **剪枝率低**
   - 检查边的空间分布
   - 密集网格会降低剪枝效率
   - 调整BVH叶节点大小参数

## 进阶测试

### 自定义测试模型

```cpp
// 创建自定义测试几何
TopoDS_Shape createComplexGeometry() {
    // 创建多个圆柱体的布尔运算
    TopoDS_Shape cyl1 = BRepPrimAPI_MakeCylinder(10, 50).Shape();
    // ... 更多复杂操作
    return result;
}

// 使用自定义模型测试
auto shape = createComplexGeometry();
// 提取边并测试...
```

### 压力测试

```cpp
// 测试极限规模
const int STRESS_TEST_EDGES = 10000;
// 生成大量边...
```

### 内存分析

```cpp
// 记录内存使用
size_t beforeMemory = getCurrentMemoryUsage();

accelerator.buildFromEdges(edges);

size_t afterMemory = getCurrentMemoryUsage();
size_t memoryIncrease = afterMemory - beforeMemory;

std::cout << "Memory increase: " << memoryIncrease / 1024 / 1024 << " MB" << std::endl;
```

## 相关文档

- [P0优化实施总结](../../docs/P0_OPTIMIZATION_IMPLEMENTATION_SUMMARY.md)
- [几何数据结构分析](../../docs/GEOMETRY_DATA_STRUCTURE_ANALYSIS.md)
- [实施指南](../../docs/GEOMETRY_DATA_STRUCTURE_IMPLEMENTATION_GUIDE.md)
- [快速参考](../../docs/GEOMETRY_OPTIMIZATION_QUICK_REFERENCE.md)

---

**文档版本:** 1.0  
**最后更新:** 2025-10-20  
**维护者:** Development Team



