#include "docking/DockContainerWidget.h"
#include "docking/DockArea.h"
#include "docking/DockWidget.h"
#include "docking/FloatingDockContainer.h"
#include <algorithm>

namespace ads {

// Define custom events
wxEventTypeTag<wxCommandEvent> DockContainerWidget::EVT_DOCK_AREAS_ADDED(wxNewEventType());
wxEventTypeTag<wxCommandEvent> DockContainerWidget::EVT_DOCK_AREAS_REMOVED(wxNewEventType());

// Event table
wxBEGIN_EVENT_TABLE(DockContainerWidget, wxPanel)
    EVT_SIZE(DockContainerWidget::onSize)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(DockSplitter, wxSplitterWindow)
    EVT_SPLITTER_SASH_POS_CHANGING(wxID_ANY, DockSplitter::OnSplitterSashPosChanging)
    EVT_SPLITTER_SASH_POS_CHANGED(wxID_ANY, DockSplitter::OnSplitterSashPosChanged)
wxEND_EVENT_TABLE()

// Private implementation
class DockContainerWidget::Private {
public:
    Private(DockContainerWidget* parent) : q(parent) {}
    
    DockContainerWidget* q;
    DockSplitter* rootSplitter = nullptr;
    wxWindow* centralWidget = nullptr;
};

// DockContainerWidget implementation
DockContainerWidget::DockContainerWidget(DockManager* dockManager, wxWindow* parent)
    : wxPanel(parent)
    , d(std::make_unique<Private>(this))
    , m_dockManager(dockManager)
    , m_floatingWidget(nullptr)
    , m_lastAddedArea(nullptr)
{
    // Create layout
    m_layout = new wxBoxSizer(wxVERTICAL);
    SetSizer(m_layout);
    
    // Create root splitter
    m_rootSplitter = new DockSplitter(this);
    m_layout->Add(m_rootSplitter, 1, wxEXPAND);
}

DockContainerWidget::~DockContainerWidget() {
    // Clear all dock areas first to ensure proper cleanup order
    m_dockAreas.clear();
    
    // The splitter and other child windows will be destroyed automatically
    // by wxWidgets parent-child mechanism
}

DockArea* DockContainerWidget::addDockWidget(DockWidgetArea area, DockWidget* dockWidget, 
                                            DockArea* targetDockArea, int index) {
    wxLogDebug("DockContainerWidget::addDockWidget - area: %d, widget: %p, targetArea: %p", 
        area, dockWidget, targetDockArea);
        
    if (!dockWidget) {
        wxLogDebug("  -> dockWidget is null");
        return nullptr;
    }
    
    // If we have a target dock area, add to it
    if (targetDockArea) {
        wxLogDebug("  -> Adding to existing target area");
        targetDockArea->addDockWidget(dockWidget);
        return targetDockArea;
    }
    
    // Create new dock area
    wxLogDebug("  -> Creating new dock area");
    DockArea* newDockArea = new DockArea(m_dockManager, this);
    newDockArea->addDockWidget(dockWidget);
    
    // Add dock area to container
    addDockArea(newDockArea, area);
    
    m_lastAddedArea = newDockArea;
    
    return newDockArea;
}

void DockContainerWidget::removeDockArea(DockArea* area) {
    if (!area) {
        return;
    }
    
    // Remove from list
    auto it = std::find(m_dockAreas.begin(), m_dockAreas.end(), area);
    if (it != m_dockAreas.end()) {
        m_dockAreas.erase(it);
    }
    
    // Remove from splitter
    if (area->GetParent() == m_rootSplitter) {
        if (wxSizer* sizer = m_rootSplitter->GetSizer()) {
            sizer->Detach(area);
        }
    }
    
    // Update layout
    Layout();
    
    // Notify
    wxCommandEvent event(EVT_DOCK_AREAS_REMOVED);
    event.SetEventObject(this);
    ProcessWindowEvent(event);
}

void DockContainerWidget::removeDockWidget(DockWidget* widget) {
    if (!widget || !widget->dockAreaWidget()) {
        return;
    }
    
    widget->dockAreaWidget()->removeDockWidget(widget);
}

DockArea* DockContainerWidget::dockAreaAt(const wxPoint& globalPos) const {
    wxPoint localPos = ScreenToClient(globalPos);
    
    for (auto* area : m_dockAreas) {
        wxRect rect = area->GetRect();
        if (rect.Contains(localPos)) {
            return area;
        }
    }
    
    return nullptr;
}

DockArea* DockContainerWidget::dockArea(int index) const {
    if (index >= 0 && index < static_cast<int>(m_dockAreas.size())) {
        return m_dockAreas[index];
    }
    return nullptr;
}

void DockContainerWidget::addDockArea(DockArea* dockArea, DockWidgetArea area) {
    wxLogDebug("DockContainerWidget::addDockArea - area: %d, dockArea: %p", area, dockArea);
    
    if (!dockArea) {
        wxLogDebug("  -> dockArea is null");
        return;
    }
    
    // Add to list
    m_dockAreas.push_back(dockArea);
    wxLogDebug("  -> Added to list, now have %d areas", (int)m_dockAreas.size());
    
    // For now, just add to the root splitter
    if (DockSplitter* splitter = dynamic_cast<DockSplitter*>(m_rootSplitter)) {
        wxLogDebug("  -> Got root splitter: %p", splitter);
        if (splitter->GetWindow1() == nullptr) {
            wxLogDebug("  -> Splitter window1 is null, initializing with dock area");
            // Reparent the dock area to the splitter before initializing
            dockArea->Reparent(splitter);
            splitter->Initialize(dockArea);
        } else if (splitter->GetWindow2() == nullptr) {
            wxLogDebug("  -> Splitter window1 exists, splitting");
            // Reparent the dock area to the splitter before splitting
            dockArea->Reparent(splitter);
            // Split based on area
            if (area == LeftDockWidgetArea || area == RightDockWidgetArea) {
                wxLogDebug("    -> Splitting vertically");
                splitter->SplitVertically(splitter->GetWindow1(), dockArea);
            } else {
                wxLogDebug("    -> Splitting horizontally");
                splitter->SplitHorizontally(splitter->GetWindow1(), dockArea);
            }
        } else {
            // Need to create sub-splitter
            DockSplitter* newSplitter = new DockSplitter(splitter);  // Create with parent splitter
            wxWindow* oldWindow = splitter->GetWindow2();
            
            // Reparent the old window to the new splitter
            oldWindow->Reparent(newSplitter);
            
            // Replace the window in the parent splitter
            splitter->ReplaceWindow(oldWindow, newSplitter);
            
            // Reparent the dock area to the new splitter
            dockArea->Reparent(newSplitter);
            
            if (area == LeftDockWidgetArea || area == RightDockWidgetArea) {
                newSplitter->SplitVertically(oldWindow, dockArea);
            } else {
                newSplitter->SplitHorizontally(oldWindow, dockArea);
            }
        }
    }
    
    // Notify
    wxCommandEvent event(EVT_DOCK_AREAS_ADDED);
    event.SetEventObject(this);
    ProcessWindowEvent(event);
}

void DockContainerWidget::splitDockArea(DockArea* dockArea, DockArea* newDockArea, 
                                       DockWidgetArea area, int splitRatio) {
    // TODO: Implement dock area splitting
    addDockArea(newDockArea, area);
}

void DockContainerWidget::setFloatingWidget(FloatingDockContainer* floatingWidget) {
    m_floatingWidget = floatingWidget;
}

void DockContainerWidget::saveState(wxString& xmlData) const {
    // TODO: Implement state saving
    xmlData = "<DockContainer />";
}

bool DockContainerWidget::restoreState(const wxString& xmlData) {
    // TODO: Implement state restoration
    return true;
}

bool DockContainerWidget::isInFrontOf(DockContainerWidget* other) const {
    // TODO: Implement z-order checking
    return false;
}

void DockContainerWidget::dumpLayout() const {
    // TODO: Implement layout debugging
}

DockManagerFeatures DockContainerWidget::features() const {
    return m_dockManager ? m_dockManager->configFlags() : DefaultConfig;
}

void DockContainerWidget::raiseAndActivate() {
    Raise();
    SetFocus();
}

void DockContainerWidget::updateSplitterHandles(wxWindow* splitter) {
    // TODO: Update splitter appearance
}

wxWindow* DockContainerWidget::createSplitter(wxOrientation orientation) {
    DockSplitter* splitter = new DockSplitter(this);
    splitter->setOrientation(orientation);
    return splitter;
}

void DockContainerWidget::adjustSplitterSizes(wxWindow* splitter, int availableSize) {
    // TODO: Adjust splitter sizes
}

DockArea* DockContainerWidget::getDockAreaBySplitterChild(wxWindow* child) const {
    for (auto* area : m_dockAreas) {
        if (area == child) {
            return area;
        }
    }
    return nullptr;
}

void DockContainerWidget::onSize(wxSizeEvent& event) {
    event.Skip();
}

void DockContainerWidget::onDockAreaDestroyed(wxWindowDestroyEvent& event) {
    // Handle dock area destruction
}

void DockContainerWidget::dropFloatingWidget(FloatingDockContainer* floatingWidget, const wxPoint& targetPos) {
    // TODO: Implement floating widget dropping
}

void DockContainerWidget::dropDockArea(DockArea* dockArea, DockWidgetArea area) {
    addDockArea(dockArea, area);
}

void DockContainerWidget::addDockAreaToContainer(DockWidgetArea area, DockArea* dockArea) {
    addDockArea(dockArea, area);
}

DockSplitter* DockContainerWidget::newSplitter(wxOrientation orientation) {
    return new DockSplitter(this);
}

// DockSplitter implementation
DockSplitter::DockSplitter(wxWindow* parent)
    : wxSplitterWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 
                      wxSP_3D | wxSP_LIVE_UPDATE)
    , m_orientation(wxHORIZONTAL)
{
    SetSashGravity(0.5);
    SetMinimumPaneSize(50);
}

