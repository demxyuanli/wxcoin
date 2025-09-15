#pragma once

#include <wx/wx.h>
#include <chrono>
#include <vector>
#include <string>

namespace ads {

/**
 * Performance monitor for dock resize operations
 */
class DockResizeMonitor {
public:
    struct ResizeMetrics {
        std::chrono::milliseconds totalDuration{0};
        std::chrono::milliseconds layoutCalculation{0};
        std::chrono::milliseconds splitterAdjustment{0};
        std::chrono::milliseconds paintTime{0};
        int layoutUpdateCount{0};
        int paintEventCount{0};
        wxSize startSize;
        wxSize endSize;
    };

    static DockResizeMonitor& getInstance();

    // Start/stop monitoring a resize operation
    void startResize(const wxSize& startSize);
    void endResize(const wxSize& endSize);
    
    // Track specific operations
    void beginLayoutCalculation();
    void endLayoutCalculation();
    
    void beginSplitterAdjustment();
    void endSplitterAdjustment();
    
    void beginPaint();
    void endPaint();
    
    // Get current metrics
    const ResizeMetrics& getCurrentMetrics() const { return m_currentMetrics; }
    
    // Get average metrics over last N resizes
    ResizeMetrics getAverageMetrics(size_t lastN = 10) const;
    
    // Generate performance report
    std::string generateReport() const;
    
    // Enable/disable monitoring
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }

private:
    DockResizeMonitor() = default;
    
    bool m_enabled{true};
    bool m_resizeInProgress{false};
    
    ResizeMetrics m_currentMetrics;
    std::vector<ResizeMetrics> m_history;
    
    // Timing helpers
    std::chrono::steady_clock::time_point m_resizeStartTime;
    std::chrono::steady_clock::time_point m_operationStartTime;
    
    void recordMetrics();
};

} // namespace ads