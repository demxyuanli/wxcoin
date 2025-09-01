# 外轮廓描边后处理实现分析

## 当前实现方法分析

### 1. 是的，当前实现确实是自定义Shader后处理方法

当前的 `ImageOutlinePass` 和 `ImageOutlinePass2` 实现完全符合你描述的后处理管线：

#### 1.1 渲染场景到纹理（RTT）
```cpp
// 创建颜色纹理
m_colorTexture = new SoSceneTexture2;
m_colorTexture->type = SoSceneTexture2::RGBA8;

// 创建深度纹理
m_depthTexture = new SoSceneTexture2;
m_depthTexture->type = SoSceneTexture2::DEPTH;
```
- 使用 `SoSceneTexture2` 节点实现渲染到纹理（相当于FBO）
- 分别捕获颜色和深度信息

#### 1.2 多Pass渲染架构
```cpp
// 第一步：渲染场景到纹理
tempSceneRoot->addChild(camera);
tempSceneRoot->addChild(m_captureRoot);
m_colorTexture->scene = tempSceneRoot;
m_depthTexture->scene = tempSceneRoot;

// 第二步：后处理着色器
m_annotation->addChild(m_program);  // GLSL着色器程序
m_annotation->addChild(m_quadSeparator);  // 全屏四边形
```

#### 1.3 GLSL着色器实现边缘检测
片段着色器中实现了多种边缘检测算法：

```glsl
// 1. Sobel算子检测颜色边缘
float colorSobel(vec2 uv, vec2 texelSize) {
    // Sobel卷积核实现
    float gx = luma(tr) + 2.0*luma(mr) + luma(br) - ...
    float gy = luma(bl) + 2.0*luma(bm) + luma(br) - ...
    return length(vec2(gx, gy));
}

// 2. Roberts Cross算子检测深度边缘
float depthEdge(vec2 uv, vec2 texelSize) {
    float robertsX = abs(center - br) + abs(tr - bl);
    float robertsY = abs(tl - br) + abs(center - tr);
    float edge = sqrt(robertsX * robertsX + robertsY * robertsY);
}

// 3. 基于法线的边缘检测
float normalEdge(vec2 uv, vec2 texelSize) {
    vec3 normal = getNormalFromDepth(uv, texelSize);
    // 通过相邻像素法线的点积判断边缘
}
```

#### 1.4 后处理叠加
```glsl
void main() {
    vec4 color = texture2D(uColorTex, vTexCoord);
    
    // 组合多种边缘检测结果
    float cEdge = colorSobel(vTexCoord, texelSize);
    float dEdge = depthEdge(vTexCoord, texelSize) * uDepthWeight;
    float nEdge = normalEdge(vTexCoord, texelSize) * uNormalWeight;
    
    float edge = clamp((cEdge + dEdge + nEdge) * uIntensity, 0.0, 1.0);
    
    // 将轮廓叠加到原始颜色上
    vec3 outlineColor = vec3(0.0); // 黑色轮廓
    gl_FragColor = vec4(mix(color.rgb, outlineColor, edge), color.a);
}
```

### 2. 实现架构完全符合标准后处理管线

#### 2.1 使用Coin3D的着色器支持
- `SoShaderProgram`: 着色器程序容器
- `SoVertexShader`: 顶点着色器
- `SoFragmentShader`: 片段着色器
- `SoShaderParameter`: 传递uniform参数

#### 2.2 场景图组织
```
SceneRoot
├── ModelRoot (原始3D场景)
└── OverlayRoot (后处理叠加层)
    └── SoAnnotation (2D叠加)
        ├── SoTransform
        ├── SoTextureUnit + SoSceneTexture2 (颜色纹理)
        ├── SoTextureUnit + SoSceneTexture2 (深度纹理)
        ├── SoShaderParameter (各种uniform参数)
        ├── SoShaderProgram (GLSL程序)
        └── QuadSeparator (全屏四边形)
```

#### 2.3 与Three.js OutlinePass的相似性
- **多纹理输入**: 颜色和深度纹理
- **边缘检测算法**: Sobel、Roberts Cross等
- **全屏后处理**: 使用全屏四边形
- **参数可调**: 边缘强度、阈值等

### 3. 当前实现的优势

1. **完全集成到Coin3D框架**: 使用标准节点和渲染动作
2. **灵活的边缘检测**: 结合颜色、深度和法线信息
3. **实时性能**: GPU加速的着色器处理
4. **参数化控制**: 可调节各种视觉参数

### 4. 为什么预览窗口显示白色矩形？

问题不在于实现方法，而可能是：

1. **Coin3D版本或配置问题**: 某些版本可能需要特殊配置来启用GLSL
2. **OpenGL上下文问题**: 预览窗口的GL上下文可能缺少某些特性
3. **场景图渲染顺序**: SoGLRenderAction可能需要特殊设置

### 5. 建议的调试步骤

1. **验证Coin3D的GLSL支持**:
   ```cpp
   // 检查是否支持着色器
   if (SoShaderProgram::isSupported(SoShaderProgram::GLSL)) {
       // 支持GLSL
   }
   ```

2. **启用调试输出**:
   ```cpp
   // 在SoGLRenderAction中启用GL错误检查
   renderAction.setGLRenderCacheMode(SoGLRenderAction::DO_NOT_CACHE);
   ```

3. **简化测试**:
   - 先测试固定颜色输出
   - 逐步添加纹理采样
   - 最后添加边缘检测算法

### 6. 结论

当前的实现完全符合标准的GPU后处理管线，使用了：
- ✅ 自定义GLSL着色器
- ✅ 多Pass渲染（RTT）
- ✅ 边缘检测算法（Sobel、Roberts Cross）
- ✅ 后处理叠加

这是一个专业的、与Three.js OutlinePass原理相同的实现。