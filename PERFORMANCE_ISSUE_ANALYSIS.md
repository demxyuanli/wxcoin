# 性能问题深入分析

## 当前状况
1. 恢复了进度定时器的原始行为（每50ms运行）
2. 移除了 onSize 中的 Refresh() 调用
3. 导入几何体功能正常，但鼠标操作卡顿

## 可能的卡顿原因

### 1. 渲染相关
- **OpenGL 上下文切换**：Canvas 被 reparent 后可能有上下文问题
- **双缓冲设置**：可能没有正确启用
- **VSync**：垂直同步可能导致帧率限制

### 2. 事件处理
- **事件队列拥堵**：大量事件积压
- **鼠标事件处理**：可能有复杂的计算在主线程

### 3. 几何体渲染
- **复杂度**：导入的几何体可能非常复杂
- **渲染优化**：可能缺少 LOD 或剔除优化

## 调试建议

### 1. 性能分析
```cpp
// 在 Canvas::onMouseEvent 中添加计时
auto start = std::chrono::high_resolution_clock::now();
// ... 处理事件
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
if (duration.count() > 16666) { // 超过 16ms (60fps)
    wxLogDebug("Mouse event took %lld us", duration.count());
}
```

### 2. 渲染分析
```cpp
// 在 Canvas::render 中添加帧率计算
static auto lastTime = std::chrono::high_resolution_clock::now();
auto now = std::chrono::high_resolution_clock::now();
auto frameDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime);
wxLogDebug("Frame time: %lld ms", frameDuration.count());
lastTime = now;
```

### 3. OpenGL 检查
- 确保硬件加速启用
- 检查 OpenGL 错误
- 验证上下文是否正确

## 临时解决方案

如果性能问题持续：
1. 降低渲染质量设置
2. 启用 LOD 系统
3. 减少渲染频率
4. 使用脏区域渲染

## 根本解决方案

可能需要：
1. 重构渲染管线
2. 实现多线程渲染
3. 优化几何体数据结构
4. 使用现代 OpenGL 特性（VAO, VBO等）