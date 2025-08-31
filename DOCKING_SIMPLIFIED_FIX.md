# Docking 布局简化修复方案

## 问题总结

左中右面板没有正确显示的原因是布局逻辑过于复杂，导致在创建过程中出现了错误。

## 修复方案

采用基于 widget 数量的简化方法，因为 `SimpleDockingFrame` 的添加顺序是固定的：
1. Center (Main View)
2. Left (Toolbox)
3. Right (Properties)
4. Top (Menu Bar)
5. Bottom (Output)

## 核心逻辑

```cpp
void DockContainerWidget::addDockAreaSimple(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area) {
    // 获取已存在的 widget 数量（不包括当前要添加的）
    int existingWidgetCount = m_dockAreas.size() - 1;
    
    if (existingWidgetCount == 0) {
        // 第一个 widget，直接初始化
        rootSplitter->Initialize(dockArea);
    }
    else if (existingWidgetCount == 1) {
        // 添加第二个 widget (Left)
        // 结构：[Left | Center]
        rootSplitter->SplitVertically(dockArea, window1);
    }
    else if (existingWidgetCount == 2) {
        // 添加第三个 widget (Right)
        // 结构：[Left | [Center | Right]]
        DockSplitter* subSplitter = new DockSplitter(rootSplitter);
        window2->Reparent(subSplitter);
        dockArea->Reparent(subSplitter);
        subSplitter->SplitVertically(window2, dockArea);
        rootSplitter->ReplaceWindow(window2, subSplitter);
    }
    else if (existingWidgetCount == 3) {
        // 添加第四个 widget (Top)
        // 重构为：[Top / [Left | Center | Right]]
        DockSplitter* middleSplitter = new DockSplitter(rootSplitter);
        // 移动现有内容到 middleSplitter
        // 然后重构 root 为水平分割
        rootSplitter->SplitHorizontally(dockArea, middleSplitter);
    }
    else if (existingWidgetCount == 4) {
        // 添加第五个 widget (Bottom)
        // 创建新的 root：[[Top / Middle] / Bottom]
        DockSplitter* newRoot = new DockSplitter(rootSplitter->GetParent());
        rootSplitter->Reparent(newRoot);
        dockArea->Reparent(newRoot);
        newRoot->SplitHorizontally(rootSplitter, dockArea);
        m_rootSplitter = newRoot;
    }
}
```

## 布局演变过程

1. **初始状态**：`[Center]`

2. **添加 Left**：
   ```
   [Left | Center]
   ```

3. **添加 Right**：
   ```
   [Left | [Center | Right]]
   ```

4. **添加 Top**：
   ```
   [Top]
   -----
   [Left | [Center | Right]]
   ```

5. **添加 Bottom**：
   ```
   [[Top]
    -----
    [Left | [Center | Right]]]
   -------------------------
   [Bottom]
   ```

## 关键点

1. **基于计数的逻辑**：根据已有 widget 数量决定如何添加新的 widget
2. **固定的添加顺序**：利用 SimpleDockingFrame 固定的添加顺序简化逻辑
3. **逐步构建**：每一步都基于前一步的结果，避免复杂的状态判断
4. **正确的父子关系**：确保 Reparent 操作在正确的时机进行

## 注意事项

1. `existingWidgetCount = m_dockAreas.size() - 1`：因为新 widget 已经被添加到列表中
2. SetSashPosition 可能需要在窗口完全显示后才能生效
3. 这个方案是针对 SimpleDockingFrame 的特定顺序优化的，对于其他顺序可能需要调整

## 测试建议

1. 编译并运行 SimpleDockingFrame
2. 检查是否所有五个区域都正确显示
3. 测试拖动分割线是否正常工作
4. 测试关闭和重新打开面板的功能