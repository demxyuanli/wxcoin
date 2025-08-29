#pragma once

#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/dcbuffer.h>
#include <vector>
#include <memory>
#include "DockManager.h"

namespace ads {

// Forward declarations
class DockWidget;
class DockArea;
class DockContainerWidget;

/**
 * @brief Drop area for dock overlay
 */
class DockOverlayDropArea {
public:
    DockOverlayDropArea(DockWidgetArea area, const wxRect& rect);
    ~DockOverlayDropArea();
    
    DockWidgetArea area() const { return m_area; }
    wxRect rect() const { return m_rect; }
    bool contains(const wxPoint& pos) const { return m_rect.Contains(pos); }
    
    void setVisible(bool visible) { m_visible = visible; }
    bool isVisible() const { return m_visible; }
    
    void setHighlighted(bool highlighted) { m_highlighted = highlighted; }
    bool isHighlighted() const { return m_highlighted; }
    
private:
    DockWidgetArea m_area;
    wxRect m_rect;
    bool m_visible;
    bool m_highlighted;
};

/**
 * @brief Overlay shown during drag and drop operations
 */
class DockOverlay : public wxFrame {
public:
    enum eMode {
        ModeDockAreaOverlay,
        ModeContainerOverlay
    };
    
    DockOverlay(wxWindow* parent, eMode mode = ModeDockAreaOverlay);
    virtual ~DockOverlay();
    
    // Configuration
    void setAllowedAreas(int areas);
    int allowedAreas() const { return m_allowedAreas; }
    
    // Drop areas
    DockWidgetArea dropAreaUnderCursor() const;
    DockWidgetArea showOverlay(wxWindow* target);
    void hideOverlay();
    
    // Update position
    void updatePosition();
    void updateDropAreas();
    
    // Paint configuration
    void setFrameColor(const wxColour& color) { m_frameColor = color; }
    void setAreaColor(const wxColour& color) { m_areaColor = color; }
    void setFrameWidth(int width) { m_frameWidth = width; }
    
    // Target widget
    void setTargetWidget(wxWindow* widget) { m_targetWidget = widget; }
    wxWindow* targetWidget() const { return m_targetWidget; }
    
    // Mode
    eMode mode() const { return m_mode; }
    
    // Allowed areas
    void setAllowedAreas(int areas) { m_allowedAreas = areas; updateDropAreas(); }
    int allowedAreas() const { return m_allowedAreas; }
    
protected:
    // Event handlers
    void onPaint(wxPaintEvent& event);
    void onMouseMove(wxMouseEvent& event);
    void onMouseLeave(wxMouseEvent& event);
    void onEraseBackground(wxEraseEvent& event) {}
    
    // Internal methods
    void createDropAreas();
    void updateDropAreaPositions();
    void paintDropAreas(wxDC& dc);
    void paintDropIndicator(wxDC& dc, const DockOverlayDropArea& dropArea);
    wxRect dropIndicatorRect(DockWidgetArea area) const;
    wxRect getPreviewRect(DockWidgetArea area) const;
    
private:
    // Member variables
    eMode m_mode;
    wxWindow* m_targetWidget;
    int m_allowedAreas;
    std::vector<std::unique_ptr<DockOverlayDropArea>> m_dropAreas;
    DockWidgetArea m_lastHoveredArea;
    wxColour m_frameColor;
    wxColour m_areaColor;
    int m_frameWidth;
    
    // Helper methods
    wxRect targetRect() const;
    wxRect areaRect(DockWidgetArea area) const;
    wxBitmap createDropIndicatorBitmap(DockWidgetArea area, int size);
    
    wxDECLARE_EVENT_TABLE();
};

/**
 * @brief Cross overlay for container drop areas
 */
class DockOverlayCross : public wxWindow {
public:
    DockOverlayCross(DockOverlay* overlay);
    virtual ~DockOverlayCross();
    
    // Update based on cursor position
    void updatePosition();
    DockWidgetArea cursorLocation() const;
    
    // Configuration
    void setIconSize(int size) { m_iconSize = size; }
    int iconSize() const { return m_iconSize; }
    
    void setIconColor(const wxColour& color) { m_iconColor = color; }
    wxColour iconColor() const { return m_iconColor; }
    
protected:
    void onPaint(wxPaintEvent& event);
    void onMouseMove(wxMouseEvent& event);
    
private:
    DockOverlay* m_overlay;
    int m_iconSize;
    wxColour m_iconColor;
    DockWidgetArea m_hoveredArea;
    
    void drawCrossIcon(wxDC& dc);
    void drawAreaIndicator(wxDC& dc, DockWidgetArea area);
    wxRect areaRect(DockWidgetArea area) const;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace ads
