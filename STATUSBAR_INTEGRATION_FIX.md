# 状态栏集成修复

## 问题描述
在新的 docking 系统中，原有的自定义状态栏（`FlatUIStatusBar`）没有被正确集成，而是创建了一个标准的 `wxStatusBar`。

## 问题原因
1. `FlatFrameDocking::InitializeDockingLayout` 使用了 `CreateStatusBar(3)` 创建标准状态栏
2. 基类 `FlatFrame` 使用的是自定义的 `FlatUIStatusBar`，通过 `addStatusBar()` 创建
3. 这导致了：
   - 丢失了自定义状态栏的样式和功能
   - 进度条功能无法正常工作
   - 状态栏的外观与整体 UI 风格不一致

## 修复方案
将 `FlatFrameDocking::InitializeDockingLayout` 中的状态栏创建代码改为使用基类的方法：

### 修改前：
```cpp
// Create status bar (at the bottom) - it will be added automatically to the frame
CreateStatusBar(3);
SetStatusText("Ready - Docking Layout Active", 0);
```

### 修改后：
```cpp
// Use the same status bar creation as base class
addStatusBar();
if (auto* bar = GetFlatUIStatusBar()) {
    bar->SetFieldsCount(3);
    bar->SetStatusText("Ready - Docking Layout Active", 0);
    bar->EnableProgressGauge(false);
    bar->SetGaugeRange(100);
    bar->SetGaugeValue(0);
}
```

## 效果
1. 恢复了原有的自定义状态栏样式
2. 保持了与整体 FlatUI 风格的一致性
3. 恢复了进度条功能（用于显示特征边生成进度等）
4. 状态栏正确集成到新的布局系统中

## 相关组件
- `FlatUIStatusBar`: 自定义状态栏类，提供了进度条集成和自定义样式
- `addStatusBar()`: 基类方法，负责创建和配置自定义状态栏
- `GetFlatUIStatusBar()`: 获取自定义状态栏实例的方法