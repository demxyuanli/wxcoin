# Dock框架API参考文档

## ModernDockManager

### 构造函数
```cpp
ModernDockManager(wxWindow* parent);
```
创建一个新的停靠管理器实例。

**参数：**
- `parent` - 父窗口指针

**示例：**
```cpp
auto* dockManager = new ModernDockManager(this);
```

### 面板管理

#### AddPanel
```cpp
void AddPanel(wxWindow* content, const wxString& title, DockArea area = DockArea::Center);
void AddPanel(wxWindow* content, const wxString& title, UnifiedDockArea area);
```
添加一个新的面板到指定区域。

**参数：**
- `content` - 面板内容窗口
- `title` - 面板标题
- `area` - 停靠区域

**特殊行为：**
- Message和Performance面板会自动合并为多标签面板
- Center和Right区域的面板会自动禁用拖拽和系统按钮

**示例：**
```cpp
// 添加单个面板
dockManager->AddPanel(myPanel, "工具面板", DockArea::Left);

// 添加多标签面板
dockManager->AddPanel(messagePanel, "Message", DockArea::Bottom);
dockManager->AddPanel(performancePanel, "Performance", DockArea::Bottom);
```

#### RemovePanel
```cpp
void RemovePanel(ModernDockPanel* panel);
void RemovePanel(wxWindow* content);
```
移除指定的面板。

**参数：**
- `panel` - 要移除的ModernDockPanel
- `content` - 要移除的内容窗口

#### FindPanel
```cpp
ModernDockPanel* FindPanel(const wxString& title) const;
```
根据标题查找面板。

**返回值：**
- 找到的面板指针，或nullptr

### 停靠控制

#### SetAreaDockingEnabled
```cpp
void SetAreaDockingEnabled(DockArea area, bool enabled);
```
启用或禁用指定区域的停靠功能。

**参数：**
- `area` - 目标区域
- `enabled` - 是否启用

**效果：**
- 禁用时：该区域面板不能拖拽，不显示系统按钮，停靠指南不响应
- 启用时：恢复正常功能

**示例：**
```cpp
// 禁用center区域停靠
dockManager->SetAreaDockingEnabled(DockArea::Center, false);
```

#### IsAreaDockingEnabled
```cpp
bool IsAreaDockingEnabled(DockArea area) const;
```
检查指定区域是否启用停靠功能。

#### SetPanelDockingEnabled
```cpp
void SetPanelDockingEnabled(ModernDockPanel* panel, bool enabled);
```
控制特定面板的拖拽功能。

#### SetPanelSystemButtonsVisible
```cpp
void SetPanelSystemButtonsVisible(ModernDockPanel* panel, bool visible);
```
控制特定面板的系统按钮可见性。

### 布局管理

#### SetLayoutStrategy
```cpp
void SetLayoutStrategy(LayoutStrategy strategy);
```
设置布局策略。

**可用策略：**
- `LayoutStrategy::IDE` - IDE风格布局
- `LayoutStrategy::CAD` - CAD风格布局
- `LayoutStrategy::Hybrid` - 混合布局

#### GetLayoutStrategy
```cpp
LayoutStrategy GetLayoutStrategy() const;
```
获取当前布局策略。

#### SetLayoutConstraints
```cpp
void SetLayoutConstraints(const LayoutConstraints& constraints);
```
设置布局约束。

**LayoutConstraints结构：**
```cpp
struct LayoutConstraints {
    wxSize minPanelSize{100, 100};
    wxSize maxPanelSize{-1, -1};
    int splitterMinSize{50};
    bool allowFloating{true};
    bool animateLayout{true};
};
```

### 状态查询

#### HasPanel
```cpp
bool HasPanel(wxWindow* content) const;
```
检查是否包含指定内容的面板。

#### GetPanelCount
```cpp
int GetPanelCount() const;
```
获取面板总数。

#### ShowDockGuides
```cpp
void ShowDockGuides(wxWindow* target);
```
显示停靠指南。

---

## ModernDockPanel

### 内容管理

#### AddContent
```cpp
void AddContent(wxWindow* content, const wxString& title, const wxBitmap& icon = wxNullBitmap, bool select = true);
```
向面板添加内容，创建新标签。

**参数：**
- `content` - 内容窗口
- `title` - 标签标题
- `icon` - 标签图标（可选）
- `select` - 是否立即选中

#### RemoveContent
```cpp
void RemoveContent(wxWindow* content);
void RemoveContent(int index);
```
移除指定的内容。

