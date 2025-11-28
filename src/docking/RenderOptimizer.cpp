#include "docking/RenderOptimizer.h"
#include "docking/PerformanceMonitor.h"
#include "docking/BatchRefreshManager.h"
#include <wx/dc.h>
#include <wx/dcbuffer.h>
#include <chrono>
#include <functional>

namespace ads {

RenderOptimizer& RenderOptimizer::getInstance() {
    static RenderOptimizer instance;
    return instance;
}

void RenderOptimizer::optimizeRefresh(wxWindow* window, const wxRect* rect) {
    if (!m_optimizationEnabled || !window || window->IsBeingDeleted()) {
        if (window && !window->IsBeingDeleted()) {
            if (rect) {
                window->RefreshRect(*rect, false);
            } else {
                window->Refresh();
            }
        }
        return;
    }
    
    if (m_batchCount > 0) {
        m_dirtyWindows.insert(window);
        if (rect) {
            BatchRefreshManager::getInstance().scheduleRefresh(window, rect);
        } else {
            BatchRefreshManager::getInstance().scheduleRefreshAll(window);
        }
    } else {
        if (rect) {
            BatchRefreshManager::getInstance().scheduleRefresh(window, rect);
        } else {
            BatchRefreshManager::getInstance().scheduleRefreshAll(window);
        }
    }
}

void RenderOptimizer::optimizePaint(wxWindow* window, std::function<void(wxDC&)> paintFunc) {
    if (!window || window->IsBeingDeleted() || !paintFunc) {
        return;
    }
    
    ScopedPerformanceTimer timer("Paint");
    PerformanceMonitor::getInstance().recordRender(0.0);
    
    wxAutoBufferedPaintDC dc(window);
    paintFunc(dc);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    // Timing is handled by ScopedPerformanceTimer
}

void RenderOptimizer::beginRenderBatch() {
    m_batchCount++;
    BatchRefreshManager::getInstance().beginBatch();
}

void RenderOptimizer::endRenderBatch() {
    if (m_batchCount > 0) {
        m_batchCount--;
        if (m_batchCount == 0) {
            BatchRefreshManager::getInstance().endBatch();
            m_dirtyWindows.clear();
        }
    }
}

void RenderOptimizer::clearCache() {
    m_dirtyWindows.clear();
    BatchRefreshManager::getInstance().clear();
}

} // namespace ads

