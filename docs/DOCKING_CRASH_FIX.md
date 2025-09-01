# Docking 崩溃修复

## 问题描述

程序在 `DockArea::setCurrentIndex` 调用 `Hide()` 时崩溃，错误信息：
- 异常：读取访问权限冲突
- this 指针值：0xFFFFFFFFFFFFFE37（无效地址）

## 问题原因

1. `m_currentDockWidget` 可能指向一个已经被删除或无效的 widget
2. 在第一次添加 widget 时，可能存在初始化顺序问题
3. 缺少对 widget 有效性的检查

## 修复内容

### 1. 增强 `setCurrentIndex` 的安全性

```cpp
// 修复前：直接调用 Hide()
if (m_currentDockWidget) {
    m_currentDockWidget->Hide();
}

// 修复后：验证 widget 有效性
if (m_currentDockWidget && m_currentDockWidget->GetParent()) {
    // 确保 widget 仍在列表中
    if (std::find(m_dockWidgets.begin(), m_dockWidgets.end(), m_currentDockWidget) != m_dockWidgets.end()) {
        m_currentDockWidget->Hide();
    }
}
```

### 2. 改进 widget 激活逻辑

```cpp
// 确保第一个 widget 被自动激活
if (activate || m_dockWidgets.size() == 1) {
    setCurrentIndex(index);
} else if (m_currentIndex < 0 && !m_dockWidgets.empty()) {
    // 如果没有活动 widget，激活第一个
    setCurrentIndex(0);
}
```

## 关键改进

1. **有效性检查**：在调用 widget 方法前验证其有效性
2. **父窗口检查**：确保 widget 有有效的父窗口
3. **列表验证**：确保 widget 仍在管理列表中
4. **自动激活**：确保至少有一个 widget 处于激活状态

## 调试建议

如果问题仍然存在，可以：

1. 在 `DockWidget` 析构函数中通知 `DockArea` 移除引用
2. 使用智能指针管理 widget 生命周期
3. 添加更多的断言检查 widget 状态

## 注意事项

1. wxWidgets 的父子窗口关系管理很重要
2. 在 reparent 操作后要确保窗口状态正确
3. 删除窗口前要清理所有引用