DockSplitter::~DockSplitter() {
}

void DockSplitter::setOrientation(wxOrientation orientation) {
    m_orientation = orientation;
}

void DockSplitter::insertWidget(int index, wxWindow* widget, bool stretch) {
    if (index < 0 || index > widgetCount()) {
        addWidget(widget, stretch);
        return;
    }
    
    // Ensure widget has this splitter as parent
    if (widget->GetParent() != this) {
        widget->Reparent(this);
    }
    
    m_widgets.insert(m_widgets.begin() + index, widget);
    updateSplitter();
}

void DockSplitter::addWidget(wxWindow* widget, bool stretch) {
    // Ensure widget has this splitter as parent
    if (widget->GetParent() != this) {
        widget->Reparent(this);
    }
    
    m_widgets.push_back(widget);
    updateSplitter();
}

wxWindow* DockSplitter::replaceWidget(wxWindow* from, wxWindow* to) {
    // Ensure 'to' widget has this splitter as parent
    if (to && to->GetParent() != this) {
        to->Reparent(this);
    }
    
    ReplaceWindow(from, to);
    
    auto it = std::find(m_widgets.begin(), m_widgets.end(), from);
    if (it != m_widgets.end()) {
        *it = to;
    }
    
    return from;
}

wxWindow* DockSplitter::widget(int index) const {
    if (index >= 0 && index < static_cast<int>(m_widgets.size())) {
        return m_widgets[index];
    }
    return nullptr;
}

