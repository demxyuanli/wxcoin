#pragma once

#include <wx/wx.h>
#include <vector>
#include <memory>
#include <map>
#include <algorithm>
#include "DockManager.h"

namespace ads {

// Forward declarations
class DockOverlay;
class DockOverlayDropArea;
class DockWidget;

// Forward declare DockOverlay::eMode
enum class DockOverlayMode {
    ModeDockAreaOverlay,
    ModeContainerOverlay
};

/**
 * @brief State manager for DockOverlay - handles state updates and position calculations
 * Separated from DockOverlay to follow Single Responsibility Principle
 */
class OverlayStateManager {
public:
    OverlayStateManager(DockOverlay* overlay);
    virtual ~OverlayStateManager();

    // Position and area management
    void updatePosition(wxWindow* targetWidget);
    void updateDropAreas(int allowedAreas);
    void updateDropAreaPositions(const wxSize& overlaySize, DockOverlayMode mode);
    void updateGlobalMode(bool isGlobalMode);

    // Drop area queries
    DockWidgetArea dropAreaUnderCursor(const wxPoint& mousePos, wxWindow* overlayWindow, bool& needsRefresh);
    wxRect getPreviewRect(DockWidgetArea area, const wxRect& clientRect) const;
    wxRect dropIndicatorRect(DockWidgetArea area, const wxSize& overlaySize, DockOverlayMode mode) const;

    // Drag hints
    void showDragHints(DockWidget* draggedWidget);
    void updateDragHints();

    // Geometry cache
    void updateDropAreaGeometryCache();
    void clearGeometryCache();

    // Access to drop areas
    std::vector<std::unique_ptr<DockOverlayDropArea>>& dropAreas() { return m_dropAreas; }
    const std::vector<std::unique_ptr<DockOverlayDropArea>>& dropAreas() const { return m_dropAreas; }

    // Configuration
    void setAllowedAreas(int areas) { m_allowedAreas = areas; }
    int allowedAreas() const { return m_allowedAreas; }
    
    // Access to overlay for mode queries
    void setOverlayMode(DockOverlayMode mode) { m_overlayMode = mode; }
    DockOverlayMode overlayMode() const { return m_overlayMode; }
    
    // Helper for position calculation
    wxRect targetRect(wxWindow* targetWidget) const;

private:
    DockOverlay* m_overlay;
    std::vector<std::unique_ptr<DockOverlayDropArea>> m_dropAreas;
    int m_allowedAreas;
    DockWidgetArea m_lastHoveredArea;
    
    // Geometry cache
    std::map<DockWidgetArea, wxRect> m_cachedGeometries;

    // Helper methods
    void createDropAreas();
    wxRect areaRect(DockWidgetArea area, const wxSize& overlaySize, DockOverlayMode mode) const;
    bool isMouseOverIcon(const wxPoint& mousePos, const wxRect& buttonRect, DockWidgetArea area) const;
    
private:
    DockOverlayMode m_overlayMode;
};

} // namespace ads

