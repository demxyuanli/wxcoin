# 渲染设置功能说明

## 功能概述

渲染设置功能为3D CAD应用程序提供了完整的材质、光照和纹理配置界面，允许用户实时调整场景的视觉效果。该功能包括一个带有三个标签页的对话框，支持配置文件持久化存储，并提供实时预览功能。

## 主要特性

### 1. 材质设置
- **环境光颜色 (Ambient Color)**: 控制物体在阴影区域的基础颜色
- **漫反射颜色 (Diffuse Color)**: 控制物体在直接光照下的主要颜色
- **镜面反射颜色 (Specular Color)**: 控制物体高光反射的颜色
- **光泽度 (Shininess)**: 控制镜面反射的锐利程度 (0-128)
- **透明度 (Transparency)**: 控制物体的透明程度 (0-100%)

### 2. 光照设置
- **环境光颜色 (Ambient Light Color)**: 控制场景整体的基础照明颜色
- **漫反射光颜色 (Diffuse Light Color)**: 控制主光源的颜色
- **镜面反射光颜色 (Specular Light Color)**: 控制光源高光的颜色
- **光照强度 (Light Intensity)**: 控制主光源的亮度 (0-200%)
- **环境光强度 (Ambient Intensity)**: 控制环境光的强度 (0-100%)

### 3. 纹理设置
- **纹理颜色 (Texture Color)**: 控制纹理的基础颜色
- **纹理强度 (Texture Intensity)**: 控制纹理效果的强度 (0-100%)
- **启用纹理 (Enable Texture)**: 开启或关闭纹理效果

## 使用方法

### 打开渲染设置对话框
1. 在主界面工具栏中找到"渲染设置"按钮
2. 点击按钮打开渲染设置对话框
3. 对话框包含三个标签页：材质、光照、纹理

### 调整设置
1. **颜色选择**: 点击颜色按钮打开颜色选择器
2. **滑块调整**: 拖动滑块调整数值参数
3. **复选框**: 勾选或取消勾选启用/禁用功能
4. **实时预览**: 点击"应用"按钮查看效果
5. **重置默认**: 点击"重置为默认值"恢复初始设置

### 保存设置
- 所有设置更改会自动保存到配置文件
- 下次启动应用程序时会自动加载上次的设置
- 配置文件位置：用户配置目录/rendering_settings.ini

## 技术实现

### 核心组件

#### 1. RenderingSettingsDialog
- **文件位置**: `include/ui/RenderingSettingsDialog.h`, `src/ui/RenderingSettingsDialog.cpp`
- **功能**: 提供用户界面，包含三个标签页和各种控件
- **主要方法**:
  - `OnApply()`: 应用当前设置
  - `OnResetDefaults()`: 重置为默认值
  - `OnColorButtonClick()`: 处理颜色按钮点击事件

#### 2. RenderingSettingsListener
- **文件位置**: `include/commands/RenderingSettingsListener.h`, `src/commands/RenderingSettingsListener.cpp`
- **功能**: 处理渲染设置按钮的点击事件
- **继承**: CommandListener基类

#### 3. RenderingConfig
- **文件位置**: `include/config/RenderingConfig.h`, `src/config/RenderingConfig.cpp`
- **功能**: 配置文件管理，提供设置的持久化存储
- **设计模式**: 单例模式
- **主要功能**:
  - 加载/保存配置文件
  - 提供默认值
  - 自动保存功能

#### 4. OCCGeometry 扩展
- **文件位置**: `include/opencascade/OCCGeometry.h`, `src/opencascade/OCCGeometry.cpp`
- **新增功能**: 材质和纹理属性管理
- **主要方法**:
  - `setMaterial*()`: 设置材质属性
  - `setTexture*()`: 设置纹理属性
  - `buildCoinRepresentation()`: 应用设置到渲染

#### 5. RenderingEngine 扩展
- **文件位置**: `include/rendering/RenderingEngine.h`, `src/rendering/RenderingEngine.cpp`
- **新增功能**: 光照设置管理
- **主要方法**:
  - `setLighting*()`: 设置光照属性
  - `updateLighting()`: 更新光照效果

### 数据结构

#### MaterialSettings
```cpp
struct MaterialSettings {
    double ambientColor[3];    // RGB值 (0.0-1.0)
    double diffuseColor[3];    // RGB值 (0.0-1.0)
    double specularColor[3];   // RGB值 (0.0-1.0)
    double shininess;          // 光泽度 (0-128)
    double transparency;       // 透明度 (0.0-1.0)
};
```

