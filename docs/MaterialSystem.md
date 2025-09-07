# 材质系统说明文档

## 概述

材质系统基于Coin3D的SoMaterial节点构建，提供了完整的材质参数化管理和渲染优化功能。

## 核心组件

### 1. SoMaterial 节点
- **功能**: Coin3D的基础材质节点
- **属性**: 环境光、漫反射、镜面反射、透明度、发光等

### 2. 材质管理器集成
- **位置**: `src/renderpreview/LightManager.cpp`
- **功能**: 在LightManager中集成材质管理

## 材质属性

### 基础属性

#### 1. 颜色属性
```cpp
// 环境光颜色 (Ambient Color)
material->ambientColor.setValue(SbColor(0.2f, 0.2f, 0.2f));

// 漫反射颜色 (Diffuse Color)
material->diffuseColor.setValue(SbColor(0.8f, 0.8f, 0.8f));

// 镜面反射颜色 (Specular Color)
material->specularColor.setValue(SbColor(0.5f, 0.5f, 0.5f));

// 发光颜色 (Emissive Color)
material->emissiveColor.setValue(SbColor(0.0f, 0.0f, 0.0f));
```

#### 2. 光学属性
```cpp
// 镜面反射指数 (Shininess)
material->shininess.setValue(0.5f);

// 透明度 (Transparency)
material->transparency.setValue(0.0f); // 0.0 = 不透明, 1.0 = 完全透明
```

### 材质类型

#### 1. 标准材质 (Standard Material)
```cpp
SoMaterial* standardMaterial = new SoMaterial();
standardMaterial->ambientColor.setValue(SbColor(0.2f, 0.2f, 0.2f));
standardMaterial->diffuseColor.setValue(SbColor(0.8f, 0.8f, 0.8f));
standardMaterial->specularColor.setValue(SbColor(0.5f, 0.5f, 0.5f));
standardMaterial->shininess.setValue(0.5f);
standardMaterial->transparency.setValue(0.0f);
```

#### 2. 金属材质 (Metal Material)
```cpp
SoMaterial* metalMaterial = new SoMaterial();
metalMaterial->ambientColor.setValue(SbColor(0.1f, 0.1f, 0.1f));
metalMaterial->diffuseColor.setValue(SbColor(0.7f, 0.7f, 0.7f));
metalMaterial->specularColor.setValue(SbColor(0.9f, 0.9f, 0.9f));
metalMaterial->shininess.setValue(0.9f);
metalMaterial->transparency.setValue(0.0f);
```

#### 3. 塑料材质 (Plastic Material)
```cpp
SoMaterial* plasticMaterial = new SoMaterial();
plasticMaterial->ambientColor.setValue(SbColor(0.3f, 0.3f, 0.3f));
plasticMaterial->diffuseColor.setValue(SbColor(0.9f, 0.9f, 0.9f));
plasticMaterial->specularColor.setValue(SbColor(0.3f, 0.3f, 0.3f));
plasticMaterial->shininess.setValue(0.3f);
plasticMaterial->transparency.setValue(0.0f);
```

#### 4. 玻璃材质 (Glass Material)
```cpp
SoMaterial* glassMaterial = new SoMaterial();
glassMaterial->ambientColor.setValue(SbColor(0.1f, 0.1f, 0.1f));
glassMaterial->diffuseColor.setValue(SbColor(0.8f, 0.8f, 0.8f));
glassMaterial->specularColor.setValue(SbColor(0.9f, 0.9f, 0.9f));
glassMaterial->shininess.setValue(0.95f);
glassMaterial->transparency.setValue(0.8f);
```

## 材质管理

### 1. 材质创建和分配

#### 基础材质创建
```cpp
// 创建材质节点
SoMaterial* material = new SoMaterial();
material->ref(); // 增加引用计数

// 设置材质属性
material->ambientColor.setValue(SbColor(0.2f, 0.2f, 0.2f));
material->diffuseColor.setValue(SbColor(0.8f, 0.8f, 0.8f));
material->specularColor.setValue(SbColor(0.5f, 0.5f, 0.5f));
material->shininess.setValue(0.5f);

// 添加到场景图
separator->addChild(material);
```

#### 动态材质更新
```cpp
// 更新材质颜色
void updateMaterialColor(SoMaterial* material, const SbColor& newColor) {
    if (material) {
        material->diffuseColor.setValue(newColor);
    }
}

// 更新材质透明度
void updateMaterialTransparency(SoMaterial* material, float transparency) {
    if (material) {
        material->transparency.setValue(transparency);
    }
}
```