#### SelectContent
```cpp
void SelectContent(int index);
void SelectContent(wxWindow* content);
```
选择指定的内容标签。

#### GetContentCount
```cpp
int GetContentCount() const;
```
获取内容数量。

#### GetContent
```cpp
wxWindow* GetContent(int index) const;
```
获取指定索引的内容窗口。

#### GetSelectedContent
```cpp
wxWindow* GetSelectedContent() const;
```
获取当前选中的内容窗口。

#### GetSelectedIndex
```cpp
int GetSelectedIndex() const;
```
获取当前选中的标签索引。

### 标签管理

#### GetContentTitle / SetContentTitle
```cpp
wxString GetContentTitle(int index) const;
void SetContentTitle(int index, const wxString& title);
```
获取或设置标签标题。

#### GetContentIcon / SetContentIcon
```cpp
wxBitmap GetContentIcon(int index) const;
void SetContentIcon(int index, const wxBitmap& icon);
```
获取或设置标签图标。

#### SetTabCloseMode
```cpp
void SetTabCloseMode(TabCloseMode mode);
```
设置标签关闭按钮模式。

**TabCloseMode枚举：**
```cpp
enum class TabCloseMode {
    ShowAlways,  // 始终显示
    ShowOnHover, // 悬停时显示
    ShowNever    // 永不显示
};
```

#### SetShowTabs
```cpp
void SetShowTabs(bool show);
```
控制是否显示标签栏。

### 停靠控制

#### SetDockingEnabled
```cpp
void SetDockingEnabled(bool enabled);
```
启用或禁用面板的拖拽功能。

#### IsDockingEnabled
```cpp
bool IsDockingEnabled() const;
```
检查是否启用拖拽功能。

#### SetSystemButtonsVisible
```cpp
void SetSystemButtonsVisible(bool visible);
```
控制系统按钮的可见性。

#### AreSystemButtonsVisible
```cpp
bool AreSystemButtonsVisible() const;
```
检查系统按钮是否可见。

### 面板属性

#### SetTitle / GetTitle
```cpp
void SetTitle(const wxString& title);
const wxString& GetTitle() const;
```
设置或获取面板标题。

#### SetState / GetState
```cpp
void SetState(DockPanelState state);
DockPanelState GetState() const;
```
设置或获取面板状态。

**DockPanelState枚举：**
```cpp
enum class DockPanelState {
    Normal,     // 正常状态
    Maximized,  // 最大化
    Minimized,  // 最小化
    Floating,   // 浮动
    Hidden      // 隐藏
};
```

#### SetFloating
```cpp
void SetFloating(bool floating);
```
设置面板为浮动状态。

#### GetDockArea
```cpp
DockArea GetDockArea() const;
```
获取面板所在的停靠区域。

### 样式配置

#### 标签样式
```cpp
void SetTabStyle(TabStyle style);
void SetTabBorderStyle(TabBorderStyle style);
void SetTabCornerRadius(int radius);
void SetTabBorderWidths(int top, int bottom, int left, int right);
void SetTabBorderColours(const wxColour& top, const wxColour& bottom, 
                        const wxColour& left, const wxColour& right);
```

#### 标签布局
```cpp
void SetTabPadding(int padding);
int GetTabPadding() const;

void SetTabSpacing(int spacing);
int GetTabSpacing() const;

void SetTabTopMargin(int margin);
int GetTabTopMargin() const;
```

#### 字体设置
```cpp
void SetTabFont(const wxFont& font);
wxFont GetTabFont() const;

void SetTitleFont(const wxFont& font);
wxFont GetTitleFont() const;
```

### 系统按钮

#### AddSystemButton
```cpp
void AddSystemButton(DockSystemButtonType type, const wxString& tooltip = wxEmptyString);
```
添加系统按钮。

**DockSystemButtonType枚举：**
```cpp
enum class DockSystemButtonType {
    Minimize,   // 最小化
    Maximize,   // 最大化
    Close,      // 关闭
    Float,      // 浮动
    Custom      // 自定义
};
```

#### RemoveSystemButton
```cpp
void RemoveSystemButton(DockSystemButtonType type);
```

#### SetSystemButtonEnabled
```cpp
void SetSystemButtonEnabled(DockSystemButtonType type, bool enabled);
```

#### SetSystemButtonIcon
```cpp
void SetSystemButtonIcon(DockSystemButtonType type, const wxBitmap& icon);
```

#### SetSystemButtonTooltip
```cpp
void SetSystemButtonTooltip(DockSystemButtonType type, const wxString& tooltip);
```

