# Render Preview System Components

## 概述

渲染预览系统由多个独立的组件组成，每个组件都可以独立运行和测试。这种模块化设计使得系统更加灵活和可维护。

## 组件列表

### 1. RenderPreviewDialog (主对话框)
**文件**: `src/renderpreview/RenderPreviewDialog.cpp`
**头文件**: `include/renderpreview/RenderPreviewDialog.h`

**功能**:
- 整合所有渲染预览组件的主对话框
- 提供完整的渲染预览界面
- 管理配置的保存和加载
- 协调各个面板之间的通信

**独立特性**:
- 可以作为完整的渲染预览应用程序运行
- 包含所有必要的UI组件和功能
- 支持配置管理和预设应用

### 2. LightManagementPanel (灯光管理面板)
**文件**: `src/renderpreview/LightManagementPanel.cpp`
**头文件**: `include/renderpreview/LightManagementPanel.h`

**功能**:
- 管理场景中的多个灯光
- 提供灯光的添加、删除和编辑功能
- 支持方向光、点光源和聚光灯
- 实时调整灯光属性（位置、方向、颜色、强度）

**独立特性**:
- 可以独立运行，不依赖其他组件
- 包含完整的灯光管理UI
- 支持灯光列表和属性编辑
- 可以保存和加载灯光配置

**UI组件**:
- 灯光列表 (wxListBox)
- 添加/删除灯光按钮
- 灯光属性编辑器 (位置、方向、颜色、强度)
- 颜色选择器
- 强度滑块

### 3. LightPresetsPanel (灯光预设面板)
**文件**: `src/renderpreview/LightPresetsPanel.cpp`
**头文件**: `include/renderpreview/LightPresetsPanel.h`

**功能**:
- 提供预定义的灯光预设
- 支持8种不同的灯光场景
- 一键应用预设到场景

**预设类型**:
1. **Studio Lighting** - 专业工作室灯光（主光、补光、轮廓光）
2. **Outdoor Lighting** - 户外自然光（太阳光、天空光）
3. **Dramatic Lighting** - 戏剧性灯光（强对比、阴影）
4. **Warm Lighting** - 温暖灯光（橙黄色调）
5. **Cool Lighting** - 冷色调灯光（蓝色调）
6. **Minimal Lighting** - 极简灯光（简单、微妙）
7. **FreeCAD Three-Light** - FreeCAD三光模型
8. **NavigationCube** - 导航立方体风格灯光

**独立特性**:
- 可以独立运行和测试
- 包含完整的预设按钮网格
- 支持预设信息显示
- 与主对话框通信应用预设

### 4. MaterialPanel (材质面板)
**文件**: `src/renderpreview/MaterialPanel.cpp`
**头文件**: `include/renderpreview/MaterialPanel.h`

**功能**:
- 控制3D对象的材质属性
- 提供材质参数的实时调整
- 支持配置的保存和加载

**材质属性**:
- **Ambient** (环境光) - 0-100%
- **Diffuse** (漫反射) - 0-100%
- **Specular** (镜面反射) - 0-100%
- **Shininess** (光泽度) - 1-128
- **Transparency** (透明度) - 0-100%

**独立特性**:
- 完全独立的材质控制面板
- 包含所有材质参数的滑块控制
- 支持默认值重置
- 配置持久化存储

### 5. PreviewCanvas (预览画布)
**文件**: `src/renderpreview/PreviewCanvas.cpp`
**头文件**: `include/renderpreview/PreviewCanvas.h`

**功能**:
- 3D场景的实时渲染
- 基于Coin3D/Open Inventor的渲染引擎
- 支持鼠标交互和相机控制
- 实时应用材质和灯光设置

**渲染特性**:
- OpenGL加速渲染
- 支持多种几何体（立方体、球体、圆锥体等）
- 实时材质更新
- 动态灯光效果
- 抗锯齿支持

**独立特性**:
- 可以作为独立的3D查看器运行
- 包含完整的OpenGL上下文管理
- 支持鼠标交互（旋转、缩放、平移）
- 实时渲染性能优化

### 6. Texture Panel (纹理面板)
**功能**:
- 控制纹理的启用/禁用
- 设置纹理模式（替换、调制、贴花、混合）
- 调整纹理缩放

**纹理模式**:
- **Replace** - 替换模式
- **Modulate** - 调制模式
- **Decal** - 贴花模式
- **Blend** - 混合模式

### 7. Anti-aliasing Panel (抗锯齿面板)
**功能**:
- 选择抗锯齿方法
- 配置MSAA采样数
- 启用/禁用FXAA

**抗锯齿选项**:
- **None** - 无抗锯齿
- **MSAA** - 多重采样抗锯齿
- **FXAA** - 快速近似抗锯齿
- **MSAA + FXAA** - 组合抗锯齿

## 独立测试

每个组件都可以通过 `RenderPreviewTestApp` 进行独立测试：

```cpp
// 测试完整对话框
RenderPreviewDialog* dialog = new RenderPreviewDialog(parent);
dialog->ShowModal();

// 测试灯光管理面板
LightManagementPanel* panel = new LightManagementPanel(parent, nullptr);

// 测试灯光预设面板
LightPresetsPanel* presets = new LightPresetsPanel(parent, nullptr);

// 测试材质面板
MaterialPanel* material = new MaterialPanel(parent, nullptr);
```

## 配置管理

所有组件都支持配置的保存和加载：

- **ConfigManager** 统一管理所有设置
- 支持INI格式的配置文件
- 自动保存用户偏好设置
- 支持默认值恢复

## 事件处理

组件间通过事件系统进行通信：

- **wxCommandEvent** - 用于按钮点击和选择变化
- **wxSpinDoubleEvent** - 用于数值输入控件
- **自定义事件** - 用于组件间通信

## 依赖关系

**最小依赖**:
- wxWidgets (UI框架)
- Coin3D (3D渲染)
- OpenGL (图形API)
- 标准C++库

**可选依赖**:
- OpenCASCADE (几何处理)
- 自定义配置管理器
- 日志系统

## 扩展性

系统设计支持轻松扩展：

1. **新面板** - 继承 `wxPanel` 并实现相应接口
2. **新预设** - 在 `LightPresetsPanel` 中添加新按钮
3. **新材质** - 扩展 `MaterialPanel` 的参数
4. **新渲染效果** - 在 `PreviewCanvas` 中添加新的渲染模式

## 使用示例

```cpp
// 创建完整的渲染预览对话框
RenderPreviewDialog dialog(parent);
if (dialog.ShowModal() == wxID_OK) {
    // 用户确认了设置
    dialog.saveConfiguration();
}

// 独立使用灯光管理
LightManagementPanel lightPanel(parent, nullptr);
lightPanel.resetToDefaults();

// 独立使用材质控制
MaterialPanel materialPanel(parent, nullptr);
float ambient = materialPanel.getAmbient();
float diffuse = materialPanel.getDiffuse();
```

这种模块化设计确保了每个组件都可以独立开发、测试和维护，同时保持了系统的整体一致性和可扩展性。 