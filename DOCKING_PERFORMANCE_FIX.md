# Docking 性能问题修复方案

## 问题根源

### 1. 标签栏的过度刷新
- `DockAreaTabBar::onMouseMotion` 在鼠标移动时频繁调用 `Refresh()`
- `DockAreaMergedTitleBar::onMouseMotion` 同样频繁刷新

### 2. 事件冒泡
即使鼠标在 Canvas 上，移动事件可能冒泡到父窗口的标签栏

## 解决方案

### 1. 优化刷新策略
将完整的 `Refresh()` 替换为局部刷新 `RefreshRect()`

### 2. 添加事件过滤
防止不必要的事件处理

### 3. 延迟刷新
使用定时器批量处理刷新请求

## 具体修改

### 修改 1：优化标签栏鼠标移动处理
```cpp
// 在 DockAreaTabBar::onMouseMotion
if (oldHovered != m_hoveredTab) {
    // 只刷新受影响的标签
    if (oldHovered >= 0 && oldHovered < m_tabs.size()) {
        RefreshRect(m_tabs[oldHovered].rect);
    }
    if (m_hoveredTab >= 0 && m_hoveredTab < m_tabs.size()) {
        RefreshRect(m_tabs[m_hoveredTab].rect);
    }
}
```

### 修改 2：防止事件冒泡
```cpp
// 在 Canvas 的鼠标事件处理中
void Canvas::onMouseEvent(wxMouseEvent& event) {
    // 处理事件
    ...
    // 不要调用 event.Skip() 除非必要
}
```

### 修改 3：检查事件来源
```cpp
// 在标签栏的鼠标事件处理中
if (event.GetEventObject() != this) {
    event.Skip();
    return;
}
```