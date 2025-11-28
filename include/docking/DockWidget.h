#pragma once

#include <wx/wx.h>
#include <wx/panel.h>
#include <memory>
#include <functional>

namespace ads {
	// Forward declarations
	class DockManager;
	class DockArea;
	class DockWidgetTab;
	class FloatingDockContainer;
	class DockContainerWidget;
	
	// Forward declaration of DockWidgetArea enum
	enum DockWidgetArea;


// Dock widget features
enum DockWidgetFeature {
    DockWidgetClosable = 0x01,
    DockWidgetMovable = 0x02,
    DockWidgetFloatable = 0x04,
    DockWidgetDeleteOnClose = 0x08,
    CustomCloseHandling = 0x10,
    DockWidgetFocusable = 0x20,
    DockWidgetForceCloseWithArea = 0x40,
    NoTab = 0x80,
    DeleteContentOnClose = 0x100,
    DockWidgetPositionLocked = 0x200,  // Position locked - cannot be moved
    DockWidgetPinned = 0x400,           // Pinned - tab is pinned to its position

    DefaultDockWidgetFeatures = DockWidgetClosable | DockWidgetMovable | DockWidgetFloatable | DockWidgetFocusable,
    AllDockWidgetFeatures = DefaultDockWidgetFeatures | DockWidgetDeleteOnClose | CustomCloseHandling,
    DockWidgetAlwaysCloseAndDelete = DockWidgetForceCloseWithArea | DockWidgetDeleteOnClose,
    NoDockWidgetFeatures = 0x00
};

// Dock widget orientation preferences
enum DockWidgetOrientation {
    OrientationAuto,      // Auto-detect based on position
    OrientationHorizontal, // Prefer horizontal layout
    OrientationVertical    // Prefer vertical layout
};

typedef int DockWidgetFeatures;

// Dock widget states
enum DockWidgetState {
    StateHidden,
    StateDocked,
    StateFloating
};

// Toggle view action modes
enum ToggleViewActionMode {
    ActionModeToggle,
    ActionModeShow
};

// Minimum size hint modes
enum MinimumSizeHintMode {
    MinimumSizeHintFromDockWidget,
    MinimumSizeHintFromContent
};

/**
 * @brief Represents a dockable widget that can be docked, floated, or tabbed
 */
class DockWidget : public wxPanel {
public:
    // Insert modes for setWidget
    enum InsertMode {
        AutoScrollArea,
        ForceScrollArea,
        ForceNoScrollArea
    };

    DockWidget(const wxString& title, wxWindow* parent = nullptr);
    virtual ~DockWidget();

    // Core widget management
    void setWidget(wxWindow* widget, InsertMode insertMode = AutoScrollArea);
    wxWindow* takeWidget();
    wxWindow* widget() const { return m_widget; }
    
    // Tab widget access
    DockWidgetTab* tabWidget() const { return m_tabWidget; }
    
    // Title bar widget
    void setTitleBarWidget(wxWindow* widget);
    wxWindow* titleBarWidget() const { return m_titleBarWidget; }
    
    // Features
    void setFeatures(DockWidgetFeatures features);
    void setFeature(DockWidgetFeature flag, bool on);
    DockWidgetFeatures features() const { return m_features; }
    bool hasFeature(DockWidgetFeature flag) const;

    // Position locking
    void setPositionLocked(bool locked) { setFeature(DockWidgetPositionLocked, locked); }
    bool isPositionLocked() const { return hasFeature(DockWidgetPositionLocked); }

    // Pinning
    void setPinned(bool pinned) { setFeature(DockWidgetPinned, pinned); }
    bool isPinned() const { return hasFeature(DockWidgetPinned); }

    // Orientation preferences
    void setOrientation(DockWidgetOrientation orientation) { m_orientation = orientation; }
    DockWidgetOrientation orientation() const { return m_orientation; }
    
    // Manager access
    DockManager* dockManager() const { return m_dockManager; }
    DockContainerWidget* dockContainer() const;
    
