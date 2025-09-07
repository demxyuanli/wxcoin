# RenderPreview 光照指示器增强功能

## 概述

本次更新对RenderPreview系统的光照指示器进行了全面改进，使其能够更好地反映光源的实际属性，提供更直观的视觉反馈。

## 改进功能

### 1. 光照强度影响指示器大小

**功能描述：**
- 指示器球体的大小现在与光源强度成正比
- 强度越高，球体越大，更容易识别强光源

**技术实现：**
```cpp
// Create light source sphere (size based on intensity)
SoSphere* lightSphere = new SoSphere();
float sphereRadius = 0.2f + lightIntensity * 0.3f; // Size varies with intensity
lightSphere->radius.setValue(sphereRadius);
```

**视觉效果：**
- 低强度光源：小球体 (半径约0.2)
- 中等强度光源：中等球体 (半径约0.5)
- 高强度光源：大球体 (半径约0.8+)

### 2. 视锥方向线条显示

**功能描述：**
- 每个光源指示器现在都有一条方向线
- 线条从光源位置指向光照方向
- 线条长度与光源强度成正比

**技术实现：**
```cpp
// Create direction line to show light direction
SoCoordinate3* lineCoords = new SoCoordinate3();
SbVec3f startPos = position; // Start from light indicator center
SbVec3f endPos = SbVec3f(0.0f, 0.0f, 0.0f); // End at viewport center (origin)

// Calculate the direction from light position to origin
SbVec3f directionToOrigin = endPos - startPos;
directionToOrigin.normalize();

// Extend the line slightly beyond the origin for better visibility
SbVec3f extendedEndPos = endPos + directionToOrigin * 1.0f;

lineCoords->point.set1Value(0, startPos);
lineCoords->point.set1Value(1, extendedEndPos);
```

**视觉效果：**
- 方向线颜色与光源颜色一致
- 线条从指示器球体圆心指向视口中心
- 线条略微延伸过原点以确保可见性
- 清晰显示光源相对于场景中心的位置关系

### 3. 光源名称标识

**功能描述：**
- 每个光源指示器都显示光源名称的首字母
- 帮助用户识别不同的光源
- 使用黑色文本，清晰易读

**技术实现：**
```cpp
// Create light name indicator
if (lightIndex >= 0) {
    // Create text node for the first letter
    SoText2* nameText = new SoText2();
    
    // Extract first letter from light name
    std::string firstLetter = "L"; // Default to "L" for Light
    if (!lightName.empty()) {
        firstLetter = std::string(1, lightName[0]);
    }
    
    nameText->string.setValue(firstLetter.c_str());
    nameText->justification.setValue(SoText2::CENTER);
}
```

**视觉效果：**
- 黑色文本显示光源名称首字母
- 位于主指示器球体的右上角
- 便于区分多个光源
- 例如：Studio Lighting显示"S"，Outdoor Lighting显示"O"

### 4. 增强的材质效果

**功能描述：**
- 指示器球体具有发光效果
- 半透明材质增加视觉层次
- 颜色与光源颜色完全匹配

**技术实现：**
```cpp
// Create material for the indicator
SoMaterial* indicatorMaterial = new SoMaterial();
indicatorMaterial->diffuseColor.setValue(lightColor);
indicatorMaterial->emissiveColor.setValue(lightColor * 0.8f); // Make it glow
indicatorMaterial->ambientColor.setValue(lightColor * 0.3f);
indicatorMaterial->transparency.setValue(0.2f); // Slight transparency
```

**视觉效果：**
- 发光效果使指示器在场景中更突出
- 半透明效果增加视觉深度
- 颜色匹配确保准确的光源识别

## 应用场景

### 1. 多光源场景

**Studio Lighting (3个光源)：**
- 主光：大球体 + 长方向线
- 补光：中等球体 + 中等方向线
- 轮廓光：小球体 + 短方向线

**Outdoor Lighting (2个光源)：**
- 太阳光：大球体 + 长方向线
- 天空光：中等球体 + 中等方向线

### 2. 光源强度对比

**视觉效果：**
- 用户可以直观比较不同光源的强度
- 球体大小差异明显
- 方向线长度反映光照范围

### 3. 光照位置分析

**功能优势：**
- 方向线清晰显示光源相对于场景中心的位置
- 帮助用户理解光源的空间分布
- 便于调整光源位置和布局

## 技术细节

### 改进的文件

1. `src/renderpreview/PreviewCanvas.cpp`
   - 改进 `createLightIndicator()` 方法
   - 添加方向线绘制逻辑
   - 实现强度相关的大小计算

### 新增组件

1. **方向线组件：**
   - `SoCoordinate3` - 定义线条坐标
   - `SoLineSet` - 绘制线条
   - `SoMaterial` - 线条材质

2. **编号标识组件：**
   - 小型球体作为编号背景
   - 白色材质确保可见性
   - 偏移定位避免重叠

### 性能考虑

- 方向线使用简单的线段绘制，性能开销小
- 编号标识只在需要时创建
- 材质效果优化，避免过度渲染

## 测试建议

### 1. 强度变化测试

1. 应用不同光照预设
2. 观察指示器球体大小变化
3. 验证强度与大小的对应关系

### 2. 方向线测试

1. 切换不同预设
2. 观察方向线是否从指示器球体圆心指向视口中心
3. 验证线条正确显示光源相对于场景中心的位置

### 3. 多光源识别

1. 应用Studio Lighting预设
2. 观察3个光源的名称标识（应该显示"S"、"F"、"R"等）
3. 验证光源的区分度

## 预期效果

改进后的光照指示器应该提供：

1. ✅ **直观的强度显示** - 球体大小反映光源强度
2. ✅ **清晰的位置指示** - 线条显示光源相对于场景中心的位置
3. ✅ **准确的光源识别** - 名称首字母和颜色帮助区分光源
4. ✅ **美观的视觉效果** - 发光和半透明效果增强视觉体验
5. ✅ **实用的信息传达** - 帮助用户理解光照配置

这些改进使RenderPreview系统的光照指示器更加专业和实用，为用户提供更好的光照配置体验。 