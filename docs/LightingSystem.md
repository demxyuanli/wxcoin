# 光照系统说明文档

## 概述

本光照系统基于Coin3D框架构建，提供了完整的参数化光照管理功能，支持动态光源创建、动画控制、性能优化和预设配置。

## 系统架构

### 核心组件

#### 1. LightManager 类
- **位置**: `include/renderpreview/LightManager.h`
- **实现**: `src/renderpreview/LightManager.cpp`
- **功能**: 光照系统的核心管理器

#### 2. RenderLightSettings 结构
- **位置**: `include/renderpreview/RenderLightSettings.h`
- **功能**: 定义光源的所有参数化属性

#### 3. PreviewCanvas 集成
- **位置**: `src/renderpreview/PreviewCanvas.cpp`
- **功能**: 将光照系统集成到预览画布中

## 功能特性

### 1. 动态光照管理

#### 光源类型支持
```cpp
// 方向光 (Directional Light)
SoDirectionalLight* dirLight = new SoDirectionalLight();
dirLight->direction.setValue(SbVec3f(0.0f, -1.0f, -1.0f));

// 点光源 (Point Light)  
SoPointLight* pointLight = new SoPointLight();
pointLight->location.setValue(SbVec3f(0.0f, 5.0f, 5.0f));

// 聚光灯 (Spot Light)
SoSpotLight* spotLight = new SoSpotLight();
spotLight->location.setValue(SbVec3f(0.0f, 5.0f, 5.0f));
spotLight->direction.setValue(SbVec3f(0.0f, -1.0f, -1.0f));
```

#### 场景图结构
```
SceneRoot
├── Camera
├── LightModel (PHONG)
├── Light1 (Directional)
├── Light2 (Point) 
├── Light3 (Spot)
├── ObjectRoot
│   ├── Geometry1
│   ├── Geometry2
│   └── LightIndicators
└── EventCallbacks
```

### 2. 动画系统

#### 轨道动画
```cpp
// 计算轨道位置
double angle = time * settings.animationSpeed * 2.0 * M_PI;
double x = settings.animationRadius * cos(angle);
double z = settings.animationRadius * sin(angle);
double y = settings.animationHeight;
```

#### 动画控制
```cpp
// 启动动画
lightManager->startAnimation();

// 停止动画
lightManager->stopAnimation();

// 设置动画帧率
lightManager->setAnimationRate(60); // 60 FPS
```

### 3. 事件系统

#### 键盘事件
- **'L'键**: 添加随机动态光源
- **'A'键**: 切换动画开关

#### 鼠标事件
- **右键**: 删除最后一个光源

### 4. 性能优化

#### 光源数量限制
```cpp
static const int MAX_LIGHTS = 8; // OpenGL标准限制
```

#### 优先级管理
```cpp
struct RenderLightSettings {
    int priority; // 光源优先级，数值越高越重要
};
```

### 5. 预设光照配置

#### 三点光照 (Three-Point Lighting)
- 主光源 (Key Light)
- 填充光 (Fill Light)  
- 轮廓光 (Rim Light)

#### 工作室光照 (Studio Lighting)
- 聚光灯主光源
- 环境光填充

#### 户外光照 (Outdoor Lighting)
- 太阳光 (暖色调)
- 天空光 (冷色调)

## API 参考

### LightManager 类方法

#### 基础管理
```cpp
// 添加光源
int addLight(const RenderLightSettings& settings);

// 移除光源
bool removeLight(int lightId);

// 更新光源
bool updateLight(int lightId, const RenderLightSettings& settings);

// 清空所有光源
void clearAllLights();
```

#### 动画控制
```cpp
// 启动动画
void startAnimation();

// 停止动画
void stopAnimation();

// 设置动画帧率
void setAnimationRate(int fps);

// 检查动画状态
bool isAnimationRunning() const;
```

#### 预设配置
```cpp
// 创建三点光照
void createThreePointLighting();

// 创建工作室光照
void createStudioLighting();

// 创建户外光照
void createOutdoorLighting();
```

### RenderLightSettings 结构

```cpp
struct RenderLightSettings {
    // 基础属性
    bool enabled;
    std::string name;
    std::string type;  // "directional", "point", "spot"
    
    // 位置和方向
    double positionX, positionY, positionZ;
    double directionX, directionY, directionZ;
    
    // 颜色和强度
    wxColour color;
    double intensity;
    
    // 动画参数
    bool animated;
    double animationSpeed;  // 旋转速度
    double animationRadius; // 轨道半径
    double animationHeight; // 轨道高度
    
    // 性能参数
    int priority;  // 光源优先级
};
```

## 使用示例

### 1. 基本使用

```cpp
// 创建光照管理器
LightManager* lightManager = new LightManager(sceneRoot, objectRoot);

// 创建三点光照
lightManager->createThreePointLighting();

// 设置事件回调
lightManager->setupEventCallbacks(sceneRoot);

// 启动动画
lightManager->startAnimation();
```

### 2. 自定义光源

```cpp
// 创建自定义光源
RenderLightSettings customLight;
customLight.name = "Custom Light";
customLight.type = "point";
customLight.positionX = 5.0;
customLight.positionY = 3.0;
customLight.positionZ = 2.0;
customLight.intensity = 1.5f;
customLight.color = wxColour(255, 200, 100); // 暖色调
customLight.animated = true;
customLight.animationSpeed = 0.5;
customLight.animationRadius = 3.0;
customLight.priority = 2;

// 添加光源
int lightId = lightManager->addLight(customLight);
```

## 注意事项

### 1. 内存管理
- 所有Coin3D节点都使用引用计数管理
- 使用 `ref()` 和 `unref()` 正确管理内存
- 避免直接使用 `delete` 删除Coin3D节点

### 2. 性能考虑
- OpenGL限制最多8个光源
- 使用优先级队列管理光源
- 定期优化光源顺序

### 3. 类型安全
- 使用 `isOfType()` 进行安全的类型转换
- 检查节点类型后再进行操作 