int DockSplitter::indexOf(wxWindow* widget) const {
    auto it = std::find(m_widgets.begin(), m_widgets.end(), widget);
    if (it != m_widgets.end()) {
        return std::distance(m_widgets.begin(), it);
    }
    return -1;
}

int DockSplitter::widgetCount() const {
    return m_widgets.size();
}

bool DockSplitter::hasVisibleContent() const {
    for (auto* widget : m_widgets) {
        if (widget && widget->IsShown()) {
            return true;
        }
    }
    return false;
}

void DockSplitter::setSizes(const std::vector<int>& sizes) {
    m_sizes = sizes;
    
    if (IsSplit() && !sizes.empty()) {
        SetSashPosition(sizes[0]);
    }
}

std::vector<int> DockSplitter::sizes() const {
    std::vector<int> result;
    
    if (IsSplit()) {
        result.push_back(GetSashPosition());
        result.push_back(GetSize().GetWidth() - GetSashPosition() - GetSashSize());
    }
    
    return result;
}

void DockSplitter::OnSplitterSashPosChanging(wxSplitterEvent& event) {
    // Allow sash position changes
}

void DockSplitter::OnSplitterSashPosChanged(wxSplitterEvent& event) {
    // Update sizes
    m_sizes = sizes();
}

void DockSplitter::updateSplitter() {
    if (m_widgets.size() >= 2 && !IsSplit()) {
        // Ensure both widgets have this splitter as parent
        if (m_widgets[0]->GetParent() != this) {
            m_widgets[0]->Reparent(this);
        }
        if (m_widgets[1]->GetParent() != this) {
            m_widgets[1]->Reparent(this);
        }
        
        if (m_orientation == wxVERTICAL) {
            SplitVertically(m_widgets[0], m_widgets[1]);
        } else {
            SplitHorizontally(m_widgets[0], m_widgets[1]);
        }
    }
}

void DockContainerWidget::dropDockWidget(DockWidget* widget, DockWidgetArea dropArea, DockArea* targetArea) {
    if (!widget || !targetArea) {
        return;
    }
    
    // Create new dock area for the widget
    DockArea* newArea = new DockArea(m_dockManager, this);
    newArea->addDockWidget(widget);
    
    // Add the new area relative to target area
    addDockAreaToContainer(dropArea, newArea);
}

} // namespace ads
