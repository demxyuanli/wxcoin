#ifndef FLATUIBAR_H
#define FLATUIBAR_H

#include "flatui/FlatUIPage.h"
#include "flatui/FlatUIPanel.h"
#include "flatui/FlatUIHomeSpace.h"
#include "flatui/FlatUIFunctionSpace.h"
#include "flatui/FlatUIProfileSpace.h"
#include "flatui/FlatUISystemButtons.h"
#include "flatui/FlatUIEventManager.h"
#include "flatui/FlatUISpacerControl.h"
#include "flatui/FlatUIUnpinButton.h"
#include "flatui/FlatUIFloatPanel.h"
#include "flatui/FlatUIFixPanel.h"
#include "flatui/FlatUIBarConfig.h"
#include "flatui/FlatUIBarStateManager.h"
#include "flatui/FlatUIPageManager.h"
#include "flatui/FlatUIBarLayoutManager.h"
#include "flatui/FlatUIBarEventDispatcher.h"
#include "flatui/FlatUIBarPerformanceManager.h"
#include "flatui/FlatBarSpaceContainer.h"
#include <wx/wx.h>
#include <wx/artprov.h>
#include <vector>
#include <memory>

// Forward declarations of the new component classes
class FlatUIPage; 
class FlatUIHomeSpace;
class FlatUIFunctionSpace;
class FlatUIProfileSpace;
class FlatUISystemButtons;
class FlatUISpacerControl;  
class FlatUIFloatPanel;
class FlatUIFixPanel;
class FlatBarSpaceContainer;
class wxButton; // Forward declare wxButton
class wxMenu;   // Forward declare wxMenu

// Custom events
wxDECLARE_EVENT(wxEVT_PIN_STATE_CHANGED, wxCommandEvent); // Backward compatibility with FlatFrame
wxDECLARE_EVENT(wxEVT_PIN_BUTTON_CLICKED, wxCommandEvent); // Pin button event from float panel

class FlatUIBar : public wxControl
{
public:
    // FLATUI_BAR_HEIGHT is now FLATUI_BAR_RENDER_HEIGHT in FlatUIConstants.h for paint calcs.
    // The GetBarHeight() static method remains important for overall height logic.
    // static const int FLATUI_BAR_HEIGHT = 30; // Removed, use constant from FlatUIConstants.h for rendering logic
    
    FlatUIBar(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0);
    virtual ~FlatUIBar();

    // --- Configuration Methods ---
    // Home Button (Dropdown Menu Icon)
    void SetHomeButtonMenu(wxMenu* menu);
    void SetHomeButtonIcon(const wxBitmap& icon = wxNullBitmap);
    void SetHomeButtonWidth(int width);

    // Page Tabs Management (now managed by PageManager)
    void AddPage(FlatUIPage* page);
    void SetActivePage(size_t index);
    size_t GetPageCount() const noexcept;
    size_t GetActivePage() const noexcept;
    FlatUIPage* GetPage(size_t index) const;
    
    // Tab Style Configuration
    enum class TabStyle {
        DEFAULT,        // Default style with top border for active tab
        UNDERLINE,      // Underline style for active tab
        BUTTON,         // Button-like appearance
        FLAT            // Completely flat, only text color changes
    };
    
    // Tab Border Style
    enum class TabBorderStyle {
        SOLID,          // Solid line border
        DASHED,         // Dashed line border
        DOTTED,         // Dotted line border
        DOUBLE,         // Double line border
        GROOVE,         // Groove style border
        RIDGE,          // Ridge style border
        ROUNDED         // Rounded corners
    };
    
    void SetTabStyle(TabStyle style);
    TabStyle GetTabStyle() const noexcept { return m_tabStyle; }
    
    // Tab border configuration
    void SetTabBorderStyle(TabBorderStyle style);
    TabBorderStyle GetTabBorderStyle() const noexcept { return m_tabBorderStyle; }
    void SetTabBorderWidths(int top, int bottom, int left, int right);
    void GetTabBorderWidths(int& top, int& bottom, int& left, int& right) const noexcept;
    void SetTabBorderColour(const wxColour& colour);
    wxColour GetTabBorderColour() const noexcept { return m_tabBorderColour; }
    
    // Individual border color configuration
    void SetTabBorderTopColour(const wxColour& colour);
    wxColour GetTabBorderTopColour() const noexcept { return m_tabBorderTopColour; }
    void SetTabBorderBottomColour(const wxColour& colour);
    wxColour GetTabBorderBottomColour() const noexcept { return m_tabBorderBottomColour; }
    void SetTabBorderLeftColour(const wxColour& colour);
    wxColour GetTabBorderLeftColour() const noexcept { return m_tabBorderLeftColour; }
    void SetTabBorderRightColour(const wxColour& colour);
    wxColour GetTabBorderRightColour() const noexcept { return m_tabBorderRightColour; }
    
