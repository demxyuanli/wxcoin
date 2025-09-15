#include "docking/DockContainerOptimized.h"
#include "docking/DockArea.h"
#include "docking/DockSplitter.h"
#include "logger/Logger.h"
#include <wx/dcbuffer.h>

namespace ads {

wxBEGIN_EVENT_TABLE(DockContainerOptimized, DockContainerWidget)
    EVT_SIZE(DockContainerOptimized::onSize)
wxEND_EVENT_TABLE()

DockContainerOptimized::DockContainerOptimized(DockManager* dockManager, wxWindow* parent)
    : DockContainerWidget(dockManager, parent)
    , m_lastResizeTime(std::chrono::steady_clock::now())
    , m_cachedSize(0, 0)
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
    
    // Throttle resize processing
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
    
    // Process resize with optimizations
    processResize();
    
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
    if (m_layoutConfig && m_layoutConfig->usePercentage) {
        // Calculate target positions once
        int targetLeftWidth = (containerSize.x * m_layoutConfig->leftAreaPercent) / 100;
        int targetBottomHeight = (containerSize.y * m_layoutConfig->bottomAreaPercent) / 100;
        
        // Ensure minimum widths
        targetLeftWidth = std::max(targetLeftWidth, 200);
        
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
    if (m_pendingResizeCount > 5) {
        return true;
    }
    
    // Skip if size is too small
    wxSize size = GetSize();
    if (size.x < 100 || size.y < 100) {
        return true;
    }
    
    return false;
}

// Helper method to efficiently adjust splitters
void DockContainerOptimized::adjustSplitterEfficient(DockSplitter* splitter, 
                                                    int targetLeftWidth, 
                                                    int targetBottomHeight) {
    if (!splitter || !splitter->IsSplit()) {
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