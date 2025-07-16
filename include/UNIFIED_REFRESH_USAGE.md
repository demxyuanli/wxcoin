# 统一刷新系统使用指南

## 概述

统一刷新系统 (UnifiedRefreshSystem) 提供了一个统一的界面来管理应用程序中的所有刷新操作。它通过命令模式集成，支持撤销/重做操作，并提供性能优化的刷新管理。

## 架构说明

新的架构使用全局服务管理器 (GlobalServices) 来访问统一刷新系统，避免了对MainApplication.h的直接依赖：

```
UI Component → GlobalServices → UnifiedRefreshSystem → CommandDispatcher → RefreshCommand → ViewRefreshManager
```

## 基本使用

### 1. 包含必要的头文件

```cpp
#include "GlobalServices.h"
#include "UnifiedRefreshSystem.h"  // 可选，如果需要直接访问类定义
```

### 2. 获取统一刷新系统实例

```cpp
UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
if (refreshSystem) {
    // 使用刷新系统
    refreshSystem->refreshView("", false);
}
```

## 刷新类型和使用场景

### refreshView() - 视图刷新
用于更新3D视图显示，通常在选择状态改变或相机移动后使用。

```cpp
UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
if (refreshSystem) {
    // 刷新整个视图
    refreshSystem->refreshView("", false);
    
    // 刷新特定对象的视图显示
    refreshSystem->refreshView("objectId", false);
}
```

### refreshMaterial() - 材质刷新
用于更新对象的材质、颜色、透明度等属性显示。

```cpp
UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
if (refreshSystem) {
    // 立即刷新所有对象的材质
    refreshSystem->refreshMaterial("", true);
    
    // 普通刷新特定对象的材质
    refreshSystem->refreshMaterial("objectId", false);
}
```

### refreshGeometry() - 几何体刷新
用于在几何体被创建、修改或删除后刷新显示。

```cpp
UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
if (refreshSystem) {
    // 立即刷新新创建的几何体
    refreshSystem->refreshGeometry("", true);
}
```

### refreshScene() - 场景刷新
用于更新整个场景的边界、布局等信息。

```cpp
UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
if (refreshSystem) {
    // 更新场景边界（通常在创建新几何体后）
    refreshSystem->refreshScene("", false);
}
```

### refreshUI() - 界面刷新
用于更新UI组件状态。

```cpp
UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
if (refreshSystem) {
    refreshSystem->refreshUI("componentType", false);
}
```

## 具体对话框使用示例

### RenderingSettingsDialog (渲染设置对话框)

```cpp
void RenderingSettingsDialog::applySettings() {
    // 应用渲染设置...
    
    // 使用统一刷新系统
    UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
    if (refreshSystem) {
        refreshSystem->refreshMaterial("", true);  // 立即刷新材质
        refreshSystem->refreshView("", false);     // 普通视图刷新
    } else {
        // 降级处理
        m_occViewer->requestViewRefresh();
    }
}
```

### TransparencyDialog (透明度对话框)

```cpp
void TransparencyDialog::applyTransparency() {
    // 设置透明度...
    
    UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
    if (refreshSystem) {
        // 为每个选中的几何体刷新材质
        for (const auto& geometry : selectedGeometries) {
            refreshSystem->refreshMaterial(geometry->getName(), true);
        }
        // 刷新视图
        refreshSystem->refreshView("", false);
    }
}
```

### PositionDialog (位置对话框)

```cpp
void PositionDialog::createGeometry() {
    // 创建几何体...
    
    UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
    if (refreshSystem) {
        refreshSystem->refreshGeometry("", true);  // 立即刷新新几何体
        refreshSystem->refreshScene("", false);   // 更新场景边界
    }
}
```

### ObjectTreePanel (对象树面板)

```cpp
void ObjectTreePanel::onSelectionChanged() {
    // 更新选择状态...
    
    UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
    if (refreshSystem) {
        refreshSystem->refreshView(geometry->getName(), false);
    }
}
```

### MeshQualityDialog (网格质量对话框)

```cpp
void MeshQualityDialog::applyQualitySettings() {
    // 应用网格质量设置...
    
    UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
    if (refreshSystem) {
        refreshSystem->refreshGeometry("", true);  // 立即刷新网格
        refreshSystem->refreshView("", false);     // 普通视图刷新
    }
}
```

## 最佳实践

### 1. immediate 参数的使用
- `true`: 立即刷新，用于需要即时反馈的操作（如材质更改、几何体创建）
- `false`: 延迟刷新，允许批量优化（如视图更新、选择更改）

### 2. objectId 参数的使用
- 空字符串 `""`: 影响所有对象或整个场景
- 具体ID: 只影响特定对象，提供更精确的控制

### 3. 错误处理
始终检查 GlobalServices::GetRefreshSystem() 的返回值，并提供降级处理：

```cpp
UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
if (refreshSystem) {
    refreshSystem->refreshView("", false);
} else {
    // 降级到直接刷新
    m_occViewer->requestViewRefresh();
}
```

### 4. 避免过度刷新
- 在一系列相关操作后只调用一次刷新
- 使用适当的刷新类型，避免不必要的全场景刷新

## 迁移指南

### 从旧的canvas->getUnifiedRefreshSystem()迁移

旧代码：
```cpp
if (canvas && canvas->getUnifiedRefreshSystem()) {
    canvas->getUnifiedRefreshSystem()->refreshView("", false);
}
```

新代码：
```cpp
UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
if (refreshSystem) {
    refreshSystem->refreshView("", false);
}
```

### 优势
1. **减少依赖**: 不需要访问Canvas实例
2. **简化代码**: 减少了复杂的父窗口查找逻辑
3. **更好的封装**: 通过全局服务访问，避免了对MainApplication的直接依赖
4. **一致性**: 所有组件都使用相同的访问模式 