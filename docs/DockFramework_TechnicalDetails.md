# Dock框架技术实现详解

## 架构概览

### 系统层次结构
```
IDockManager (接口层)
    ↓
ModernDockManager (管理层)
    ↓
┌─────────────────┬─────────────────┬─────────────────┐
│ ModernDockPanel │   DockGuides    │ DragDropController │
│   (面板层)       │   (视觉反馈)     │   (拖拽控制)      │
└─────────────────┴─────────────────┴─────────────────┘
    ↓
LayoutEngine (布局引擎)
    ↓
wxWidgets (底层UI框架)
```

## 核心组件详解

### 1. ModernDockManager

#### 1.1 类设计
```cpp
class ModernDockManager : public wxPanel, public IDockManager {
private:
    std::map<DockArea, std::vector<ModernDockPanel*>> m_panels;
    std::unique_ptr<DockGuides> m_dockGuides;
    std::unique_ptr<GhostWindow> m_ghostWindow;
    std::unique_ptr<DragDropController> m_dragController;
    std::unique_ptr<LayoutEngine> m_layoutEngine;
    
    DragState m_dragState;
    ModernDockPanel* m_draggedPanel;
    ModernDockPanel* m_hoveredPanel;
};
```

#### 1.2 关键方法
- **AddPanel()**：面板添加和初始化
- **SetAreaDockingEnabled()**：区域停靠控制
- **InitializeComponents()**：组件初始化和回调设置

#### 1.3 状态管理
```cpp
enum class DragState {
    None,       // 无拖拽操作
    Started,    // 拖拽开始
    Active,     // 拖拽进行中
    Completing  // 拖拽完成中
};
```

### 2. ModernDockPanel

#### 2.1 多标签架构
```cpp
struct ContentItem {
    wxWindow* content;     // 实际内容窗口
    wxString title;        // 标签标题
    wxBitmap icon;         // 标签图标
};

class ModernDockPanel {
private:
    std::vector<std::unique_ptr<ContentItem>> m_contents;
    int m_selectedIndex;           // 当前选中的标签索引
    int m_hoveredTabIndex;         // 悬停的标签索引
    bool m_dockingEnabled;         // 是否允许拖拽
    bool m_systemButtonsVisible;  // 系统按钮是否可见
};
```

#### 2.2 渲染系统
```cpp
// 绘制流程
void OnPaint(wxPaintEvent& event) {
    if (m_contents.size() > 1) {
        RenderTitleBarTabs(gc);  // 多标签：标题栏显示标签
    } else {
        RenderTitleBar(gc);      // 单标签：标题栏显示标题
    }
}
```

#### 2.3 事件处理
```cpp
// 鼠标事件处理链
OnLeftDown() → StartDrag() → OnMouseMove() → OnLeftUp()
                ↓              ↓              ↓
            记录起始位置    检查拖拽阈值    完成拖拽操作
```

### 3. 拖拽系统

#### 3.1 拖拽状态机
```
[点击] → [记录位置] → [移动检测] → [超过阈值?] → [开始拖拽] → [拖拽中] → [释放] → [完成拖拽]
           ↓                         ↓              ↓          ↓         ↓
       StartDrag()              OnMouseMove()   CaptureMouse() 影子窗口  ReleaseMouse()
```

#### 3.2 阈值控制
```cpp
static constexpr int DRAG_THRESHOLD = 8; // 8像素拖拽阈值

// 阈值检测逻辑
if (abs(screenPos.x - m_dragStartPos.x) > DRAG_THRESHOLD ||
    abs(screenPos.y - m_dragStartPos.y) > DRAG_THRESHOLD) {
    // 开始真正的拖拽操作
    m_dragging = true;
    m_manager->StartDrag(this, screenPos);
    CaptureMouse();
}
```

#### 3.3 回调机制
```cpp
// 拖拽回调设置
m_dragController->SetDragStartCallback([this](const DragSession& session) {
    // 显示影子窗口和停靠指南
});

m_dragController->SetDragUpdateCallback([this](const DragSession& session) {
    // 更新影子窗口位置和停靠预览
});

m_dragController->SetDragCompleteCallback([this](const DragSession& session, const DropValidation& validation) {
    // 执行实际的停靠操作
});
```

### 4. 停靠指示器系统

#### 4.1 指示器组件
```cpp
class DockGuides {
private:
    std::unique_ptr<CentralDockGuides> m_centralGuides;  // 中心指示器
    std::unique_ptr<EdgeDockGuides> m_edgeGuides;        // 边缘指示器
    bool m_centerEnabled, m_leftEnabled, m_rightEnabled, m_topEnabled, m_bottomEnabled;
};
```

