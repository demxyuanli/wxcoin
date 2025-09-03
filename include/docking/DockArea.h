#pragma once

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/settings.h>
#include <vector>
#include <memory>
#include "DockManager.h"  // For DockWidgetArea enum
#include "config/ThemeManager.h"  // For CFG_* macros

// ThemeManager macros for consistent usage across docking components
#define DOCK_COLOUR(key) CFG_COLOUR(key)
#define DOCK_FONT() CFG_DEFAULTFONT()
#define DOCK_INT(key) CFG_INT(key)

namespace ads {

// Forward declarations
class DockWidget;
class DockManager;
class DockContainerWidget;
class DockAreaTabBar;
class DockAreaTitleBar;
// Forward declaration for merged title bar
class DockAreaMergedTitleBar;
class FloatingDockContainer;
class FloatingDragPreview;

// Docking Styles - Similar to FlatUIBar Tab Styles
enum class DockStyle {
    DEFAULT,     // Default style with borders on all sides
    UNDERLINE,   // Only bottom border, flat top
    BUTTON,      // Raised button style
    FLAT         // Flat style with minimal borders
};

// Style configuration for docking elements
struct DockStyleConfig {
    DockStyle style = DockStyle::DEFAULT;

    // Border widths
    int borderTop = 1;
    int borderBottom = 1;
    int borderLeft = 1;
    int borderRight = 1;

    // Colors
    wxColour borderTopColour = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
    wxColour borderBottomColour = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
    wxColour borderLeftColour = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
    wxColour borderRightColour = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);

    // Background colors
    wxColour backgroundColour = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
    wxColour activeBackgroundColour = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    wxColour hoverBackgroundColour = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNHIGHLIGHT);

    // Text colors
    wxColour textColour = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT);
    wxColour activeTextColour = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    wxColour inactiveTextColour = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNTEXT);

    // Corner radius for rounded styles
    int cornerRadius = 0;

    // Font configuration (from ThemeManager)
    wxFont font = *wxNORMAL_FONT;

    // Size configurations
    int tabHeight = 24;           // Tab height (changed to 24)
    int titleBarHeight = 30;      // Title bar height (changed to 30)
    int tabTopMargin = 4;         // Tab top margin from title bar (changed to 4)
    int tabSpacing = 4;           // Spacing between tabs (changed to 4)
    int borderWidth = 1;          // Left/right border width
    int buttonSize = 12;          // Button size (fixed 12x12)
    int contentMargin = 2;        // Margin between tab content and internal controls

    // Icon configuration
    bool useSvgIcons = true;      // Whether to use SVG icons
    wxString closeIconName = "close";
    wxString pinIconName = "pin";
    wxString autoHideIconName = "auto_hide";
    wxString menuIconName = "menu";

    // Set style with predefined configurations
    void SetStyle(DockStyle newStyle) {
        style = newStyle;

        // Set common properties
        tabTopMargin = 2;

        switch (style) {
        case DockStyle::DEFAULT:
            borderTop = 1;
            borderBottom = 1;
            borderLeft = 1;
            borderRight = 1;
            cornerRadius = 0;
            break;
        case DockStyle::UNDERLINE:
            borderTop = 0;
            borderBottom = 2;
            borderLeft = 0;
            borderRight = 0;
            cornerRadius = 0;
            break;
        case DockStyle::BUTTON:
            borderTop = 1;
            borderBottom = 1;
            borderLeft = 1;
            borderRight = 1;
            cornerRadius = 3;
            break;
        case DockStyle::FLAT:
            borderTop = 1;
            borderBottom = 0;
            borderLeft = 0;
            borderRight = 0;
            cornerRadius = 0;
            break;
        }
    }

    // Initialize with ThemeManager values
    void InitializeFromThemeManager();
};

// Dock area flags
enum DockAreaFlag {
    HideSingleWidgetTitleBar = 0x0001,
    DefaultFlags = 0x0000
};

