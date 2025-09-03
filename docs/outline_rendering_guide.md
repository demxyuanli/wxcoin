# 实时几何轮廓线渲染技术指南

## 概述

实时轮廓线（Outline/Silhouette）渲染是计算机图形学中的重要技术，广泛应用于CAD软件、游戏引擎、技术插图等领域。本指南详细介绍了多种实时轮廓线渲染方法及其实现。

## 目录

1. [轮廓线渲染技术分类](#轮廓线渲染技术分类)
2. [基于几何的方法](#基于几何的方法)
3. [基于图像空间的方法](#基于图像空间的方法)
4. [混合方法](#混合方法)
5. [性能优化策略](#性能优化策略)
6. [实现示例](#实现示例)

## 轮廓线渲染技术分类

### 1. 基于几何的方法（Geometry-based）
- **优点**：精确、可控、支持样式化
- **缺点**：计算复杂度高、需要拓扑信息
- **适用场景**：CAD软件、技术插图

### 2. 基于图像空间的方法（Image-space）
- **优点**：实现简单、性能好、与几何复杂度无关
- **缺点**：精度受限于分辨率、难以控制线条样式
- **适用场景**：游戏引擎、实时渲染

### 3. 混合方法（Hybrid）
- **优点**：结合两种方法的优势
- **缺点**：实现复杂、需要更多资源
- **适用场景**：高质量渲染需求

## 基于几何的方法

### 1. 边缘检测法（Edge Detection）

#### 原理
通过分析几何的拓扑结构，识别不同类型的边缘：
- **轮廓边（Silhouette Edges）**：相邻面法向量与视线方向的点积符号相反
- **折痕边（Crease Edges）**：相邻面法向量夹角大于阈值
- **边界边（Boundary Edges）**：只有一个相邻面的边

#### 实现步骤
```cpp
// 伪代码
for each edge in geometry:
    if edge.isBoundary():
        markAsOutline(edge)
    else:
        face1 = edge.adjacentFace1
        face2 = edge.adjacentFace2
        
        // 轮廓边检测
        dot1 = dot(face1.normal, viewDirection)
        dot2 = dot(face2.normal, viewDirection)
        if (dot1 * dot2 < 0):
            markAsOutline(edge)
        
        // 折痕边检测
        angle = acos(dot(face1.normal, face2.normal))
        if (angle > creaseThreshold):
            markAsOutline(edge)
```

### 2. 法向量扩展法（Normal Extrusion）

#### 原理
沿着顶点法向量方向扩展几何体，创建轮廓效果。

#### 实现步骤
1. 第一遍：渲染扩展后的几何体（只渲染背面）
2. 第二遍：正常渲染几何体

```glsl
// 顶点着色器
uniform float outlineWidth;
void main() {
    vec4 pos = vec4(position + normal * outlineWidth, 1.0);
    gl_Position = projectionMatrix * modelViewMatrix * pos;
}
```

### 3. 模板缓冲法（Stencil Buffer）

#### 原理
使用模板缓冲区创建轮廓效果。

#### 实现步骤
1. 渲染物体到模板缓冲区
2. 扩展渲染物体，只在模板测试失败的地方绘制

## 基于图像空间的方法

### 1. 深度边缘检测（Depth Edge Detection）

#### 原理
在屏幕空间分析深度缓冲区，检测深度不连续的地方。

#### Sobel算子实现
```glsl
float depthSobel(sampler2D depthTex, vec2 uv, vec2 texelSize) {
    // 3x3 Sobel kernel
    float tl = texture(depthTex, uv + vec2(-1, -1) * texelSize).r;
    float tm = texture(depthTex, uv + vec2( 0, -1) * texelSize).r;
    float tr = texture(depthTex, uv + vec2( 1, -1) * texelSize).r;
    float ml = texture(depthTex, uv + vec2(-1,  0) * texelSize).r;
    float mm = texture(depthTex, uv).r;
    float mr = texture(depthTex, uv + vec2( 1,  0) * texelSize).r;
    float bl = texture(depthTex, uv + vec2(-1,  1) * texelSize).r;
    float bm = texture(depthTex, uv + vec2( 0,  1) * texelSize).r;
    float br = texture(depthTex, uv + vec2( 1,  1) * texelSize).r;
    
    // Sobel X and Y
    float gx = -tl - 2.0*ml - bl + tr + 2.0*mr + br;
    float gy = -tl - 2.0*tm - tr + bl + 2.0*bm + br;
    
    return length(vec2(gx, gy));
}
```

### 2. 法向量边缘检测（Normal Edge Detection）

#### 原理
从深度缓冲区重建法向量，检测法向量不连续的地方。

#### 实现
```glsl
vec3 getNormalFromDepth(vec2 uv, float depth) {
    vec3 positionWS = reconstructWorldPosition(uv, depth);
    vec3 ddx = dFdx(positionWS);
    vec3 ddy = dFdy(positionWS);
    return normalize(cross(ddy, ddx));
}

float normalEdge(vec2 uv, vec2 texelSize) {
    vec3 normal = getNormalFromDepth(uv, texture(depthTex, uv).r);
    vec3 normalRight = getNormalFromDepth(uv + vec2(texelSize.x, 0), ...);
    vec3 normalUp = getNormalFromDepth(uv + vec2(0, texelSize.y), ...);
    
    float edge = 1.0 - min(dot(normal, normalRight), dot(normal, normalUp));
    return smoothstep(threshold, threshold * 2.0, edge);
}
```

### 3. 颜色边缘检测（Color Edge Detection）

#### 原理
检测颜色缓冲区中的颜色变化。

## 混合方法

### 1. 几何预处理 + 图像后处理

#### 实现流程
1. 使用几何方法标记潜在的轮廓边
2. 渲染标记的边到单独的缓冲区
3. 使用图像空间方法增强和平滑轮廓

### 2. 多层次细节（LOD）轮廓

#### 实现策略
- 近处物体：使用精确的几何方法
- 远处物体：使用快速的图像空间方法
- 中等距离：使用简化的几何方法

## 性能优化策略

### 1. 空间数据结构优化
- 使用 BVH/Octree 加速边缘查询
- 缓存拓扑信息避免重复计算

### 2. GPU并行化
```glsl
// 使用Compute Shader并行处理边缘检测
[numthreads(8, 8, 1)]
void EdgeDetectionCS(uint3 id : SV_DispatchThreadID) {
    uint2 pixel = id.xy;
    float edge = detectEdge(pixel);
    outputTexture[pixel] = edge;
}
```

### 3. 时间相干性优化
- 缓存上一帧的轮廓信息
- 只更新变化的部分

### 4. 自适应采样
```cpp
// 根据物体距离调整采样率
float sampleRate = clamp(1.0 / distance, 0.1, 1.0);
int sampleCount = int(baseCount * sampleRate);
```

## 实现示例

### 完整的轮廓渲染管线

```cpp
class OutlineRenderer {
public:
    void render(const Scene& scene, const Camera& camera) {
        // 1. 几何轮廓提取
        extractGeometryOutlines(scene, camera);
        
        // 2. 渲染到G-Buffer
        renderToGBuffer(scene, camera);
        
        // 3. 图像空间边缘检测
        detectImageSpaceEdges();
        
        // 4. 合成最终图像
        compositeOutlines();
    }
    
private:
    void extractGeometryOutlines(const Scene& scene, const Camera& camera) {
        for (const auto& object : scene.objects) {
            // 视锥体剔除
            if (!camera.frustum.contains(object.boundingBox))
                continue;
                
            // 提取轮廓边
            auto edges = extractSilhouetteEdges(object, camera.position);
            m_outlineEdges.insert(edges.begin(), edges.end());
        }
    }
    
    void detectImageSpaceEdges() {
        // 深度边缘检测
        m_depthEdges = sobelFilter(m_depthBuffer);
        
        // 法向量边缘检测
        m_normalEdges = detectNormalDiscontinuities(m_normalBuffer);
        
        // 合并边缘
        m_finalEdges = combine(m_depthEdges, m_normalEdges);
    }
};
```

## 高级技术

### 1. 风格化轮廓（Stylized Outlines）
- 变宽度线条
- 纹理化线条
- 断续线条效果

### 2. 时序抗锯齿（Temporal Anti-aliasing）
- 使用历史帧信息平滑轮廓
- 减少闪烁和锯齿

### 3. 自适应轮廓（Adaptive Outlines）
- 根据物体重要性调整轮廓强度
- 根据场景复杂度动态调整质量

## 总结

选择合适的轮廓渲染技术需要考虑：
- **性能要求**：实时应用优先考虑图像空间方法
- **质量要求**：离线渲染可使用复杂的几何方法
- **硬件限制**：移动设备需要更轻量的算法
- **艺术风格**：不同方法产生不同的视觉效果

建议从简单的图像空间方法开始，根据需求逐步增加复杂度。在实际应用中，混合多种方法往往能获得最佳效果。