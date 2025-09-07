# 统一的轮廓渲染算法

## 概述

预览窗口和主应用程序的 ImageOutlinePass 现在使用相同的后处理轮廓算法。

## 算法详情

### 1. 深度边缘检测（Roberts Cross）
```glsl
// Roberts Cross operators
float robertsX = abs(center - br) + abs(tr - bl);
float robertsY = abs(tl - br) + abs(center - tr);
float edge = sqrt(robertsX * robertsX + robertsY * robertsY);
```

### 2. 法线边缘检测
- 从深度缓冲区重建法线
- 检测法线变化来识别边缘

### 3. 参数控制
- `depthWeight`: 深度边缘权重
- `normalWeight`: 法线边缘权重
- `depthThreshold`: 深度边缘阈值
- `normalThreshold`: 法线边缘阈值
- `edgeIntensity`: 总体轮廓强度
- `thickness`: 轮廓粗细

## 实现对比

### ImageOutlinePass（主应用）
- 使用 Coin3D 的 SoSceneTexture2 捕获场景
- 使用 SoShaderProgram 和 SoFragmentShader
- 支持相机矩阵用于世界空间重建

### OutlinePreviewCanvas（预览窗口）
- 使用原生 OpenGL FBO
- 使用 GLSL 着色器
- 简化的法线重建（不需要相机矩阵）

## 渲染流程

1. **第一遍**：渲染场景到 FBO
   - 颜色附件：场景颜色
   - 深度附件：深度信息

2. **第二遍**：后处理
   - 读取颜色和深度纹理
   - 应用边缘检测算法
   - 混合轮廓颜色

## 优势

1. **一致性**：预览和最终效果相同
2. **准确性**：基于深度的边缘检测更精确
3. **可控性**：多个参数可调节效果
4. **性能**：后处理方法效率高

## 与其他方法对比

### 几何方法（如模板缓冲区）
- 优点：简单实现
- 缺点：对复杂几何体效果不佳

### 后处理方法（当前实现）
- 优点：效果最好，参数可控
- 缺点：需要 FBO 和着色器支持

## 调试

如果轮廓不显示：
1. 检查 FBO 是否创建成功
2. 检查着色器是否编译成功
3. 检查深度纹理是否正确
4. 调整参数值（特别是阈值）