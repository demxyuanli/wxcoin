#include "docking/DockWidget.h"
#include "docking/DockManager.h"
#include "docking/DockArea.h"
#include "docking/FloatingDockContainer.h"
#include "docking/DockContainerWidget.h"
#include <wx/scrolwin.h>

namespace ads {

// Define custom events - static member initialization
wxEventTypeTag<wxCommandEvent> DockWidget::EVT_DOCK_WIDGET_CLOSED(wxNewEventType());
wxEventTypeTag<wxCommandEvent> DockWidget::EVT_DOCK_WIDGET_CLOSING(wxNewEventType());
wxEventTypeTag<wxCommandEvent> DockWidget::EVT_DOCK_WIDGET_VISIBILITY_CHANGED(wxNewEventType());
wxEventTypeTag<wxCommandEvent> DockWidget::EVT_DOCK_WIDGET_FEATURES_CHANGED(wxNewEventType());

// Event table
wxBEGIN_EVENT_TABLE(DockWidget, wxPanel)
    EVT_CLOSE(DockWidget::onCloseEvent)
wxEND_EVENT_TABLE()

// Private implementation
class DockWidget::Private {
public:
    Private(DockWidget* parent) : q(parent) {}
    
    DockWidget* q;
    bool isClosing = false;
    bool isFloatingTopLevel = false;
};

// Constructor
DockWidget::DockWidget(const wxString& title, wxWindow* parent)
    : wxPanel(parent)
    , d(std::make_unique<Private>(this))
    , m_dockManager(nullptr)
    , m_dockArea(nullptr)
    , m_tabWidget(nullptr)
    , m_widget(nullptr)
    , m_titleBarWidget(nullptr)
    , m_toggleViewAction(nullptr)
    , m_features(DefaultDockWidgetFeatures)
    , m_minimumSizeHintMode(MinimumSizeHintFromDockWidget)
    , m_title(title)
    , m_closed(false)
    , m_tabIndex(-1)
    , m_toggleViewActionMode(ActionModeToggle)
    , m_userData(nullptr)
{
    SetName(title);
    
    // Create layout
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(sizer);
    
    // Create toggle view action
    m_toggleViewAction = new wxMenuItem(nullptr, wxID_ANY, title, "", wxITEM_CHECK);
}

DockWidget::~DockWidget() {
    delete m_toggleViewAction;
    
    // Notify manager
    if (m_dockManager) {
        m_dockManager->unregisterDockWidget(this);
    }
}

void DockWidget::setWidget(wxWindow* widget, InsertMode insertMode) {
    if (m_widget) {
        GetSizer()->Detach(m_widget);
    }
    
    m_widget = widget;
    
    if (widget) {
        wxWindow* contentToAdd = widget;
        
        // Handle scroll area mode
        if (insertMode == AutoScrollArea || insertMode == ForceScrollArea) {
            bool needsScrollArea = false;
            
            if (insertMode == ForceScrollArea) {
                needsScrollArea = true;
            } else {
                // Auto mode - check if widget needs scroll area
                wxSize minSize = widget->GetMinSize();
                needsScrollArea = (minSize.GetWidth() > 200 || minSize.GetHeight() > 200);
            }
            
            if (needsScrollArea) {
                wxScrolledWindow* scrollArea = new wxScrolledWindow(this);
                scrollArea->SetScrollRate(10, 10);
                
                wxBoxSizer* scrollSizer = new wxBoxSizer(wxVERTICAL);
                scrollSizer->Add(widget, 1, wxEXPAND);
                scrollArea->SetSizer(scrollSizer);
                
                contentToAdd = scrollArea;
            }
        }
        
        GetSizer()->Add(contentToAdd, 1, wxEXPAND);
        widget->Reparent(this);
    }
    
    Layout();
}

wxWindow* DockWidget::takeWidget() {
    wxWindow* w = m_widget;
    setWidget(nullptr);
    return w;
}

void DockWidget::setTitleBarWidget(wxWindow* widget) {
    if (m_titleBarWidget) {
        GetSizer()->Detach(m_titleBarWidget);
        m_titleBarWidget->Destroy();
    }
    
    m_titleBarWidget = widget;
    
    if (widget) {
        GetSizer()->Insert(0, widget, 0, wxEXPAND);
        widget->Reparent(this);
    }
    
    Layout();
}

void DockWidget::setFeatures(DockWidgetFeatures features) {
    if (m_features == features) {
        return;
    }
    
    m_features = features;
    
    // Notify change
    wxCommandEvent event(EVT_DOCK_WIDGET_FEATURES_CHANGED);
    event.SetEventObject(this);
    ProcessWindowEvent(event);
}

void DockWidget::setFeature(DockWidgetFeature flag, bool on) {
    DockWidgetFeatures features = m_features;
    
    if (on) {
        features |= flag;
    } else {
        features &= ~flag;
    }
    
    setFeatures(features);
}

bool DockWidget::hasFeature(DockWidgetFeature flag) const {
    return (m_features & flag) == flag;
}

DockContainerWidget* DockWidget::dockContainer() const {
    if (m_dockArea) {
        return m_dockArea->dockContainer();
    }
    return nullptr;
}

FloatingDockContainer* DockWidget::floatingDockContainer() const {
    DockContainerWidget* container = dockContainer();
    if (container) {
        return container->floatingWidget();
    }
    return nullptr;
}

bool DockWidget::isFloating() const {
    return floatingDockContainer() != nullptr;
}

bool DockWidget::isInFloatingContainer() const {
    FloatingDockContainer* floatingContainer = floatingDockContainer();
    if (!floatingContainer) {
        return false;
    }
    
    // Check if we're the only widget in the floating container
    return floatingContainer->hasTopLevelDockWidget() && 
           floatingContainer->topLevelDockWidget() == this;
}

bool DockWidget::isClosed() const {
    return m_closed;
}

bool DockWidget::isVisible() const {
    return !m_closed && IsShown();
}

void DockWidget::toggleView(bool open) {
    if (open == !m_closed) {
        return;
    }
    
    toggleViewInternal(open);
}

void DockWidget::toggleViewInternal(bool open) {
    if (!m_dockManager) {
        return;
    }
    
    if (!open && !hasFeature(DockWidgetClosable)) {
        return;
    }
    
    m_closed = !open;
    
    if (open) {
        // Show the widget
        if (m_dockArea) {
            m_dockArea->toggleView(true);
            m_dockArea->setCurrentDockWidget(this);
            Show();
        } else {
            // Create new dock area
            m_dockManager->addDockWidget(CenterDockWidgetArea, this);
        }
    } else {
        // Hide the widget
        Hide();
        
        if (m_dockArea && m_dockArea->dockWidgetsCount() == 1) {
            m_dockArea->toggleView(false);
        }
    }
    
    // Update toggle action
    if (m_toggleViewAction) {
        m_toggleViewAction->Check(open);
    }
    
    // Notify change
    wxCommandEvent event(EVT_DOCK_WIDGET_VISIBILITY_CHANGED);
    event.SetEventObject(this);
    ProcessWindowEvent(event);
}

void DockWidget::setMinimumSizeHintMode(MinimumSizeHintMode mode) {
    m_minimumSizeHintMode = mode;
}

void DockWidget::setIcon(const wxBitmap& icon) {
    m_icon = icon;
    
    // Update tab if exists
    if (m_tabWidget) {
        // TODO: Update tab icon
    }
}

void DockWidget::setTitle(const wxString& title) {
    if (m_title == title) {
        return;
    }
    
    m_title = title;
    SetName(title);
    
    // Update toggle action
    if (m_toggleViewAction) {
        m_toggleViewAction->SetItemLabel(title);
    }
    
    // Update tab if exists
    if (m_tabWidget) {
        // TODO: Update tab title
    }
    
    // Update floating container title if floating
    FloatingDockContainer* floatingContainer = floatingDockContainer();
    if (floatingContainer) {
        floatingContainer->updateWindowTitle();
    }
}

int DockWidget::tabIndex() const {
    if (m_dockArea) {
        return m_dockArea->indexOfDockWidget(const_cast<DockWidget*>(this));
    }
    return -1;
}

void DockWidget::setTabIndex(int index) {
    m_tabIndex = index;
}

wxMenuItem* DockWidget::toggleViewAction() const {
    return m_toggleViewAction;
}

void DockWidget::setToggleViewActionMode(ToggleViewActionMode mode) {
    m_toggleViewActionMode = mode;
}

void DockWidget::setCloseHandler(std::function<bool()> handler) {
    m_closeHandler = handler;
}

void DockWidget::closeDockWidget() {
    closeDockWidgetInternal();
}

bool DockWidget::closeDockWidgetInternal(bool force) {
    if (!force && !hasFeature(DockWidgetClosable)) {
        return false;
    }
    
    if (!force && hasFeature(CustomCloseHandling) && m_closeHandler) {
        if (!m_closeHandler()) {
            return false;
        }
    }
    
    // Notify closing
    wxCommandEvent closingEvent(EVT_DOCK_WIDGET_CLOSING);
    closingEvent.SetEventObject(this);
    ProcessWindowEvent(closingEvent);
    
    if (!force && closingEvent.GetSkipped()) {
        return false;
    }
    
    d->isClosing = true;
    
    // Close the widget
    toggleViewInternal(false);
    
    // Notify closed
    wxCommandEvent closedEvent(EVT_DOCK_WIDGET_CLOSED);
    closedEvent.SetEventObject(this);
    ProcessWindowEvent(closedEvent);
    
    // Delete if needed
    if (hasFeature(DockWidgetDeleteOnClose)) {
        deleteDockWidget();
    }
    
    d->isClosing = false;
    
    return true;
}

void DockWidget::setFloating() {
    if (!m_dockManager || !hasFeature(DockWidgetFloatable)) {
        return;
    }
    
    if (isFloating()) {
        return;
    }
    
    // Remove from current area
    DockArea* oldArea = m_dockArea;
    if (oldArea) {
        oldArea->removeDockWidget(this);
    }
    
    // Create floating container
    m_dockManager->addDockWidgetFloating(this);
}

void DockWidget::deleteDockWidget() {
    if (m_dockManager) {
        m_dockManager->removeDockWidget(this);
    }
    
    delete this;
}

void DockWidget::setAsCurrentTab() {
    if (m_dockArea && !m_closed) {
        m_dockArea->setCurrentDockWidget(this);
    }
}

bool DockWidget::isCurrentTab() const {
    if (m_dockArea) {
        return m_dockArea->currentDockWidget() == this;
    }
    return false;
}

void DockWidget::raise() {
    if (isInFloatingContainer()) {
        FloatingDockContainer* floatingContainer = floatingDockContainer();
        if (floatingContainer) {
            floatingContainer->Raise();
            floatingContainer->SetFocus();
        }
    } else {
        setAsCurrentTab();
        SetFocus();
    }
}

bool DockWidget::isAutoHide() const {
    if (!m_dockManager) {
        return false;
    }
    
    // Check if this widget is in auto-hide mode
    // This would be tracked by the dock manager
    return m_dockManager->isAutoHide(const_cast<DockWidget*>(this));
}

void DockWidget::setAutoHide(bool enable) {
    if (!m_dockManager || !hasFeature(DockWidgetFloatable)) {
        return;
    }
    
    if (enable == isAutoHide()) {
        return;
    }
    
    if (enable) {
        // Move to auto-hide
        m_dockManager->setAutoHide(this, LeftDockWidgetArea); // Default to left
    } else {
        // Restore from auto-hide
        m_dockManager->restoreFromAutoHide(this);
    }
}

int DockWidget::autoHidePriority() const {
    // Priority for auto-hide tab ordering
    return 0;
}

void DockWidget::setDockManager(DockManager* dockManager) {
    m_dockManager = dockManager;
}

void DockWidget::setDockArea(DockArea* dockArea) {
    m_dockArea = dockArea;
}

void DockWidget::setTabWidget(DockWidgetTab* tabWidget) {
    m_tabWidget = tabWidget;
}

void DockWidget::setToggleViewActionChecked(bool checked) {
    if (m_toggleViewAction) {
        m_toggleViewAction->Check(checked);
    }
}

void DockWidget::setClosedState(bool closed) {
    m_closed = closed;
}

void DockWidget::emitTopLevelChanged(bool floating) {
    d->isFloatingTopLevel = floating;
}

void DockWidget::setTopLevelWidget(wxWindow* widget) {
    // TODO: Implement if needed
}

void DockWidget::flagAsUnassigned() {
    m_closed = true;
    SetParent(nullptr);
}

void DockWidget::saveState(wxString& xmlData) const {
    // TODO: Implement state saving
    xmlData = wxString::Format("<DockWidget name=\"%s\" closed=\"%d\" />", 
                              m_objectName, m_closed ? 1 : 0);
}

bool DockWidget::restoreState(const wxString& xmlData) {
    // TODO: Implement state restoration
    return true;
}

void DockWidget::onCloseEvent(wxCloseEvent& event) {
    if (!closeDockWidgetInternal()) {
        event.Veto();
    }
}

void DockWidget::onToggleViewActionTriggered(wxCommandEvent& event) {
    toggleView(event.IsChecked());
}

} // namespace ads