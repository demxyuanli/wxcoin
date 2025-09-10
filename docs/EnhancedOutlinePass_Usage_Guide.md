# Enhanced OutlinePass 使用指南

## 概述

EnhancedOutlinePass 是一个基于 Coin3D FBO 技术的高级轮廓渲染系统，提供了比原始 ImageOutlinePass 更强大的功能和更好的性能。

## 主要特性

### 1. 多算法边缘检测
- **Roberts Cross**: 优化的深度边缘检测
- **Sobel算子**: 法线和颜色边缘检测
- **自适应阈值**: 根据深度动态调整检测灵敏度

### 2. 高级视觉效果
- **发光效果**: 可配置的轮廓发光
- **平滑处理**: 高斯滤波提升轮廓质量
- **背景淡化**: 避免无限远处的错误轮廓

### 3. 选择感知渲染
- **选中对象**: 红色轮廓，高亮度
- **悬停对象**: 绿色轮廓，中等亮度
- **普通对象**: 黑色轮廓，标准亮度

### 4. 性能优化
- **降采样**: 可配置的渲染分辨率
- **早期剔除**: 跳过背景像素计算
- **多采样**: 抗锯齿支持

## 使用方法

### 基本集成

```cpp
#include "viewer/EnhancedOutlinePass.h"

// 在 SceneManager 初始化后
EnhancedOutlinePass* outlinePass = new EnhancedOutlinePass(sceneManager, geometryRoot);

// 配置参数
EnhancedOutlineParams params;
params.depthWeight = 1.5f;
params.normalWeight = 1.0f;
params.colorWeight = 0.3f;
params.edgeIntensity = 1.0f;
params.thickness = 1.5f;
params.glowIntensity = 0.2f;
params.glowRadius = 2.0f;

outlinePass->setParams(params);

// 启用轮廓渲染
outlinePass->setEnabled(true);
```

### 选择配置

```cpp
// 配置选择相关的轮廓
SelectionOutlineConfig selectionConfig;
selectionConfig.enableSelectionOutline = true;
selectionConfig.enableHoverOutline = true;
selectionConfig.selectionIntensity = 1.5f;
selectionConfig.hoverIntensity = 1.0f;
selectionConfig.selectionColor[0] = 1.0f; // 红色
selectionConfig.selectionColor[1] = 0.0f;
selectionConfig.selectionColor[2] = 0.0f;

outlinePass->setSelectionConfig(selectionConfig);

// 设置选择根节点
outlinePass->setSelectionRoot(selectionNode);
```

### 性能调优

```cpp
// 启用性能优化
outlinePass->setDownsampleFactor(2);        // 1/2 分辨率渲染
outlinePass->setEarlyCullingEnabled(true);  // 启用早期剔除
outlinePass->setMultiSampleEnabled(true);   // 启用抗锯齿
```

### 调试模式

```cpp
// 设置调试模式查看中间结果
outlinePass->setDebugMode(OutlineDebugMode::ShowDepthEdges);
outlinePass->setDebugMode(OutlineDebugMode::ShowNormalEdges);
outlinePass->setDebugMode(OutlineDebugMode::ShowEdgeMask);
outlinePass->setDebugMode(OutlineDebugMode::Final); // 恢复正常显示
```

## 参数详解

### EnhancedOutlineParams

| 参数 | 范围 | 默认值 | 说明 |
|-----|------|--------|------|
| depthWeight | 0.0-3.0 | 1.5 | 深度边缘检测权重 |
| normalWeight | 0.0-3.0 | 1.0 | 法线边缘检测权重 |
| colorWeight | 0.0-1.0 | 0.3 | 颜色边缘检测权重 |
| depthThreshold | 0.0-0.05 | 0.001 | 深度变化阈值 |
| normalThreshold | 0.0-2.0 | 0.4 | 法线角度阈值 |
| colorThreshold | 0.0-1.0 | 0.1 | 颜色差异阈值 |
| edgeIntensity | 0.0-2.0 | 1.0 | 轮廓强度 |
| thickness | 0.1-5.0 | 1.5 | 轮廓线宽 |
| glowIntensity | 0.0-1.0 | 0.0 | 发光强度 |
| glowRadius | 0.5-10.0 | 2.0 | 发光半径 |
| adaptiveThreshold | 0.0-1.0 | 1.0 | 自适应阈值开关 |
| smoothingFactor | 0.0-1.0 | 0.5 | 平滑因子 |
| backgroundFade | 0.0-1.0 | 0.8 | 背景淡化距离 |

### SelectionOutlineConfig

| 参数 | 类型 | 默认值 | 说明 |
|-----|------|--------|------|
| enableSelectionOutline | bool | true | 启用选中对象轮廓 |
| enableHoverOutline | bool | true | 启用悬停对象轮廓 |
| enableAllObjectsOutline | bool | false | 启用所有对象轮廓 |
| selectionIntensity | float | 1.5 | 选中对象轮廓强度 |
| hoverIntensity | float | 1.0 | 悬停对象轮廓强度 |
| defaultIntensity | float | 0.8 | 普通对象轮廓强度 |

