#pragma once

#include <wx/wx.h>
#include <vector>
#include <set>
#include "PerformanceMonitor.h"
#include "BatchRefreshManager.h"

namespace ads {

class RenderOptimizer {
public:
    static RenderOptimizer& getInstance();
    
    void optimizeRefresh(wxWindow* window, const wxRect* rect = nullptr);
    void optimizePaint(wxWindow* window, std::function<void(wxDC&)> paintFunc);
    
    void beginRenderBatch();
    void endRenderBatch();
    
    void setOptimizationEnabled(bool enabled) { m_optimizationEnabled = enabled; }
    bool isOptimizationEnabled() const { return m_optimizationEnabled; }
    
    void clearCache();
    
private:
    RenderOptimizer() = default;
    ~RenderOptimizer() = default;
    RenderOptimizer(const RenderOptimizer&) = delete;
    RenderOptimizer& operator=(const RenderOptimizer&) = delete;
    
    bool m_optimizationEnabled = true;
    int m_batchCount = 0;
    std::set<wxWindow*> m_dirtyWindows;
};

} // namespace ads

