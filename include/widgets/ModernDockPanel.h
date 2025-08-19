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

class ModernDockManager;
class DockTabBar;
class DockContent;

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
    void UpdateLayout();
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
    
    // Colors (will be theme-aware)
    wxColour m_backgroundColor;
    wxColour m_tabActiveColor;
    wxColour m_tabInactiveColor;
    wxColour m_tabHoverColor;
    wxColour m_textColor;
    wxColour m_borderColor;
    
    // Constants
    static constexpr int DEFAULT_TAB_HEIGHT = 28;
    static constexpr int DEFAULT_TAB_MIN_WIDTH = 60;
    static constexpr int DEFAULT_TAB_MAX_WIDTH = 200;
    static constexpr int DEFAULT_TAB_SPACING = 0;
    static constexpr int DEFAULT_CLOSE_BUTTON_SIZE = 16;
    static constexpr int DEFAULT_CONTENT_MARGIN = 2;
    static constexpr int DRAG_THRESHOLD = 5;
    static constexpr int ANIMATION_FPS = 60;

    wxDECLARE_EVENT_TABLE();
};

#endif // MODERN_DOCK_PANEL_H

