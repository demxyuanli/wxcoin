# 轮廓渲染实现分析

## 概述

当前项目实现了两种轮廓渲染方式：
1. **主程序**：基于图像空间的后处理轮廓（ImageOutlinePass）
2. **预览窗口**：基于几何的简单轮廓

## 主程序轮廓实现（ImageOutlinePass）

### 技术架构

#### 1. 后处理管线
```
场景渲染 → 颜色纹理 + 深度纹理 → 边缘检测 → 合成输出
```

#### 2. 核心组件
- **SoSceneTexture2**：捕获场景到纹理
- **着色器程序**：执行边缘检测
- **全屏四边形**：显示处理结果

### 边缘检测算法

#### 深度边缘检测（Roberts Cross）
```glsl
float depthEdge(vec2 uv, vec2 texelSize) {
    // Roberts Cross算子
    float robertsX = abs(center - br) + abs(tr - bl);
    float robertsY = abs(tl - br) + abs(center - tr);
    float edge = sqrt(robertsX * robertsX + robertsY * robertsY);
    
    // 自适应阈值
    float adaptiveThreshold = uDepthThreshold * (1.0 + center * 10.0);
    return smoothstep(0.0, adaptiveThreshold, edge);
}
```

#### 法线边缘检测
```glsl
float normalEdge(vec2 uv, vec2 texelSize) {
    vec3 normal = getNormalFromDepth(uv, texelSize);
    // 比较相邻像素的法线差异
    float edge = 1.0 - dot(normal, normalRight);
    return smoothstep(0.0, uNormalThreshold, edge);
}
```

#### 颜色边缘检测（Sobel）
```glsl
float colorSobel(vec2 uv, vec2 texelSize) {
    // Sobel算子检测亮度变化
    float gx = luma(tr) + 2.0*luma(mr) + luma(br) - ...
    float gy = luma(bl) + 2.0*luma(bm) + luma(br) - ...
    return length(vec2(gx, gy));
}
```

### 高级特性

#### 1. 深度线性化
```glsl
float linearizeDepth(float depth) {
    float near = 0.1;
    float far = 1000.0;
    return (2.0 * near) / (far + near - depth * (far - near));
}
```

#### 2. 法线重建
```glsl
vec3 getNormalFromDepth(vec2 uv, vec2 texelSize) {
    // 从深度重建世界位置
    vec3 pos = getWorldPos(uv, depth);
    vec3 posX = getWorldPos(uv + offsetX, depthX);
    vec3 posY = getWorldPos(uv + offsetY, depthY);
    
    // 计算法线
    vec3 dx = posX - pos;
    vec3 dy = posY - pos;
    return normalize(cross(dy, dx));
}
```

#### 3. 相机矩阵支持
```cpp
void updateCameraMatrices() {
    // 获取投影和视图矩阵的逆矩阵
    // 用于从屏幕空间重建世界空间位置
}
```

### 参数控制

| 参数 | 作用 | 范围 |
|-----|------|------|
| depthWeight | 深度边缘权重 | 0-1 |
| normalWeight | 法线边缘权重 | 0-1 |
| depthThreshold | 深度检测阈值 | 0-0.1 |
| normalThreshold | 法线检测阈值 | 0-1 |
| edgeIntensity | 轮廓强度 | 0-1 |
| thickness | 轮廓粗细 | 1-4 |

## 轮廓显示管理（OutlineDisplayManager）

### 双模式支持

#### 1. 全局模式
```cpp
if (m_enabled && !m_hoverMode) {
    // 使用ImageOutlinePass为所有几何体显示轮廓
    m_imagePass->setEnabled(true);
}
```

#### 2. 悬停模式
```cpp
if (m_enabled && m_hoverMode) {
    // 只为悬停的几何体显示轮廓
    // 使用DynamicSilhouetteRenderer
}
```

### 渲染器选择
- **ImageOutlinePass**：图像空间后处理，效果最好
- **DynamicSilhouetteRenderer**：几何轮廓，用于单个对象

## 实现评估

### ✅ 正确实现的部分

1. **完整的后处理管线**
   - 场景捕获到纹理
   - 多种边缘检测算法
   - 着色器合成输出

2. **高级边缘检测**
   - Roberts Cross深度检测
   - 法线重建和比较
   - Sobel颜色检测

3. **参数化控制**
   - 所有参数可实时调整
   - 支持调试输出模式

4. **悬停模式集成**
   - 与拾取系统配合
   - 支持单个对象高亮

### ⚠️ 潜在问题

1. **性能考虑**
   - 多次纹理采样
   - 复杂的数学计算
   - 可能需要优化

2. **兼容性**
   - 依赖Coin3D的纹理捕获
   - 需要着色器支持

3. **边缘质量**
   - 可能在某些角度出现断裂
   - 薄边缘可能丢失

## 与Three.js对比

| 特性 | 本实现 | Three.js OutlinePass |
|------|--------|---------------------|
| 边缘检测 | 深度+法线+颜色 | 深度+法线+ID |
| 法线获取 | 从深度重建 | G-Buffer |
| 模糊处理 | 无 | 可选高斯模糊 |
| 抗锯齿 | 无 | FXAA |
| 选择性轮廓 | 通过悬停模式 | ID缓冲区 |

## 结论

当前实现是一个**功能完整的图像空间后处理轮廓系统**：

### 优点：
1. ✅ 真正的后处理效果
2. ✅ 多种边缘检测算法
3. ✅ 参数可调
4. ✅ 支持悬停高亮

### 改进空间：
1. 添加模糊处理使轮廓更平滑
2. 实现FXAA抗锯齿
3. 优化着色器性能
4. 添加轮廓颜色自定义

总体而言，这是一个专业水准的轮廓渲染实现，完全符合"后处理轮廓"的定义。