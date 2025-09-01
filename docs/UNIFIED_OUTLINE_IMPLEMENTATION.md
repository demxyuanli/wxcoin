# 统一的轮廓渲染实现

## 概述

通过引入 `IOutlineRenderer` 接口和 `ImageOutlinePass2`，实现了主应用程序 (`Canvas`) 和预览窗口 (`OutlinePreviewCanvas`) 使用相同的轮廓渲染算法。

## 架构设计

### 1. IOutlineRenderer 接口

```cpp
class IOutlineRenderer {
public:
    virtual wxGLCanvas* getGLCanvas() const = 0;
    virtual SoCamera* getCamera() const = 0;
    virtual SoSeparator* getSceneRoot() const = 0;
    virtual void requestRedraw() = 0;
};
```

这个接口抽象了 `ImageOutlinePass` 需要的核心功能：
- 获取 OpenGL 画布
- 获取相机节点
- 获取场景根节点
- 请求重绘

### 2. ImageOutlinePass2

基于原始 `ImageOutlinePass` 的改进版本：
- 使用 `IOutlineRenderer` 接口而不是 `SceneManager`
- 保持相同的着色器算法
- 支持任何实现 `IOutlineRenderer` 的渲染器

### 3. 适配器模式

#### Canvas 适配器
```cpp
class CanvasOutlineRenderer : public IOutlineRenderer {
    SceneManager* m_sceneManager;
    // 将 SceneManager 的功能适配到 IOutlineRenderer 接口
};
```

#### OutlinePreviewCanvas 直接实现
```cpp
class OutlinePreviewCanvas : public wxGLCanvas, public IOutlineRenderer {
    // 直接实现 IOutlineRenderer 接口
};
```

## 实现细节

### 1. 算法一致性
- 使用相同的顶点和片段着色器
- 相同的深度边缘检测（Roberts Cross）
- 相同的法线重建算法
- 相同的参数控制

### 2. 场景图集成
```cpp
// ImageOutlinePass2 构造
ImageOutlinePass2(IOutlineRenderer* renderer, SoSeparator* captureRoot);

// 场景捕获
m_captureNode->scene = m_tempSceneRoot;  // 包含相机和几何体
```

### 3. 参数传递
```cpp
void OutlinePreviewCanvas::updateOutlineParams(const ImageOutlineParams& params) {
    m_outlineParams = params;
    if (m_outlinePass) {
        m_outlinePass->setParams(params);  // 直接传递给 ImageOutlinePass2
    }
}
```

## 优势

1. **真正的算法统一**
   - 预览和最终效果完全一致
   - 消除了算法差异导致的问题

2. **代码复用**
   - 核心算法只在一处实现
   - 易于维护和改进

3. **灵活架构**
   - 通过接口解耦
   - 可以支持更多类型的渲染器

4. **保持性能**
   - 没有额外的开销
   - 直接使用 Coin3D 的渲染管线

## 使用方式

### 在 Canvas 中
```cpp
auto renderer = std::make_unique<CanvasOutlineRenderer>(m_sceneManager);
m_outlinePass = std::make_unique<ImageOutlinePass2>(renderer.get(), m_objectRoot);
```

### 在 OutlinePreviewCanvas 中
```cpp
m_outlinePass = std::make_unique<ImageOutlinePass2>(this, m_modelRoot);
m_outlinePass->setEnabled(m_outlineEnabled);
m_outlinePass->setParams(m_outlineParams);
```

## 参数支持

所有 `ImageOutlineParams` 参数都得到完整支持：
- `depthWeight`: 深度边缘权重
- `normalWeight`: 法线边缘权重
- `depthThreshold`: 深度阈值
- `normalThreshold`: 法线阈值
- `edgeIntensity`: 边缘强度
- `thickness`: 轮廓厚度

## 未来扩展

1. **颜色支持**：可以在着色器中添加 uniform 来支持自定义轮廓颜色
2. **选择性渲染**：支持对特定对象应用不同的轮廓效果
3. **性能优化**：根据需要优化纹理大小和渲染次数

## 总结

通过这个统一实现：
- ✅ Canvas 和 OutlinePreviewCanvas 使用完全相同的算法
- ✅ 预览效果与最终效果一致
- ✅ 代码维护性大大提高
- ✅ 架构更加灵活和可扩展