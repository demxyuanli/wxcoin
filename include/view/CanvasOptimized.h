#pragma once

#include "Canvas.h"
#include <wx/timer.h>
#include <atomic>
#include <chrono>

/**
 * Optimized Canvas with progressive rendering during resize
 */
class CanvasOptimized : public Canvas {
public:
    CanvasOptimized(wxWindow* parent,
                    wxWindowID id = wxID_ANY,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize,
                    long style = 0,
                    const wxString& name = "CanvasOptimized");
    
    virtual ~CanvasOptimized();

    // Enable/disable progressive rendering
    void setProgressiveRenderingEnabled(bool enabled) { m_progressiveRenderingEnabled = enabled; }
    bool isProgressiveRenderingEnabled() const { return m_progressiveRenderingEnabled; }

protected:
    // Override paint event for optimized rendering
    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    
    // Progressive rendering methods
    void renderLowQuality();
    void renderHighQuality();
    void scheduleHighQualityRender();
    
private:
    // Progressive rendering state
    bool m_progressiveRenderingEnabled{true};
    std::atomic<bool> m_isResizing{false};
    std::atomic<bool> m_needsHighQualityRender{false};
    
    // Timing
    std::chrono::steady_clock::time_point m_lastResizeTime;
    wxTimer* m_highQualityTimer{nullptr};
    
    // Note: Since RenderingEngine doesn't expose quality settings,
    // we use the fastMode parameter of render() method instead
    
    void onHighQualityTimer(wxTimerEvent& event);
    
    wxDECLARE_EVENT_TABLE();
};