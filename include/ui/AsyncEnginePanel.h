#pragma once

#include <wx/wx.h>
#include <wx/timer.h>
#include <string>
#include <vector>
#include <memory>
#include "async/AsyncEngineIntegration.h"

/**
 * @brief Async Engine monitoring panel
 * 
 * Displays real-time statistics and performance metrics for the async compute engine.
 * Shows task queue status, worker thread activity, completion rates, and performance graphs.
 */
class AsyncEnginePanel : public wxPanel {
public:
    AsyncEnginePanel(wxWindow* parent, async::AsyncEngineIntegration* asyncEngine = nullptr);
    ~AsyncEnginePanel() override;
    
    void setAsyncEngine(async::AsyncEngineIntegration* engine);

private:
    void OnTimer(wxTimerEvent&);
    void OnPaint(wxPaintEvent&);
    void OnSize(wxSizeEvent&);
    
    void fetchLatest();
    void drawCardBackground(wxDC& dc, int x, int y, int w, int h, const wxColour& borderColor);
    void drawMetricCard(wxDC& dc, int x, int y, int w, int h, 
                       const wxString& title, 
                       const wxString& value, 
                       const wxString& subtitle,
                       const wxColour& accentColor);
    void drawSparkline(wxDC& dc, int x, int y, int w, int h, 
                      const std::vector<size_t>& history, 
                      const wxColour& color,
                      const wxString& label);
    void drawStatusIndicator(wxDC& dc, int x, int y, int size, bool active);
    void drawProgressBar(wxDC& dc, int x, int y, int w, int h,
                        double value, double maxValue,
                        const wxColour& fillColor,
                        const wxString& label);
    
    static void clampPush(std::vector<size_t>& hist, size_t value, size_t maxSize);
    static size_t dynamicMaxFromHist(const std::vector<size_t>& hist, size_t minCap);
    static wxColour lerpColor(const wxColour& a, const wxColour& b, double t);
    static bool hasEnoughContrast(const wxColour& textColor, const wxColour& bgColor);

private:
    wxTimer m_timer;
    async::AsyncEngineIntegration* m_asyncEngine{nullptr};
    
    // Current statistics
    size_t m_queuedTasks{0};
    size_t m_runningTasks{0};
    size_t m_completedTasks{0};
    size_t m_failedTasks{0};
    size_t m_totalProcessed{0};
    double m_avgExecutionTimeMs{0.0};
    bool m_isRunning{false};
    
    // History for sparklines
    std::vector<size_t> m_histQueuedTasks;
    std::vector<size_t> m_histRunningTasks;
    std::vector<size_t> m_histCompletedTasks;
    std::vector<size_t> m_histAvgTimeMs;  // Changed from double to size_t for consistency
    static constexpr size_t kHistorySize = 60;
    
    // Smoothed display values
    double m_dispQueuedTasks{0.0};
    double m_dispRunningTasks{0.0};
    double m_dispAvgTimeMs{0.0};
    
    // UI colors
    wxColour m_accentBlue{66, 134, 244};
    wxColour m_accentGreen{72, 201, 176};
    wxColour m_accentOrange{255, 152, 0};
    wxColour m_accentRed{244, 67, 54};
    wxColour m_accentPurple{156, 39, 176};
    
    wxDECLARE_EVENT_TABLE();
};

