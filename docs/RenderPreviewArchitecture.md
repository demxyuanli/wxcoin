# Render Preview System Architecture - Redesign

## 概述

根据用户的要求，我们重新设计了渲染预览系统的架构，明确区分了全局设置和对象级设置。这种设计使得系统更加清晰、模块化，并且符合实际的使用场景。

## 架构重新设计

### 1. 全局设置 (Global Settings)

**影响范围**: 整个场景和视图
**应用对象**: 所有对象

#### 1.1 GlobalSettingsPanel (全局设置面板)
**文件**: 
- `include/renderpreview/GlobalSettingsPanel.h`
- `src/renderpreview/GlobalSettingsPanel.cpp`

**功能**:
- **抗锯齿设置** - 影响整个渲染管线的质量
  - 抗锯齿方法选择 (None, MSAA, FXAA, MSAA + FXAA)
  - MSAA采样数配置 (2-16)
  - FXAA启用/禁用
- **渲染模式设置** - 影响整个视图的渲染模式
  - Solid (实体)
  - Wireframe (线框)
  - Points (点)
  - Hidden Line (隐藏线)
  - Shaded (着色)

**特点**:
- 设置影响整个场景
- 所有对象都会应用相同的全局设置
- 配置保存在 `Global.*` 命名空间下

#### 1.2 LightManagementPanel (灯光管理面板)
**功能**: 管理场景中的多个灯光
- 添加、删除、编辑灯光
- 支持方向光、点光源、聚光灯
- 实时调整灯光属性

#### 1.3 LightPresetsPanel (灯光预设面板)
**功能**: 提供预定义的灯光场景
- 8种不同的灯光预设
- 一键应用预设到整个场景

### 2. 对象级设置 (Object-level Settings)

**影响范围**: 特定几何对象
**应用对象**: 选中的对象

#### 2.1 ObjectSettingsPanel (对象设置面板)
**文件**: 
- `include/renderpreview/ObjectSettingsPanel.h`
- `src/renderpreview/ObjectSettingsPanel.cpp`

**功能**:
- **纹理设置** - 针对特定几何对象的纹理映射
  - 纹理启用/禁用
  - 纹理模式 (Replace, Modulate, Decal, Blend)
  - 纹理缩放

**特点**:
- 设置只影响选中的对象
- 不同对象可以有不同的纹理设置
- 配置保存在 `Object.*` 命名空间下

#### 2.2 MaterialPanel (材质面板)
**功能**: 控制3D对象的材质属性
- Ambient (环境光) - 0-100%
- Diffuse (漫反射) - 0-100%
- Specular (镜面反射) - 0-100%
- Shininess (光泽度) - 1-128
- Transparency (透明度) - 0-100%

## 架构优势

### 1. 清晰的职责分离
- **全局设置**: 影响整个场景的渲染质量和模式
- **对象设置**: 影响特定对象的表面属性

### 2. 符合实际使用场景
- 灯光和抗锯齿确实是全局设置
- 材质和纹理确实是对象级设置
- 用户界面反映了这种逻辑分离

### 3. 更好的可维护性
- 每个面板都有明确的职责
- 配置管理更加清晰
- 代码结构更加模块化

### 4. 扩展性
- 可以轻松添加新的全局设置
- 可以轻松添加新的对象级设置
- 面板之间相互独立

## 配置管理

### 全局设置配置
```ini
[RenderPreview]
Global.AntiAliasing.Method=1
Global.AntiAliasing.MSAASamples=4
Global.AntiAliasing.FXAAEnabled=false
Global.RenderingMode=4
```

### 对象设置配置
```ini
[RenderPreview]
Object.Texture.Enabled=false
Object.Texture.Mode=0
Object.Texture.Scale=100
```

### 材质设置配置
```ini
[RenderPreview]
Material.Ambient=30
Material.Diffuse=70
Material.Specular=50
Material.Shininess=32
Material.Transparency=0
```

## 用户界面组织

### 标签页结构
1. **Global Settings** - 全局设置
   - 抗锯齿配置
   - 渲染模式选择
2. **Light Management** - 灯光管理
   - 多灯光管理
   - 灯光属性编辑
3. **Light Presets** - 灯光预设
   - 8种预设场景
4. **Object Settings** - 对象设置
   - 纹理配置
5. **Material** - 材质设置
   - 材质属性控制

### 应用按钮
- **Apply Global Settings** - 应用全局设置
- **Apply Object Settings** - 应用对象设置
- **Apply** - 应用所有设置

## 技术实现

### 1. 面板独立性
每个面板都是独立的 `wxPanel` 子类，具有：
- 自己的UI创建逻辑
- 自己的事件处理
- 自己的配置管理
- 自己的默认值重置

### 2. 事件处理
- 全局设置变化触发 `updateGlobal*()` 方法
- 对象设置变化触发 `applyObjectSettingsToCanvas()`
- 主对话框协调各个面板的通信

### 3. 渲染更新
- 全局设置更新整个场景
- 对象设置只更新选中的对象
- 支持实时预览和批量应用

## 使用示例

### 全局设置应用
```cpp
// 应用全局抗锯齿设置
globalSettingsPanel->setAntiAliasingMethod(1); // MSAA
globalSettingsPanel->setMSAASamples(8);
globalSettingsPanel->setFXAAEnabled(true);

// 应用全局渲染模式
globalSettingsPanel->setRenderingMode(4); // Shaded
```

### 对象设置应用
```cpp
// 应用对象纹理设置
objectSettingsPanel->setTextureEnabled(true);
objectSettingsPanel->setTextureMode(1); // Modulate
objectSettingsPanel->setTextureScale(1.5f);

// 应用材质设置
materialPanel->setAmbient(0.3f);
materialPanel->setDiffuse(0.7f);
materialPanel->setSpecular(0.5f);
```

## 总结

重新设计的架构明确区分了全局设置和对象级设置，使得：

1. **用户界面更加直观** - 用户清楚地知道哪些设置影响整个场景，哪些只影响选中的对象
2. **代码结构更加清晰** - 每个面板都有明确的职责和边界
3. **配置管理更加合理** - 全局设置和对象设置分别管理
4. **扩展性更好** - 可以轻松添加新的设置类型

这种设计符合实际CAD软件的使用习惯，用户期望灯光和渲染质量是全局的，而材质和纹理是针对特定对象的。 