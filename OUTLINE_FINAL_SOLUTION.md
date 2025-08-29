# 轮廓渲染最终解决方案

## 问题总结
预览窗口的轮廓渲染多次尝试后仍然无法正常显示。主要问题：
1. 复杂的场景图结构导致单独渲染对象困难
2. OpenGL状态管理复杂
3. Coin3D与原生OpenGL混合使用的兼容性问题

## 最终方案
使用**最简单的线框渲染**方法：

```cpp
// 1. 先正常渲染场景
renderAction.apply(m_sceneRoot);

// 2. 再用线框模式渲染一次
glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
glLineWidth(2.0f);
glColor3f(0.0f, 0.0f, 0.0f);  // 黑色轮廓
wireAction.apply(m_sceneRoot);
```

## 实现特点

### 简单可靠
- 不需要复杂的对象提取
- 不需要几何缩放
- 不需要多通道渲染

### 悬停效果
- 无悬停：黑色线框
- 有悬停：橙色线框（所有对象）

### 技术细节
- 使用`GL_POLYGON_OFFSET_LINE`避免z-fighting
- 使用`GL_LINE_SMOOTH`平滑线条
- 禁用光照确保纯色显示

## 如果仍然不工作

可能的原因：
1. **m_outlineEnabled默认为false**
   - 需要确保初始化为true
   
2. **场景结构问题**
   - Coin3D的材质或状态可能覆盖了GL设置
   
3. **深度缓冲问题**
   - 线框可能被实体遮挡

## 建议的调试步骤

1. 在构造函数中添加：
   ```cpp
   m_outlineEnabled = true;
   ```

2. 检查材质设置：
   ```cpp
   // 在创建对象时
   SoMaterial* mat = new SoMaterial;
   mat->diffuseColor.setValue(0.8f, 0.8f, 0.8f);
   mat->emissiveColor.setValue(0, 0, 0);  // 确保无自发光
   ```

3. 如果还是不行，可以尝试：
   - 使用固定的轮廓颜色测试
   - 增大线宽到5.0f
   - 完全禁用深度测试

## 结论

当前实现已经是最简单可靠的方案。如果仍然看不到轮廓，问题可能在于：
- 参数初始化
- Coin3D材质设置
- OpenGL上下文状态

建议检查这些基础设置而不是继续修改渲染算法。