    // Individual border width configuration
    void SetTabBorderTopWidth(int width);
    int GetTabBorderTopWidth() const noexcept { return m_tabBorderTop; }
    void SetTabBorderBottomWidth(int width);
    int GetTabBorderBottomWidth() const noexcept { return m_tabBorderBottom; }
    void SetTabBorderLeftWidth(int width);
    int GetTabBorderLeftWidth() const noexcept { return m_tabBorderLeft; }
    void SetTabBorderRightWidth(int width);
    int GetTabBorderRightWidth() const noexcept { return m_tabBorderRight; }
    
    void SetTabCornerRadius(int radius);  // For rounded style
    int GetTabCornerRadius() const noexcept { return m_tabCornerRadius; }
    
    // Tab colors
    void SetActiveTabBackgroundColour(const wxColour& colour);
    wxColour GetActiveTabBackgroundColour() const noexcept { return m_activeTabBgColour; }
    void SetActiveTabTextColour(const wxColour& colour);
    wxColour GetActiveTabTextColour() const noexcept { return m_activeTabTextColour; }
    void SetInactiveTabTextColour(const wxColour& colour);
    wxColour GetInactiveTabTextColour() const noexcept { return m_inactiveTabTextColour; }

    // Custom Spaces
    void SetFunctionSpaceControl(wxWindow* funcControl, int width = -1);
    void SetProfileSpaceControl(wxWindow* profControl, int width = -1);
    void ToggleFunctionSpaceVisibility();
    void ToggleProfileSpaceVisibility();
    void SetFunctionSpaceCenterAlign(bool center);
    void SetProfileSpaceRightAlign(bool rightAlign);
    
    void SetTabFunctionSpacerAutoExpand(bool autoExpand);
    
    void SetFunctionProfileSpacerAutoExpand(bool autoExpand);
    
    // New unified spacer management
    enum SpacerPosition {
        SPACER_BEFORE,
        SPACER_AFTER
    };
    
    enum SpacerLocation {
        SPACER_TAB_FUNCTION,      // Between tabs and function space
        SPACER_FUNCTION_PROFILE   // Between function and profile space
    };
    
    void AddSpaceSeparator(SpacerLocation location, int width, bool drawSeparator = false, bool canDrag = true, bool autoExpand = false);

    static int GetBarHeight(); // Renamed from GetTabAreaHeight for clarity

    // Bar margin configuration
    void SetBarTopMargin(int margin);
    int GetBarTopMargin() const noexcept { return m_barTopMargin; }

    void SetBarBottomMargin(int margin);
    int GetBarBottomMargin() const noexcept { return m_barBottomMargin; }

    // Override to provide best size hint
    virtual wxSize DoGetBestSize() const override;

    void OnPaint(wxPaintEvent& evt);
    void OnSize(wxSizeEvent& evt);
    void OnMouseDown(wxMouseEvent& evt); // Will primarily handle tab clicks now
    // OnMouseMove and OnMouseLeave might be less relevant here if sub-controls handle their own hover


    FlatUISpacerControl* GetTabFunctionSpacer() { return m_tabFunctionSpacer; }
    FlatUISpacerControl* GetFunctionProfileSpacer() { return m_functionProfileSpacer; }

    // Component access methods for layout manager
    FlatUIHomeSpace* GetHomeSpace() { return m_homeSpace; }
    FlatUISystemButtons* GetSystemButtons() { return m_systemButtons; }
    FlatUIFunctionSpace* GetFunctionSpace() { return m_functionSpace; }
    FlatUIProfileSpace* GetProfileSpace() { return m_profileSpace; }
    
    // Layout data access
    void SetTabAreaRect(const wxRect& rect) { m_tabAreaRect = rect; }
    wxRect GetTabAreaRect() const { return m_tabAreaRect; }
    
    // Layout configuration access
    bool GetFunctionSpaceCenterAlign() const { return m_functionSpaceCenterAlign; }
    bool GetProfileSpaceRightAlign() const { return m_profileSpaceRightAlign; }

    // Pin management methods (now using StateManager)
    bool IsBarPinned() const;
    void SetGlobalPinned(bool pinned);
    bool IsGlobalPinned() const;
    void ToggleGlobalPinState();
    
    // Helper method to determine if pages should be visible
    bool ShouldShowPages() const;

    // Fixed panel for pinned state
    FlatUIFixPanel* GetFixPanel() const { return m_fixPanel; }
    
    // Float panel for unpinned state
    FlatUIFloatPanel* m_floatPanel;
    
    // Manager access for subcomponents
    FlatUIBarStateManager* GetStateManager() const { return m_stateManager.get(); }
    FlatUIBarPerformanceManager* GetPerformanceManager() const { return m_performanceManager.get(); }
    
    // Methods for float panel
    void ShowPageInFloatPanel(FlatUIPage* page);
    void HideFloatPanel();
    void OnFloatPanelDismissed(wxCommandEvent& event);
    
    // State change handling (called by EventDispatcher)
    void OnGlobalPinStateChanged(bool isPinned);

    // Methods for tab truncation and dropdown
    wxButton* GetTabsDropdownButton() const { return m_tabsDropdownButton; }
    void UpdateHiddenTabsMenu(const std::vector<size_t>& hiddenIndices);
    void SetVisibleTabsCount(size_t count);
    size_t GetVisibleTabsCount() const;

