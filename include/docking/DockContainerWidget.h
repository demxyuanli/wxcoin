#pragma once

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/splitter.h>
#include <vector>
#include <memory>
#include "DockManager.h"
#include "DockSplitter.h"

namespace ads {
// Forward declarations
class DockArea;
class DockWidget;
class FloatingDockContainer;

/**
 * @brief Container widget that manages the layout of dock areas using splitters
 */
class DockContainerWidget : public wxPanel {
public:
    DockContainerWidget(DockManager* dockManager, wxWindow* parent);
    virtual ~DockContainerWidget();
    
    // Manager access  
    DockManager* dockManager() const { return m_dockManager; }

    // Area management
    DockArea* addDockWidget(DockWidgetArea area, DockWidget* dockWidget, 
                           DockArea* targetDockArea = nullptr, int index = -1);
    void removeDockArea(DockArea* area);
    void removeDockWidget(DockWidget* widget);
    DockArea* dockAreaAt(const wxPoint& globalPos) const;
    DockArea* dockArea(int index) const;
    std::vector<DockArea*> dockAreas() const { return m_dockAreas; }
    int dockAreaCount() const { return static_cast<int>(m_dockAreas.size()); }
    DockWidgetArea dockAreaOf(DockArea* area) const;
    DockArea* findAdjacentDockArea(DockArea* area) const;
    
    // Splitter management
    void addDockArea(DockArea* dockArea, DockWidgetArea area = CenterDockWidgetArea);
    void addDockAreaRelativeTo(DockArea* newArea, DockWidgetArea area, DockArea* targetArea);
    void splitDockArea(DockArea* dockArea, DockArea* newDockArea, 
                      DockWidgetArea area, int splitRatio = 50);
    
    // Floating widgets
    FloatingDockContainer* floatingWidget() const { return m_floatingWidget; }
    void setFloatingWidget(FloatingDockContainer* floatingWidget);
    
    // Layout
    void saveState(wxString& xmlData) const;
    bool restoreState(const wxString& xmlData);
    wxWindow* rootSplitter() const { return m_rootSplitter; }
    
    // Visibility
    bool isInFrontOf(DockContainerWidget* other) const;
    void dumpLayout() const;
    
    // Features
    DockManagerFeatures features() const;
    
    // Raise and activate
    void raiseAndActivate();
    
    // Last added area
    DockArea* lastAddedDockArea() const { return m_lastAddedArea; }
    // Resize state
    bool isResizeInProgress() const { return m_isResizing; }
    
    // Theme support
    void RefreshTheme();

    // Layout configuration
    void applyLayoutConfig();
    void applyProportionalResize(const wxSize& oldSize, const wxSize& newSize);
    void applyFixedSizeDocks();
    void cacheSplitterRatios();
    void restoreSplitterRatios();
    void markUserAdjustedLayout();
    
    // Helper for collecting splitter ratios
    void collectSplitterRatios(wxWindow* window);
    
    // New layout calculation based on fixed docks
    int calculateAreaSizeBasedOnFixedDocks(DockWidgetArea area, 
                                          const wxSize& containerSize, 
                                          const DockLayoutConfig& config) const;
    
    // Helper methods to identify splitters that control fixed docks
    bool isLeftDockSplitter(DockSplitter* splitter) const;
    bool isBottomDockSplitter(DockSplitter* splitter) const;
    
    // Public access for layout algorithms
    int getConfiguredAreaSize(DockWidgetArea area) const;

    // Global docking support
    void enableGlobalDockingMode(bool enable);
    bool isGlobalDockingEnabled() const;
    void handleGlobalDockDrop(DockWidget* widget, DockWidgetArea area);
    void updateGlobalDockingHints();
    
    // Events
    static wxEventTypeTag<wxCommandEvent> EVT_DOCK_AREAS_ADDED;
    static wxEventTypeTag<wxCommandEvent> EVT_DOCK_AREAS_REMOVED;
    
protected:
    // Internal layout management
    void updateSplitterHandles(wxWindow* splitter);
    wxWindow* createSplitter(wxOrientation orientation);
    void adjustSplitterSizes(wxWindow* splitter, int availableSize);
    DockArea* getDockAreaBySplitterChild(wxWindow* child) const;
    
    // Event handlers
    void onSize(wxSizeEvent& event);
    void onDockAreaDestroyed(wxWindowDestroyEvent& event);
    void onResizeTimer(wxTimerEvent& event);
    void onLayoutUpdateTimer(wxTimerEvent& event);
    
protected:
    // Protected members for derived classes
    std::vector<DockArea*> m_dockAreas;
    wxWindow* m_rootSplitter;
    std::unique_ptr<DockLayoutConfig> m_layoutConfig;
    
private:
    // Private implementation details
    class Private;
    std::unique_ptr<Private> d;
    
    // Member variables
    DockManager* m_dockManager;
    wxBoxSizer* m_layout;
    FloatingDockContainer* m_floatingWidget;
    DockArea* m_lastAddedArea;
    wxTimer* m_resizeTimer;
    wxTimer* m_layoutUpdateTimer;
    
    // Proportional resize support
    struct SplitterRatio {
        wxWindow* splitter;
        double ratio; // 0.0 to 1.0
        bool isValid;
    };
    std::vector<SplitterRatio> m_splitterRatios;
    wxSize m_lastContainerSize;
    bool m_hasUserAdjustedLayout;
    // Resize coalescing
    bool m_isResizeFreezeActive;
    bool m_isResizing;
    
    // Helper methods
    void dropFloatingWidget(FloatingDockContainer* floatingWidget, const wxPoint& targetPos);
    void dropDockArea(DockArea* dockArea, DockWidgetArea area);
    void addDockAreaToContainer(DockWidgetArea area, DockArea* dockArea);
    void dropDockWidget(DockWidget* widget, DockWidgetArea dropArea, DockArea* targetArea);
    DockSplitter* newSplitter(wxOrientation orientation);
    void addDockAreaToSplitter(DockSplitter* splitter, DockArea* dockArea, DockWidgetArea area);

    // Global docking helper methods
    void saveCurrentLayoutState();
    void restoreLayoutState();
    std::unique_ptr<DockLayoutConfig> m_savedLayoutConfig;
    
    // New helper methods for proper 5-zone layout
    void handleTopBottomArea(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area);
    void handleMiddleLayerArea(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area);
    wxWindow* findOrCreateMiddleLayer(DockSplitter* rootSplitter);
    void addDockAreaToMiddleSplitter(DockSplitter* middleSplitter, DockArea* dockArea, DockWidgetArea area);
    
    // Simplified layout methods
    void addDockAreaSimple(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area);
    void ensureTopBottomLayout(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area);
    void addToMiddleLayer(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area);
    void addToVerticalSplitter(DockSplitter* splitter, DockArea* dockArea, DockWidgetArea area);
    void createMiddleSplitter(DockSplitter* rootSplitter, DockArea* existingArea, DockArea* newArea, DockWidgetArea area);
    void create3WaySplit(DockSplitter* splitter, DockArea* dockArea, DockWidgetArea area);
    void addToHorizontalLayout(DockSplitter* splitter, DockArea* dockArea, DockWidgetArea area);
    void restructureForTopBottom(DockSplitter* rootSplitter, DockArea* dockArea, DockWidgetArea area);
    void ensureAllChildrenVisible(wxWindow* window);
    
    friend class DockManager;
    friend class DockArea;
    friend class FloatingDockContainer;
    friend class DockWidget;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace ads