### 拖拽操作

#### StartDrag
```cpp
void StartDrag(int tabIndex, const wxPoint& startPos);
```
开始拖拽操作（通常由内部调用）。

#### IsDragging
```cpp
bool IsDragging() const;
```
检查是否正在拖拽。

#### GetDraggedTabIndex
```cpp
int GetDraggedTabIndex() const;
```
获取被拖拽的标签索引。

### 点击测试

#### HitTestTab
```cpp
int HitTestTab(const wxPoint& pos) const;
```
测试指定位置是否在标签上。

#### HitTestCloseButton
```cpp
bool HitTestCloseButton(const wxPoint& pos, int& tabIndex) const;
```
测试指定位置是否在关闭按钮上。

#### GetTabRect
```cpp
wxRect GetTabRect(int index) const;
```
获取指定标签的矩形区域。

#### GetContentRect
```cpp
wxRect GetContentRect() const;
```
获取内容区域的矩形。

### 动画支持

#### AnimateTabInsertion
```cpp
void AnimateTabInsertion(int index);
```
播放标签插入动画。

#### AnimateTabRemoval
```cpp
void AnimateTabRemoval(int index);
```
播放标签移除动画。

#### AnimateResize
```cpp
void AnimateResize(const wxSize& targetSize);
```
播放大小调整动画。

---

## 枚举和结构

### DockArea
```cpp
enum class DockArea {
    Left,      // 左侧区域
    Right,     // 右侧区域（默认禁用）
    Top,       // 顶部区域
    Bottom,    // 底部区域
    Center,    // 中心区域（默认禁用）
    Floating   // 浮动区域
};
```

### TabStyle
```cpp
enum class TabStyle {
    DEFAULT,   // 默认样式
    FLAT,      // 扁平样式
    ROUNDED    // 圆角样式
};
```

### TabBorderStyle
```cpp
enum class TabBorderStyle {
    SOLID,     // 实线边框
    DASHED,    // 虚线边框
    DOTTED     // 点线边框
};
```

### LayoutStrategy
```cpp
enum class LayoutStrategy {
    IDE,       // IDE风格布局
    CAD,       // CAD风格布局
    Hybrid,    // 混合布局
    Flexible,  // 灵活布局
    Custom     // 自定义布局
};
```

---

## 常量定义

### 尺寸常量
```cpp
static constexpr int DEFAULT_TAB_HEIGHT = 24;
static constexpr int DEFAULT_TAB_MIN_WIDTH = 80;
static constexpr int DEFAULT_TAB_MAX_WIDTH = 200;
static constexpr int DEFAULT_TAB_SPACING = 2;
static constexpr int DEFAULT_CLOSE_BUTTON_SIZE = 16;
static constexpr int DEFAULT_CONTENT_MARGIN = 4;
static constexpr int DRAG_THRESHOLD = 8;
```

### 动画常量
```cpp
static constexpr int ANIMATION_FPS = 60;
static constexpr int DEFAULT_ANIMATION_DURATION = 250; // 毫秒
```

---

## 使用示例

### 基本用法
```cpp
// 创建停靠管理器
auto* dockManager = new ModernDockManager(mainFrame);

// 添加面板
auto* toolPanel = new MyToolPanel(dockManager);
dockManager->AddPanel(toolPanel, "工具", DockArea::Left);

// 添加多标签面板
auto* messagePanel = new MyMessagePanel(dockManager);
auto* performancePanel = new MyPerformancePanel(dockManager);
dockManager->AddPanel(messagePanel, "Message", DockArea::Bottom);
dockManager->AddPanel(performancePanel, "Performance", DockArea::Bottom);
```

### 自定义配置
```cpp
// 配置面板样式
auto* panel = dockManager->FindPanel("工具");
if (panel) {
    panel->SetTabCloseMode(TabCloseMode::ShowOnHover);
    panel->SetTabFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
}

// 禁用特定区域
dockManager->SetAreaDockingEnabled(DockArea::Center, false);
dockManager->SetAreaDockingEnabled(DockArea::Right, false);
```

### 事件处理
```cpp
// 绑定面板事件（如果实现了事件系统）
panel->Bind(wxEVT_DOCK_PANEL_CLOSING, &MyFrame::OnPanelClosing, this);
panel->Bind(wxEVT_DOCK_TAB_CHANGED, &MyFrame::OnTabChanged, this);
```

---

本API参考文档提供了Dock框架所有公共接口的详细说明，为开发者提供完整的API使用指南。