    // Tab width calculation (for layout purposes)
    int CalculateTabsWidth(wxDC& dc) const;

    // User toggle state management
    bool GetFunctionSpaceUserVisible() const { return m_functionSpaceUserVisible; }
    void SetFunctionSpaceUserVisible(bool visible) { m_functionSpaceUserVisible = visible; }
    bool GetProfileSpaceUserVisible() const { return m_profileSpaceUserVisible; }
    void SetProfileSpaceUserVisible(bool visible) { m_profileSpaceUserVisible = visible; }

private:
    // Core managers - centralized logic
    std::unique_ptr<FlatUIBarStateManager> m_stateManager;
    std::unique_ptr<FlatUIPageManager> m_pageManager;
    std::unique_ptr<FlatUIBarLayoutManager> m_layoutManager;
    std::unique_ptr<FlatUIBarEventDispatcher> m_eventDispatcher;
    std::unique_ptr<FlatUIBarPerformanceManager> m_performanceManager;

    // Legacy support - will be gradually removed
    FlatUIPage* m_temporarilyShownPage;
    int m_barUnpinnedHeight;

    // Event handlers - simplified
    void OnGlobalMouseDown(wxMouseEvent& event);
    void OnPinButtonClicked(wxCommandEvent& event);
    void OnUnpinButtonClicked(wxCommandEvent& event);
    void OnShow(wxShowEvent& event);
    void OnTabsDropdown(wxCommandEvent& event);
    void OnHiddenTabMenuItem(wxCommandEvent& event);

    // Helper methods - simplified
    bool IsPointInBarArea(const wxPoint& point) const;
    void SetupGlobalMouseCapture();
    void ReleaseGlobalMouseCapture();
    void HideTemporarilyShownPage();
    
    // Legacy methods - kept for compatibility
    void ShowAllContent();
    void HideAllContentExceptBarSpace();
    void UpdateButtonVisibility();

    // --- Child Component Controls ---
    FlatBarSpaceContainer* m_barContainer;  // New container for all bar components
    FlatUIHomeSpace* m_homeSpace;
    // TabSpace is currently handled directly by FlatUIBar's m_pages and PaintTabs
    FlatUIFunctionSpace* m_functionSpace;
    FlatUIProfileSpace* m_profileSpace;
    FlatUISystemButtons* m_systemButtons;
    // Pin button is now handled by FlatUIFloatPanel
    FlatUIUnpinButton* m_unpinButton;
    
    // Fixed panel for containing pages in pinned state
    FlatUIFixPanel* m_fixPanel;
    
    // Dropdown for hidden tabs
    wxButton* m_tabsDropdownButton;
    wxMenu* m_hiddenTabsMenu;

    FlatUISpacerControl* m_tabFunctionSpacer;    
    FlatUISpacerControl* m_functionProfileSpacer; 

    wxRect m_tabAreaRect; // Stores the calculated rectangle for drawing tabs
    
    // Tab style configuration
    TabStyle m_tabStyle;
    TabBorderStyle m_tabBorderStyle;

    int m_tabTopSpacing;
    int m_tabBorderTop;
    int m_tabBorderBottom;
    int m_tabBorderLeft;
    int m_tabBorderRight;
    int m_tabCornerRadius;
    wxColour m_tabBorderColour;
    wxColour m_tabBorderTopColour;
    wxColour m_tabBorderBottomColour;
    wxColour m_tabBorderLeftColour;
    wxColour m_tabBorderRightColour;
    wxColour m_activeTabBgColour;
    wxColour m_activeTabTextColour;
    wxColour m_inactiveTabTextColour;
    
    // Bar margin
    int m_barTopMargin;
    int m_barBottomMargin;
    
    // --- Configuration for direct Tab drawing by FlatUIBar ---
    // static const int TAB_PADDING = 10; // Removed
    // static const int TAB_SPACING = 1;  // Removed
    
    // --- General Layout Constants (can be adjusted) ---
    // static const int ELEMENT_SPACING = 5; // Removed
    // static const int BAR_PADDING = 2;     // Removed

    // --- Helper methods ---
    void DrawTabBorder(wxDC& dc, const wxRect& tabRect, bool isActive); // Draw tab border with style (for container use)

    // GetTopLevelFrame might now be primarily used by m_systemButtons internally
    // wxFrame* GetTopLevelFrame() const; 

    void DrawBackground(wxDC& dc);
    void DrawBarSeparator(wxDC& dc);
    
    // Hardware-accelerated drawing methods
    void DrawBackgroundOptimized(wxGraphicsContext& gc);
    void DrawBarSeparatorOptimized(wxGraphicsContext& gc);

    bool m_functionSpaceCenterAlign;
    bool m_profileSpaceRightAlign;

    // Helper methods for floating window
    
    // User toggle states for spaces
    bool m_functionSpaceUserVisible;  // Track user's toggle state for function space
    bool m_profileSpaceUserVisible;   // Track user's toggle state for profile space

    size_t m_visibleTabsCount; // Number of tabs that fit in the current layout

};

#endif // FLATUIBAR_H 