# 轮廓参数流程

## 参数传递流程

```
OutlineSettingsDialog
    ├── 滑块控件 (Sliders)
    │   └── onSliderChange()
    │       └── updatePreview()
    │           ├── 更新 m_params
    │           └── m_previewCanvas->updateOutlineParams(m_params)
    │               └── m_outlinePass->setParams(params)
    │
    └── 颜色选择器 (Color Pickers)
        └── onColorChange()
            └── m_previewCanvas->setXxxColor()
```

## 关键组件

### 1. OutlineSettingsDialog
负责：
- 提供用户界面（滑块、颜色选择器）
- 收集用户输入
- 管理参数值
- 触发预览更新

### 2. OutlinePreviewCanvas
负责：
- 接收参数更新
- 传递参数到 ImageOutlinePass
- 管理场景渲染
- 处理用户交互

### 3. ImageOutlinePass
负责：
- 接收渲染参数
- 执行轮廓渲染算法
- 管理着色器和 FBO

## 参数更新流程

### 1. 用户调整滑块
```cpp
void OutlineSettingsDialog::onSliderChange(wxCommandEvent& event) {
    updateLabels();    // 更新显示标签
    updatePreview();   // 更新预览
}
```

### 2. 更新预览参数
```cpp
void OutlineSettingsDialog::updatePreview() {
    // 从滑块读取值
    m_params.depthWeight = m_depthW->GetValue() / 100.0f;
    m_params.normalWeight = m_normalW->GetValue() / 100.0f;
    // ... 其他参数
    
    // 应用到预览画布
    if (m_previewCanvas) {
        m_previewCanvas->updateOutlineParams(m_params);
    }
}
```

### 3. 预览画布更新
```cpp
void OutlinePreviewCanvas::updateOutlineParams(const ImageOutlineParams& params) {
    m_outlineParams = params;
    if (m_outlinePass) {
        m_outlinePass->setParams(params);  // 传递给 ImageOutlinePass
    }
    m_needsRedraw = true;
    Refresh(false);
}
```

### 4. ImageOutlinePass 应用参数
```cpp
void ImageOutlinePass::setParams(const ImageOutlineParams& p) {
    m_params = p;
    // 更新着色器 uniforms
    if (m_uIntensity) m_uIntensity->value = m_params.edgeIntensity;
    if (m_uDepthWeight) m_uDepthWeight->value = m_params.depthWeight;
    // ... 其他 uniforms
    refresh();
}
```

## 参数列表

### 基础参数 (ImageOutlineParams)
- `depthWeight`: 深度边缘检测权重
- `normalWeight`: 法线边缘检测权重  
- `depthThreshold`: 深度边缘阈值
- `normalThreshold`: 法线边缘阈值
- `edgeIntensity`: 总体轮廓强度
- `thickness`: 轮廓线粗细

### 扩展参数 (ExtendedOutlineParams)
- `backgroundColor`: 背景颜色
- `outlineColor`: 轮廓颜色
- `hoverColor`: 悬停轮廓颜色
- `geometryColor`: 几何体材质颜色

## 实时预览

参数调整后的效果：
1. **即时响应**：滑块移动时立即更新
2. **平滑过渡**：参数变化平滑，无跳变
3. **准确预览**：使用相同的 ImageOutlinePass，效果一致

## 最佳实践

1. **参数验证**：在 updatePreview 中验证参数范围
2. **防抖处理**：频繁更新时可考虑添加防抖
3. **默认值**：提供合理的默认参数值
4. **重置功能**：允许用户重置到默认值

## 调试技巧

如果参数不生效：
1. 检查 `m_outlinePass` 是否已创建
2. 确认 `updateOutlineParams` 被调用
3. 验证着色器 uniforms 是否正确更新
4. 查看 `ImageOutlinePass::refresh()` 是否执行