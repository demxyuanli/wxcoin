# 统一刷新系统使用指南

## 概述

统一刷新系统（UnifiedRefreshSystem）提供了一个基于命令模式的解耦刷新机制，用于替代分散在各个组件中的直接刷新调用。

## 架构优势

1. **解耦**: 组件不再直接调用其他组件的刷新方法
2. **统一**: 所有刷新操作通过命令系统进行
3. **可追踪**: 刷新操作通过日志系统记录
4. **灵活**: 支持立即刷新和延迟刷新
5. **向后兼容**: 保留直接刷新方法作为后备

## 初始化

```cpp
// 在主应用程序或主窗口中初始化
class MainApplication {
private:
    std::unique_ptr<UnifiedRefreshSystem> m_refreshSystem;
    std::unique_ptr<CommandDispatcher> m_commandDispatcher;
    
public:
    void initialize() {
        // 创建命令分发器
        m_commandDispatcher = std::make_unique<CommandDispatcher>();
        
        // 创建统一刷新系统
        m_refreshSystem = std::make_unique<UnifiedRefreshSystem>(
            m_canvas, m_occViewer, m_sceneManager);
        
        // 初始化刷新系统
        m_refreshSystem->initialize(m_commandDispatcher.get());
    }
};
```

## 使用方式

### 1. 通过统一刷新系统

```cpp
// 替代前: 直接调用刷新
m_canvas->Refresh();
m_occViewer->requestViewRefresh();

// 替代后: 使用统一刷新系统
m_refreshSystem->refreshView("", true); // 立即刷新整个视图
m_refreshSystem->refreshObject("Box1", false); // 延迟刷新特定对象
m_refreshSystem->refreshMaterial("Sphere1", true); // 立即刷新材质
```

### 2. 通过命令分发器

```cpp
// 使用命令分发器直接分发刷新命令
std::unordered_map<std::string, std::string> params;
params["objectId"] = "Cylinder1";
params["immediate"] = "true";
m_commandDispatcher->dispatchCommand(cmd::CommandType::RefreshGeometry, params);
```

### 3. 通过字符串命令

```cpp
// 使用字符串命令（适用于脚本或配置文件）
std::unordered_map<std::string, std::string> params;
params["componentType"] = "objecttree";
params["immediate"] = "false";
m_commandDispatcher->dispatchCommand("REFRESH_UI", params);
```

## 刷新类型

### RefreshView

- 刷新视口和视图相关内容
- 参数: objectId（可选）, immediate（布尔值）

### RefreshScene

- 刷新场景范围和3D内容
- 参数: objectId（可选）, immediate（布尔值）

### RefreshObject

- 刷新特定对象或所有对象
- 参数: objectId（必须，空字符串表示所有对象）, immediate（布尔值）

### RefreshMaterial

- 刷新材质属性
- 参数: objectId（可选）, immediate（布尔值）

### RefreshGeometry

- 刷新几何体网格
- 参数: objectId（可选）, immediate（布尔值）

### RefreshUI

- 刷新UI组件
- 参数: componentType（objecttree, properties等）, immediate（布尔值）

## 迁移示例

### 透明度对话框

```cpp
// 替代前:
void TransparencyDialog::onApply() {
    // 设置透明度...
    m_occViewer->requestViewRefresh();
    
    wxWindow* parent = GetParent();
    while (parent) {
        if (parent->GetName() == "Canvas") {
            parent->Refresh();
            break;
        }
        parent = parent->GetParent();
    }
}

// 替代后:
void TransparencyDialog::onApply() {
    // 设置透明度...
    if (m_refreshSystem) {
        m_refreshSystem->refreshMaterial("", true); // 立即刷新所有材质
    } else {
        // 后备方案
        m_occViewer->requestViewRefresh();
    }
}
```

### 渲染设置对话框

```cpp
// 替代前:
void RenderingSettingsDialog::applySettings() {
    // 应用设置...
    m_occViewer->requestViewRefresh();
}

// 替代后:
void RenderingSettingsDialog::applySettings() {
    // 应用设置...
    if (m_refreshSystem) {
        m_refreshSystem->refreshView("", true);
        m_refreshSystem->refreshMaterial("", false);
    } else {
        m_occViewer->requestViewRefresh();
    }
}
```

### 对象树面板

```cpp
// 替代前:
void ObjectTreePanel::onSelectionChanged() {
    // 更新选择...
    m_canvas->Refresh();
}

// 替代后:
void ObjectTreePanel::onSelectionChanged() {
    // 更新选择...
    if (m_refreshSystem) {
        m_refreshSystem->refreshUI("objecttree", false);
        m_refreshSystem->refreshView("", false);
    } else {
        m_canvas->Refresh();
    }
}
```

## 最佳实践

1. **优先使用统一刷新系统**: 新代码应该使用统一刷新系统
2. **保留后备方案**: 在系统未初始化时使用直接刷新
3. **选择合适的刷新类型**: 根据实际需要选择最精确的刷新类型
4. **使用延迟刷新**: 对于非关键操作使用延迟刷新以提高性能
5. **记录日志**: 系统会自动记录刷新操作，便于调试

## 性能考虑

- 延迟刷新使用防抖动机制，避免过度刷新
- 立即刷新用于用户交互需要即时反馈的场景
- 统一刷新系统会自动选择最优的刷新路径
- 失败时自动回退到直接刷新方式

## 调试

系统会记录以下日志信息：

- 刷新命令的创建和执行
- 命令分发的成功/失败状态
- 后备方案的使用
- 性能相关的防抖动信息

使用日志过滤器 "RefreshCommand" 或 "UnifiedRefreshSystem" 来查看相关日志。
