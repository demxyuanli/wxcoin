#pragma once

#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/aui/aui.h>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace ads {

	// Forward declarations
class DockWidget;
class DockArea;
class DockSplitter;
class FloatingDockContainer;
class DockOverlay;

// Dock widget area flags
enum DockWidgetArea {
    NoDockWidgetArea = 0x00,
    LeftDockWidgetArea = 0x01,
    RightDockWidgetArea = 0x02,
    TopDockWidgetArea = 0x04,
    BottomDockWidgetArea = 0x08,
    CenterDockWidgetArea = 0x10,
    
    InvalidDockWidgetArea = NoDockWidgetArea,
    OuterDockAreas = TopDockWidgetArea | LeftDockWidgetArea | RightDockWidgetArea | BottomDockWidgetArea,
    AllDockAreas = OuterDockAreas | CenterDockWidgetArea
};

// Dock manager features
enum DockManagerFeature {
    DefaultNonOpaqueConfig = 0x00,
    OpaqueSplitterResize = 0x01,
    XmlAutoFormattingEnabled = 0x02,
    AlwaysShowTabs = 0x04,
    AllTabsHaveCloseButton = 0x08,
    TabCloseButtonIsToolButton = 0x10,
    DockAreaHasCloseButton = 0x20,
    DockAreaCloseButtonClosesTab = 0x40,
    FocusHighlighting = 0x80,
    EqualSplitOnInsertion = 0x100,
    FloatingContainerForceNativeTitleBar = 0x200,
    
    DefaultConfig = DefaultNonOpaqueConfig | OpaqueSplitterResize | 
                   DockAreaHasCloseButton | FocusHighlighting
};

// Configuration flags
typedef int DockManagerFeatures;

/**
 * @brief The central dock manager that handles all dock widgets
 */
class DockManager : public wxEvtHandler {
public:
    DockManager(wxWindow* parent);
    virtual ~DockManager();

    // Core functionality
    DockArea* addDockWidget(DockWidgetArea area, DockWidget* dockWidget, 
                           DockArea* targetDockArea = nullptr);
    DockArea* addDockWidgetTab(DockWidgetArea area, DockWidget* dockWidget);
    DockArea* addDockWidgetTabToArea(DockWidget* dockWidget, DockArea* targetDockArea);
    
    // Dock widget management
    void removeDockWidget(DockWidget* dockWidget);
    std::vector<DockWidget*> dockWidgets() const;
    DockWidget* findDockWidget(const wxString& objectName) const;
    
    // Layout management
    void saveState(wxString& xmlData) const;
    bool restoreState(const wxString& xmlData);
    
    // Floating widgets
    FloatingDockContainer* addDockWidgetFloating(DockWidget* dockWidget);
    void setFloatingContainersTitle(const wxString& title);
    
    // Features and configuration
    void setConfigFlags(DockManagerFeatures features);
    DockManagerFeatures configFlags() const;
    void setConfigFlag(DockManagerFeature flag, bool on = true);
    bool testConfigFlag(DockManagerFeature flag) const;
    
    // Styling
    void setStyleSheet(const wxString& styleSheet);
    wxString styleSheet() const;
    
    // Events
    typedef std::function<void(DockWidget*)> DockWidgetCallback;
    void onDockWidgetAdded(const DockWidgetCallback& callback);
    void onDockWidgetRemoved(const DockWidgetCallback& callback);
    void onDockWidgetAboutToClose(const DockWidgetCallback& callback);
    
    // Container widget access
    wxWindow* containerWidget() const { return m_containerWidget; }
    
    // Active dock widget
    DockWidget* activeDockWidget() const { return m_activeDockWidget; }
    void setActiveDockWidget(DockWidget* widget);

    // Focus management
    DockWidget* focusedDockWidget() const;
    
    // Dock area management
    std::vector<DockArea*> dockAreas() const;
    std::vector<FloatingDockContainer*> floatingWidgets() const;
    
    // Overlay for drag and drop
    DockOverlay* dockOverlay() const { return m_dockOverlay; }
    
    // Central widget
    void setCentralWidget(wxWindow* widget);
    wxWindow* centralWidget() const { return m_centralWidget; }
    
    // Auto-hide functionality
    void setAutoHide(DockWidget* widget, DockWidgetArea area);
    void restoreFromAutoHide(DockWidget* widget);
    bool isAutoHide(DockWidget* widget) const;
    std::vector<DockWidget*> autoHideWidgets() const;
    
    // Perspective management
    class PerspectiveManager* perspectiveManager() const;
    
    // Overlay access for drag & drop
    DockOverlay* dockAreaOverlay() const;
    DockOverlay* containerOverlay() const;

    // Performance optimization methods
    void beginBatchOperation();
    void endBatchOperation();
    void updateLayout();
    void optimizeDragOperation(DockWidget* draggedWidget);
    void optimizeMemoryUsage();

protected:
    // Internal methods
    void registerDockWidget(DockWidget* dockWidget);
    void unregisterDockWidget(DockWidget* dockWidget);
    void registerDockArea(DockArea* dockArea);
    void unregisterDockArea(DockArea* dockArea);
    void registerFloatingWidget(FloatingDockContainer* floatingWidget);
    void unregisterFloatingWidget(FloatingDockContainer* floatingWidget);

    // Event handling
    void onDockAreaCreated(DockArea* dockArea);
    void onDockAreaAboutToClose(DockArea* dockArea);
    void onFloatingWidgetCreated(FloatingDockContainer* floatingWidget);
    void onFloatingWidgetAboutToClose(FloatingDockContainer* floatingWidget);

    // Performance optimization handlers
    void onLayoutUpdateTimer(wxTimerEvent& event);
    void updateDragTargets();
    void collectDropTargets(wxWindow* window);
    void cleanupUnusedResources();
    void initializePerformanceVariables();
    
private:
    // Private implementation
    class Private;
    std::unique_ptr<Private> d;
    
    // Member variables
    wxWindow* m_parent;
    wxWindow* m_containerWidget;
    wxWindow* m_centralWidget;
    DockOverlay* m_dockOverlay;
    DockWidget* m_activeDockWidget;
    DockManagerFeatures m_configFlags;
    wxString m_styleSheet;

    // Performance optimization variables
    wxTimer* m_layoutUpdateTimer;
    int m_batchOperationCount;
    bool m_isProcessingDrag;
    wxPoint m_lastMousePos;
    std::vector<wxWindow*> m_cachedDropTargets;

    // Drag state enumeration
    enum DragState {
        DragInactive,
        DragStarting,
        DragActive,
        DragEnding
    };
    DragState m_dragState;

    // Containers
    std::vector<DockWidget*> m_dockWidgets;
    std::vector<DockArea*> m_dockAreas;
    std::vector<FloatingDockContainer*> m_floatingWidgets;
    std::map<wxString, DockWidget*> m_dockWidgetsMap;

    // Callbacks
    std::vector<DockWidgetCallback> m_dockWidgetAddedCallbacks;
    std::vector<DockWidgetCallback> m_dockWidgetRemovedCallbacks;
    std::vector<DockWidgetCallback> m_dockWidgetAboutToCloseCallbacks;
    
    friend class DockWidget;
    friend class DockArea;
    friend class FloatingDockContainer;
    friend class DockContainerWidget;
};

} // namespace ads
