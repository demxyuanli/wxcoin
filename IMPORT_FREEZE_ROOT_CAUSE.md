# 导入几何后程序卡住的根本原因分析

## 日志分析

### 1. 进度更新时序问题
```
[20:35:46] [DBG] STEP import progress: 3% - initialize
[20:35:47] [DBG] STEP import progress: 50% - done
...
[20:35:47] [DBG] Setting gauge value to: 3  // 延迟执行
[20:35:47] [DBG] Setting gauge value to: 50
```

**问题**：进度回调立即记录，但UI更新被延迟到导入完成后才执行。

### 2. 鼠标事件后卡住
```
[20:36:15] [INF] Mouse button event - Mode: 0, LeftDown: 1
// 程序卡住
```

## 根本原因

### 1. 主线程阻塞
`STEPReader::readSTEPFile` 可能在主线程执行，阻塞了事件循环：
- `CallAfter` 的回调无法执行（需要事件循环）
- 进度更新被延迟
- 导入完成后，积压的事件可能导致问题

### 2. OpenGL 上下文问题
导入后的几何渲染可能影响了 OpenGL 上下文，导致后续渲染操作卡住。

### 3. 事件处理冲突
积压的 `CallAfter` 事件在导入完成后集中执行，可能与正常的事件处理冲突。

## 解决方案

### 方案1：确保导入在后台线程执行
```cpp
std::thread importThread([&]() {
    auto result = STEPReader::readSTEPFile(...);
    // 处理结果
});
importThread.join();
```

### 方案2：使用 wxYield 处理事件
在导入过程中定期调用 `wxYield()` 让事件循环运行。

### 方案3：改进进度更新机制
不使用 `CallAfter`，而是使用更直接的机制。