# Docking 布局最终修复总结

## 问题描述

在创建五区域布局时，左侧 Toolbox 显示了，但中间的 Main View 和右侧的 Properties 没有显示。

## 问题原因

1. **wxSplitter 的 ReplaceWindow 限制**：在某些情况下，ReplaceWindow 方法可能不会正确更新子窗口的显示状态。

2. **窗口 reparent 时机**：在 splitter 已经包含窗口的情况下直接 reparent 可能导致显示问题。

3. **布局更新不完整**：仅调用 UpdateSize 可能不足以触发完整的布局更新。

## 解决方案

采用更可靠的窗口管理方法：

```cpp
// 1. 先 unsplit 清空 splitter
rootSplitter->Unsplit();

// 2. 重新组织窗口结构
window1->Reparent(rootSplitter);  // Left 保留在根
window2->Reparent(subSplitter);   // Center 移到子splitter
dockArea->Reparent(subSplitter);  // Right 添加到子splitter

// 3. 重新设置分割
subSplitter->SplitVertically(window2, dockArea);
rootSplitter->SplitVertically(window1, subSplitter);

// 4. 确保所有窗口显示
window1->Show();
window2->Show();
dockArea->Show();
subSplitter->Show();

// 5. 强制布局更新
parent->Layout();
parent->Refresh();
```

## 关键改进

1. **使用 Unsplit**：先清空 splitter，避免在已有窗口的情况下进行复杂操作。

2. **显式调用 Show()**：确保所有窗口都被显式设置为可见。

3. **完整的布局更新**：调用 Layout() 和 Refresh() 确保视觉更新。

4. **保存和恢复 sash 位置**：维持用户期望的面板大小。

## 布局结构

成功创建的布局结构应该是：
```
Root Splitter (Vertical)
├── Window1: Left (Toolbox)
└── Sub-Splitter (Vertical)
    ├── Window1: Center (Main View)
    └── Window2: Right (Properties)
```

## 调试提示

如果还有问题，可以通过以下方式调试：

1. 检查调试日志中的窗口类型信息
2. 验证每个窗口的 IsShown() 状态
3. 检查每个 splitter 的 IsSplit() 状态
4. 使用 wxWindow::GetChildren() 遍历窗口树

## 注意事项

1. wxSplitter 只能包含两个子窗口，所以三向分割需要嵌套 splitter。
2. Reparent 操作应该在窗口从原父窗口移除后进行。
3. 布局更新可能需要延迟到下一个事件循环才能完全生效。