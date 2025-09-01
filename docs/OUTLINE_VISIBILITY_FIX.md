# 轮廓线可见性修复

## 问题描述
用户报告轮廓线（默认和悬停）都不可见，尽管颜色配置功能已实现。

## 问题分析

### 原始实现问题
1. **渲染顺序错误**：先渲染轮廓，再渲染场景，导致轮廓被覆盖
2. **深度缓冲清除**：清除深度缓冲后，轮廓总是在后面
3. **缩放中心**：从原点缩放可能导致轮廓偏移

### 当前实现（两遍渲染）
```
1. 渲染正常场景
2. 清除深度缓冲
3. 渲染放大的背面作为轮廓
4. 再次渲染正常场景覆盖内部
```

## 可能的问题

### 1. 轮廓被场景完全覆盖
- 轮廓厚度可能太小（当前：thickness * 0.01f）
- 深度测试导致轮廓不可见

### 2. OpenGL状态问题
- 背面剔除设置可能不正确
- 多边形模式可能影响渲染

### 3. Coin3D渲染问题
- SoGLRenderAction可能覆盖OpenGL状态
- 场景图结构可能影响渲染

## 解决方案

### 方案1：调整轮廓厚度
```cpp
float outlineScale = 1.0f + (m_outlineParams.thickness * 0.05f); // 增大到0.05f
```

### 方案2：使用模板缓冲区
```cpp
// 1. 渲染对象到模板缓冲区
glEnable(GL_STENCIL_TEST);
glStencilFunc(GL_ALWAYS, 1, 0xFF);
glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
// 渲染正常对象

// 2. 渲染放大的轮廓，只在模板值为0的地方
glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
// 渲染放大的对象
```

### 方案3：使用深度偏移
```cpp
glEnable(GL_POLYGON_OFFSET_FILL);
glPolygonOffset(-1.0f, -1.0f); // 将轮廓推向相机
```

### 方案4：简单线框轮廓
```cpp
// 在场景上方渲染线框
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
glLineWidth(2.0f);
glDepthFunc(GL_LEQUAL);
glDisable(GL_LIGHTING);
```

## 调试步骤

1. **验证轮廓是否被渲染**
   - 临时禁用最后的场景渲染
   - 使用亮色（如红色）测试轮廓

2. **检查缩放效果**
   - 增大缩放系数到0.1f或更大
   - 确认对象确实被放大

3. **测试深度设置**
   - 禁用深度测试查看轮廓
   - 使用深度偏移避免z-fighting

4. **简化测试**
   - 只渲染一个对象
   - 使用固定颜色而非配置颜色

## 最终建议

如果两遍渲染方法仍然不工作，建议使用：
1. **线框轮廓**：最简单可靠
2. **模板缓冲区**：更精确的轮廓
3. **后处理**：使用FBO和着色器（但预览窗口可能不支持）