typedef int DockAreaFlags;

/**
 * @brief DockArea holds multiple dock widgets in a tabbed interface
 */
class DockArea : public wxPanel {
public:
    DockArea(DockManager* dockManager, DockContainerWidget* parent);
    virtual ~DockArea();

    // Dock widget management
    void addDockWidget(DockWidget* dockWidget);
    void removeDockWidget(DockWidget* dockWidget);
    void insertDockWidget(int index, DockWidget* dockWidget, bool activate = true);
    DockWidget* currentDockWidget() const;
    void setCurrentDockWidget(DockWidget* dockWidget);
    int currentIndex() const;
    void setCurrentIndex(int index);
    int indexOfDockWidget(DockWidget* dockWidget) const;
    int dockWidgetsCount() const { return static_cast<int>(m_dockWidgets.size()); }
    std::vector<DockWidget*> dockWidgets() const { return m_dockWidgets; }
    DockWidget* dockWidget(int index) const;
    
    // Title bar
    DockAreaTitleBar* titleBar() const { return m_titleBar; }

    // Merged title bar (new combined tabs + buttons)
    DockAreaMergedTitleBar* mergedTitleBar() const { return m_mergedTitleBar; }

    // Tab bar
    DockAreaTabBar* tabBar() const { return m_tabBar; }
    
    // Features and flags
    void setDockAreaFlags(DockAreaFlags flags);
    DockAreaFlags dockAreaFlags() const { return m_flags; }
    void setDockAreaFlag(DockAreaFlag flag, bool on);
    bool testDockAreaFlag(DockAreaFlag flag) const;
    
    // Manager and container access
    DockManager* dockManager() const { return m_dockManager; }
    DockContainerWidget* dockContainer() const { return m_containerWidget; }
    
    // Visibility
    void toggleView(bool open);
    void setVisible(bool visible);
    void updateTitleBarVisibility();
    
    // State
    bool isAutoHide() const;
    bool isHidden() const { return !IsShown(); }
    bool isCurrent() const;
    void saveState(wxString& xmlData) const;
    
    // Closing
    void closeArea();
    void closeOtherAreas();
    
    // Title
    wxString currentTabTitle() const;
    
    // Events
    static wxEventTypeTag<wxCommandEvent> EVT_DOCK_AREA_CURRENT_CHANGED;
    static wxEventTypeTag<wxCommandEvent> EVT_DOCK_AREA_CLOSING;
    static wxEventTypeTag<wxCommandEvent> EVT_DOCK_AREA_CLOSED;
    static wxEventTypeTag<wxCommandEvent> EVT_DOCK_AREA_TAB_ABOUT_TO_CLOSE;

    // Style configuration
    static void SetDockStyle(DockStyle style);
    static void SetDockStyleConfig(const DockStyleConfig& config);
    static const DockStyleConfig& GetDockStyleConfig();
    
    // Friend classes that need access to protected members
    friend class DockAreaTabBar;
    
protected:
    // Internal methods
    void onTabCloseRequested(int index);
    void onCurrentTabChanged(int index);
    void onTitleBarButtonClicked();
    void updateTitleBarButtonStates();
    void updateTabBar();
    void internalSetCurrentDockWidget(DockWidget* dockWidget);
    void markTitleBarMenuOutdated();
    
    // Event handlers
    void onSize(wxSizeEvent& event);
    void onClose(wxCloseEvent& event);
    
private:
    // Private implementation
    class Private;
    std::unique_ptr<Private> d;
    
    // Member variables
    DockManager* m_dockManager;
    DockContainerWidget* m_containerWidget;
    std::vector<DockWidget*> m_dockWidgets;
    DockAreaTitleBar* m_titleBar;
    DockAreaTabBar* m_tabBar;
    DockAreaMergedTitleBar* m_mergedTitleBar;
    wxBoxSizer* m_layout;
    wxPanel* m_contentArea;
    wxBoxSizer* m_contentSizer;
    DockWidget* m_currentDockWidget;
    bool m_isClosing;
    int m_currentIndex;
    DockAreaFlags m_flags;
    bool m_updateTitleBarButtons;
    bool m_menuOutdated;
    
