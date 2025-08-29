# 预览窗口调试指南

## 问题症状
预览窗口显示白色，看不到任何内容

## 可能的原因

1. **OpenGL扩展加载失败**
   - FBO函数未正确加载
   - 着色器函数未正确加载
   
2. **着色器编译失败**
   - OpenGL版本不兼容
   - GLSL语法错误
   
3. **FBO创建失败**
   - 纹理格式不支持
   - FBO不完整

## 调试步骤

### 1. 检查日志输出
查看以下日志信息：
- "OpenGL version: xxx" - 确认OpenGL版本
- "FBO functions not available" - 检查FBO支持
- "Shader functions not available" - 检查着色器支持
- "Shader compilation failed: xxx" - 着色器编译错误
- "Shader program linking failed: xxx" - 着色器链接错误
- "FBO creation failed with status: xxx" - FBO创建错误

### 2. 当前的回退机制
如果FBO或着色器不可用，预览窗口会自动回退到简单的线框轮廓模式：
- 使用固定管线渲染
- 通过glPolygonMode(GL_LINE)绘制线框
- 不需要着色器或FBO支持

### 3. 解决方案

#### 如果是OpenGL版本问题：
- 确保显卡驱动已更新
- 确认OpenGL 2.0+支持

#### 如果是着色器编译问题：
- 检查GLSL版本兼容性
- 已将着色器版本降到110以提高兼容性

#### 如果是FBO问题：
- 检查GL_ARB_framebuffer_object扩展
- 确认纹理格式支持

## 技术细节

### 着色器版本
- 使用GLSL 1.10（兼容OpenGL 2.0）
- 移除了所有#version声明以提高兼容性

### 渲染流程
1. 检查资源可用性
2. 如果FBO/着色器可用：
   - Pass 1: 渲染法线到纹理
   - Pass 2: 渲染颜色到纹理
   - Pass 3: 后处理轮廓
3. 如果不可用：
   - 使用简单线框模式

### 简化措施
- 移除了深度边缘检测
- 简化了法线边缘检测算法
- 减少了uniform参数数量