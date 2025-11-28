#pragma once

#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <vector>
#include <memory>
#include "DockManager.h"

namespace ads {

// Forward declarations
class DockOverlay;
class DockOverlayDropArea;
class DockWidget;

// Forward declaration for DockOverlayDropArea
// Note: This is defined in DockOverlay.h, but we need it here
// We'll include DockOverlay.h in the .cpp file

/**
 * @brief Renderer for DockOverlay - handles all drawing operations
 * Separated from DockOverlay to follow Single Responsibility Principle
 */
class OverlayRenderer {
public:
    OverlayRenderer(DockOverlay* overlay);
    virtual ~OverlayRenderer();

    // Main rendering method
    void render(wxDC& dc, const wxRect& clientRect, bool isGlobalMode);

    // Drop area rendering
    void renderDropAreas(wxDC& dc, const std::vector<std::unique_ptr<DockOverlayDropArea>>& dropAreas);
    void renderDropIndicator(wxDC& dc, const DockOverlayDropArea& dropArea);
    
    // Access to drop areas for hover checking
    void setDropAreasForHoverCheck(const std::vector<std::unique_ptr<DockOverlayDropArea>>* dropAreas) {
        m_dropAreasForHover = dropAreas;
    }

    // Area rendering
    void renderAreaIcon(wxDC& dc, const wxRect& rect, DockWidgetArea area, const wxColour& color);
    void renderPreviewArea(wxDC& dc, DockWidgetArea area, bool isDirectionIndicator = false);

    // Global mode rendering
    void renderGlobalModeHints(wxDC& dc, const wxRect& clientRect);
    void renderGlobalModeTextHints(wxDC& dc, const wxRect& clientRect);

    // Direction indicators
    void renderDirectionIndicators(wxDC& dc, const std::vector<std::unique_ptr<DockOverlayDropArea>>& dropAreas);
    void renderDirectionArrow(wxDC& dc, const wxRect& rect, DockWidgetArea area);

    // Configuration
    void setFrameColor(const wxColour& color) { m_frameColor = color; }
    void setAreaColor(const wxColour& color) { m_areaColor = color; }
    void setFrameWidth(int width) { m_frameWidth = width; }
    void setBackgroundColor(const wxColour& color) { m_backgroundColor = color; }
    void setGlobalBackgroundColor(const wxColour& color) { m_globalBackgroundColor = color; }
    void setBorderColor(const wxColour& color) { m_borderColor = color; }
    void setBorderWidth(int width) { m_borderWidth = width; }
    void setCornerRadius(int radius) { m_cornerRadius = radius; }
    void setDropAreaColors(const wxColour& normalBg, const wxColour& normalBorder,
                          const wxColour& highlightBg, const wxColour& highlightBorder,
                          const wxColour& iconColor, const wxColour& highlightIconColor);
    
    // Access to overlay for size/position queries
    void setOverlaySize(const wxSize& size) { m_overlaySize = size; }
    void setOverlayClientRect(const wxRect& rect) { m_overlayClientRect = rect; }

private:
    DockOverlay* m_overlay;
    
    // Colors and styling
    wxColour m_frameColor;
    wxColour m_areaColor;
    int m_frameWidth;
    wxColour m_backgroundColor;
    wxColour m_globalBackgroundColor;
    wxColour m_borderColor;
    int m_borderWidth;
    wxColour m_dropAreaNormalBg;
    wxColour m_dropAreaNormalBorder;
    wxColour m_dropAreaHighlightBg;
    wxColour m_dropAreaHighlightBorder;
    wxColour m_dropAreaIconColor;
    wxColour m_dropAreaHighlightIconColor;
    int m_cornerRadius;
    
    // Reference to drop areas for hover checking
    const std::vector<std::unique_ptr<DockOverlayDropArea>>* m_dropAreasForHover;
    
    // Overlay size/position cache
    wxSize m_overlaySize;
    wxRect m_overlayClientRect;

    // Helper methods
    wxRect getPreviewRect(DockWidgetArea area) const;
    wxRect dropIndicatorRect(DockWidgetArea area) const;
    wxBitmap createDropIndicatorBitmap(DockWidgetArea area, int size);
};

} // namespace ads

