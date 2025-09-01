# 进度条 100% 后不消失问题修复

## 问题描述
导入几何完成后，进度条显示 100% 但不会自动消失。

## 问题原因
之前的修复移除了立即隐藏进度条的代码，期望依赖 FlatFrame 的定时器管理。但是 FlatFrame 的 `m_progressTimer` 只处理特征边生成的进度，不会处理导入操作的进度条。

## 修复方案

### 1. 成功导入后延迟隐藏
```cpp
if (statusBar) { 
    statusBar->SetGaugeValue(100); 
    // 使用定时器延迟隐藏
    wxTimer* hideTimer = new wxTimer();
    hideTimer->Bind(wxEVT_TIMER, [statusBar, hideTimer](wxTimerEvent&) {
        if (statusBar) {
            statusBar->EnableProgressGauge(false);
        }
        hideTimer->Stop();
        delete hideTimer;
    });
    hideTimer->StartOnce(1000); // 1秒后隐藏
}
```

### 2. 错误/警告/取消时立即隐藏
- 导入取消：立即隐藏
- 无效几何：立即隐藏
- 导入错误：立即隐藏

## 效果
1. **成功导入**：进度条显示 100% 持续 1 秒，然后自动消失
2. **失败情况**：进度条立即消失，避免残留
3. **用户体验**：用户能看到完成状态，但不会一直显示

## 实现细节
- 使用 `wxTimer::StartOnce(1000)` 实现一次性延迟
- 在 lambda 中捕获定时器指针并在回调后删除
- 添加日志记录隐藏操作