# Three.js OutlinePass 实现原理分析

## Three.js 的实现方法

Three.js 的 OutlinePass 使用了多通道渲染技术：

### 1. **第一通道：渲染选中物体的Mask**
```javascript
// 将选中的物体渲染到一个单独的渲染目标
// 只渲染物体的ID或者纯色
renderTargetMaskBuffer
```

### 2. **第二通道：边缘检测**
```javascript
// 使用Sobel算子或类似的边缘检测算法
// 在Mask图像上检测边缘
// 生成只包含边缘的图像
edgeDetectionMaterial
```

### 3. **第三通道：模糊处理（可选）**
```javascript
// 对边缘进行高斯模糊
// 创建发光效果
blurMaterial
```

### 4. **第四通道：合成**
```javascript
// 将边缘图像叠加到原始场景上
// 使用加法混合或其他混合模式
compositeMaterial
```

## 关键差异

### Three.js 方法的优势：
1. **基于图像空间**：在2D图像上进行边缘检测，精确度高
2. **不改变几何体**：轮廓线完全基于原始几何体的投影
3. **支持复杂形状**：能处理凹凸不平的表面
4. **可控制线宽**：通过模糊和膨胀控制线条粗细

### 我们当前实现的问题：
1. **几何缩放法**：放大模型会导致轮廓偏移
2. **不够精确**：无法准确贴合复杂形状
3. **内部边缘**：可能显示不该显示的边

## 改进方案

### 方案1：简化的边缘检测（适合预览）
```cpp
// 1. 渲染深度图
// 2. 使用简单的边缘检测
// 3. 只显示深度不连续的地方
```

### 方案2：轮廓线渲染（Silhouette Rendering）
```cpp
// 1. 计算每个边是否为轮廓边
// 2. 轮廓边 = 相邻面一个朝向视点，一个背向视点
// 3. 只渲染轮廓边
```

### 方案3：后处理管线（类似Three.js）
```cpp
// 1. 渲染物体到纹理
// 2. 边缘检测着色器
// 3. 合成到最终图像
```

## 预览窗口的最佳实践

考虑到预览窗口的限制，建议使用：

### 改进的两通道方法：
```cpp
// 第一通道：渲染物体剪影
glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
glEnable(GL_DEPTH_TEST);
glDepthFunc(GL_LESS);
glDepthMask(GL_TRUE);
// 渲染物体，只写入深度

// 第二通道：渲染轮廓
glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
glDepthFunc(GL_EQUAL);
glDepthMask(GL_FALSE);
glLineWidth(thickness);
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
// 渲染物体轮廓
```

这样可以确保轮廓线精确贴合几何体。