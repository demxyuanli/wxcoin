# 统一参数管理系统 (Unified Parameter Management System)

## 概述

统一参数管理系统是一个用于管理几何表示与绘制控制参数的综合性解决方案。它提供了一个统一的参数对象树结构，智能的批量更新机制，以及强大的参数依赖关系管理功能。

## 主要特性

### 1. 统一参数树结构
- **层次化参数组织**: 支持嵌套的参数结构，便于管理和查找
- **多类型参数支持**: 支持布尔值、整数、浮点数、字符串、向量等多种数据类型
- **参数分组**: 支持逻辑分组和容器节点
- **路径访问**: 使用点分隔的路径系统访问参数

### 2. 智能批量更新
- **自动批量处理**: 自动将多个参数变更合并为批量操作
- **多种更新策略**: 支持立即、批量、节流、延迟等更新策略
- **依赖关系管理**: 自动处理参数间的依赖关系
- **性能优化**: 减少重复更新，提高系统性能

### 3. 系统集成
- **现有系统兼容**: 与RenderingConfig、MeshParameterManager、LightingConfig等现有系统无缝集成
- **双向同步**: 支持参数在统一系统和现有系统间的双向同步
- **桥接器模式**: 为每个系统提供专门的桥接器

### 4. 高级功能
- **预设管理**: 支持参数预设的保存、加载和删除
- **性能监控**: 提供详细的性能统计和监控信息
- **并发安全**: 支持多线程环境下的安全操作
- **错误处理**: 完善的错误处理和恢复机制

## 系统架构

```
┌─────────────────────────────────────────────────────────────┐
│                统一参数集成管理器                              │
│              (UnifiedParameterIntegration)                 │
└─────────────────────┬───────────────────────────────────────┘
                      │
        ┌─────────────┼─────────────┐
        │             │             │
┌───────▼──────┐ ┌───▼────┐ ┌──────▼──────┐
│   参数注册表   │ │更新协调器│ │  参数树工厂   │
│ParameterRegistry│ │UpdateCoordinator│ │ParameterTreeFactory│
└───────┬──────┘ └───┬────┘ └──────┬──────┘
        │            │             │
        └────────────┼─────────────┘
                     │
        ┌────────────▼────────────┐
        │     统一参数树           │
        │  (UnifiedParameterTree) │
        └─────────────────────────┘
```

## 核心组件

### 1. UnifiedParameterTree (统一参数树)
负责参数树结构的创建、管理和访问。

```cpp
// 创建参数树
auto tree = ParameterTreeFactory::createCompleteParameterTree();

// 设置参数值
tree->setParameterValue("rendering.material.diffuse.r", 0.8);

// 获取参数值
auto value = tree->getParameterValue("rendering.material.diffuse.r");

// 批量设置参数
std::unordered_map<std::string, ParameterValue> params;
params["geometry.position.x"] = 10.0;
params["geometry.position.y"] = 20.0;
tree->setParameterValues(params);
```

### 2. ParameterRegistry (参数注册表)
管理多个参数系统，提供统一的参数访问接口。

```cpp
auto& registry = ParameterRegistry::getInstance();

// 注册参数系统
registry.registerParameterSystem(SystemType::RENDERING, renderingTree);

// 跨系统参数操作
registry.setParameterByFullPath("rendering.material.diffuse.r", 0.8);
auto value = registry.getParameterByFullPath("rendering.material.diffuse.r");
```

### 3. UpdateCoordinator (更新协调器)
智能协调参数变更和渲染更新。

```cpp
auto& coordinator = UpdateCoordinator::getInstance();

// 提交参数变更任务
coordinator.submitParameterChange("rendering.material.diffuse.r", 0.5, 0.8);

// 调度几何重建
coordinator.scheduleGeometryRebuild("geometry.main_object");

// 调度渲染更新
coordinator.scheduleRenderingUpdate("main_viewport");
```

### 4. UnifiedParameterIntegration (统一参数集成管理器)
提供高级的集成和管理功能。

```cpp
auto& integration = UnifiedParameterIntegration::getInstance();

// 初始化
IntegrationConfig config;
config.autoSyncEnabled = true;
config.enableSmartBatching = true;
integration.initialize(config);

// 集成现有系统
integration.integrateRenderingConfig(&renderingConfig);
integration.integrateMeshParameterManager(&meshManager);

// 统一参数操作
integration.setParameter("rendering.material.diffuse.r", 0.8);
auto value = integration.getParameter("rendering.material.diffuse.r");
```

## 使用示例

### 基本使用

