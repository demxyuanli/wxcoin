# 预览窗口FBO轮廓实现

## 实现概述

为预览窗口添加了完整的FBO（帧缓冲对象）和着色器支持，实现了类似Three.js的后处理轮廓效果。

## 技术架构

### 1. 渲染管线

```
Pass 1: 渲染法线到纹理
  ↓
Pass 2: 渲染颜色到纹理
  ↓
Pass 3: 后处理轮廓检测
```

### 2. 核心组件

#### FBO资源
- **颜色纹理**：存储场景的颜色信息
- **深度纹理**：存储深度信息用于边缘检测
- **法线纹理**：存储法线信息用于边缘检测

#### 着色器程序
- **法线着色器**：将世界空间法线编码到纹理
- **轮廓着色器**：执行边缘检测和轮廓绘制

### 3. 边缘检测算法

```glsl
// 深度边缘检测
float depthEdge(vec2 uv) {
    // 采样周围深度值
    // 计算深度差异
    // 返回边缘强度
}

// 法线边缘检测
float normalEdge(vec2 uv) {
    // 采样周围法线
    // 计算法线夹角
    // 返回边缘强度
}
```

## 实现细节

### 1. OpenGL扩展加载
```cpp
// 动态加载所需的OpenGL扩展函数
glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)wglGetProcAddress("glGenFramebuffers");
// ... 其他扩展函数
```

### 2. FBO初始化
```cpp
void initializeFBO(int width, int height) {
    // 创建FBO
    glGenFramebuffers(1, &m_fbo);
    
    // 创建纹理附件
    // - 颜色纹理 (RGBA8)
    // - 深度纹理 (DEPTH_COMPONENT24)
    // - 法线纹理 (RGBA8)
}
```

### 3. 多通道渲染
```cpp
// Pass 1: 渲染法线
glUseProgram(m_normalShader);
renderScene();

// Pass 2: 渲染颜色
glUseProgram(0);
renderScene();

// Pass 3: 后处理
glUseProgram(m_outlineShader);
renderFullscreenQuad();
```

## 参数映射

| 参数 | 作用 | 着色器变量 |
|-----|------|-----------|
| depthThreshold | 深度边缘灵敏度 | uDepthThreshold |
| normalThreshold | 法线边缘灵敏度 | uNormalThreshold |
| thickness | 轮廓线宽度 | uThickness |
| edgeIntensity | 轮廓强度 | uIntensity |

## 优势

1. **精确贴合**：轮廓完全贴合几何体，无偏移
2. **高质量**：基于图像空间的边缘检测，效果平滑
3. **可调节**：参数实时调整，立即看到效果
4. **性能好**：GPU加速，渲染效率高

## 与Three.js对比

| 特性 | 我们的实现 | Three.js |
|-----|----------|----------|
| 边缘检测 | 深度+法线 | 深度+法线+ID |
| 渲染通道 | 3次 | 4-5次 |
| 模糊效果 | 无 | 可选 |
| 选择性轮廓 | 全部物体 | 可选择特定物体 |

## 注意事项

1. **OpenGL版本**：需要支持FBO和着色器（OpenGL 2.0+）
2. **扩展加载**：必须正确加载OpenGL扩展函数
3. **纹理格式**：确保GPU支持所需的纹理格式
4. **性能影响**：多通道渲染会增加GPU负担

## 效果展示

现在预览窗口能够：
- 显示精确贴合的轮廓线
- 实时响应参数调整
- 提供与主程序一致的轮廓效果
- 真正的后处理轮廓，而非几何近似