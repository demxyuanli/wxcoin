#pragma once

#include "Canvas.h"
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
    virtual void onPaint(wxPaintEvent& event) override;
    virtual void onSize(wxSizeEvent& event) override;
    
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
    
    // Quality settings
    int m_lowQualityAntiAliasing{0};  // No AA during resize
    int m_highQualityAntiAliasing{4}; // Full AA when static
    
    void onHighQualityTimer(wxTimerEvent& event);
    
    wxDECLARE_EVENT_TABLE();
};