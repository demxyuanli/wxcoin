# PreviewCanvas 光照系统重构总结

## 重构目标

将PreviewCanvas的光照系统从硬编码的固定光照改为完全基于LightManager的动态光照系统，使GlobalSettingsPanel中的光照设置能够有效影响预览视口的渲染。

## 主要变更

### 1. 构造函数修改
- 移除了在`initializeScene()`中调用`setupLighting()`
- 在LightManager初始化后调用`setupDefaultLighting()`

### 2. 光照初始化重构
- **原方法**: `setupLighting()` - 直接创建硬编码的SoDirectionalLight节点
- **新方法**: `setupDefaultLighting()` - 使用LightManager创建默认三点光照系统

#### 默认光照配置
```cpp
// 主光（45度顶光）- 强度1.0
RenderLightSettings mainLight;
mainLight.name = "Main Light";
mainLight.type = "directional";
mainLight.directionX = 0.0f;
mainLight.directionY = -0.707f;
mainLight.directionZ = -0.707f;
mainLight.intensity = 1.0f;

// 左侧填充光 - 强度0.6
RenderLightSettings leftLight;
leftLight.name = "Fill Light";
leftLight.type = "directional";
leftLight.directionX = -1.0f;
leftLight.intensity = 0.6f;

// 顶部边缘光 - 强度0.8
RenderLightSettings topLight;
topLight.name = "Rim Light";
topLight.type = "directional";
topLight.directionY = 1.0f;
topLight.intensity = 0.8f;
```

### 3. 光照更新方法重构
- **原方法**: 直接操作场景中的SoDirectionalLight节点
- **新方法**: 通过LightManager更新光照设置

#### updateLighting()方法改进
```cpp
void PreviewCanvas::updateLighting(float ambient, float diffuse, float specular, const wxColour& color, float intensity)
{
    // 更新材质属性
    m_lightMaterial->ambientColor.setValue(...);
    
    // 通过LightManager获取和更新光照
    auto currentLights = m_lightManager->getAllLightSettings();
    std::vector<RenderLightSettings> updatedLights;
    
    for (auto& light : currentLights) {
        light.color = color;
        light.intensity = light.intensity * intensity;
        updatedLights.push_back(light);
    }
    
    // 应用更新
    m_lightManager->updateMultipleLights(updatedLights);
}
```

### 4. 新增方法
- `resetToDefaultLighting()` - 重置为默认三点光照
- `clearAllLights()` - 清除所有光照
- `hasLights()` - 检查是否有光照
- `getLightManager()` - 获取LightManager实例

### 5. 头文件更新
- 将`setupLighting()`改为`setupDefaultLighting()`
- 添加了新的光照管理接口方法

## 架构优势

### 1. 统一的光照管理
- 所有光照操作都通过LightManager进行
- 支持动态添加、删除、修改光照
- 光照指示器自动管理

### 2. 与GlobalSettingsPanel的完全集成
- GlobalSettingsPanel的光照设置现在能够直接影响预览视口
- 支持多光照配置
- 实时光照更新

### 3. 向后兼容性
- 保留了`updateLighting()`等旧方法
- 通过`updateMultiLighting()`提供兼容性接口

### 4. 扩展性
- 支持不同类型的光照（方向光、点光源、聚光灯）
- 支持光照预设
- 支持光照动画

## 验证要点

1. **默认光照**: 预览视口启动时显示默认的三点光照系统
2. **光照更新**: GlobalSettingsPanel中的光照设置能够实时反映到预览视口
3. **光照管理**: 可以动态添加、删除、修改光照
4. **渲染质量**: 光照效果与重构前保持一致

## 测试建议

1. 启动预览视口，确认默认三点光照正常显示
2. 在GlobalSettingsPanel中修改光照参数，确认预览视口实时更新
3. 添加新的光照，确认光照指示器正确显示
4. 测试光照预设功能
5. 验证光照强度、颜色、方向等参数的调整效果

## 注意事项

1. 确保LightManager正确初始化
2. 光照更新后需要调用render()方法
3. 材质更新通过LightManager的updateMaterialsForLighting()方法
4. 光照指示器由LightManager自动管理

## 结论

重构成功将PreviewCanvas的光照系统从硬编码改为完全基于LightManager的动态系统，实现了与GlobalSettingsPanel的完全集成，提供了更好的用户体验和扩展性。 


# Coin3D等光照系统必须在几何创建之前加入场景图，这个限制必须遵守

设计场景图结构：

使用 SoSeparator 作为根节点或光照系统的容器节点，管理光照节点和相关状态。
动态创建光照节点（如 SoDirectionalLight、SoPointLight、SoSpotLight）并添加到场景图。


动态添加光照节点：

在运行时根据需求（用户输入、外部数据或其他逻辑）创建新的光照节点。
使用 SoSeparator::addChild() 将新光照节点插入场景图。


使用传感器或事件触发动态更新：

使用 SoTimerSensor 或 SoFieldSensor 定期检查是否需要添加新光照系统。
或者通过 SoEventCallback 响应用户输入（如按键或鼠标点击）来触发光照系统添加。


管理光照状态：

确保新添加的光照节点正确影响后续几何节点。
使用 SoSeparator 隔离光照状态，避免干扰其他场景部分。


性能优化：

限制光源数量（OpenGL 通常限制为 8 个光源）。
动态管理场景图时，使用引用计数（ref()/unref()）避免内存泄漏。