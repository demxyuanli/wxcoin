#include "docking/DockContainerOptimized.h"
#include "docking/DockArea.h"
#include "docking/DockSplitter.h"
#include "docking/DockLayoutConfig.h"
#include "logger/Logger.h"
#include <wx/dcbuffer.h>
#include <wx/stopwatch.h>

namespace ads {

wxBEGIN_EVENT_TABLE(DockContainerOptimized, DockContainerWidget)
    EVT_SIZE(DockContainerOptimized::onSize)
wxEND_EVENT_TABLE()

DockContainerOptimized::DockContainerOptimized(DockManager* dockManager, wxWindow* parent)
    : DockContainerWidget(dockManager, parent)
    , m_lastResizeTime(std::chrono::steady_clock::now())
    , m_cachedSize(0, 0)
    , m_resizeThrottleMs(75) // Increased from 16ms to 75ms for better performance
{
    // Enable optimizations
    SetBackgroundStyle(wxBG_STYLE_PAINT); // Prevent automatic background erase
    
    // Pre-allocate dirty regions vector
    m_dirtyRegions.reserve(10);
}

DockContainerOptimized::~DockContainerOptimized() {
}

void DockContainerOptimized::onSize(wxSizeEvent& event) {
    wxSize newSize = event.GetSize();
    
    // Quick exit if size hasn't actually changed
    if (newSize == m_cachedSize) {
        event.Skip();
        return;
    }
    
    // Check if we should skip this resize event
    if (shouldSkipLayout()) {
        event.Skip();
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastResize = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_lastResizeTime).count();
    
    // Throttle resize processing - increased to 75ms for better performance
    if (timeSinceLastResize < m_resizeThrottleMs) {
        m_pendingResizeCount++;
        event.Skip();
        return;
    }
    
    m_lastResizeTime = now;
    m_resizeInProgress = true;
    
    // Store old size for incremental updates
    wxSize oldSize = m_cachedSize;
    m_cachedSize = newSize;
    
    // Batch updates with Freeze/Thaw to reduce flicker
    wxStopWatch stopwatch;
    Freeze();
    processResize();
    Thaw();
    
    // Performance monitoring - log if resize takes too long
    long elapsed = stopwatch.Time();
    if (elapsed > 20) { // Log if resize takes more than 20ms
        wxLogDebug("DockContainerOptimized::onSize took %ldms (size: %dx%d)", 
                   elapsed, newSize.x, newSize.y);
    }
    
    m_resizeInProgress = false;
    m_pendingResizeCount = 0;
    
    event.Skip();
}

void DockContainerOptimized::processResize() {
    // Clear dirty regions
    m_dirtyRegions.clear();
    
    // Use incremental layout if enabled
    if (m_useIncrementalLayout) {
        updateLayoutIncremental();
    } else {
        // Fall back to standard layout
        Layout();
    }
    
    // Schedule optimized paint
    if (!m_deferPaintDuringResize || m_pendingResizeCount == 0) {
        schedulePaintRegions();
    }
}

void DockContainerOptimized::updateLayoutIncremental() {
    // Get current container size
    wxSize containerSize = GetSize();
    
    // Only update splitters that actually need adjustment
    if (m_layoutConfig) {
        // Use the new layout calculation system
        int targetLeftWidth = calculateAreaSizeBasedOnFixedDocks(LeftDockWidgetArea, containerSize, *m_layoutConfig);
        int targetBottomHeight = calculateAreaSizeBasedOnFixedDocks(BottomDockWidgetArea, containerSize, *m_layoutConfig);
        
        // Ensure minimum widths
        targetLeftWidth = std::max(targetLeftWidth, 240);
        
        // Find and adjust only the main splitters
        if (DockSplitter* rootSplitter = dynamic_cast<DockSplitter*>(m_rootSplitter)) {
            adjustSplitterEfficient(rootSplitter, targetLeftWidth, targetBottomHeight);
        }
    }
    
    // Only layout areas that have changed size
    for (auto* area : m_dockAreas) {
        if (area && area->IsShown()) {
            wxRect oldRect = area->GetRect();
            area->Layout();
            wxRect newRect = area->GetRect();
            
            // Track dirty regions for optimized painting
            if (oldRect != newRect) {
                m_dirtyRegions.push_back(oldRect);
                m_dirtyRegions.push_back(newRect);
            }
        }
    }
}

void DockContainerOptimized::schedulePaintRegions() {
    if (m_dirtyRegions.empty()) {
        // No regions to paint
        return;
    }
    
    // Merge overlapping dirty regions
    std::vector<wxRect> mergedRegions;
    mergedRegions.reserve(m_dirtyRegions.size());
    
    for (const auto& rect : m_dirtyRegions) {
        bool merged = false;
        for (auto& mergedRect : mergedRegions) {
            if (mergedRect.Intersects(rect)) {
                mergedRect.Union(rect);
                merged = true;
                break;
            }
        }
        if (!merged) {
            mergedRegions.push_back(rect);
        }
    }
    
    // Schedule paint for merged regions only
    for (const auto& rect : mergedRegions) {
        RefreshRect(rect, false);
    }
}

bool DockContainerOptimized::shouldSkipLayout() const {
    // Skip layout if we're in the middle of multiple rapid resizes
    if (m_pendingResizeCount > 3) { // Reduced from 5 to 3 for more responsive UI
        return true;
    }
    
    // Skip if size is too small
    wxSize size = GetSize();
    if (size.x < 100 || size.y < 100) {
        return true;
    }
    
    // Skip if resize is in progress to avoid recursive calls
    if (m_resizeInProgress) {
        return true;
    }
    
    // Skip if container is being destroyed
    if (IsBeingDeleted()) {
        return true;
    }
    
    return false;
}

void DockContainerOptimized::adjustSplitterEfficient(DockSplitter* splitter, 
                                                    int targetLeftWidth, 
                                                    int targetBottomHeight) {
    if (!splitter || !splitter->IsSplit()) {
        return;
    }
    
    // Check if this splitter controls a fixed-size dock
    bool isFixedSizeSplitter = false;
    if (m_layoutConfig) {
        if (splitter->GetSplitMode() == wxSPLIT_VERTICAL && 
            m_layoutConfig->showLeftArea && m_layoutConfig->leftAreaFixed &&
            isLeftDockSplitter(splitter)) {
            isFixedSizeSplitter = true;
        } else if (splitter->GetSplitMode() == wxSPLIT_HORIZONTAL && 
                   m_layoutConfig->showBottomArea && m_layoutConfig->bottomAreaFixed &&
                   isBottomDockSplitter(splitter)) {
            isFixedSizeSplitter = true;
        }
    }
    
    // Don't adjust fixed-size dock splitters
    if (isFixedSizeSplitter) {
        return;
    }
    
    int currentPos = splitter->GetSashPosition();
    bool needsUpdate = false;
    
    if (splitter->GetSplitMode() == wxSPLIT_VERTICAL) {
        // Only adjust if difference is significant (>5 pixels)
        if (std::abs(currentPos - targetLeftWidth) > 5) {
            splitter->SetSashPosition(targetLeftWidth);
            needsUpdate = true;
        }
    } else if (splitter->GetSplitMode() == wxSPLIT_HORIZONTAL) {
        int targetPos = splitter->GetSize().GetHeight() - targetBottomHeight;
        if (std::abs(currentPos - targetPos) > 5) {
            splitter->SetSashPosition(targetPos);
            needsUpdate = true;
        }
    }
    
    // Only recurse if we made changes
    if (needsUpdate) {
        // Check children for nested splitters
        wxWindow* win1 = splitter->GetWindow1();
        wxWindow* win2 = splitter->GetWindow2();
        
        if (DockSplitter* childSplitter = dynamic_cast<DockSplitter*>(win1)) {
            adjustSplitterEfficient(childSplitter, targetLeftWidth, targetBottomHeight);
        }
        if (DockSplitter* childSplitter = dynamic_cast<DockSplitter*>(win2)) {
            adjustSplitterEfficient(childSplitter, targetLeftWidth, targetBottomHeight);
        }
    }
}

} // namespace ads