#### 4.2 启用状态控制
```cpp
void SetEnabledDirections(bool center, bool left, bool right, bool top, bool bottom) {
    m_centerEnabled = center;    // false: 禁用center停靠
    m_leftEnabled = left;        // true:  启用left停靠
    m_rightEnabled = right;      // false: 禁用right停靠
    m_topEnabled = top;          // true:  启用top停靠
    m_bottomEnabled = bottom;    // true:  启用bottom停靠
}

void SetCentralVisible(bool visible) {
    m_showCentral = visible;     // true:  始终显示中心指示器
}
```

## 关键技术实现

### 1. 多标签标题栏渲染

#### 1.1 标签计算
```cpp
void RenderTitleBarTabs(wxGraphicsContext* gc) {
    int x = 0;
    for (size_t i = 0; i < m_contents.size(); ++i) {
        // 计算标签宽度
        wxSize textSize = dc.GetTextExtent(m_contents[i]->title);
        int tabWidth = textSize.GetWidth() + m_tabPadding * 2;
        if (m_tabCloseMode != TabCloseMode::ShowNever) {
            tabWidth += m_closeButtonSize + 4;
        }
        
        // 绘制标签背景和边框
        if (selected) {
            // FlatBar风格的活动标签
            gc->SetBrush(wxBrush(activeTabBgColour));
            gc->DrawRectangle(x, 2, tabWidth, 22);
            
            // 蓝色顶部边框
            gc->SetPen(wxPen(tabBorderTopColour, 2));
            gc->StrokeLine(x, 1, x + tabWidth, 1);
        }
        
        // 绘制标签文本
        gc->DrawText(m_contents[i]->title, x + padding, centerY);
        
        x += tabWidth + m_tabSpacing;
    }
}
```

#### 1.2 点击测试
```cpp
int HitTestTitleBarTab(const wxPoint& pos) const {
    if (pos.y >= 24) return -1; // 不在标题栏范围内
    
    int x = 0;
    for (size_t i = 0; i < m_contents.size(); ++i) {
        int tabWidth = CalculateTabWidth(i);
        if (pos.x >= x && pos.x < x + tabWidth) {
            return static_cast<int>(i);
        }
        x += tabWidth + m_tabSpacing;
    }
    return -1;
}
```

### 2. 区域停靠控制

#### 2.1 双层禁用机制
```cpp
// Layer 1: DockGuides层面 - 视觉反馈控制
m_dockGuides->SetEnabledDirections(false, true, false, true, true);
//                                center left right  top   bottom

// Layer 2: ModernDockPanel层面 - 功能控制
panel->SetDockingEnabled(false);         // 禁止拖拽
panel->SetSystemButtonsVisible(false);   // 隐藏系统按钮
```

#### 2.2 面板创建时的自动应用
```cpp
void ModernDockManager::AddPanel(wxWindow* content, const wxString& title, DockArea area) {
    auto* panel = new ModernDockPanel(this, this, title);
    panel->SetDockArea(area);
    
    // 根据区域自动应用限制
    if (area == DockArea::Center || area == DockArea::Right) {
        panel->SetDockingEnabled(false);
        panel->SetSystemButtonsVisible(false);
    }
    
    // ... 继续初始化
}
```

### 3. 智能拖拽启动

#### 3.1 状态追踪
```cpp
// 拖拽状态变量
bool m_dragging;              // 是否正在拖拽
int m_draggedTabIndex;        // 被拖拽的标签索引
wxPoint m_dragStartPos;       // 拖拽起始位置
wxPoint m_lastMousePos;       // 最后鼠标位置

// 拖拽启动逻辑
void StartDrag(int tabIndex, const wxPoint& startPos) {
    if (!m_dockingEnabled) return;  // 禁用检查
    
    // 不立即开始拖拽，只记录状态
    m_draggedTabIndex = tabIndex;
    m_dragStartPos = startPos;
    // m_dragging = false; // 暂不设置为true
}
```

#### 3.2 阈值检测
```cpp
void OnMouseMove(wxMouseEvent& event) {
    if (m_dragging) {
        // 已经在拖拽，更新位置
        UpdateDragOperation(event.GetPosition());
    } else if (m_draggedTabIndex >= 0) {
        // 检查是否超过阈值
        wxPoint screenPos = ClientToScreen(event.GetPosition());
        if (abs(screenPos.x - m_dragStartPos.x) > DRAG_THRESHOLD ||
            abs(screenPos.y - m_dragStartPos.y) > DRAG_THRESHOLD) {
            // 超过阈值，开始真正的拖拽
            m_dragging = true;
            m_manager->StartDrag(this, screenPos);
            CaptureMouse();
        }
    }
}
```

