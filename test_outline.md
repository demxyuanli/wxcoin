# 轮廓功能测试步骤

## 问题诊断

当前轮廓功能可能无法显示的原因：

1. **场景纹理渲染问题**
   - SoSceneTexture2需要包含相机的完整场景图
   - 已修复：创建包含相机的临时场景根

2. **调试输出模式**
   - 默认设置为ShowEdge(2)只显示边缘检测结果
   - 已修复：改为Final(0)显示最终效果

3. **渲染顺序问题**
   - 全屏四边形需要正确的材质和光照设置
   - 已修复：添加了BASE_COLOR光照模型

## 调试建议

如果轮廓仍然不显示，请检查：

1. **检查日志输出**
   ```
   查找包含 "ImageOutlinePass" 的日志：
   - "attachOverlay begin/end"
   - "RTT scenes set"
   - "shader program applied"
   - "fullscreen quad added"
   ```

2. **验证OpenGL支持**
   - 检查GL_MAX_TEXTURE_IMAGE_UNITS >= 2
   - 确保着色器编译成功

3. **调整参数**
   - 增加边缘强度：edgeIntensity = 1.0
   - 增加厚度：thickness = 2.0
   - 降低阈值：depthThreshold = 0.0005

4. **切换调试模式**
   在OutlineSettingsDialog中可以通过修改代码添加调试模式切换：
   - 0 = Final (最终效果)
   - 1 = ShowColor (只显示颜色)
   - 2 = ShowEdge (只显示边缘)

## 可能需要的额外修复

1. **确保场景包含光源**
   场景纹理渲染需要光源才能正确计算法线

2. **检查深度缓冲精度**
   深度纹理可能需要更高精度：
   ```cpp
   m_depthTexture->type = SoSceneTexture2::DEPTH24;
   ```

3. **调整投影矩阵范围**
   near/far平面可能影响深度精度

## 测试方法

1. 启动程序
2. 加载3D模型
3. 点击toggleoutline按钮
4. 打开outline settings调整参数
5. 查看控制台日志确认各步骤执行