#ifndef MODERN_DOCK_PANEL_H
#define MODERN_DOCK_PANEL_H

#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/graphics.h>
#include <wx/timer.h>
#include <wx/bitmap.h>
#include <memory>
#include <vector>
#include "widgets/DockTypes.h"
#include "widgets/DockSystemButtons.h"

class ModernDockManager;
class DockTabBar;
class DockContent;

// Tab style enumeration
enum class TabStyle {
    DEFAULT,        // Default style with top border for active tab
    UNDERLINE,      // Underline style for active tab
    BUTTON,         // Button-like appearance
    FLAT            // Completely flat, only text color changes
};

// Tab border style enumeration
enum class TabBorderStyle {
    SOLID,          // Solid line border
    DASHED,         // Dashed line border
    DOTTED,         // Dotted line border
    DOUBLE,         // Double line border
    GROOVE,         // Groove style border
    RIDGE,          // Ridge style border
    ROUNDED         // Rounded corners
};

// Modern dock panel with VS2022-style appearance
class ModernDockPanel : public wxPanel {
public:
    ModernDockPanel(ModernDockManager* manager, wxWindow* parent, 
                   const wxString& title = wxEmptyString);
    ~ModernDockPanel() override;

    // Content management
    void AddContent(wxWindow* content, const wxString& title, 
                   const wxBitmap& icon = wxNullBitmap, bool select = true);
    void RemoveContent(wxWindow* content);
    void RemoveContent(int index);
    void SelectContent(int index);
    void SelectContent(wxWindow* content);
    
    // Tab management
    int GetContentCount() const;
    wxWindow* GetContent(int index) const;
    wxWindow* GetContent() const { return GetSelectedContent(); }  // Convenience method
    wxWindow* GetSelectedContent() const;
    int GetSelectedIndex() const;
    wxString GetContentTitle(int index) const;
    void SetContentTitle(int index, const wxString& title);
    wxBitmap GetContentIcon(int index) const;
    void SetContentIcon(int index, const wxBitmap& icon);
    
    // Panel properties
    void SetTitle(const wxString& title);
    wxString GetTitle() const { return m_title; }
    void SetDockArea(DockArea area) { m_dockArea = area; }
    DockArea GetDockArea() const { return m_dockArea; }
    
    // State management
    void SetState(DockPanelState state);
    DockPanelState GetState() const { return m_state; }
    void SetFloating(bool floating);
    bool IsFloating() const { return m_state == DockPanelState::Floating; }
    
    // Visual settings
    void SetTabCloseMode(TabCloseMode mode);
    TabCloseMode GetTabCloseMode() const { return m_tabCloseMode; }
    void SetShowTabs(bool show);
    bool IsShowingTabs() const { return m_showTabs; }
    
    // Style configuration
    void SetTabStyle(TabStyle style);
    TabStyle GetTabStyle() const { return m_tabStyle; }
    
    void SetTabBorderStyle(TabBorderStyle style);
    TabBorderStyle GetTabBorderStyle() const { return m_tabBorderStyle; }
    
    void SetTabCornerRadius(int radius);
    int GetTabCornerRadius() const { return m_tabCornerRadius; }
    
    void SetTabBorderWidths(int top, int bottom, int left, int right);
    void GetTabBorderWidths(int& top, int& bottom, int& left, int& right) const;
    
    void SetTabBorderColours(const wxColour& top, const wxColour& bottom, const wxColour& left, const wxColour& right);
    void GetTabBorderColours(wxColour& top, wxColour& bottom, wxColour& left, wxColour& right) const;
    
    void SetTabPadding(int padding);
    int GetTabPadding() const { return m_tabPadding; }
    
    void SetTabSpacing(int spacing);
    int GetTabSpacing() const { return m_tabSpacing; }
    
    void SetTabTopMargin(int margin);
    int GetTabTopMargin() const { return m_tabTopMargin; }
    
    // Font configuration
    void SetTabFont(const wxFont& font);
    wxFont GetTabFont() const { return m_tabFont; }
    
    void SetTitleFont(const wxFont& font);
    wxFont GetTitleFont() const { return m_titleFont; }
    
    // System buttons management
    void AddSystemButton(DockSystemButtonType type, const wxString& tooltip = wxEmptyString);
    void RemoveSystemButton(DockSystemButtonType type);
    void SetSystemButtonEnabled(DockSystemButtonType type, bool enabled);
    void SetSystemButtonVisible(DockSystemButtonType type, bool visible);
    void SetSystemButtonIcon(DockSystemButtonType type, const wxBitmap& icon);
    void SetSystemButtonTooltip(DockSystemButtonType type, const wxString& tooltip);
    
    // Drag operations
    void StartDrag(int tabIndex, const wxPoint& startPos);
    bool IsDragging() const { return m_dragging; }
    int GetDraggedTabIndex() const { return m_draggedTabIndex; }
    
    // Hit testing
    int HitTestTab(const wxPoint& pos) const;
    bool HitTestCloseButton(const wxPoint& pos, int& tabIndex) const;
    wxRect GetTabRect(int index) const;
    wxRect GetContentRect() const;
    