### 4. 主题集成

#### 4.1 颜色获取
```cpp
void UpdateThemeColors() {
    // 从主题管理器获取颜色
    m_backgroundColor = CFG_COLOUR("PanelBgColour");
    m_tabActiveColor = CFG_COLOUR("TabActiveColour");
    m_titleBarBgColor = CFG_COLOUR("BarBackgroundColour");
    m_titleBarTextColor = CFG_COLOUR("BarActiveTextColour");
    
    // FlatBar兼容的颜色
    wxColour activeTabBgColour = CFG_COLOUR("BarActiveTabBgColour");
    wxColour tabBorderTopColour = CFG_COLOUR("BarTabBorderTopColour");
}
```

#### 4.2 字体设置
```cpp
void InitializePanel() {
    // 从配置获取字体
    m_tabFont = CFG_FONT();
    m_titleFont = CFG_FONT();
    
    // DPI缩放支持
    double dpiScale = DPIManager::getInstance().getDPIScale();
    m_tabHeight = static_cast<int>(DEFAULT_TAB_HEIGHT * dpiScale);
    m_closeButtonSize = static_cast<int>(DEFAULT_CLOSE_BUTTON_SIZE * dpiScale);
}
```

## 性能优化

### 1. 渲染优化
```cpp
// 双缓冲绘制
void OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    
    // 使用硬件加速的图形上下文
    if (gc) {
        RenderContent(gc);
        delete gc;
    }
}

// 背景样式设置
SetBackgroundStyle(wxBG_STYLE_PAINT);
SetDoubleBuffered(true);
```

### 2. 事件优化
```cpp
// 避免频繁刷新
void OnMouseMove(wxMouseEvent& event) {
    int newHoveredTab = HitTestTab(pos);
    if (newHoveredTab != m_hoveredTabIndex) {
        m_hoveredTabIndex = newHoveredTab;
        Refresh(); // 只在状态改变时刷新
    }
}
```

### 3. 内存管理
```cpp
// 智能指针管理组件
std::unique_ptr<DockGuides> m_dockGuides;
std::unique_ptr<GhostWindow> m_ghostWindow;

// 内容项的RAII管理
std::vector<std::unique_ptr<ContentItem>> m_contents;
```

## 扩展点

### 1. 自定义渲染器
```cpp
class CustomTabRenderer {
public:
    virtual void RenderTab(wxGraphicsContext* gc, const TabInfo& info) = 0;
    virtual void RenderTitleBar(wxGraphicsContext* gc, const TitleBarInfo& info) = 0;
};
```

### 2. 布局策略
```cpp
enum class LayoutStrategy {
    IDE,        // IDE布局
    CAD,        // CAD布局
    Hybrid,     // 混合布局
    Custom      // 自定义布局
};
```

### 3. 事件回调
```cpp
// 面板事件回调
using PanelEventCallback = std::function<void(ModernDockPanel*, PanelEvent)>;
using LayoutChangeCallback = std::function<void(const LayoutChange&)>;
```

## 调试支持

### 1. 日志系统
```cpp
#ifdef DEBUG
    Logger::Debug("DockPanel", "StartDrag: tabIndex=%d, pos=(%d,%d)", 
                  tabIndex, startPos.x, startPos.y);
#endif
```

### 2. 状态检查
```cpp
void DumpPanelState() const {
    wxString state = wxString::Format(
        "Panel: %s, Area: %d, Docking: %s, Tabs: %d",
        m_title, static_cast<int>(m_dockArea),
        m_dockingEnabled ? "Enabled" : "Disabled",
        static_cast<int>(m_contents.size())
    );
    Logger::Info("DockPanel", state);
}
```

## 兼容性说明

### 1. 平台支持
- Windows 10/11
- 高DPI支持
- 多显示器支持

### 2. wxWidgets版本
- 最低版本：3.1.x
- 推荐版本：3.2.x
- 需要OpenGL支持

### 3. 编译器要求
- C++17或更高
- MSVC 2019或更高
- 支持智能指针和lambda表达式

---

本技术文档详细描述了Dock框架的内部实现机制，为开发者提供深入理解和扩展的技术基础。
