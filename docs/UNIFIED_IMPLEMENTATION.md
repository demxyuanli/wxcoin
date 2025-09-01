# 统一的轮廓渲染实现

## 重构概述

预览窗口现在直接使用 `ImageOutlinePass` 类，而不是重复实现轮廓渲染算法。

## 架构改进

### 之前的问题
- OutlinePreviewCanvas 重复实现了轮廓算法
- 维护两套相同的着色器代码
- 预览效果可能与实际效果不一致

### 新的架构
```
OutlinePreviewCanvas
    ├── SceneManager（场景管理）
    ├── ImageOutlinePass（轮廓渲染）
    └── Coin3D场景图（几何模型）
```

## 关键改变

### 1. 移除重复代码
- 删除了所有 FBO 管理代码
- 删除了着色器编译和管理
- 删除了 OpenGL 扩展加载

### 2. 使用 ImageOutlinePass
```cpp
// 创建和初始化
m_outlinePass = std::make_unique<ImageOutlinePass>(m_sceneManager.get());
m_outlinePass->setEnabled(m_outlineEnabled);
m_outlinePass->setParams(m_outlineParams);
```

### 3. 简化渲染流程
```cpp
void OutlinePreviewCanvas::render() {
    // 设置背景色
    glClearColor(...);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // ImageOutlinePass 会自动处理轮廓渲染
    SoGLRenderAction renderAction(viewport);
    renderAction.apply(m_sceneRoot);
    
    SwapBuffers();
}
```

## 优势

1. **代码复用**：不再维护重复的算法实现
2. **一致性保证**：预览和实际效果完全一致
3. **易于维护**：算法改进只需在一处修改
4. **减少错误**：避免两处实现不同步的问题

## 职责分离

### OutlinePreviewCanvas 负责：
- 创建预览场景（立方体、球体等）
- 处理用户交互（鼠标旋转）
- 管理 OpenGL 上下文
- 设置材质颜色

### ImageOutlinePass 负责：
- 实现轮廓渲染算法
- 管理 FBO 和着色器
- 处理后处理效果
- 参数调节

## 悬停效果

悬停效果的实现需要 ImageOutlinePass 支持选择性渲染：
- 可以考虑添加掩码纹理
- 或者支持多个轮廓颜色
- 这是未来的改进方向

## 总结

通过这次重构，我们实现了真正的算法统一：
- 预览窗口专注于场景构建
- ImageOutlinePass 专注于渲染算法
- 两者通过 SceneManager 协作
- 确保了预览效果与实际效果的完全一致