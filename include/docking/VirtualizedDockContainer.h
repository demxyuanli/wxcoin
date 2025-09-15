#pragma once

#include "docking/DockContainerWidget.h"
#include <wx/timer.h>

namespace ads {

/**
 * Virtualized dock container that only updates visible areas during resize
 * 
 * Key optimizations:
 * 1. Only layout visible dock areas
 * 2. Defer layout of hidden/minimized areas
 * 3. Use viewport culling for large layouts
 */
class VirtualizedDockContainer : public DockContainerWidget {
public:
    VirtualizedDockContainer(DockManager* dockManager, wxWindow* parent);
    virtual ~VirtualizedDockContainer();
    
    // Enable/disable virtualization
    void setVirtualizationEnabled(bool enabled) { m_virtualizationEnabled = enabled; }
    
protected:
    void onSize(wxSizeEvent& event) override;
    
private:
    bool m_virtualizationEnabled{true};
    wxRect m_viewport;
    wxTimer* m_updateTimer;
    
    // Track which areas need update
    std::set<DockArea*> m_dirtyAreas;
    std::set<DockArea*> m_visibleAreas;
    
    // Virtualization methods
    void updateViewport();
    void updateVisibleAreas();
    void layoutVisibleAreasOnly();
    void scheduleHiddenAreaUpdate();
    
    // Check if area is visible
    bool isAreaVisible(DockArea* area) const;
    wxRect getAreaBounds(DockArea* area) const;
    
    void onUpdateTimer(wxTimerEvent& event);
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace ads
