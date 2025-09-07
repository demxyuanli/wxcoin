# 预览窗口简化方案

## 问题背景

原计划让预览窗口使用 `ImageOutlinePass` 以确保算法一致，但遇到以下问题：

1. **SceneManager 依赖性**：
   - `ImageOutlinePass` 需要 `SceneManager`
   - `SceneManager` 的构造函数需要 `Canvas*`
   - `OutlinePreviewCanvas` 是 `wxGLCanvas`，不是 `Canvas`

2. **架构不兼容**：
   - `Canvas` 是主应用的复杂渲染画布
   - `OutlinePreviewCanvas` 是轻量级预览窗口
   - 两者的架构和职责完全不同

## 解决方案

采用简化的预览渲染方案：

### 1. 直接使用 Coin3D
- 不依赖 `SceneManager` 和 `ImageOutlinePass`
- 直接创建和管理 Coin3D 场景图
- 使用 OpenGL 实现简单的轮廓效果

### 2. 简化的轮廓渲染
```cpp
// 使用线框叠加来模拟轮廓效果
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
glLineWidth(m_outlineParams.thickness * 2.0f);
glColor4f(r, g, b, m_outlineParams.edgeIntensity);
```

### 3. 参数映射
- 仍然接收相同的 `ImageOutlineParams`
- 将参数映射到简化的渲染效果
- 保持参数调整的实时反馈

## 优缺点分析

### 优点
1. **独立性**：不依赖复杂的主应用架构
2. **轻量级**：适合预览窗口的需求
3. **响应快**：简单的渲染，性能好
4. **易维护**：代码简单，容易理解

### 缺点
1. **效果差异**：与主应用的精确效果可能略有不同
2. **功能限制**：无法展示所有高级效果

## 参数映射说明

| ImageOutlineParams | 预览窗口映射 |
|-------------------|------------|
| thickness | 线框宽度 |
| edgeIntensity | 透明度 |
| depthWeight | (视觉参考) |
| normalWeight | (视觉参考) |
| depthThreshold | (视觉参考) |
| normalThreshold | (视觉参考) |

## 用户体验

虽然预览效果与最终效果不完全相同，但：
1. **参数变化可见**：用户能看到参数调整的影响
2. **颜色准确**：轮廓颜色、背景色等完全一致
3. **交互一致**：鼠标悬停等交互效果相同

## 未来改进

如果需要更精确的预览，可以考虑：
1. 创建轻量级的 `ImageOutlinePass` 版本
2. 实现独立的后处理管线
3. 使用 FBO 和着色器实现相似算法

## 总结

当前的简化方案是一个实用的折中：
- 满足参数预览的基本需求
- 避免复杂的架构依赖
- 保持代码的可维护性
- 提供足够好的用户体验