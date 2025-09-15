#include "docking/DockContainerWidget.h"
#include "docking/DockArea.h"
#include "docking/DockWidget.h"
#include "docking/FloatingDockContainer.h"
#include "docking/DockManager.h"
#include "docking/DockSplitter.h"
#include "docking/DockLayoutConfig.h"
#include <algorithm>

namespace ads {

// Define custom events
wxEventTypeTag<wxCommandEvent> DockContainerWidget::EVT_DOCK_AREAS_ADDED(wxNewEventType());
wxEventTypeTag<wxCommandEvent> DockContainerWidget::EVT_DOCK_AREAS_REMOVED(wxNewEventType());

// Event table
wxBEGIN_EVENT_TABLE(DockContainerWidget, wxPanel)
    EVT_SIZE(DockContainerWidget::onSize)
    EVT_TIMER(wxID_ANY, DockContainerWidget::onResizeTimer)
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
    , m_layoutConfig(nullptr)
    , m_resizeTimer(nullptr)
    , m_layoutUpdateTimer(nullptr)
    , m_lastContainerSize(wxSize(0, 0))
    , m_hasUserAdjustedLayout(false)
    , m_isResizeFreezeActive(false)
    , m_isResizing(false)
{
    // Create layout
    m_layout = new wxBoxSizer(wxVERTICAL);
    SetSizer(m_layout);

    // Enable double buffering for smoother resize repaint
    SetDoubleBuffered(true);

    // Create root splitter
    m_rootSplitter = new DockSplitter(this);
    m_layout->Add(m_rootSplitter, 1, wxEXPAND);

    // Initialize layout configuration
    if (dockManager) {
        m_layoutConfig = std::make_unique<DockLayoutConfig>();
        if (m_layoutConfig) {
            m_layoutConfig->LoadFromConfig();
        }
    }
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
    
    // Check if this is a floating container and it's now empty
    if (m_floatingWidget && m_dockAreas.empty()) {
        // Close the floating window
        m_floatingWidget->Close();
        return; // Don't destroy the area, the floating window will handle cleanup
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








void DockContainerWidget::onResizeTimer(wxTimerEvent& event) {
    // Apply layout configuration if using percentage mode
    applyLayoutConfig();
    
    // Note: Individual dock areas will handle their own refresh with debounce
    // No need to force refresh all areas here as it causes performance issues

    // Clear resizing flag after debounce fires and we apply config
    m_isResizing = false;
}

void DockContainerWidget::onLayoutUpdateTimer(wxTimerEvent& event) {
    // Perform the actual layout update and refresh
    // Coalesce painting during final layout
    if (!m_isResizeFreezeActive) {
        Freeze();
        m_isResizeFreezeActive = true;
    }

    Layout();
    
    // Use RefreshRect for specific areas instead of full refresh for better performance
    wxRect dirtyRect = GetClientRect();
    RefreshRect(dirtyRect, false);

    // Thaw once after coalesced updates
    if (m_isResizeFreezeActive) {
        Thaw();
        m_isResizeFreezeActive = false;
    }
}

void DockContainerWidget::onSize(wxSizeEvent& event) {
    wxSize newSize = event.GetSize();
    
    // Use proportional resize if we have cached ratios and user has adjusted layout
    if (m_hasUserAdjustedLayout && m_lastContainerSize.GetWidth() > 0 && m_lastContainerSize.GetHeight() > 0) {
        // Rebuild ratios to avoid dangling splitter pointers after structural changes
        cacheSplitterRatios();
        applyProportionalResize(m_lastContainerSize, newSize);
        m_lastContainerSize = newSize;
        event.Skip();
        return;
    }
    
    // Mark resizing and freeze early to avoid flicker while dragging the frame sash
    m_isResizing = true;
    if (!m_isResizeFreezeActive) {
        Freeze();
        m_isResizeFreezeActive = true;
    }

    // Use debounced layout update to prevent excessive recalculations during resize
    if (!m_resizeTimer) {
        m_resizeTimer = new wxTimer(this);
        Bind(wxEVT_TIMER, &DockContainerWidget::onResizeTimer, this, m_resizeTimer->GetId());
    }

    // Cancel any pending layout update
    if (m_resizeTimer->IsRunning()) {
        m_resizeTimer->Stop();
    }

    // Schedule layout update with slightly lower rate to cut CPU usage
    m_resizeTimer->Start(24, wxTIMER_ONE_SHOT); // ~40-50fps debounce

    // Update global docking hints if in global mode
    updateGlobalDockingHints();
    
    // Cache the new size for next resize
    m_lastContainerSize = newSize;

    event.Skip();
}







// Global docking hints implementation
void DockContainerWidget::updateGlobalDockingHints() {
    // This method is called during resize to update any global docking hints
    // For now, it's a placeholder for future enhancements

    // In the future, this could:
    // 1. Update screen edge detection for global docking
    // 2. Refresh overlay positions
    // 3. Update visual hints based on window size changes
}

void DockContainerWidget::enableGlobalDockingMode(bool enable) {
    wxLogDebug("DockContainerWidget::enableGlobalDockingMode - enable: %d", enable);

    if (enable) {
        // Store current layout state before entering global mode
        saveCurrentLayoutState();

        // Prepare for global docking operations
        // This might include:
        // - Temporarily adjusting splitter proportions
        // - Preparing overlay systems
        // - Setting up global drop zones

        wxLogDebug("Global docking mode enabled");
    } else {
        // Restore normal docking mode
        restoreLayoutState();

        wxLogDebug("Global docking mode disabled");
    }
}

void DockContainerWidget::saveCurrentLayoutState() {
    // Save current splitter positions and layout configuration
    // This allows us to restore the layout after global docking operations

    if (m_layoutConfig) {
        m_savedLayoutConfig = std::make_unique<DockLayoutConfig>(*m_layoutConfig);
        wxLogDebug("Layout state saved for global docking");
    }
}

void DockContainerWidget::restoreLayoutState() {
    // Restore previously saved layout state
    if (m_savedLayoutConfig && m_layoutConfig) {
        *m_layoutConfig = *m_savedLayoutConfig;
        applyLayoutConfig();
        wxLogDebug("Layout state restored after global docking");
    }
}

bool DockContainerWidget::isGlobalDockingEnabled() const {
    // Check if we're currently in global docking mode
    // This could be determined by checking various state flags
    return false; // Placeholder - implement based on actual state tracking
}

void DockContainerWidget::handleGlobalDockDrop(DockWidget* widget, DockWidgetArea area) {
    wxLogDebug("DockContainerWidget::handleGlobalDockDrop - widget: %p, area: %d", widget, area);

    if (!widget) {
        wxLogDebug("  -> Invalid widget");
        return;
    }

    // Handle global docking drop operations
    // This is called when a widget is dropped in global docking mode

    // For global docking, we typically want to:
    // 1. Create a new dock area at the specified edge/corner
    // 2. Add the widget to that area
    // 3. Adjust the overall layout to accommodate the new area

    switch (area) {
    case LeftDockWidgetArea:
    case RightDockWidgetArea:
    case TopDockWidgetArea:
    case BottomDockWidgetArea:
        // Create new area at the specified edge
        wxLogDebug("  -> Creating new area at edge: %d", area);
        addDockWidget(area, widget);
        break;

    case CenterDockWidgetArea:
        // For center drops in global mode, add as a new central area
        wxLogDebug("  -> Adding to center area");
        addDockWidget(area, widget);
        break;

    default:
        wxLogDebug("  -> Invalid drop area for global docking");
        break;
    }

    // Force layout update after global drop
    Layout();
    Refresh();
}

} // namespace ads
