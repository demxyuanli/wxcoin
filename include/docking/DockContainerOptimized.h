#pragma once

#include "docking/DockContainerWidget.h"
#include <chrono>
#include <atomic>

namespace ads {

/**
 * Optimized DockContainerWidget with improved resize performance
 */
class DockContainerOptimized : public DockContainerWidget {
public:
    DockContainerOptimized(DockManager* dockManager, wxWindow* parent);
    virtual ~DockContainerOptimized();

protected:
    // Override resize handling for better performance
    virtual void onSize(wxSizeEvent& event) override;
    
    // New optimized methods
    void processResize();
    void updateLayoutIncremental();
    void schedulePaintRegions();
    bool shouldSkipLayout() const;
    void adjustSplitterEfficient(DockSplitter* splitter, int targetLeftWidth, int targetBottomHeight);
    
private:
    // Performance tracking
    std::chrono::steady_clock::time_point m_lastResizeTime;
    std::atomic<bool> m_resizeInProgress{false};
    std::atomic<int> m_pendingResizeCount{0};
    
    // Layout caching
    wxSize m_cachedSize;
    std::vector<wxRect> m_dirtyRegions;
    
    // Optimization flags
    bool m_useIncrementalLayout{true};
    bool m_deferPaintDuringResize{true};
    int m_resizeThrottleMs{16}; // ~60fps
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace ads