#### LightingSettings
```cpp
struct LightingSettings {
    double ambientColor[3];    // RGB值 (0.0-1.0)
    double diffuseColor[3];    // RGB值 (0.0-1.0)
    double specularColor[3];   // RGB值 (0.0-1.0)
    double lightIntensity;     // 光照强度 (0.0-2.0)
    double ambientIntensity;   // 环境光强度 (0.0-1.0)
};
```

#### TextureSettings
```cpp
struct TextureSettings {
    double textureColor[3];    // RGB值 (0.0-1.0)
    double textureIntensity;   // 纹理强度 (0.0-1.0)
    bool textureEnabled;       // 是否启用纹理
};
```

## 配置文件格式

配置文件采用INI格式，包含三个主要部分：

### rendering_settings.ini
```ini
[Material]
AmbientColorR=0.200000
AmbientColorG=0.200000
AmbientColorB=0.200000
DiffuseColorR=0.680000
DiffuseColorG=0.850000
DiffuseColorB=1.000000
SpecularColorR=0.900000
SpecularColorG=0.900000
SpecularColorB=0.900000
Shininess=60.000000
Transparency=0.000000

[Lighting]
AmbientColorR=0.200000
AmbientColorG=0.200000
AmbientColorB=0.200000
DiffuseColorR=1.000000
DiffuseColorG=1.000000
DiffuseColorB=1.000000
SpecularColorR=1.000000
SpecularColorG=1.000000
SpecularColorB=1.000000
LightIntensity=1.000000
AmbientIntensity=0.200000

[Texture]
TextureColorR=1.000000
TextureColorG=1.000000
TextureColorB=1.000000
TextureIntensity=0.500000
TextureEnabled=false
```

## 默认值说明

### 材质默认值
- **环境光**: 深灰色 (0.2, 0.2, 0.2)
- **漫反射**: 浅绿色 (0.68, 0.85, 1.0) - 保持原有的浅绿色外观
- **镜面反射**: 浅灰色 (0.9, 0.9, 0.9)
- **光泽度**: 60
- **透明度**: 0% (完全不透明)

### 光照默认值
- **环境光**: 深灰色 (0.2, 0.2, 0.2)
- **漫反射光**: 白色 (1.0, 1.0, 1.0)
- **镜面反射光**: 白色 (1.0, 1.0, 1.0)
- **光照强度**: 100%
- **环境光强度**: 20%

### 纹理默认值
- **纹理颜色**: 白色 (1.0, 1.0, 1.0)
- **纹理强度**: 50%
- **纹理状态**: 禁用

## 集成说明

### 命令系统集成
- 新增 `RenderingSettings` 命令类型
- 通过 `CommandDispatcher` 处理渲染设置请求
- 支持命令模式的撤销/重做功能

### UI集成
- 在主界面工具栏添加渲染设置按钮
- 按钮ID: `ID_RENDERING_SETTINGS`
- 事件处理: 通过 `CommandDispatcher` 分发到 `RenderingSettingsListener`

### 构建系统集成
- 更新各模块的 `CMakeLists.txt` 文件
- 添加新的源文件到构建系统
- 确保正确的依赖关系

## 故障排除

### 常见问题
1. **黑色初始化问题**: 通过配置文件提供合理的默认值解决
2. **设置不生效**: 确保调用 `requestViewRefresh()` 更新视图
3. **配置文件丢失**: 系统会自动创建默认配置文件

### 调试建议
1. 检查配置文件是否正确加载
2. 验证颜色值范围 (0.0-1.0)
3. 确认渲染引擎正确应用设置
4. 检查UI控件事件绑定

## 扩展建议

### 未来增强
1. **预设管理**: 添加预设配置的保存和加载功能
2. **实时预览**: 在对话框中提供小型预览窗口
3. **高级材质**: 支持更复杂的材质属性
4. **纹理文件**: 支持从文件加载纹理图像
5. **动画效果**: 添加光照动画和材质变化动画

### 性能优化
1. **延迟更新**: 避免每次参数变化都立即更新渲染
2. **批量处理**: 将多个设置更改合并为单次更新
3. **缓存机制**: 缓存计算结果以提高性能

## 版本历史

### v1.0 (当前版本)
- 基础材质、光照、纹理设置功能
- 配置文件持久化存储
- 实时预览和应用功能
- 默认值和重置功能
- 完整的UI集成

---

*本文档描述了渲染设置功能的完整实现，包括用户界面、技术架构和配置管理。如有问题或建议，请联系开发团队。* 