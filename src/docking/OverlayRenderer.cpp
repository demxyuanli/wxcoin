#include "docking/OverlayRenderer.h"
#include "docking/DockOverlay.h"
#include <wx/log.h>
#include <algorithm>

namespace ads {

OverlayRenderer::OverlayRenderer(DockOverlay* overlay)
    : m_overlay(overlay)
    , m_frameColor(wxColour(0, 122, 204))
    , m_areaColor(wxColour(0, 122, 204, 102))
    , m_frameWidth(2)
    , m_backgroundColor(wxColour(240, 240, 240, 102))
    , m_globalBackgroundColor(wxColour(200, 220, 240, 102))
    , m_borderColor(wxColour(0, 122, 204))
    , m_borderWidth(2)
    , m_dropAreaNormalBg(wxColour(255, 255, 255, 102))
    , m_dropAreaNormalBorder(wxColour(217, 217, 217))
    , m_dropAreaHighlightBg(wxColour(0, 122, 204, 102))
    , m_dropAreaHighlightBorder(wxColour(0, 122, 204))
    , m_dropAreaIconColor(wxColour(96, 96, 96))
    , m_dropAreaHighlightIconColor(wxColour(255, 255, 255))
    , m_cornerRadius(4)
    , m_dropAreasForHover(nullptr)
    , m_overlaySize(0, 0)
    , m_overlayClientRect(0, 0, 0, 0)
{
}

OverlayRenderer::~OverlayRenderer() {
}

void OverlayRenderer::render(wxDC& dc, const wxRect& clientRect, bool isGlobalMode) {
    // Optimized background rendering based on mode
    if (isGlobalMode) {
        // Global mode: more prominent background
        dc.SetBrush(wxBrush(m_globalBackgroundColor));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(clientRect);

        // Add subtle border for global mode
        dc.SetPen(wxPen(wxColour(m_borderColor.Red(), m_borderColor.Green(), m_borderColor.Blue(), 200), m_borderWidth));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(wxRect(0, 0, clientRect.GetWidth() - 1, clientRect.GetHeight() - 1));
    } else {
        // Normal mode: subtle background
        dc.SetBrush(wxBrush(m_backgroundColor));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(clientRect);
    }
}

void OverlayRenderer::setDropAreaColors(const wxColour& normalBg, const wxColour& normalBorder,
                                        const wxColour& highlightBg, const wxColour& highlightBorder,
                                        const wxColour& iconColor, const wxColour& highlightIconColor) {
    m_dropAreaNormalBg = normalBg;
    m_dropAreaNormalBorder = normalBorder;
    m_dropAreaHighlightBg = highlightBg;
    m_dropAreaHighlightBorder = highlightBorder;
    m_dropAreaIconColor = iconColor;
    m_dropAreaHighlightIconColor = highlightIconColor;
}

// Migrated from DockOverlay::paintDropAreas
void OverlayRenderer::renderDropAreas(wxDC& dc, const std::vector<std::unique_ptr<DockOverlayDropArea>>& dropAreas) {
    wxLogDebug("OverlayRenderer::renderDropAreas - %d areas", (int)dropAreas.size());
    
    for (const auto& dropArea : dropAreas) {
        wxLogDebug("  Area %d visible: %d", dropArea->area(), dropArea->isVisible());
        if (dropArea->isVisible()) {
            renderDropIndicator(dc, *dropArea);
        }
    }
}

// Migrated from DockOverlay::paintDropIndicator
void OverlayRenderer::renderDropIndicator(wxDC& dc, const DockOverlayDropArea& dropArea) {
    wxRect rect = dropArea.rect();
    DockWidgetArea area = dropArea.area();

    wxLogDebug("OverlayRenderer::renderDropIndicator - Area %d: rect(%d,%d,%d,%d) highlighted:%d", 
               area, rect.x, rect.y, rect.width, rect.height, dropArea.isHighlighted());

    // Use configured colors
    wxColour normalBg = m_dropAreaNormalBg;
    wxColour normalBorder = m_dropAreaNormalBorder;
    wxColour highlightBg = m_dropAreaHighlightBg;
    wxColour highlightBorder = m_dropAreaHighlightBorder;
    wxColour iconColor = m_dropAreaIconColor;
    wxColour highlightIconColor = m_dropAreaHighlightIconColor;

    // Draw the indicator button with configured appearance
    if (dropArea.isHighlighted()) {
        // Highlighted state - use red border for better visual feedback
        wxLogDebug("OverlayRenderer::renderDropIndicator - Drawing highlighted area %d with RED border", area);
        dc.SetPen(wxPen(wxColour(255, 0, 0), m_borderWidth + 1));  // Red border, slightly thicker
        dc.SetBrush(wxBrush(highlightBg));
    } else {
        // Normal state
        wxLogDebug("OverlayRenderer::renderDropIndicator - Drawing normal area %d", area);
        dc.SetPen(wxPen(normalBorder, 1));
        dc.SetBrush(wxBrush(normalBg));
    }

    // Draw rounded rectangle with configured corner radius
    dc.DrawRoundedRectangle(rect, m_cornerRadius);

    // Draw subtle inner shadow for depth
    if (!dropArea.isHighlighted()) {
        wxRect innerRect = rect;
        innerRect.Deflate(1);
        dc.SetPen(wxPen(wxColour(240, 240, 240), 1));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRoundedRectangle(innerRect, m_cornerRadius - 1);
    }

    // Draw the icon
    wxColour currentIconColor = dropArea.isHighlighted() ? highlightIconColor : iconColor;
    renderAreaIcon(dc, rect, area, currentIconColor);
}

// Migrated from DockOverlay::drawAreaIcon
void OverlayRenderer::renderAreaIcon(wxDC& dc, const wxRect& rect, DockWidgetArea area, const wxColour& color) {
    // Calculate icon drawing area (centered in rect) - VS style proportions
    int iconSize = 16;  // Slightly larger for better visibility
    int arrowSize = 6;  // Arrow head size
    int lineWidth = 2;  // Slightly thicker lines for VS style

    wxPoint center(rect.x + rect.width / 2, rect.y + rect.height / 2);

    // Check if this area is currently hovered for red feedback
    bool isHovered = false;
    if (m_dropAreasForHover) {
        for (const auto& dropArea : *m_dropAreasForHover) {
            if (dropArea->area() == area && dropArea->isHighlighted()) {
                isHovered = true;
                break;
            }
        }
    }

    // Use red color for hovered areas, otherwise use the provided color
    wxColour iconColor = isHovered ? wxColour(255, 0, 0) : color;  // Red for hovered, original color otherwise

    dc.SetPen(wxPen(iconColor, lineWidth));
    dc.SetBrush(wxBrush(iconColor));

    switch (area) {
    case TopDockWidgetArea:
        {
            // Draw upward arrow (VS style - cleaner and more prominent)
            wxPoint arrow[3];
            arrow[0] = wxPoint(center.x, center.y - iconSize/2);
            arrow[1] = wxPoint(center.x - arrowSize, center.y - iconSize/2 + arrowSize);
            arrow[2] = wxPoint(center.x + arrowSize, center.y - iconSize/2 + arrowSize);
            dc.DrawPolygon(3, arrow);

            // Draw arrow shaft - thicker and cleaner
            dc.DrawLine(center.x, center.y - iconSize/2 + arrowSize - 1,
                       center.x, center.y + iconSize/2);
        }
        break;

    case BottomDockWidgetArea:
        {
            // Draw downward arrow (VS style)
            wxPoint arrow[3];
            arrow[0] = wxPoint(center.x, center.y + iconSize/2);
            arrow[1] = wxPoint(center.x - arrowSize, center.y + iconSize/2 - arrowSize);
            arrow[2] = wxPoint(center.x + arrowSize, center.y + iconSize/2 - arrowSize);
            dc.DrawPolygon(3, arrow);

            // Draw arrow shaft
            dc.DrawLine(center.x, center.y + iconSize/2 - arrowSize + 1,
                       center.x, center.y - iconSize/2);
        }
        break;

    case LeftDockWidgetArea:
        {
            // Draw leftward arrow (VS style)
            wxPoint arrow[3];
            arrow[0] = wxPoint(center.x - iconSize/2, center.y);
            arrow[1] = wxPoint(center.x - iconSize/2 + arrowSize, center.y - arrowSize);
            arrow[2] = wxPoint(center.x - iconSize/2 + arrowSize, center.y + arrowSize);
            dc.DrawPolygon(3, arrow);

            // Draw arrow shaft
            dc.DrawLine(center.x - iconSize/2 + arrowSize - 1, center.y,
                       center.x + iconSize/2, center.y);
        }
        break;

    case RightDockWidgetArea:
        {
            // Draw rightward arrow (VS style)
            wxPoint arrow[3];
            arrow[0] = wxPoint(center.x + iconSize/2, center.y);
            arrow[1] = wxPoint(center.x + iconSize/2 - arrowSize, center.y - arrowSize);
            arrow[2] = wxPoint(center.x + iconSize/2 - arrowSize, center.y + arrowSize);
            dc.DrawPolygon(3, arrow);

            // Draw arrow shaft
            dc.DrawLine(center.x + iconSize/2 - arrowSize + 1, center.y,
                       center.x - iconSize/2, center.y);
        }
        break;

    case CenterDockWidgetArea:
        {
            // Draw center indicator - four small rectangles in a square pattern (VS style)
            int rectSize = 4;  // Slightly larger for better visibility
            int spacing = 2;   // Small spacing between rectangles

            dc.SetBrush(wxBrush(color));

            // Top-left
            dc.DrawRectangle(center.x - rectSize - spacing/2,
                           center.y - rectSize - spacing/2,
                           rectSize, rectSize);
            // Top-right
            dc.DrawRectangle(center.x + spacing/2,
                           center.y - rectSize - spacing/2,
                           rectSize, rectSize);
            // Bottom-left
            dc.DrawRectangle(center.x - rectSize - spacing/2,
                           center.y + spacing/2,
                           rectSize, rectSize);
            // Bottom-right
            dc.DrawRectangle(center.x + spacing/2,
                           center.y + spacing/2,
                           rectSize, rectSize);
        }
        break;
    }
}

// Migrated from DockOverlay::drawPreviewArea
void OverlayRenderer::renderPreviewArea(wxDC& dc, DockWidgetArea area, bool isDirectionIndicator) {
    wxRect previewRect = getPreviewRect(area);
    if (previewRect.IsEmpty()) {
        return;
    }

    wxColour previewBorder, previewFill;

    if (isDirectionIndicator) {
        // Enhanced direction indicator colors for better visibility - fully opaque
        previewBorder = wxColour(255, 255, 255);      // Bright white border, fully opaque
        previewFill = wxColour(0, 122, 204);          // Bright blue fill, fully opaque
    } else {
        // Strong contrast colors for target area preview
        previewBorder = wxColour(255, 0, 0, 102);     // Bright red border with 40% transparency
        previewFill = wxColour(255, 0, 0, 102);       // Bright red fill with 40% transparency
    }

    // Draw preview rectangle with enhanced visibility
    dc.SetPen(wxPen(previewBorder, 3));  // Thicker border for direction indicators
    dc.SetBrush(wxBrush(previewFill));
    dc.DrawRectangle(previewRect);

    // Add a subtle inner highlight for depth
    wxRect innerRect = previewRect;
    innerRect.Deflate(2);
    dc.SetPen(wxPen(wxColour(255, 255, 255, 102), 1));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(innerRect);
}

// Migrated from DockOverlay::drawGlobalModeHints
void OverlayRenderer::renderGlobalModeHints(wxDC& dc, const wxRect& clientRect) {
    // Draw screen edge indicators for global docking
    wxColour edgeColor(0, 122, 204, 102);  // VS blue with 40% transparency
    dc.SetPen(wxPen(edgeColor, 2));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);

