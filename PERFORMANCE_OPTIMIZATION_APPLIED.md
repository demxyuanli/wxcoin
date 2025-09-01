# 已应用的性能优化

## 问题诊断
通过深入分析发现，Canvas 鼠标操作卡顿的主要原因是：

1. **过度刷新**：DockArea 的标签栏和标题栏在每次鼠标移动时都调用 `Refresh()`，刷新整个控件
2. **事件冒泡**：Canvas 的鼠标事件通过 `event.Skip()` 传播到父窗口，触发了不必要的处理

## 应用的优化

### 1. 局部刷新代替全局刷新
**位置**：`DockArea.cpp` - `DockAreaTabBar::onMouseMotion` 和 `DockAreaMergedTitleBar::onMouseMotion`

**修改前**：
```cpp
if (oldHovered != m_hoveredTab) {
    Refresh();  // 刷新整个标签栏
}
```

**修改后**：
```cpp
if (oldHovered != m_hoveredTab) {
    // 只刷新受影响的标签区域
    if (oldHovered >= 0 && oldHovered < static_cast<int>(m_tabs.size())) {
        RefreshRect(m_tabs[oldHovered].rect);
    }
    if (m_hoveredTab >= 0 && m_hoveredTab < static_cast<int>(m_tabs.size())) {
        RefreshRect(m_tabs[m_hoveredTab].rect);
    }
}
```

### 2. 事件源检查
**位置**：`DockArea.cpp` - 鼠标移动事件处理函数

**添加的代码**：
```cpp
// 只处理来自自身的事件
if (event.GetEventObject() != this) {
    event.Skip();
    return;
}
```

### 3. 移除 onSize 中的 Refresh
**位置**：`FlatFrameDocking.cpp` - `onSize` 事件处理

**理由**：Docking 系统会自行处理布局更新，不需要强制刷新

## 效果

这些优化应该能够：
1. **减少重绘次数**：从全控件刷新改为局部刷新
2. **避免事件干扰**：防止 Canvas 的鼠标事件触发标签栏的更新
3. **提升响应速度**：减少不必要的渲染开销

## 进一步优化建议

如果性能问题仍然存在，可以考虑：
1. **实现脏区域管理**：跟踪需要更新的区域，批量处理
2. **使用双缓冲**：减少闪烁
3. **优化 OpenGL 渲染**：检查 Canvas 的渲染管线
4. **性能分析**：使用工具定位具体的性能瓶颈