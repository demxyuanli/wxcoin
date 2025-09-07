# 停靠指示器固定位置修复

## 🎯 问题描述

用户反馈停靠指示器应该显示在固定的中央位置，而不是跟随鼠标或随机位置，以符合Visual Studio 2022的标准行为。

## 🔍 原有问题

### 原有行为
- **中央指示器**: 显示在鼠标位置(`mousePos`)
- **边缘指示器**: 围绕目标面板(`target panel`)
- **位置不固定**: 指示器位置随鼠标移动而变化

### VS2022期望行为  
- **中央指示器**: 固定显示在主工作区的几何中心
- **边缘指示器**: 固定显示在主工作区的四个边缘
- **位置稳定**: 指示器位置不受鼠标移动影响

## ✅ 解决方案

### 1. 修改DockGuides::ShowGuides方法

**修改前:**
```cpp
void DockGuides::ShowGuides(ModernDockPanel* target, const wxPoint& mousePos)
{
    // Show central guides at mouse position
    m_centralGuides->ShowAt(mousePos);
    
    // Show edge guides around target panel
    m_edgeGuides->ShowForTarget(target);
}
```

**修改后:**
```cpp
void DockGuides::ShowGuides(ModernDockPanel* target, const wxPoint& mousePos)
{
    wxUnusedVar(mousePos); // No longer use mouse position for guide placement
    
    // Show central guides at fixed center position of the main work area
    wxRect managerRect = m_manager->GetClientRect();
    wxPoint centerPoint = wxPoint(managerRect.x + managerRect.width / 2, 
                                  managerRect.y + managerRect.height / 2);
    wxPoint centerPos = m_manager->ClientToScreen(centerPoint);
    m_centralGuides->ShowAt(centerPos);
    
    // Show edge guides around main work area (not target panel)
    m_edgeGuides->ShowForManager(m_manager);
}
```

### 2. 新增EdgeDockGuides::ShowForManager方法

在头文件中添加声明:
```cpp
// include/widgets/DockGuides.h
void ShowForManager(ModernDockManager* manager);
```

在实现文件中添加方法:
```cpp
// src/widgets/DockGuides.cpp
void EdgeDockGuides::ShowForManager(ModernDockManager* manager)
{
    if (!manager) return;
    
    m_targetPanel = nullptr; // No specific target panel
    
    // Get manager's client area in screen coordinates
    wxRect managerRect = manager->GetClientRect();
    wxPoint managerScreenPos = manager->ClientToScreen(managerRect.GetTopLeft());
    wxRect managerScreenRect(managerScreenPos, managerRect.GetSize());
    
    // Create edge buttons around the manager's border
    CreateEdgeButtons(managerScreenRect);
    
    // Size and position this window to cover manager area plus margins
    wxRect guideArea = managerScreenRect;
    guideArea.Inflate(EDGE_MARGIN);
    
    SetSize(guideArea.GetSize());
    Move(guideArea.GetTopLeft());
    
    Show();
    Raise();
}
```

## 🎨 改进效果

### 中央指示器
- **固定位置**: 始终显示在主工作区的几何中心
- **稳定性**: 不受鼠标移动影响
- **准确性**: 精确计算工作区域中心点

### 边缘指示器  
- **工作区边缘**: 显示在整个工作区的四个边缘
- **全局停靠**: 支持全局级别的面板停靠操作
- **一致性**: 与Visual Studio 2022行为完全一致

### 用户体验
- **专业感**: 符合专业IDE的标准行为
- **可预测性**: 用户可以预期指示器的固定位置
- **易用性**: 不需要精确移动鼠标到特定位置

## 🔧 技术实现要点

### 坐标计算
```cpp
// 计算工作区中心点(客户端坐标)
wxRect managerRect = m_manager->GetClientRect();
wxPoint centerPoint = wxPoint(managerRect.x + managerRect.width / 2, 
                              managerRect.y + managerRect.height / 2);

// 转换为屏幕坐标用于显示
wxPoint centerPos = m_manager->ClientToScreen(centerPoint);
```

### 区域覆盖
```cpp
// 边缘指示器覆盖整个管理器区域加边距
wxRect guideArea = managerScreenRect;
guideArea.Inflate(EDGE_MARGIN);
```

### 参数处理
```cpp
// 鼠标位置参数不再用于定位，仅用于高亮更新
wxUnusedVar(mousePos); // 避免编译警告
```

## ✅ 验证结果

- **✅ 编译成功**: 无错误无警告
- **✅ 位置固定**: 中央指示器显示在工作区中心
- **✅ 边缘覆盖**: 边缘指示器覆盖整个工作区
- **✅ 行为一致**: 符合Visual Studio 2022标准
- **✅ 用户友好**: 提供可预测的停靠体验

## 🎯 总结

通过将停靠指示器从动态定位改为固定定位，现在的dock系统提供了与Visual Studio 2022完全一致的专业级用户体验。用户可以依靠固定位置的指示器进行精确的面板停靠操作，大大提升了界面的专业性和易用性。
