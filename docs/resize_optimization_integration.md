# Resize优化集成说明

## 已添加的文件

### 1. Docking模块
- **源文件**：
  - `src/docking/DockContainerOptimized.cpp` - 优化的容器实现
  - `src/docking/DockResizeMonitor.cpp` - 性能监控组件
- **头文件**：
  - `include/docking/DockContainerOptimized.h`
  - `include/docking/DockResizeMonitor.h`

### 2. View模块
- **源文件**：
  - `src/view/CanvasOptimized.cpp` - 渐进式渲染Canvas
- **头文件**：
  - `include/view/CanvasOptimized.h`

### 3. 文档
- `docs/docking_resize_optimization_guide.md` - 详细优化指南
- `docs/resize_optimization_integration.md` - 本文档

## CMake配置更新

### 1. src/docking/CMakeLists.txt
```cmake
# 已添加到 DOCKING_SOURCES:
DockContainerOptimized.cpp
DockResizeMonitor.cpp

# 已添加到 DOCKING_HEADERS:
${CMAKE_SOURCE_DIR}/include/docking/DockContainerOptimized.h
${CMAKE_SOURCE_DIR}/include/docking/DockResizeMonitor.h
```

### 2. src/view/CMakeLists.txt
```cmake
# 已添加到 CADVIEW_SOURCES:
${CMAKE_SOURCE_DIR}/src/view/CanvasOptimized.cpp

# 已添加到 CADVIEW_HEADERS:
${CMAKE_SOURCE_DIR}/include/view/CanvasOptimized.h
```

## 修改的现有文件

### 1. FlatFrameDocking
- `src/ui/FlatFrameDocking.cpp` - 优化了resize处理逻辑，集成了性能监控

### 2. DockContainerWidget
- `src/docking/DockContainerWidget.cpp` - 优化了onSize处理，减少Freeze/Thaw使用

## 使用方法

### 1. 使用优化的DockContainer（可选）
如果需要使用DockContainerOptimized替代标准DockContainerWidget：
```cpp
// 在DockManager.cpp中
m_containerWidget = new DockContainerOptimized(this, parent);
```

### 2. 使用优化的Canvas（可选）
如果需要使用渐进式渲染：
```cpp
// 在FlatFrameDocking.cpp的CreateCanvasDockWidget()中
#include "view/CanvasOptimized.h"
canvas = new CanvasOptimized(dock);
canvas->setProgressiveRenderingEnabled(true);
```

### 3. 查看性能报告
```cpp
// 在适当的地方（如菜单命令或调试输出）
#include "docking/DockResizeMonitor.h"
std::string report = ads::DockResizeMonitor::getInstance().generateReport();
LOG_INF(report, "ResizePerformance");
```

## 编译说明

1. 清理构建目录：
```bash
rm -rf build
```

2. 重新配置CMake：
```bash
mkdir build && cd build
cmake ..
```

3. 编译项目：
```bash
make -j$(nproc)
```

## 验证优化效果

1. 运行程序并调整主窗口大小
2. 观察resize的流畅度
3. 查看性能日志中的耗时数据
4. 使用性能监控报告分析瓶颈

## 注意事项

1. 优化代码已经集成到现有的FlatFrameDocking中，默认生效
2. DockContainerOptimized和CanvasOptimized是可选组件，需要手动启用
3. 性能监控默认开启，可通过`DockResizeMonitor::getInstance().setEnabled(false)`关闭