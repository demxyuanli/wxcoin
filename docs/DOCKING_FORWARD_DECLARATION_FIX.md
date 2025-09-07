# Docking 前向声明错误修复

## 问题描述

编译时出现以下错误：
```
DockLayoutConfig.h(91,22): error C2143: 语法错误: 缺少";"(在"*"的前面)
DockLayoutConfig.h(91,5): error C4430: 缺少类型说明符 - 假定为 int
DockLayoutConfig.cpp: error C2065: "m_previewPanel": 未声明的标识符
```

## 根本原因

在 `DockLayoutConfig.h` 中，`DockLayoutConfigDialog` 类使用了 `DockLayoutPreview*` 类型的成员变量：
```cpp
DockLayoutPreview* m_previewPanel;  // 第91行
```

但是 `DockLayoutPreview` 类的定义在文件的后面（第118行），导致编译器在遇到这个声明时还不知道 `DockLayoutPreview` 是什么类型。

## 解决方案

在文件开头添加前向声明：

```cpp
namespace ads {

// Forward declarations
class DockLayoutPreview;

/**
 * @brief Configuration structure for dock layout
 */
struct DockLayoutConfig {
    // ...
};
```

## 为什么这样可以解决问题

1. **前向声明**告诉编译器 `DockLayoutPreview` 是一个类，稍后会定义
2. 对于**指针和引用**类型，编译器只需要知道类型存在即可，不需要完整定义
3. 这允许我们在 `DockLayoutConfigDialog` 中使用 `DockLayoutPreview*` 指针

## 最佳实践

1. 将相互依赖的类放在同一个头文件时，使用前向声明
2. 尽可能将类的定义按照依赖顺序排列
3. 对于只使用指针或引用的情况，优先使用前向声明而不是包含整个头文件

## 其他可能的解决方案

1. **调整类定义顺序**：将 `DockLayoutPreview` 的定义移到 `DockLayoutConfigDialog` 之前
2. **分离头文件**：将每个类放在独立的头文件中
3. **使用 pimpl 习惯用法**：将实现细节隐藏在实现文件中

在这个案例中，简单的前向声明是最合适的解决方案。