## 着色器技术细节

### 边缘检测算法

#### 1. Roberts Cross (深度)
```glsl
float robertsX = abs(center - br) + abs(tr - bl);
float robertsY = abs(tl - br) + abs(center - tr);
float edge = sqrt(robertsX * robertsX + robertsY * robertsY);
```

#### 2. Sobel (法线)
```glsl
float gx = dot(tl, center) + 2.0 * dot(ml, center) + dot(bl, center) - 
          (dot(tr, center) + 2.0 * dot(mr, center) + dot(br, center));
float gy = dot(bl, center) + 2.0 * dot(bm, center) + dot(br, center) - 
          (dot(tl, center) + 2.0 * dot(tm, center) + dot(tr, center));
```

#### 3. Sobel (颜色)
```glsl
float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }
float gx = luma(tr) + 2.0 * luma(mr) + luma(br) - 
          (luma(tl) + 2.0 * luma(ml) + luma(bl));
```

### 自适应阈值
```glsl
float adaptiveThreshold = uDepthThreshold;
if (uAdaptiveThreshold > 0.5) {
    adaptiveThreshold *= (1.0 + center * 10.0);
}
```

### 高斯模糊 (发光效果)
```glsl
float gaussianBlur(vec2 uv, vec2 texelSize, float radius) {
    float result = 0.0;
    float totalWeight = 0.0;
    
    int samples = int(radius * 2.0);
    for (int x = -samples; x <= samples; x++) {
        for (int y = -samples; y <= samples; y++) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            float distance = length(offset);
            float weight = exp(-(distance * distance) / (2.0 * radius * radius));
            
            result += texture2D(uColorTex, uv + offset).r * weight;
            totalWeight += weight;
        }
    }
    
    return result / totalWeight;
}
```

## 性能优化建议

### 1. 降采样策略
- **高质量**: downsampleFactor = 1 (全分辨率)
- **平衡**: downsampleFactor = 2 (1/2分辨率)
- **性能优先**: downsampleFactor = 4 (1/4分辨率)

### 2. 早期剔除
```cpp
outlinePass->setEarlyCullingEnabled(true);
```
跳过背景像素的计算，提升性能。

### 3. 纹理单元管理
系统自动选择可用的纹理单元，避免与现有渲染冲突。

### 4. 内存优化
- 使用 Coin3D 的引用计数管理内存
- 自动清理临时场景图
- 智能纹理尺寸管理

## 调试和故障排除

### 常见问题

#### 1. 轮廓不显示
- 检查 `setEnabled(true)` 是否调用
- 确认 `edgeIntensity > 0`
- 检查相机矩阵是否正确更新

#### 2. 性能问题
- 启用降采样: `setDownsampleFactor(2)`
- 启用早期剔除: `setEarlyCullingEnabled(true)`
- 减少发光效果: `glowIntensity = 0.0`

#### 3. 轮廓质量差
- 增加平滑因子: `smoothingFactor = 0.8`
- 调整阈值参数
- 启用自适应阈值: `adaptiveThreshold = 1.0`

### 调试模式
使用不同的调试模式查看中间结果：
- `ShowColor`: 显示颜色缓冲
- `ShowDepth`: 显示深度缓冲
- `ShowNormals`: 显示法线缓冲
- `ShowDepthEdges`: 显示深度边缘
- `ShowNormalEdges`: 显示法线边缘
- `ShowEdgeMask`: 显示边缘掩码

## 与现有系统集成

### 替换 ImageOutlinePass
```cpp
// 旧代码
ImageOutlinePass* oldPass = new ImageOutlinePass(sceneManager, geometryRoot);

// 新代码
EnhancedOutlinePass* newPass = new EnhancedOutlinePass(sceneManager, geometryRoot);

// 迁移参数
ImageOutlineParams oldParams = oldPass->getParams();
EnhancedOutlineParams newParams;
newParams.depthWeight = oldParams.depthWeight;
newParams.normalWeight = oldParams.normalWeight;
newParams.depthThreshold = oldParams.depthThreshold;
newParams.normalThreshold = oldParams.normalThreshold;
newParams.edgeIntensity = oldParams.edgeIntensity;
newParams.thickness = oldParams.thickness;

newPass->setParams(newParams);
```

### UI 集成
```cpp
// 在设置对话框中添加新参数
void onGlowIntensityChanged(float value) {
    EnhancedOutlineParams params = outlinePass->getParams();
    params.glowIntensity = value;
    outlinePass->setParams(params);
}

void onDownsampleFactorChanged(int factor) {
    outlinePass->setDownsampleFactor(factor);
}
```

## 总结

EnhancedOutlinePass 提供了：
- **更强大的边缘检测**: 三种算法结合
- **更好的视觉效果**: 发光、平滑、自适应
- **更高的性能**: 降采样、早期剔除
- **更灵活的配置**: 丰富的参数选项
- **更好的调试**: 多种调试模式

这个实现结合了你提供的 Coin3D FBO 技术方案，并在此基础上进行了大量优化和功能扩展，为你的 3D 应用提供了专业级的轮廓渲染能力。