    friend class DockContainerWidget;
    friend class DockWidget;
    friend class DockManager;
    friend class DockAreaMergedTitleBar;
    
    wxDECLARE_EVENT_TABLE();
};

/**
 * @brief Merged title bar that combines tabs and system buttons in one row
 */
class DockAreaMergedTitleBar : public wxPanel {
public:
    DockAreaMergedTitleBar(DockArea* dockArea);
    virtual ~DockAreaMergedTitleBar();

    void updateTitle();
    void updateButtonStates();
    void insertTab(int index, DockWidget* dockWidget);
    void removeTab(DockWidget* dockWidget);
    void setCurrentIndex(int index);
    int getTabCount() const { return static_cast<int>(m_tabs.size()); }
    DockWidget* getTabWidget(int index) const;

    void showCloseButton(bool show) { m_showCloseButton = show; Refresh(); }
    void showAutoHideButton(bool show) { m_showAutoHideButton = show; Refresh(); }
    void showPinButton(bool show) { m_showPinButton = show; Refresh(); }

protected:
    void onPaint(wxPaintEvent& event);
    void onMouseLeftDown(wxMouseEvent& event);
    void onMouseLeftUp(wxMouseEvent& event);
    void onMouseMotion(wxMouseEvent& event);
    void onMouseLeave(wxMouseEvent& event);
    void onSize(wxSizeEvent& event);

private:
    struct TabInfo {
        DockWidget* widget = nullptr;
        wxRect rect;
        wxRect closeButtonRect;
        bool closeButtonHovered = false;
        bool hovered = false;
        bool showCloseButton = true;  // Whether to show close button for this tab
    };

    DockArea* m_dockArea;
    std::vector<TabInfo> m_tabs;
    int m_currentIndex;
    int m_hoveredTab;
    int m_buttonSize;
    int m_buttonSpacing;
    wxRect m_pinButtonRect;
    wxRect m_closeButtonRect;
    wxRect m_autoHideButtonRect;
    bool m_showCloseButton;
    bool m_showAutoHideButton;
    bool m_showPinButton;

    // Drag and drop support
    int m_draggedTab;
    wxPoint m_dragStartPos;
    bool m_dragStarted;
    FloatingDragPreview* m_dragPreview;

    // Button hover states
    bool m_pinButtonHovered;
    bool m_closeButtonHovered;
    bool m_autoHideButtonHovered;

    // Overflow support
    bool m_hasOverflow;
    int m_firstVisibleTab;
    wxRect m_overflowButtonRect;

    void updateTabRects();
    void drawTab(wxDC& dc, int index);
    void drawButton(wxDC& dc, const wxRect& rect, const wxString& text, bool hovered);
    int getTabAt(const wxPoint& pos) const;
    wxRect getButtonRect(int buttonIndex) const; // 0=pin, 1=close, 2=auto hide
    void showTabOverflowMenu();

    // Drag and drop helpers
    bool isDraggingTab() const { return m_dragStarted && m_draggedTab >= 0; }
    void updateDragCursor(int dropArea);
    void showDragFeedback(bool showMergeHint = false);

    // Target detection
    wxWindow* findTargetWindowUnderMouse(const wxPoint& screenPos, wxWindow* dragPreview) const;

    wxDECLARE_EVENT_TABLE();
};

/**
 * @brief Tab bar for dock area
 */
class DockAreaTabBar : public wxPanel {
public:
    DockAreaTabBar(DockArea* dockArea);
    virtual ~DockAreaTabBar();
    
    // Tab management
    void insertTab(int index, DockWidget* dockWidget);
    void removeTab(DockWidget* dockWidget);
    void setCurrentIndex(int index);
    int currentIndex() const { return m_currentIndex; }
    bool isTabOpen(int index) const;
    
