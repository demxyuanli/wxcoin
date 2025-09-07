# 鼠标捕获丢失错误修复

## 问题描述
```
assert ""Assert failure"" failed in DoNotifyWindowAboutCaptureLost(): 
window that captured the mouse didn't process wxEVT_MOUSE_CAPTURE_LOST
```

当窗口捕获鼠标后，如果失去捕获（例如另一个窗口获得焦点），需要处理`wxEVT_MOUSE_CAPTURE_LOST`事件。

## 修复方案

### 1. OutlinePreviewCanvas
已添加以下修复：

**事件表**：
```cpp
EVT_MOUSE_CAPTURE_LOST(OutlinePreviewCanvas::onMouseCaptureLost)
```

**事件处理**：
```cpp
void OutlinePreviewCanvas::onMouseCaptureLost(wxMouseCaptureLostEvent& event) {
    m_mouseDown = false;
    // 不需要调用 ReleaseMouse()，因为捕获已经丢失
}
```

### 2. 最佳实践

#### 捕获鼠标时：
```cpp
if (event.LeftDown()) {
    if (!HasCapture()) {
        CaptureMouse();
    }
}
```

#### 释放鼠标时：
```cpp
if (event.LeftUp()) {
    if (HasCapture()) {
        ReleaseMouse();
    }
}
```

#### 处理捕获丢失：
```cpp
void onMouseCaptureLost(wxMouseCaptureLostEvent& event) {
    // 重置所有鼠标相关状态
    m_mouseDown = false;
    m_dragging = false;
    // 不要调用 ReleaseMouse()
}
```

## 常见场景

1. **切换窗口**：用户按Alt+Tab切换到其他程序
2. **弹出对话框**：有模态对话框弹出
3. **系统事件**：系统级的鼠标捕获（如任务栏菜单）

## 调试建议

如果仍然出现断言错误：

1. 检查所有调用`CaptureMouse()`的地方
2. 确保每个捕获鼠标的窗口都处理了`wxEVT_MOUSE_CAPTURE_LOST`
3. 使用条件捕获：
   ```cpp
   if (!HasCapture()) {
       CaptureMouse();
   }
   ```

## 相关文档
- wxWidgets文档：[Mouse Capture](https://docs.wxwidgets.org/stable/classwx_window.html#a9fcadadc85c5df6c7c0c6017c2f0ad66)
- 事件处理：[wxMouseCaptureLostEvent](https://docs.wxwidgets.org/stable/classwx_mouse_capture_lost_event.html)