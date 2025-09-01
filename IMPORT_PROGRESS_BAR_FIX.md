# 导入几何进度条显示问题修复

## 问题描述
导入STEP文件时，进度条没有在状态栏上正确显示。

## 问题分析

### 1. 状态栏获取问题
- `ImportStepListener` 通过 `GetFlatUIStatusBar()` 获取状态栏
- 在 `FlatFrameDocking` 中，状态栏可能没有被正确初始化或暴露

### 2. 线程安全问题
- `STEPReader::readSTEPFile` 使用多线程处理
- 进度回调可能从工作线程调用
- 直接在工作线程更新UI会导致问题

### 3. UI刷新问题
- 即使设置了进度值，UI可能没有及时刷新

## 修复方案

### 1. 添加调试日志
```cpp
if (statusBar) {
    LOG_INF_S("FlatUIStatusBar found, enabling progress gauge");
    statusBar->SetGaugeRange(100);
    statusBar->SetGaugeValue(0);
    statusBar->EnableProgressGauge(true);
    statusBar->Refresh();
    statusBar->Update();
} else {
    LOG_WRN_S("FlatUIStatusBar not found!");
}
```

### 2. 使用线程安全的UI更新
```cpp
// 使用 CallAfter 确保UI更新在主线程执行
wxTheApp->CallAfter([statusBar, flatFrame, mapped, stage]() {
    if (statusBar) {
        statusBar->SetGaugeValue(mapped);
        statusBar->Update();
    }
    if (flatFrame) {
        flatFrame->appendMessage(wxString::Format("[%d%%] Import stage: %s", mapped, stage));
    }
});
```

### 3. 强制UI刷新
- 添加 `statusBar->Update()` 确保进度条立即更新
- 在初始化时添加 `Refresh()` 和 `Update()`

## 修复效果
- 进度条更新现在在主线程执行，避免线程安全问题
- 添加了调试日志，方便诊断状态栏是否正确获取
- 强制UI刷新，确保进度条实时更新

## 后续建议
如果问题仍然存在，可以：
1. 检查日志确认状态栏是否被正确获取
2. 确认 `FlatUIStatusBar` 的进度条组件是否正确初始化
3. 检查进度条是否被其他UI元素遮挡