    // Count
    int count() const { return static_cast<int>(m_tabs.size()); }
    
    // Overflow menu
    bool hasTabOverflow() const { return m_hasOverflow; }
    void showTabOverflowMenu();
    void showTabContextMenu(int tab, const wxPoint& pos);
    
    // Events
    static wxEventTypeTag<wxCommandEvent> EVT_TAB_CLOSE_REQUESTED;
    static wxEventTypeTag<wxCommandEvent> EVT_TAB_CURRENT_CHANGED;
    static wxEventTypeTag<wxCommandEvent> EVT_TAB_MOVED;
    
protected:
    void onPaint(wxPaintEvent& event);
    void onMouseLeftDown(wxMouseEvent& event);
    void onMouseLeftUp(wxMouseEvent& event);
    void onMouseRightDown(wxMouseEvent& event);
    void onMouseMotion(wxMouseEvent& event);
    void onMouseLeave(wxMouseEvent& event);
    void onSize(wxSizeEvent& event);
    
private:
    struct TabInfo {
        DockWidget* widget;
        wxRect rect;
        wxRect closeButtonRect;
        bool closeButtonHovered;
    };
    
    DockArea* m_dockArea;
    std::vector<TabInfo> m_tabs;
    int m_currentIndex;
    int m_hoveredTab;
    int m_draggedTab;
    wxPoint m_dragStartPos;
    bool m_dragStarted;
    class FloatingDragPreview* m_dragPreview;
    bool m_hasOverflow;
    int m_firstVisibleTab;
    wxRect m_overflowButtonRect;
    
    int getTabAt(const wxPoint& pos);
    void updateTabRects();
    void drawTab(wxDC& dc, int index);
    void checkTabOverflow();
    wxRect getTabCloseRect(int index) const;
    bool isOverCloseButton(int tabIndex, const wxPoint& pos) const;
    
    wxDECLARE_EVENT_TABLE();
    
    friend class DockArea;
};

/**
 * @brief Title bar for dock area
 */
class DockAreaTitleBar : public wxPanel {
public:
    DockAreaTitleBar(DockArea* dockArea);
    virtual ~DockAreaTitleBar();
    
    // Title
    void updateTitle();
    wxString titleText() const { return m_titleLabel->GetLabel(); }
    
    // Buttons
    void updateButtonStates();
    void showCloseButton(bool show);
    void showAutoHideButton(bool show);
    
    // Events
    static wxEventTypeTag<wxCommandEvent> EVT_TITLE_BAR_BUTTON_CLICKED;
    
protected:
    void onPaint(wxPaintEvent& event);
    void onCloseButtonClicked(wxCommandEvent& event);
    void onAutoHideButtonClicked(wxCommandEvent& event);
    void onMenuButtonClicked(wxCommandEvent& event);
    void onPinButtonClicked(wxCommandEvent& event);
    
private:
    DockArea* m_dockArea;
    wxStaticText* m_titleLabel;
    wxButton* m_closeButton;
    wxButton* m_autoHideButton;
    wxButton* m_menuButton;
    wxBoxSizer* m_layout;
    
    void createButtons();
    
    wxDECLARE_EVENT_TABLE();
};

// Global functions for style management
void EnsureThemeManagerInitialized();
const DockStyleConfig& GetDockStyleConfig();

// Style helper functions
void DrawStyledRect(wxDC& dc, const wxRect& rect, const DockStyleConfig& style,
                   bool isActive, bool isHovered, bool isTitleBar);
void SetStyledTextColor(wxDC& dc, const DockStyleConfig& style, bool isActive);
void DrawCloseButton(wxDC& dc, const wxRect& rect, const DockStyleConfig& style, bool isHovered);
void DrawSvgButton(wxDC& dc, const wxRect& rect, const wxString& iconName,
                  const DockStyleConfig& style, bool isHovered);

} // namespace ads
