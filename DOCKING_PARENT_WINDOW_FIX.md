# 修复停靠时的父窗口断言错误

## 问题描述

在拖动到 DockArea 的方向指示器上停靠时，出现以下断言错误：
```
assert "window1->GetParent() == this && window2->GetParent() == this" 
failed in wxSplitterWindow::DoSplit(): windows in the splitter should have it as parent!
```

## 根本原因

wxWidgets 的 `wxSplitterWindow` 要求在调用 `SplitHorizontally` 或 `SplitVertically` 时，两个子窗口必须已经是分割器的子窗口。

原代码的问题：
1. 创建了新的子分割器
2. 立即尝试分割，但此时窗口的父窗口仍然是旧的分割器
3. 之后才替换窗口

## 解决方案

调整操作顺序：

### 1. 先替换窗口
```cpp
// 先在父分割器中替换窗口
if (targetIsWindow1) {
    parentSplitter->ReplaceWindow(window1, subSplitter);
} else {
    parentSplitter->ReplaceWindow(window2, subSplitter);
}
```

### 2. 重新设置父窗口
```cpp
// 将窗口的父窗口设置为新的子分割器
targetArea->Reparent(subSplitter);
newArea->Reparent(subSplitter);
```

### 3. 执行分割
```cpp
// 现在可以安全地执行分割
switch (area) {
case TopDockWidgetArea:
    subSplitter->SplitHorizontally(newArea, targetArea);
    break;
// ... 其他方向
}
```

## 其他改进

### 1. 分割条位置设置
- 对于顶部和左侧停靠：直接设置配置的大小
- 对于底部和右侧停靠：需要在布局后计算正确位置

### 2. 延迟设置分割条位置
```cpp
if (area == BottomDockWidgetArea || area == RightDockWidgetArea) {
    // 强制布局以获取正确的尺寸
    subSplitter->Layout();
    wxSize size = subSplitter->GetSize();
    
    // 根据实际尺寸设置分割条位置
    if (area == BottomDockWidgetArea && size.GetHeight() > 0) {
        subSplitter->SetSashPosition(size.GetHeight() - getConfiguredAreaSize(area));
    }
}
```

## 操作流程

正确的操作流程：
1. 创建新的 DockArea（父窗口为 DockContainerWidget）
2. 创建新的子分割器（父窗口为现有的父分割器）
3. 在父分割器中用子分割器替换目标区域
4. 将目标区域和新区域的父窗口改为子分割器
5. 执行分割操作
6. 设置分割条位置
7. 显示所有窗口并更新布局

## 测试验证

修复后应该测试：
1. 在各个方向上停靠
2. 在复杂布局中停靠
3. 验证分割条位置正确
4. 确认没有断言错误