    // Area access
    DockArea* dockAreaWidget() const { return m_dockArea; }
    
    // Floating container
    FloatingDockContainer* floatingDockContainer() const;
    bool isFloating() const;
    bool isInFloatingContainer() const;
    
    // Visibility and state
    bool isClosed() const;
    bool isVisible() const;
    void toggleView(bool open = true);
    void toggleViewInternal(bool open);
    
    // Size hints
    void setMinimumSizeHintMode(MinimumSizeHintMode mode);
    MinimumSizeHintMode minimumSizeHintMode() const { return m_minimumSizeHintMode; }
    
    // Icon and title
    void setIcon(const wxBitmap& icon);
    wxBitmap icon() const { return m_icon; }
    void setTitle(const wxString& title);
    wxString title() const { return m_title; }
    
    // Tab index
    int tabIndex() const;
    void setTabIndex(int index);
    
    // Actions
    wxMenuItem* toggleViewAction() const;
    void setToggleViewActionMode(ToggleViewActionMode mode);
    
    // Custom close handling
    void setCloseHandler(std::function<bool()> handler);
    
    // Events
    void closeDockWidget();
    bool closeDockWidgetInternal(bool force = false);
    
    // State
    void setFloating();
    void deleteDockWidget();
    void setAsCurrentTab();
    bool isCurrentTab() const;
    
    // Override Destroy for safe cleanup
    virtual bool Destroy() override;
    void raise();
    
    // Auto hide
    bool isAutoHide() const;
    void setAutoHide(bool enable);
    int autoHidePriority() const;
    
    // User data
    void setUserData(void* userData) { m_userData = userData; }
    void* userData() const { return m_userData; }
    
    // Object name (for saving/restoring state)
    void setObjectName(const wxString& name) { m_objectName = name; }
    wxString objectName() const { return m_objectName; }
    
    // Events - static members for event types
    static wxEventTypeTag<wxCommandEvent> EVT_DOCK_WIDGET_CLOSED;
    static wxEventTypeTag<wxCommandEvent> EVT_DOCK_WIDGET_CLOSING;
    static wxEventTypeTag<wxCommandEvent> EVT_DOCK_WIDGET_VISIBILITY_CHANGED;
    static wxEventTypeTag<wxCommandEvent> EVT_DOCK_WIDGET_FEATURES_CHANGED;

protected:
    // Internal methods
    void setDockManager(DockManager* dockManager);
    void setDockArea(DockArea* dockArea);
    void setTabWidget(DockWidgetTab* tabWidget);
    void setToggleViewActionChecked(bool checked);
    void setClosedState(bool closed);
    void emitTopLevelChanged(bool floating);
    void setTopLevelWidget(wxWindow* widget);
    void flagAsUnassigned();
    void saveState(wxString& xmlData) const;
    bool restoreState(const wxString& xmlData);
    
    // Event handlers
    void onCloseEvent(wxCloseEvent& event);
    void onToggleViewActionTriggered(wxCommandEvent& event);
    
private:
    // Private implementation
    class Private;
    std::unique_ptr<Private> d;
    
    // Member variables
    DockManager* m_dockManager;
    DockArea* m_dockArea;
    DockWidgetTab* m_tabWidget;
    wxWindow* m_widget;
    wxWindow* m_titleBarWidget;
    wxMenuItem* m_toggleViewAction;
    DockWidgetFeatures m_features;
    MinimumSizeHintMode m_minimumSizeHintMode;
    wxBitmap m_icon;
    wxString m_title;
    wxString m_objectName;
    bool m_closed;
    int m_tabIndex;
    ToggleViewActionMode m_toggleViewActionMode;
    std::function<bool()> m_closeHandler;
    void* m_userData;
    DockWidgetOrientation m_orientation;
    
    // Store original location for toggle restore
    DockWidgetArea m_savedArea;
    DockArea* m_savedTargetArea;
    
    friend class DockManager;
    friend class DockArea;
    friend class FloatingDockContainer;
    friend class DockContainerWidget;
    friend class DockWidgetTab;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace ads