    const int edgeThickness = 3;
    const int edgeOffset = 5;

    // Top edge indicator
    dc.DrawRectangle(edgeOffset, edgeOffset, clientRect.GetWidth() - 2 * edgeOffset, edgeThickness);

    // Bottom edge indicator
    dc.DrawRectangle(edgeOffset, clientRect.GetHeight() - edgeOffset - edgeThickness,
                    clientRect.GetWidth() - 2 * edgeOffset, edgeThickness);

    // Left edge indicator
    dc.DrawRectangle(edgeOffset, edgeOffset, edgeThickness, clientRect.GetHeight() - 2 * edgeOffset);

    // Right edge indicator
    dc.DrawRectangle(clientRect.GetWidth() - edgeOffset - edgeThickness, edgeOffset,
                    edgeThickness, clientRect.GetHeight() - 2 * edgeOffset);

    // Draw center area hint with special styling for global mode
    // Note: This requires access to drop areas - will be passed as parameter
    // For now, we'll skip this part and handle it in the calling code
}

// Migrated from DockOverlay::drawGlobalModeTextHints
void OverlayRenderer::renderGlobalModeTextHints(wxDC& dc, const wxRect& clientRect) {
    wxString hintText = "Drag to dock globally";
    wxFont hintFont(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    dc.SetFont(hintFont);
    dc.SetTextForeground(wxColour(80, 80, 80));

    // Calculate text position (bottom center)
    wxSize textSize = dc.GetTextExtent(hintText);
    int textX = (clientRect.GetWidth() - textSize.GetWidth()) / 2;
    int textY = clientRect.GetHeight() - textSize.GetHeight() - 10;

    // Draw text background for better readability
    wxRect textBgRect(textX - 5, textY - 2, textSize.GetWidth() + 10, textSize.GetHeight() + 4);
    dc.SetBrush(wxBrush(wxColour(255, 255, 255, 102)));
    dc.SetPen(wxPen(wxColour(200, 200, 200), 1));
    dc.DrawRoundedRectangle(textBgRect, 3);

    // Draw the hint text
    dc.DrawText(hintText, textX, textY);
}

// Migrated from DockOverlay::drawDirectionIndicators
void OverlayRenderer::renderDirectionIndicators(wxDC& dc, const std::vector<std::unique_ptr<DockOverlayDropArea>>& dropAreas) {
    // Only draw direction indicators for highlighted drop areas
    for (const auto& dropArea : dropAreas) {
        if (dropArea->isVisible() && dropArea->isHighlighted()) {
            renderPreviewArea(dc, dropArea->area(), true);  // Direction indicator preview
        }
    }
}

// Migrated from DockOverlay::drawDirectionArrow
void OverlayRenderer::renderDirectionArrow(wxDC& dc, const wxRect& rect, DockWidgetArea area) {
    wxPoint center(rect.x + rect.width / 2, rect.y + rect.height / 2);
    int arrowSize = 8;
    int lineWidth = 3;
    
    // Check if this area is currently hovered for red feedback
    bool isHovered = false;
    if (m_dropAreasForHover) {
        for (const auto& dropArea : *m_dropAreasForHover) {
            if (dropArea->area() == area && dropArea->isHighlighted()) {
                isHovered = true;
                break;
            }
        }
    }
    
    // Use red color for hovered areas, white otherwise
    wxColour arrowColor = isHovered ? wxColour(255, 0, 0) : wxColour(255, 255, 255);
    
    dc.SetPen(wxPen(arrowColor, lineWidth));
    dc.SetBrush(wxBrush(arrowColor));
    
    switch (area) {
    case TopDockWidgetArea:
        {
            // Draw upward arrow
            wxPoint arrow[3];
            arrow[0] = wxPoint(center.x, center.y - arrowSize);
            arrow[1] = wxPoint(center.x - arrowSize/2, center.y);
            arrow[2] = wxPoint(center.x + arrowSize/2, center.y);
            dc.DrawPolygon(3, arrow);
        }
        break;
    case BottomDockWidgetArea:
        {
            // Draw downward arrow
            wxPoint arrow[3];
            arrow[0] = wxPoint(center.x, center.y + arrowSize);
            arrow[1] = wxPoint(center.x - arrowSize/2, center.y);
            arrow[2] = wxPoint(center.x + arrowSize/2, center.y);
            dc.DrawPolygon(3, arrow);
        }
        break;
    case LeftDockWidgetArea:
        {
            // Draw leftward arrow
            wxPoint arrow[3];
            arrow[0] = wxPoint(center.x - arrowSize, center.y);
            arrow[1] = wxPoint(center.x, center.y - arrowSize/2);
            arrow[2] = wxPoint(center.x, center.y + arrowSize/2);
            dc.DrawPolygon(3, arrow);
        }
        break;
    case RightDockWidgetArea:
        {
            // Draw rightward arrow
            wxPoint arrow[3];
            arrow[0] = wxPoint(center.x + arrowSize, center.y);
            arrow[1] = wxPoint(center.x, center.y - arrowSize/2);
            arrow[2] = wxPoint(center.x, center.y + arrowSize/2);
            dc.DrawPolygon(3, arrow);
        }
        break;
    case CenterDockWidgetArea:
        {
            // Draw center indicator (square)
            wxRect square(center.x - arrowSize/2, center.y - arrowSize/2, arrowSize, arrowSize);
            dc.DrawRectangle(square);
        }
        break;
    default:
        break;
    }
}

// Migrated from DockOverlay::getPreviewRect
wxRect OverlayRenderer::getPreviewRect(DockWidgetArea area) const {
    wxRect clientRect = m_overlayClientRect;
    if (clientRect.IsEmpty()) {
        return wxRect();
    }
    
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

wxRect OverlayRenderer::dropIndicatorRect(DockWidgetArea area) const {
    // TODO: Migrate from DockOverlay::dropIndicatorRect
    return wxRect();
}

wxBitmap OverlayRenderer::createDropIndicatorBitmap(DockWidgetArea area, int size) {
    // TODO: Migrate from DockOverlay::createDropIndicatorBitmap
    return wxBitmap();
}

} // namespace ads

