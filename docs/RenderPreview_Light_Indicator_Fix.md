# RenderPreview 光照指示器修复说明

## 问题描述

在之前的版本中，当用户点击不同的光照预设按钮时，预览视口中的光照指示器会不断累积，而不是替换旧的指示器。这导致：

1. 每次应用预设都会添加新的指示器
2. 旧的指示器没有被清除
3. 最终视口中显示过多的指示器，影响视觉效果
4. 左上角的主光源视锥指示器始终存在，不会被清除

## 问题原因

在 `updateMultiLighting()` 方法中：
- 每次调用都会创建新的 `lightIndicatorsContainer`
- 但没有正确清除之前创建的容器
- 导致多个容器同时存在于场景图中
- 主光源的指示器（`m_lightIndicator`）没有被清除，导致左上角的视锥指示器始终存在

## 修复方案

### 1. 添加成员变量跟踪

在 `PreviewCanvas` 类中添加了成员变量：
```cpp
SoSeparator* m_lightIndicatorsContainer;
```

### 2. 修改构造函数

在构造函数中初始化新成员变量：
```cpp
, m_lightIndicatorsContainer(nullptr)
```

### 3. 改进清理逻辑

在 `updateMultiLighting()` 方法中：
```cpp
// Clear existing light indicators container
if (m_lightIndicatorsContainer) {
    m_sceneRoot->removeChild(m_lightIndicatorsContainer);
    m_lightIndicatorsContainer = nullptr;
}

// Clear main light indicator
if (m_lightIndicator) {
    m_objectRoot->removeChild(m_lightIndicator);
    m_lightIndicator->unref();
    m_lightIndicator = nullptr;
}

// Create new light indicators container
m_lightIndicatorsContainer = new SoSeparator();
m_sceneRoot->addChild(m_lightIndicatorsContainer);
```

## 修复效果

修复后，光照预设的行为应该是：

1. ✅ 点击预设按钮时，旧的指示器被完全清除
2. ✅ 新的指示器正确显示
3. ✅ 每个预设只显示对应数量的指示器
4. ✅ 不会出现指示器累积的问题

## 测试步骤

1. 运行 `CADNav.exe`
2. 打开 Render Preview 对话框
3. 点击 "Light Presets" 标签
4. 依次点击不同的预设按钮：
   - Studio Lighting (应该显示3个指示器)
   - Outdoor Lighting (应该显示2个指示器)
   - Dramatic Lighting (应该显示1个指示器)
   - 其他预设...

## 预期结果

- 每次切换预设时，只显示当前预设对应的指示器数量
- 不会出现指示器累积
- 指示器颜色和位置正确反映光源设置
- 3D对象的光照效果正确更新

## 技术细节

### 修复的文件

1. `include/renderpreview/PreviewCanvas.h`
   - 添加 `m_lightIndicatorsContainer` 成员变量

2. `src/renderpreview/PreviewCanvas.cpp`
   - 修改构造函数初始化
   - 改进 `updateMultiLighting()` 方法的清理逻辑

### 关键改进

- 使用成员变量跟踪指示器容器
- 在创建新容器前正确清除旧容器
- 清除主光源指示器（`m_lightIndicator`）以避免视锥指示器残留
- 避免使用复杂的场景图搜索和 `getParent()` 调用
- 简化了清理逻辑，提高了可靠性

这个修复确保了光照预设功能的正确工作，用户现在可以正常切换不同的光照配置而不会遇到指示器累积的问题。 