```cpp
#include "param/UnifiedParameterIntegration.h"

int main() {
    // 1. 初始化统一参数集成管理器
    auto& integration = UnifiedParameterIntegration::getInstance();
    
    IntegrationConfig config;
    config.autoSyncEnabled = true;
    config.enableSmartBatching = true;
    integration.initialize(config);
    
    // 2. 集成现有系统
    auto& renderingConfig = RenderingConfig::getInstance();
    integration.integrateRenderingConfig(&renderingConfig);
    
    // 3. 设置参数
    integration.setParameter("rendering.material.diffuse.r", 0.8);
    integration.setParameter("rendering.material.diffuse.g", 0.6);
    integration.setParameter("rendering.material.diffuse.b", 0.4);
    
    // 4. 获取参数
    auto diffuseR = integration.getParameter("rendering.material.diffuse.r");
    std::cout << "Diffuse R: " << std::get<double>(diffuseR) << std::endl;
    
    return 0;
}
```

### 批量参数操作

```cpp
// 批量设置参数
std::unordered_map<std::string, ParameterValue> batchParams;
batchParams["geometry.position.x"] = 10.0;
batchParams["geometry.position.y"] = 20.0;
batchParams["geometry.position.z"] = 30.0;
batchParams["rendering.material.transparency"] = 0.5;

integration.setParameters(batchParams);

// 批量获取参数
std::vector<std::string> paramPaths = {
    "geometry.position.x", "geometry.position.y", "geometry.position.z"
};
auto values = integration.getParameters(paramPaths);
```

### 智能批量更新

```cpp
// 快速连续设置多个参数（会被自动批量处理）
for (int i = 0; i < 100; ++i) {
    integration.scheduleParameterChange(
        "rendering.material.diffuse.r", 
        0.5 + i * 0.001, 
        0.5 + (i + 1) * 0.001
    );
}

// 系统会自动将这些变更合并为批量操作
```

### 预设管理

```cpp
// 保存当前状态为预设
integration.saveCurrentStateAsPreset("my_preset");

// 修改参数
integration.setParameter("rendering.material.diffuse.r", 1.0);

// 加载预设
integration.loadPreset("my_preset");

// 参数会恢复到保存时的状态
```

### 参数依赖关系

```cpp
// 添加参数依赖
integration.addParameterDependency(
    "rendering.material.transparency", 
    "rendering.material.diffuse.r"
);

// 当diffuse.r改变时，transparency会自动更新
integration.setParameter("rendering.material.diffuse.r", 0.5);
```

## 参数路径规范

参数路径使用点分隔的层次结构：

```
系统类型.分组.子分组.参数名
```

### 预定义路径

#### 几何参数 (geometry.*)
- `geometry.position.x/y/z` - 位置坐标
- `geometry.rotation.x/y/z` - 旋转角度
- `geometry.scale.x/y/z` - 缩放比例
- `geometry.visible` - 可见性
- `geometry.selected` - 选中状态

#### 渲染参数 (rendering.*)
- `rendering.material.ambient.r/g/b` - 环境光颜色
- `rendering.material.diffuse.r/g/b` - 漫反射颜色
- `rendering.material.specular.r/g/b` - 镜面反射颜色
- `rendering.material.shininess` - 光泽度
- `rendering.material.transparency` - 透明度
- `rendering.display.showEdges` - 显示边线
- `rendering.display.showVertices` - 显示顶点

#### 网格参数 (mesh.*)
- `mesh.deflection` - 网格偏差
- `mesh.angularDeflection` - 角度偏差
- `mesh.relative` - 相对偏差
- `mesh.inParallel` - 并行计算

#### 光照参数 (lighting.*)
- `lighting.main.enabled` - 主光源启用
- `lighting.main.type` - 光源类型
- `lighting.main.position.x/y/z` - 光源位置
- `lighting.main.direction.x/y/z` - 光源方向
- `lighting.main.color.r/g/b` - 光源颜色
- `lighting.main.intensity` - 光源强度

## 性能优化

### 1. 智能批量处理
系统会自动检测连续的参数变更，并将其合并为批量操作：

```cpp
// 这些操作会被自动批量处理
integration.setParameter("rendering.material.diffuse.r", 0.8);
integration.setParameter("rendering.material.diffuse.g", 0.6);
integration.setParameter("rendering.material.diffuse.b", 0.4);
```

### 2. 更新策略
支持多种更新策略以优化性能：

```cpp
// 立即更新（高优先级）
coordinator.submitParameterChange(path, oldVal, newVal, UpdateStrategy::IMMEDIATE);

// 批量更新（默认）
coordinator.submitParameterChange(path, oldVal, newVal, UpdateStrategy::BATCHED);

// 节流更新
coordinator.submitParameterChange(path, oldVal, newVal, UpdateStrategy::THROTTLED);

// 延迟更新
coordinator.submitParameterChange(path, oldVal, newVal, UpdateStrategy::DEFERRED);
```

