# 导入几何后程序卡住 - 完整修复方案

## 问题诊断

从日志分析发现：
1. 进度更新被延迟执行（`CallAfter` 的回调在导入完成后才执行）
2. 鼠标点击后程序卡住
3. 这表明主线程在导入期间被阻塞

## 根本原因

`STEPReader::readSTEPFile` 在主线程执行，阻塞了 wxWidgets 的事件循环：
- `CallAfter` 的回调无法及时执行
- UI 无法响应
- 事件积压导致后续操作异常

## 修复方案

### 1. 检测线程并相应处理
```cpp
if (wxThread::IsMain()) {
    // 主线程：直接更新 UI 并处理事件
    statusBar->SetGaugeValue(mapped);
    wxTheApp->ProcessPendingEvents();
} else {
    // 工作线程：使用 CallAfter
    wxTheApp->CallAfter(...);
}
```

### 2. 定期处理事件
- 导入前：`wxTheApp->ProcessPendingEvents()`
- 进度回调中：如果在主线程，调用 `ProcessPendingEvents()`
- 导入后：再次处理积压事件

### 3. 保持 UI 响应
通过在关键点调用 `ProcessPendingEvents()`，确保：
- 进度条实时更新
- UI 保持响应
- 避免事件积压

## 实施效果

1. **实时进度更新**：进度条在导入过程中实时更新
2. **UI 保持响应**：导入期间窗口不会冻结
3. **避免卡死**：导入完成后程序正常响应鼠标操作

## 注意事项

- `ProcessPendingEvents()` 只在主线程调用
- 避免过于频繁调用，可能影响性能
- 确保关键操作前后处理事件