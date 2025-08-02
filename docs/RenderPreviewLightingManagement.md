# RenderPreviewDialog 灯光管理功能

## 概述

为RenderPreviewDialog实现了详细的灯光管理功能，参考LightingSettingsDialog的设计，提供了多灯光管理、灯光属性编辑、预设灯光等功能。

## 新增功能

### 1. 多灯光管理 (Light Management)

#### 功能特性
- **灯光列表**: 显示当前场景中的所有灯光
- **添加灯光**: 可以动态添加新的灯光到场景中
- **删除灯光**: 可以删除不需要的灯光（至少保留一个灯光）
- **灯光选择**: 点击列表中的灯光可以选中并编辑其属性

#### 灯光属性编辑
每个灯光包含以下可编辑属性：
- **名称**: 灯光的显示名称
- **类型**: 支持Directional、Point、Spot三种类型
- **启用状态**: 可以启用或禁用灯光
- **位置**: X、Y、Z坐标位置
- **方向**: X、Y、Z方向向量
- **颜色**: 通过颜色选择器设置灯光颜色
- **强度**: 通过滑块调节灯光强度

### 2. 灯光预设 (Light Presets)

提供了8种典型的灯光预设，每种预设都有不同的灯光配置：

#### 预设类型
1. **Studio Lighting**: 专业工作室灯光，包含主光、补光和轮廓光
2. **Outdoor Lighting**: 户外自然光，包含太阳光和天空光
3. **Dramatic Lighting**: 戏剧性灯光，强对比度和阴影
4. **Warm Lighting**: 温暖灯光，橙黄色调
5. **Cool Lighting**: 冷色调灯光，蓝色调
6. **Minimal Lighting**: 极简灯光，微妙阴影
7. **FreeCAD Three-Light**: 经典FreeCAD三灯模型
8. **Navigation Cube**: NavigationCube风格的多方向灯光

#### 预设功能
- 一键应用预设到场景
- 实时预览效果
- 预设信息显示
- 用户反馈提示

### 3. 界面布局

#### 标签页结构
1. **Light Management**: 多灯光管理界面
2. **Light Presets**: 灯光预设选择界面
3. **Material**: 材质设置
4. **Texture**: 纹理设置
5. **Anti-aliasing**: 抗锯齿设置

#### 布局特点
- 左侧配置面板（固定450px宽度）
- 右侧预览画布（自适应宽度）
- 底部按钮栏（重置、保存、取消）

### 4. 实时更新

#### 功能特性
- **实时预览**: 灯光参数改变时立即应用到预览画布
- **即时反馈**: 用户操作后立即看到效果
- **状态同步**: UI状态与内部数据保持同步

## 技术实现

### 数据结构

```cpp
struct RenderLightSettings {
    bool enabled;
    std::string name;
    std::string type;  // "directional", "point", "spot"
    
    // Position and direction
    double positionX, positionY, positionZ;
    double directionX, directionY, directionZ;
    
    // Color and intensity
    wxColour color;
    double intensity;
    
    // Spot light specific
    double spotAngle;
    double spotExponent;
};
```

### 主要方法

#### 灯光管理
- `updateLightList()`: 更新灯光列表显示
- `updateLightProperties()`: 更新灯光属性编辑界面
- `onAddLight()`: 添加新灯光
- `onRemoveLight()`: 删除灯光
- `onLightPropertyChanged()`: 处理灯光属性变化

#### 预设管理
- `applyPresetAndUpdate()`: 应用预设并更新界面
- `onStudioPreset()`: 工作室预设
- `onOutdoorPreset()`: 户外预设
- 等等...

#### 画布更新
- `applyLightingToCanvas()`: 将灯光设置应用到预览画布

### 配置管理

#### 保存配置
- 保存传统灯光设置
- 保存材质、纹理、抗锯齿设置
- 配置持久化存储

#### 加载配置
- 加载保存的配置
- 初始化默认灯光设置
- 恢复用户偏好

## 使用说明

### 基本操作

1. **打开对话框**: 通过菜单或按钮打开RenderPreviewDialog
2. **选择标签页**: 根据需要选择相应的功能标签页
3. **管理灯光**: 在Light Management标签页中添加、删除、编辑灯光
4. **应用预设**: 在Light Presets标签页中选择并应用预设
5. **实时预览**: 在右侧画布中查看效果
6. **保存设置**: 点击保存按钮保存当前配置

### 灯光编辑

1. **选择灯光**: 在灯光列表中点击要编辑的灯光
2. **修改属性**: 在右侧属性面板中修改灯光参数
3. **实时预览**: 参数修改后立即在画布中看到效果
4. **保存更改**: 点击保存按钮保存更改

### 预设使用

1. **选择预设**: 在Light Presets标签页中点击预设按钮
2. **查看效果**: 预设应用后立即在画布中看到效果
3. **进一步调整**: 可以在Light Management中进一步调整预设的灯光

## 兼容性

### 简化设计
- 移除了传统的灯光控制功能，专注于新的多灯光管理系统
- 界面更加简洁，用户体验更好
- 专注于高级灯光管理功能

### 扩展性
- 可以轻松添加新的灯光类型
- 可以扩展预设系统
- 可以添加更多灯光属性

## 总结

新的灯光管理功能为RenderPreviewDialog提供了强大的多灯光管理能力，用户可以：

1. **精确控制**: 对每个灯光进行精确的位置、方向、颜色、强度控制
2. **快速设置**: 通过预设快速应用典型的灯光配置
3. **实时预览**: 所有修改都能立即在预览画布中看到效果
4. **灵活管理**: 可以添加、删除、启用、禁用灯光
5. **配置持久化**: 所有设置都可以保存和恢复

这些功能大大提升了RenderPreviewDialog的实用性和用户体验，使其成为一个功能完整的渲染预览工具。 