### 3. 性能监控
提供详细的性能统计信息：

```cpp
auto report = integration.getPerformanceReport();
std::cout << "Total parameters: " << report.totalParameters << std::endl;
std::cout << "Executed updates: " << report.executedUpdates << std::endl;
std::cout << "Average update time: " << report.averageUpdateTime.count() << "ms" << std::endl;
```

## 错误处理

系统提供完善的错误处理机制：

```cpp
// 参数验证
bool isValid = integration.validateAllParameters();
if (!isValid) {
    auto errors = integration.getValidationErrors();
    for (const auto& error : errors) {
        std::cerr << "Validation error: " << error << std::endl;
    }
}

// 系统诊断
auto diagnostics = integration.getSystemDiagnostics();
std::cout << diagnostics << std::endl;
```

## 编译和构建

### 依赖项
- C++17 或更高版本
- OpenCASCADE
- wxWidgets
- Coin3D (Inventor)

### CMake 配置

```cmake
# 设置C++标准
set(CMAKE_CXX_STANDARD 17)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 创建库
add_library(UnifiedParameterSystem STATIC ${UNIFIED_PARAM_SOURCES})

# 链接库
target_link_libraries(UnifiedParameterSystem
    ${OpenCASCADE_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    ${Inventor_LIBRARIES}
)
```

### 编译命令

```bash
mkdir build
cd build
cmake ..
make UnifiedParameterSystem
```

## 测试

系统包含完整的测试套件：

```bash
# 运行基本示例
./UnifiedParameterExample

# 运行性能测试
./UnifiedParameterTest
```

## 扩展性

### 添加新的参数系统

1. 创建系统适配器：

```cpp
class MySystemAdapter : public ParameterSystemAdapter {
public:
    SystemType getSystemType() const override { return SystemType::MY_SYSTEM; }
    std::string getSystemName() const override { return "MySystem"; }
    // ... 实现其他方法
};
```

2. 注册到参数注册表：

```cpp
auto& registry = ParameterRegistry::getInstance();
registry.registerParameterSystem(SystemType::MY_SYSTEM, mySystemTree);
```

3. 集成到统一管理器：

```cpp
auto& integration = UnifiedParameterIntegration::getInstance();
integration.integrateMySystem(&mySystem);
```

### 自定义参数类型

```cpp
// 使用std::any支持任意类型
ParameterValue customValue = std::any(MyCustomType{});
tree->setParameterValue("custom.parameter", customValue);
```

## 最佳实践

### 1. 参数命名
- 使用有意义的参数名称
- 遵循层次化命名规范
- 避免过深的嵌套层级

### 2. 性能考虑
- 使用批量操作处理大量参数变更
- 合理设置更新策略
- 定期监控性能指标

### 3. 错误处理
- 始终检查操作返回值
- 使用参数验证功能
- 实现适当的错误恢复机制

### 4. 内存管理
- 及时清理不需要的参数
- 使用智能指针管理资源
- 避免参数泄漏

## 故障排除

### 常见问题

1. **参数设置失败**
   - 检查参数路径是否正确
   - 验证参数值是否在有效范围内
   - 确认参数类型是否匹配

2. **性能问题**
   - 检查是否启用了智能批量处理
   - 调整批量处理超时时间
   - 监控系统性能指标

3. **同步问题**
   - 确认双向同步已启用
   - 检查系统集成状态
   - 验证桥接器实现

### 调试技巧

1. **启用详细日志**
```cpp
// 在Logger中启用DEBUG级别
Logger::setLevel(LogLevel::DEBUG);
```

2. **使用性能监控**
```cpp
auto report = integration.getPerformanceReport();
// 分析性能瓶颈
```

3. **参数验证**
```cpp
bool isValid = integration.validateAllParameters();
if (!isValid) {
    auto errors = integration.getValidationErrors();
    // 分析验证错误
}
```

## 版本历史

### v1.0.0 (当前版本)
- 初始版本发布
- 完整的统一参数树系统
- 智能批量更新机制
- 现有系统集成支持
- 性能监控和优化

## 贡献指南

欢迎贡献代码和建议！请遵循以下步骤：

1. Fork 项目
2. 创建功能分支
3. 提交更改
4. 创建 Pull Request

## 许可证

本项目采用 MIT 许可证。详情请参阅 LICENSE 文件。

## 联系方式

如有问题或建议，请通过以下方式联系：

- 项目主页: [GitHub Repository]
- 问题报告: [GitHub Issues]
- 邮箱: [Contact Email]

---

*最后更新: 2024年*