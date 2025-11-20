#pragma once

#include <wx/wx.h>
#include "viewer/PickingService.h"
#include <chrono>

/**
 * @brief Overlay panel to display face picking information in canvas corner
 */
class FaceInfoOverlay {
public:
    FaceInfoOverlay();
    
    // Set face query result to display
    void setPickingResult(const PickingResult& result);
    
    // Clear the overlay
    void clear();
    
    // Check if overlay is visible
    bool isVisible() const { return m_visible; }
    
    // Draw the overlay on canvas
    void draw(wxDC& dc, const wxSize& canvasSize);
    
    // Auto-hide after timeout
    void update();
    
private:
    bool m_visible;
    PickingResult m_result;
    std::chrono::steady_clock::time_point m_showTime;
    int m_autoHideSeconds; // 0 = no auto-hide
};


