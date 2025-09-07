# Docking 配置编译错误修复

## 问题描述

编译时出现以下错误：
1. `error C2027: 使用了未定义类型"ads::DockManager::DockLayoutConfig"`
2. `error C2039: "LoadFromConfig": 不是 "std::unique_ptr" 的成员`
3. `error C2556: 重载函数与"ads::DockManager::getLayoutConfig"只是在返回类型上不同`

## 根本原因

1. 编译器将 `DockLayoutConfig` 误认为是 `DockManager` 的内部类
2. 在头文件中只有前向声明，但方法返回值类型需要完整定义
3. `unique_ptr` 的使用方式不正确

## 修复方案

### 1. 添加前向声明
在 `DockManager.h` 中添加了 `DockLayoutConfig` 的前向声明：
```cpp
namespace ads {
    // Forward declarations
    class DockWidget;
    class DockArea;
    class DockSplitter;
    class FloatingDockContainer;
    class DockOverlay;
    class DockLayoutConfig;  // 添加这一行
```

### 2. 修改返回类型为引用
将 `getLayoutConfig()` 的返回类型从值改为 const 引用：
```cpp
// 原来：
DockLayoutConfig getLayoutConfig() const;

// 修改为：
const DockLayoutConfig& getLayoutConfig() const;
```

### 3. 更新实现
```cpp
const DockLayoutConfig& DockManager::getLayoutConfig() const {
    if (!m_layoutConfig) {
        // 如果未初始化，创建默认配置
        const_cast<DockManager*>(this)->m_layoutConfig = std::make_unique<DockLayoutConfig>();
    }
    return *m_layoutConfig;
}
```

### 4. 更新使用处
在 `SimpleDockingFrame.cpp` 中：
```cpp
void OnConfigureLayout(wxCommandEvent&) {
    const DockLayoutConfig& currentConfig = m_dockManager->getLayoutConfig();
    DockLayoutConfig config = currentConfig;  // 制作副本
    DockLayoutConfigDialog dlg(this, config);
    // ...
}
```

在 `DockContainerWidget.cpp` 中：
```cpp
const DockLayoutConfig& config = m_dockManager->getLayoutConfig();
```

## 优点

1. 避免了需要在头文件中包含完整的 `DockLayoutConfig.h`
2. 减少了不必要的对象复制
3. 保持了 API 的简洁性
4. 解决了所有编译错误

## 注意事项

- 返回引用意味着调用者不应该保存这个引用太久，因为对象的生命周期由 `DockManager` 控制
- 如果需要修改配置，应该先复制一份，修改后再通过 `setLayoutConfig` 设置回去