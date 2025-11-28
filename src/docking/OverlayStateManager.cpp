#include "docking/OverlayStateManager.h"
#include "docking/DockOverlay.h"
#include "docking/DockWidget.h"
#include <wx/log.h>
#include <algorithm>

namespace ads {

OverlayStateManager::OverlayStateManager(DockOverlay* overlay)
    : m_overlay(overlay)
    , m_allowedAreas(AllDockAreas)
    , m_lastHoveredArea(InvalidDockWidgetArea)
    , m_overlayMode(DockOverlayMode::ModeDockAreaOverlay)
{
    createDropAreas();
}

OverlayStateManager::~OverlayStateManager() {
}

void OverlayStateManager::createDropAreas() {
    m_dropAreas.clear();
    
    // Create drop areas for each dock area
    m_dropAreas.push_back(std::make_unique<DockOverlayDropArea>(
        TopDockWidgetArea, wxRect()));
    m_dropAreas.push_back(std::make_unique<DockOverlayDropArea>(
        BottomDockWidgetArea, wxRect()));
    m_dropAreas.push_back(std::make_unique<DockOverlayDropArea>(
        LeftDockWidgetArea, wxRect()));
    m_dropAreas.push_back(std::make_unique<DockOverlayDropArea>(
        RightDockWidgetArea, wxRect()));
    m_dropAreas.push_back(std::make_unique<DockOverlayDropArea>(
        CenterDockWidgetArea, wxRect()));
}

void OverlayStateManager::updateDropAreas(int allowedAreas) {
    m_allowedAreas = allowedAreas;
    for (auto& dropArea : m_dropAreas) {
        DockWidgetArea area = dropArea->area();
        bool allowed = (m_allowedAreas & area) == area;
        dropArea->setVisible(allowed);
    }
}

// Migrated from DockOverlay::updatePosition
void OverlayStateManager::updatePosition(wxWindow* targetWidget) {
    if (!targetWidget) {
        return;
    }
    
    wxRect rect = targetRect(targetWidget);
    wxLogDebug("OverlayStateManager::updatePosition - target rect: %d,%d %dx%d", 
        rect.x, rect.y, rect.width, rect.height);
}

// Migrated from DockOverlay::updateDropAreaPositions
void OverlayStateManager::updateDropAreaPositions(const wxSize& overlaySize, DockOverlayMode mode) {
    int dropSize = 32;  // Larger size for VS-style indicators
    int margin = 8;     // Slightly larger margin for better spacing

    // Update positions for each drop area
    for (size_t i = 0; i < m_dropAreas.size(); ++i) {
        DockWidgetArea area = m_dropAreas[i]->area();
        bool visible = m_dropAreas[i]->isVisible();
        wxRect rect = areaRect(area, overlaySize, mode);

        // Create new drop area with updated rect but preserve visibility
        m_dropAreas[i] = std::make_unique<DockOverlayDropArea>(area, rect);
        m_dropAreas[i]->setVisible(visible);
    }

    // Make sure allowed areas are visible
    updateDropAreas(m_allowedAreas);
}

// Placeholder - will be implemented when needed
void OverlayStateManager::updateGlobalMode(bool isGlobalMode) {
    // TODO: Migrate from DockOverlay::updateGlobalMode if needed
}

// Migrated from DockOverlay::dropAreaUnderCursor
DockWidgetArea OverlayStateManager::dropAreaUnderCursor(const wxPoint& mousePos, wxWindow* overlayWindow, bool& needsRefresh) {
    wxPoint localPos = overlayWindow->ScreenToClient(mousePos);

    wxLogDebug("OverlayStateManager::dropAreaUnderCursor - mouse pos: %d,%d local: %d,%d",
        mousePos.x, mousePos.y, localPos.x, localPos.y);

    // Track which areas should be highlighted
    std::vector<DockWidgetArea> areasToHighlight;

    for (const auto& dropArea : m_dropAreas) {
        wxRect rect = dropArea->rect();
        bool isMouseOver = dropArea->isVisible() && isMouseOverIcon(localPos, dropArea->rect(), dropArea->area());
        wxLogDebug("  Area %d: rect(%d,%d,%d,%d) visible:%d mouseOver:%d",
            dropArea->area(), rect.x, rect.y, rect.width, rect.height,
            dropArea->isVisible(), isMouseOver);

        if (isMouseOver) {
            areasToHighlight.push_back(dropArea->area());
        }
    }

    // Update highlights for all areas independently
    needsRefresh = false;
    for (auto& dropArea : m_dropAreas) {
        bool shouldHighlight = std::find(areasToHighlight.begin(), areasToHighlight.end(), dropArea->area()) != areasToHighlight.end();
        bool wasHighlighted = dropArea->isHighlighted();

        if (shouldHighlight != wasHighlighted) {
            dropArea->setHighlighted(shouldHighlight);
            wxLogDebug("  Area %d: highlight changed %d -> %d", dropArea->area(), wasHighlighted, shouldHighlight);
            needsRefresh = true;
        }
    }

    // Return the primary hovered area (first one found, if any)
    return areasToHighlight.empty() ? InvalidDockWidgetArea : areasToHighlight[0];
}

// Migrated from DockOverlay::getPreviewRect
wxRect OverlayStateManager::getPreviewRect(DockWidgetArea area, const wxRect& clientRect) const {
    wxRect previewRect;
    int splitRatio = 50; // 50% split
    
    switch (area) {
    case TopDockWidgetArea:
        previewRect = wxRect(0, 0, clientRect.width, clientRect.height * splitRatio / 100);
        break;
    case BottomDockWidgetArea:
        previewRect = wxRect(0, clientRect.height * (100 - splitRatio) / 100, 
                           clientRect.width, clientRect.height * splitRatio / 100);
        break;
    case LeftDockWidgetArea:
        previewRect = wxRect(0, 0, clientRect.width * splitRatio / 100, clientRect.height);
        break;
    case RightDockWidgetArea:
        previewRect = wxRect(clientRect.width * (100 - splitRatio) / 100, 0, 
                           clientRect.width * splitRatio / 100, clientRect.height);
        break;
    case CenterDockWidgetArea:
        // For center, show the entire area
        previewRect = clientRect;
        break;
    default:
        break;
    }
    
    return previewRect;
}

// Migrated from DockOverlay::dropIndicatorRect
wxRect OverlayStateManager::dropIndicatorRect(DockWidgetArea area, const wxSize& overlaySize, DockOverlayMode mode) const {
    return areaRect(area, overlaySize, mode);
}

void OverlayStateManager::showDragHints(DockWidget* draggedWidget) {
    // TODO: Migrate from DockOverlay::showDragHints
}

void OverlayStateManager::updateDragHints() {
    // TODO: Migrate from DockOverlay::updateDragHints
}

void OverlayStateManager::updateDropAreaGeometryCache() {
    // TODO: Migrate from DockOverlay::updateDropAreaGeometryCache
}

void OverlayStateManager::clearGeometryCache() {
    m_cachedGeometries.clear();
}

// Migrated from DockOverlay::targetRect
wxRect OverlayStateManager::targetRect(wxWindow* targetWidget) const {
    if (!targetWidget) {
        return wxRect();
    }
    
    wxPoint pos = targetWidget->GetScreenPosition();
    wxSize size = targetWidget->GetSize();
    return wxRect(pos, size);
}

// Migrated from DockOverlay::areaRect
wxRect OverlayStateManager::areaRect(DockWidgetArea area, const wxSize& overlaySize, DockOverlayMode mode) const {
    int dropSize = 32;  // Match the size from updateDropAreaPositions
    int margin = 8;     // Match the margin from updateDropAreaPositions

    // Different layouts for different overlay modes - VS style positioning
    if (mode == DockOverlayMode::ModeDockAreaOverlay) {
        // For DockArea overlay: indicators positioned like VS - more spread out
        int centerX = overlaySize.GetWidth() / 2;
        int centerY = overlaySize.GetHeight() / 2;
        int spacing = dropSize + 12; // More space between center and surrounding indicators (VS style)

        switch (area) {
        case TopDockWidgetArea:
            return wxRect(centerX - dropSize/2, centerY - spacing - dropSize/2, dropSize, dropSize);
        case BottomDockWidgetArea:
            return wxRect(centerX - dropSize/2, centerY + spacing - dropSize/2, dropSize, dropSize);
        case LeftDockWidgetArea:
            return wxRect(centerX - spacing - dropSize/2, centerY - dropSize/2, dropSize, dropSize);
        case RightDockWidgetArea:
            return wxRect(centerX + spacing - dropSize/2, centerY - dropSize/2, dropSize, dropSize);
        case CenterDockWidgetArea:
            return wxRect(centerX - dropSize/2, centerY - dropSize/2, dropSize, dropSize);
        default:
            return wxRect();
        }
    } else {
        // For Container overlay: indicators at edges with better positioning (VS style)
        switch (area) {
        case TopDockWidgetArea:
            return wxRect((overlaySize.GetWidth() - dropSize) / 2, margin, dropSize, dropSize);
        case BottomDockWidgetArea:
            return wxRect((overlaySize.GetWidth() - dropSize) / 2, overlaySize.GetHeight() - margin - dropSize, dropSize, dropSize);
        case LeftDockWidgetArea:
            return wxRect(margin, (overlaySize.GetHeight() - dropSize) / 2, dropSize, dropSize);
        case RightDockWidgetArea:
            return wxRect(overlaySize.GetWidth() - margin - dropSize, (overlaySize.GetHeight() - dropSize) / 2, dropSize, dropSize);
        case CenterDockWidgetArea:
            return wxRect((overlaySize.GetWidth() - dropSize) / 2, (overlaySize.GetHeight() - dropSize) / 2, dropSize, dropSize);
        default:
            return wxRect();
        }
    }
}

// Migrated from DockOverlay::isMouseOverIcon
bool OverlayStateManager::isMouseOverIcon(const wxPoint& mousePos, const wxRect& buttonRect, DockWidgetArea area) const {
    // For direction indicators, we want to respond immediately when mouse enters the button area
    // Use the entire button rectangle for easier detection
    return buttonRect.Contains(mousePos);
}

} // namespace ads

