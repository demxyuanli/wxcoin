#ifndef FLATUIBAR_LAYOUT_MANAGER_H
#define FLATUIBAR_LAYOUT_MANAGER_H

#include <wx/wx.h>
#include <vector>

// Forward declarations
class FlatUIBar;
class FlatUIHomeSpace;
class FlatUIFunctionSpace;
class FlatUIProfileSpace;
class FlatUISystemButtons;
class FlatUISpacerControl;
class FlatUIFixPanel;
class FlatUIFloatPanel;

struct LayoutElementInfo {
    wxWindow* control;
    wxPoint position;
    wxSize size;
    bool visible;
    
    LayoutElementInfo() : control(nullptr), visible(false) {}
    LayoutElementInfo(wxWindow* ctrl, const wxPoint& pos, const wxSize& sz, bool vis)
        : control(ctrl), position(pos), size(sz), visible(vis) {}
};

// Helper structure for tab layout calculation results
struct TabLayoutParams {
    int visibleWidth = 0;
    size_t visibleCount = 0;
    std::vector<size_t> hiddenIndices;
};

class FlatUIBarLayoutManager {
public:
    explicit FlatUIBarLayoutManager(FlatUIBar* bar);
    ~FlatUIBarLayoutManager() = default;
    
    // Main layout methods
    void UpdateLayout(const wxSize& barClientSize);
    void UpdateLayoutIfNeeded(const wxSize& barClientSize);
    void MarkLayoutDirty();
    void ForceRefresh();
    void DeferredRefresh();
    
    // Element positioning
    void PositionHomeSpace(int& currentX, int elementY, int innerHeight);
    void PositionTabs(int& currentX, int elementY, int innerHeight, const wxSize& barSize);
    void PositionSystemButtons(int& availableWidth, int elementY, int innerHeight, const wxSize& barSize);
    void PositionFunctionSpace(int& currentX, int elementY, int innerHeight, int availableWidth);
    void PositionProfileSpace(int& currentX, int elementY, int innerHeight, int availableWidth);
    void PositionSpacers(int& currentX, int elementY, int innerHeight, int availableWidth);
    void PositionFixPanel(const wxSize& barSize);
    void PositionFloatPanel();
    
    // Layout calculations
    int CalculateTabsWidth(wxDC& dc) const;
    int CalculateAvailableSpaceForFlexibleElements(int currentX, int totalWidth, int systemButtonsWidth) const;
    wxRect CalculateTabAreaRect(int currentX, int elementY, int tabsWidth, int barHeight) const;
    
    // Element visibility management
    void UpdateElementVisibility();
    void ShowElement(wxWindow* element, bool show);
    
    // Layout validation
    bool ValidateLayout() const;
    void LogLayoutInfo(const wxString& context) const;
    
private:
    FlatUIBar* m_bar;
    
    // Layout state
    wxRect m_tabAreaRect;
    wxSize m_lastBarSize;
    bool m_layoutValid;
    bool m_layoutDirty;
    bool m_refreshPending;
    
    // Element info cache
    LayoutElementInfo m_homeSpaceInfo;
    LayoutElementInfo m_functionSpaceInfo;
    LayoutElementInfo m_profileSpaceInfo;
    LayoutElementInfo m_systemButtonsInfo;
    
    // Helper methods
    void CacheElementInfo();
    void ApplyElementLayout(const LayoutElementInfo& info);
    bool ShouldShowElement(wxWindow* element) const;
    int GetElementSpacing() const;
    int GetBarPadding() const;
    TabLayoutParams CalculateVisibleTabs(wxDC& dc, int availableWidth) const;
    
    // Specific layout logic
    void HandleCenteredFunctionSpace(int& currentX, int elementY, int innerHeight, 
                                   int availableWidth, int totalFixedWidth);
    void HandleSequentialLayout(int& currentX, int elementY, int innerHeight, int availableWidth);
    
    // Panel management
    void ManageFixPanelLayout(const wxSize& barSize);
    void ManageFloatPanelLayout();
    
    // Layout constraints
    static constexpr int MIN_ELEMENT_WIDTH = 10;
    static constexpr int MIN_ELEMENT_HEIGHT = 10;
};

#endif // FLATUIBAR_LAYOUT_MANAGER_H 