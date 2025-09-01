# 导入几何后程序冻结问题分析与修复

## 问题描述
导入STEP文件后，整个程序无法响应鼠标操作，看起来像是冻结了。

## 问题原因
在 `ImportStepListener::executeCommand` 中，导入成功后会显示一个模态对话框：
```cpp
wxMessageDialog dialog(m_frame, performanceMsg, "Batch Import Complete",
    wxOK | wxICON_INFORMATION);
dialog.ShowModal();  // 这里会阻塞直到用户关闭对话框
```

在新的 docking 布局系统中，这个对话框可能：
1. 被其他窗口遮挡
2. 出现在错误的位置（屏幕外）
3. 没有获得焦点

导致用户看不到对话框，而程序一直在等待用户关闭它。

## 临时解决方案

### 方案1：注释掉模态对话框
最快的解决方法是暂时禁用这些对话框，只在消息面板中显示结果。

### 方案2：使用非阻塞通知
将模态对话框改为状态栏消息或非阻塞的通知。

### 方案3：确保对话框在最前面
使用 `dialog.CenterOnParent()` 和 `dialog.Raise()` 确保对话框可见。

## 推荐修复

建议采用方案2，将导入结果显示在状态栏和消息面板中，而不是使用阻塞的模态对话框。