### 2. 材质搜索和更新

#### 场景材质搜索
```cpp
// 搜索场景中的所有材质节点
SoSearchAction searchAction;
searchAction.setType(SoMaterial::getClassTypeId(), true);
searchAction.setSearchingAll(true);
searchAction.apply(sceneRoot);

// 遍历找到的材质节点
for (int i = 0; i < searchAction.getPaths().getLength(); ++i) {
    SoFullPath* path = static_cast<SoFullPath*>(searchAction.getPaths()[i]);
    if (path) {
        SoNode* tailNode = path->getTail();
        if (tailNode && tailNode->isOfType(SoMaterial::getClassTypeId())) {
            SoMaterial* material = static_cast<SoMaterial*>(tailNode);
            // 更新材质属性
            updateMaterialProperties(material);
        }
    }
}
```

## 光照与材质交互

### 1. 光照模型

#### Phong光照模型
```cpp
// 设置Phong光照模型
SoLightModel* lightModel = new SoLightModel();
lightModel->model.setValue(SoLightModel::PHONG);
sceneRoot->addChild(lightModel);
```

#### 光照计算
```cpp
// 材质颜色与光照的交互
// 最终颜色 = 环境光 + 漫反射 + 镜面反射 + 发光

// 环境光计算
SbColor ambientResult = material->ambientColor[0] * light->color[0] * light->intensity[0];

// 漫反射计算
SbColor diffuseResult = material->diffuseColor[0] * light->color[0] * light->intensity[0];

// 镜面反射计算
SbColor specularResult = material->specularColor[0] * light->color[0] * light->intensity[0];
```

### 2. 材质更新机制

#### 自动材质更新
```cpp
void LightManager::updateMaterialsForLighting() {
    // Coin3D自动处理材质光照
    // 不需要手动调整材质颜色
    LOG_INF_S("LightManager::updateMaterialsForLighting: Coin3D handles material lighting automatically");
}
```

## 使用示例

### 1. 基础材质使用

```cpp
// 创建几何体
SoCube* cube = new SoCube();
cube->width.setValue(2.0f);
cube->height.setValue(2.0f);
cube->depth.setValue(2.0f);

// 创建材质
SoMaterial* material = new SoMaterial();
material->ambientColor.setValue(SbColor(0.2f, 0.2f, 0.2f));
material->diffuseColor.setValue(SbColor(0.8f, 0.8f, 0.8f));
material->specularColor.setValue(SbColor(0.5f, 0.5f, 0.5f));
material->shininess.setValue(0.5f);

// 创建分离器并添加节点
SoSeparator* cubeGroup = new SoSeparator();
cubeGroup->addChild(material);
cubeGroup->addChild(cube);

// 添加到场景
sceneRoot->addChild(cubeGroup);
```

### 2. 动态材质更新

```cpp
// 更新材质颜色
void updateMaterialColor(SoMaterial* material, float r, float g, float b) {
    if (material) {
        SbColor newColor(r, g, b);
        material->diffuseColor.setValue(newColor);
    }
}

// 更新材质透明度
void updateMaterialTransparency(SoMaterial* material, float alpha) {
    if (material) {
        material->transparency.setValue(alpha);
    }
}
```

## 故障排除

### 常见问题

1. **材质不显示**
   - 检查材质节点是否正确添加到场景图
   - 确认材质引用计数是否正确
   - 验证材质属性设置是否合理

2. **光照效果不正确**
   - 检查SoLightModel是否正确设置
   - 确认光源和材质的颜色设置
   - 验证光照强度参数

3. **透明度问题**
   - 检查透明度值范围 (0.0 - 1.0)
   - 确认渲染顺序是否正确
   - 验证混合模式设置

## 最佳实践

### 1. 材质管理
- 使用材质缓存减少重复创建
- 正确管理引用计数
- 及时清理不需要的材质节点

### 2. 性能优化
- 批量更新材质属性
- 使用材质预设减少计算
- 优化材质搜索算法

### 3. 内存管理
- 正确使用ref()和unref()
- 避免材质节点泄漏
- 及时清理材质缓存

### 4. 类型安全
- 使用isOfType()检查节点类型
- 安全的类型转换
- 验证材质节点有效性 