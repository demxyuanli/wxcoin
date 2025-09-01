# Canvas 性能问题分析

## 症状
- 能创建和导入几何体
- 导入几何体后 Canvas 异常卡顿
- 鼠标失去响应

## 可能的原因

### 1. OpenGL 上下文问题
Canvas 继承自 `wxGLCanvas`，在 reparent 操作时可能出现：
- OpenGL 上下文丢失或损坏
- 渲染状态未正确重置
- GPU 资源未正确管理

### 2. 事件处理问题
- 鼠标事件可能被多次绑定
- 事件处理器可能进入死循环
- 事件传播可能被阻塞

### 3. 渲染循环问题
- 可能有多个渲染循环在运行
- 渲染请求可能过于频繁
- VSync 设置可能不正确

### 4. 内存或资源泄漏
- 导入几何体时可能有资源未释放
- 旧的 Canvas 实例可能仍在后台运行

## 建议的解决方案

### 1. 避免 Reparent Canvas
由于 OpenGL canvas 的特殊性，最好避免 reparent 操作：

```cpp
// 在 CreateCanvasDockWidget 中，总是创建新的 Canvas
Canvas* canvas = new Canvas(dock);
// 然后将基类的 canvas 数据迁移过来
if (GetCanvas()) {
    // 迁移场景数据而不是 reparent
    canvas->getSceneManager()->copyFrom(GetCanvas()->getSceneManager());
}
```

### 2. 确保正确的 OpenGL 上下文管理
```cpp
// 在 reparent 之前保存上下文
if (canvas->IsShownOnScreen()) {
    canvas->SetCurrent();
    // 保存必要的 OpenGL 状态
}

// Reparent
canvas->Reparent(dock);

// 恢复上下文
canvas->SetCurrent();
// 恢复 OpenGL 状态
```

### 3. 检查事件绑定
确保事件只绑定一次，在 reparent 之前解绑旧的事件处理器。

### 4. 调试步骤
1. 在导入几何体前后检查 CPU 和 GPU 使用率
2. 使用 OpenGL 调试工具检查渲染调用
3. 添加日志记录渲染帧率
4. 检查是否有异常的定时器或空闲事件

## 临时解决方案
如果问题持续，可以考虑：
1. 不使用基类的 Canvas，总是创建新的
2. 在 docking 系统初始化时禁用某些 Canvas 功能，逐步启用以定位问题
3. 添加更多日志来追踪问题发生的确切时机