    // Animation support
    void AnimateTabInsertion(int index);
    void AnimateTabRemoval(int index);
    void AnimateResize(const wxSize& targetSize);

protected:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnLeftUp(wxMouseEvent& event);
    void OnRightDown(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnMouseEnter(wxMouseEvent& event);
    void OnTimer(wxTimerEvent& event);
    void OnContextMenu(wxContextMenuEvent& event);

private:
    void InitializePanel();
    void UpdateThemeColors();
    void UpdateLayout();
    void RenderTitleBar(wxGraphicsContext* gc);
    void RenderTabBar(wxGraphicsContext* gc);
    void RenderTab(wxGraphicsContext* gc, int index, const wxRect& rect, bool selected, bool hovered);
    void RenderCloseButton(wxGraphicsContext* gc, const wxRect& rect, bool hovered);
    void RenderContent(wxGraphicsContext* gc);
    
    // Tab layout calculation
    void CalculateTabLayout();
    wxRect CalculateTabRect(int index) const;
    wxRect CalculateCloseButtonRect(const wxRect& tabRect) const;
    
    // Event handling
    void HandleTabClick(int tabIndex, const wxPoint& pos);
    void HandleTabDoubleClick(int tabIndex);
    void HandleTabRightClick(int tabIndex, const wxPoint& pos);
    void HandleCloseButtonClick(int tabIndex);
    
    // Animation
    void StartAnimation(int durationMs);
    void UpdateAnimation();
    void StopAnimation();
    
    // Content storage
    struct ContentItem {
        wxWindow* content;
        wxString title;
        wxBitmap icon;
        bool visible;
        wxRect animRect;
        
        ContentItem(wxWindow* c, const wxString& t, const wxBitmap& i) 
            : content(c), title(t), icon(i), visible(true) {}
    };
    
    ModernDockManager* m_manager;
    std::vector<std::unique_ptr<ContentItem>> m_contents;
    wxString m_title;
    DockArea m_dockArea;
    DockPanelState m_state;
    
    // Visual state
    int m_selectedIndex;
    int m_hoveredTabIndex;
    int m_hoveredCloseIndex;
    bool m_showTabs;
    TabCloseMode m_tabCloseMode;
    
    // Drag state
    bool m_dragging;
    int m_draggedTabIndex;
    wxPoint m_dragStartPos;
    wxPoint m_lastMousePos;
    
    // Layout
    wxRect m_tabBarRect;
    wxRect m_contentRect;
    std::vector<wxRect> m_tabRects;
    std::vector<wxRect> m_closeButtonRects;
    
    // Animation
    wxTimer m_animationTimer;
    bool m_animating;
    double m_animationProgress;
    int m_animationDuration;
    wxSize m_animationStartSize;
    wxSize m_animationTargetSize;
    
    // Styling
    int m_tabHeight;
    int m_tabMinWidth;
    int m_tabMaxWidth;
    int m_tabSpacing;
    int m_closeButtonSize;
    int m_contentMargin;
    
    // Style configuration
    TabStyle m_tabStyle;
    TabBorderStyle m_tabBorderStyle;
    int m_tabCornerRadius;
    int m_tabBorderTop;
    int m_tabBorderBottom;
    int m_tabBorderLeft;
    int m_tabBorderRight;
    int m_tabPadding;
    int m_tabTopMargin;
    
    // Fonts
    wxFont m_tabFont;
    wxFont m_titleFont;
    
    // Colors (theme-aware)
    wxColour m_backgroundColor;
    wxColour m_tabActiveColor;
    wxColour m_tabInactiveColor;
    wxColour m_tabHoverColor;
    wxColour m_textColor;
    wxColour m_borderColor;
    
    // Extended colors
    wxColour m_tabBorderTopColor;
    wxColour m_tabBorderBottomColor;
    wxColour m_tabBorderLeftColor;
    wxColour m_tabBorderRightColor;
    wxColour m_tabActiveTextColor;
    wxColour m_tabHoverTextColor;
    wxColour m_closeButtonNormalColor;
    wxColour m_titleBarBgColor;
    wxColour m_titleBarTextColor;
    wxColour m_titleBarBorderColor;
    
    // Theme change handling
    void OnThemeChanged();
    
    // System buttons
    DockSystemButtons* m_systemButtons;
    
    // Constants
    static constexpr int DEFAULT_TAB_HEIGHT = 28;
    static constexpr int DEFAULT_TAB_MIN_WIDTH = 60;
    static constexpr int DEFAULT_TAB_MAX_WIDTH = 200;
    static constexpr int DEFAULT_TAB_SPACING = 0;
    static constexpr int DEFAULT_CLOSE_BUTTON_SIZE = 16;
    static constexpr int DEFAULT_CONTENT_MARGIN = 2;
    static constexpr int DEFAULT_TAB_PADDING = 8;
    static constexpr int DEFAULT_TAB_TOP_MARGIN = 4;
    static constexpr int DEFAULT_TAB_CORNER_RADIUS = 4;
    static constexpr int DEFAULT_TAB_BORDER_TOP = 2;
    static constexpr int DEFAULT_TAB_BORDER_BOTTOM = 1;
    static constexpr int DEFAULT_TAB_BORDER_LEFT = 1;
    static constexpr int DEFAULT_TAB_BORDER_RIGHT = 1;
    static constexpr int DRAG_THRESHOLD = 5;
    static constexpr int ANIMATION_FPS = 60;

    wxDECLARE_EVENT_TABLE();
};

#endif // MODERN_DOCK_PANEL_H

