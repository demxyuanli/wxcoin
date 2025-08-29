# FBO轮廓调试指南

## 启用日志窗口

1. 打开轮廓设置对话框
2. 点击"Show Log"按钮
3. 日志窗口支持：
   - 右键复制选中文本
   - "Copy All"按钮复制全部日志
   - 自动滚动显示最新消息

## 检查清单

### 1. OpenGL环境
查看日志中的：
```
OpenGL version: 4.6.0
OpenGL vendor: NVIDIA Corporation
OpenGL renderer: GeForce GTX 1080
```

### 2. 扩展函数加载
检查是否有错误：
```
FBO functions not available
Shader functions not available
```

### 3. 着色器编译
查看着色器创建日志：
```
Initializing shaders...
Normal shader created: 1
Outline shader created: 2
Shaders initialized: normal=1, outline=2
```

如果有错误会显示：
```
Shader compilation failed: <错误信息>
Shader program linking failed: <错误信息>
```

### 4. FBO初始化
检查FBO创建：
```
Initializing FBO: 800x600
FBO created: 1
```

如果失败会显示：
```
FBO creation failed with status: 0x8CD6
```

### 5. 渲染模式
查看当前使用的渲染模式：
```
Attempting FBO render: fbo=1, normal=1, outline=2
```

## 常见问题及解决方案

### 问题1：白屏
- 检查着色器编译错误
- 确认FBO创建成功
- 查看OpenGL错误日志

### 问题2：仍显示线框
- 检查FBO是否成功创建
- 确认着色器都正确编译
- 查看"Attempting FBO render"日志

### 问题3：轮廓效果不明显
调整参数：
- Thickness: 增大线宽
- Intensity: 增大强度
- Normal Weight: 增加法线权重

## 调试步骤

1. **清空日志**
   - 点击"Clear"按钮清空日志

2. **重新打开对话框**
   - 关闭并重新打开轮廓设置对话框
   - 查看初始化日志

3. **调整预览窗口大小**
   - 拖动分隔条改变预览窗口大小
   - 查看FBO重新初始化日志

4. **复制完整日志**
   - 点击"Copy All"
   - 粘贴到文本编辑器分析

## 高级调试

### 强制使用FBO模式
如果看到"VAO not supported"但仍想尝试FBO：
- 代码会自动回退到VBO模式
- 检查是否有其他OpenGL错误

### 检查纹理格式
FBO可能因纹理格式不支持而失败：
- GL_RGBA8: 颜色纹理
- GL_DEPTH_COMPONENT24: 深度纹理

### 验证着色器
确保着色器语法兼容：
- 使用GLSL 1.10（无版本声明）
- 避免使用高版本特性