#include "docking/DockContainerWidget.h"
#include "docking/DockArea.h"
#include "docking/DockWidget.h"
#include "docking/FloatingDockContainer.h"
#include "docking/DockManager.h"
#include "docking/DockSplitter.h"
#include <algorithm>

namespace ads {

// Define custom events
wxEventTypeTag<wxCommandEvent> DockContainerWidget::EVT_DOCK_AREAS_ADDED(wxNewEventType());
wxEventTypeTag<wxCommandEvent> DockContainerWidget::EVT_DOCK_AREAS_REMOVED(wxNewEventType());

// Event table
wxBEGIN_EVENT_TABLE(DockContainerWidget, wxPanel)
    EVT_SIZE(DockContainerWidget::onSize)
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
    wxLogDebug("  -> Current dock areas count: %d", (int)m_dockAreas.size());
    
    if (!dockWidget) {
        wxLogDebug("  -> dockWidget is null");
        return nullptr;
    }
    
    // If we have a target dock area and want to dock in center, add as tab
    if (targetDockArea && area == CenterDockWidgetArea) {
        wxLogDebug("  -> Adding to existing target area as tab (center docking)");
        targetDockArea->addDockWidget(dockWidget);
        return targetDockArea;
    }
    
    // If we already have many areas, add as tab to avoid complex layouts
    if (m_dockAreas.size() >= 5) {
        wxLogDebug("  -> Too many areas (%d), adding as tab", (int)m_dockAreas.size());
        // Find the best area to add to
        DockArea* bestArea = nullptr;
        
        // Try to find an area in the same general position
        for (auto* existingArea : m_dockAreas) {
            if (existingArea && existingArea->GetParent()) {
                // For now, just use the first valid area
                bestArea = existingArea;
                break;
            }
        }
        
        if (bestArea) {
            bestArea->addDockWidget(dockWidget);
            return bestArea;
        }
    }
    
    // Create new dock area
    wxLogDebug("  -> Creating new dock area");
    DockArea* newDockArea = new DockArea(m_dockManager, this);
    newDockArea->addDockWidget(dockWidget);
    
    // If we have a target area (but not center docking), we need to split relative to it
    if (targetDockArea && area != CenterDockWidgetArea) {
        wxLogDebug("  -> Splitting relative to target area");
        addDockAreaRelativeTo(newDockArea, area, targetDockArea);
    } else {
        // Add dock area to container (general case)
        addDockArea(newDockArea, area);
    }
    
    m_lastAddedArea = newDockArea;
    
    return newDockArea;
}

void DockContainerWidget::removeDockArea(DockArea* area) {
    if (!area) {
        return;
    }
    
    wxLogDebug("DockContainerWidget::removeDockArea - removing area %p", area);
    
    // Remove from list
    auto it = std::find(m_dockAreas.begin(), m_dockAreas.end(), area);
    if (it != m_dockAreas.end()) {
        m_dockAreas.erase(it);
    }
    
    // Handle splitter removal properly
    wxWindow* parent = area->GetParent();
    if (DockSplitter* splitter = dynamic_cast<DockSplitter*>(parent)) {
        wxLogDebug("  -> Parent is a splitter");
        
        // Get the other window in the splitter
        wxWindow* otherWindow = nullptr;
        if (splitter->GetWindow1() == area) {
            otherWindow = splitter->GetWindow2();
        } else if (splitter->GetWindow2() == area) {
            otherWindow = splitter->GetWindow1();
        }
        
        // First, remove the area from the splitter
        bool wasUnsplit = splitter->Unsplit(area);
        if (!wasUnsplit) {
            wxLogDebug("  -> Failed to unsplit area!");
        }
        
        // Detach the area from its parent to prevent double deletion
        area->Reparent(this);
        area->Hide();
        
        // If the splitter now has only one child, we may need to simplify the layout
        if (otherWindow && splitter->GetParent()) {
            wxWindow* splitterParent = splitter->GetParent();
            
            // If parent is also a splitter, replace this splitter with the remaining window
            if (DockSplitter* parentSplitter = dynamic_cast<DockSplitter*>(splitterParent)) {
                wxLogDebug("  -> Parent is also a splitter, simplifying layout");
                
                // Reparent the other window to the parent splitter
                otherWindow->Reparent(parentSplitter);
                
                // Replace this splitter with the other window
                if (parentSplitter->GetWindow1() == splitter) {
                    parentSplitter->ReplaceWindow(splitter, otherWindow);
                } else if (parentSplitter->GetWindow2() == splitter) {
                    parentSplitter->ReplaceWindow(splitter, otherWindow);
                }
                
                // Now we can safely destroy the empty splitter
                splitter->Destroy();
            } else if (splitter == m_rootSplitter) {
                wxLogDebug("  -> This is the root splitter with one child");
                if (!otherWindow) {
                    wxLogDebug("    -> No other window in root splitter!");
                    // Root splitter is now empty, this shouldn't happen
                    // but if it does, we need to handle it
                }
                // For root splitter, just leave it with one window
                // The splitter will handle drawing correctly
            }
        }
    } else {
        wxLogDebug("  -> Parent is not a splitter");
        // Just detach from parent
        if (parent) {
            if (wxSizer* sizer = parent->GetSizer()) {
                sizer->Detach(area);
            }
        }
    }
    
    // Update layout
    Layout();
    
    // Notify
    wxCommandEvent event(EVT_DOCK_AREAS_REMOVED);
    event.SetEventObject(this);
    ProcessWindowEvent(event);
    
    // Update remaining dock area title bar button states
    for (auto* remainingArea : m_dockAreas) {
        remainingArea->updateTitleBarButtonStates();
    }
    
    // Now safely destroy the area
    area->Destroy();
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
    
    // Get the root splitter
    if (DockSplitter* splitter = dynamic_cast<DockSplitter*>(m_rootSplitter)) {
        wxLogDebug("  -> Got root splitter: %p", splitter);
        
        // Simple approach: build the layout step by step
        addDockAreaSimple(splitter, dockArea, area);
    }
    
    // Notify
    wxCommandEvent event(EVT_DOCK_AREAS_ADDED);
    event.SetEventObject(this);
    ProcessWindowEvent(event);
    
    // Update all dock area title bar button states
    for (auto* area : m_dockAreas) {
        area->updateTitleBarButtonStates();
    }
    
    // IMPORTANT: Ensure all windows in the hierarchy are visible
    ensureAllChildrenVisible(m_rootSplitter);
    
    // Force a complete layout update
    Layout();
    // Use RefreshRect for specific areas instead of full refresh
    wxRect dirtyRect = GetClientRect();
    RefreshRect(dirtyRect, false);
}








void DockContainerWidget::onSize(wxSizeEvent& event) {
    // Apply layout configuration if using percentage mode
    applyLayoutConfig();
    
    // Force refresh of all dock areas to prevent ghosting during window resize
    for (auto* area : m_dockAreas) {
        if (area && area->mergedTitleBar()) {
            area->mergedTitleBar()->Refresh();
            area->mergedTitleBar()->Update();
        }
    }
    
    event.Skip();
}







} // namespace ads
