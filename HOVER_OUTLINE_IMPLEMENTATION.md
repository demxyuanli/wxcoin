# 悬停轮廓功能实现说明

## 功能描述

实现了鼠标悬停高亮功能，具体行为如下：

1. **开启toggleoutline开关**：
   - 鼠标悬停到模型上时，该模型显示轮廓高亮
   - 鼠标移开时，轮廓消失
   - 只有鼠标下的模型会显示轮廓

2. **关闭toggleoutline开关**：
   - 所有轮廓效果关闭
   - 鼠标悬停不再触发轮廓显示

## 实现方案

### 1. OutlineDisplayManager增强

添加了悬停模式支持：
```cpp
// 新增接口
void setHoverMode(bool hover);
bool isHoverMode() const;
void setHoveredGeometry(std::shared_ptr<OCCGeometry> geometry);

// 新增成员变量
bool m_hoverMode{ true };  // 默认启用悬停模式
std::weak_ptr<OCCGeometry> m_hoveredGeometry;
```

### 2. 鼠标事件处理

在Canvas中添加了鼠标移动和离开事件处理：
```cpp
// 鼠标移动时更新悬停轮廓
if (event.GetEventType() == wxEVT_MOTION && m_occViewer) {
    wxPoint screenPos = event.GetPosition();
    m_occViewer->updateHoverSilhouetteAt(screenPos);
}

// 鼠标离开窗口时清除轮廓
if (event.GetEventType() == wxEVT_LEAVE_WINDOW && m_occViewer) {
    m_occViewer->updateHoverSilhouetteAt(wxPoint(-1, -1));
}
```

### 3. OCCViewer集成

修改了updateHoverSilhouetteAt方法：
- 当OutlineManager启用且处于悬停模式时，使用后处理轮廓
- 否则使用传统的HoverSilhouetteManager

### 4. 轮廓渲染模式

OutlineDisplayManager现在支持两种模式：
1. **悬停模式（默认）**：只为鼠标下的几何体显示轮廓
2. **全局模式**：为所有几何体显示轮廓（通过setHoverMode(false)切换）

## 使用方式

1. 点击toggleoutline按钮开启轮廓功能
2. 移动鼠标到3D模型上，该模型会显示轮廓高亮
3. 移开鼠标，轮廓自动消失
4. 再次点击toggleoutline按钮关闭功能

## 技术细节

### 轮廓切换逻辑

```cpp
void OutlineDisplayManager::setEnabled(bool enabled) {
    if (enabled && m_hoverMode) {
        // 悬停模式：清除所有轮廓，等待鼠标悬停
        clearAll();
    } else if (enabled && !m_hoverMode) {
        // 全局模式：启用后处理轮廓
        m_imagePass->setEnabled(true);
    } else {
        // 禁用：清除所有轮廓
        clearAll();
        if (m_imagePass) m_imagePass->setEnabled(false);
    }
}
```

### 悬停几何体管理

```cpp
void OutlineDisplayManager::setHoveredGeometry(geometry) {
    // 清除之前的悬停轮廓
    if (prevHovered && prevHovered != geometry) {
        disableOutline(prevHovered);
    }
    
    // 设置新的悬停轮廓
    if (geometry) {
        enableOutline(geometry);
    }
}
```

## 优势

1. **性能优化**：只渲染鼠标下的模型轮廓，减少GPU负担
2. **视觉清晰**：避免所有模型都有轮廓导致的视觉混乱
3. **交互直观**：用户能清楚知道鼠标指向哪个模型
4. **灵活切换**：保留了全局轮廓模式的可能性

## 后续优化建议

1. **轮廓样式**：可以为悬停轮廓设置不同的颜色或厚度
2. **过渡效果**：添加淡入淡出效果使轮廓切换更平滑
3. **性能缓存**：缓存最近使用的轮廓渲染器避免重复创建
4. **配置选项**：允许用户选择悬停模式或全局模式