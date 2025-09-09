#include "docking/DockOverlay.h"
#include "docking/DockWidget.h"
#include "docking/DockArea.h"
#include "docking/DockContainerWidget.h"
#include <wx/dcbuffer.h>
#include "config/ThemeManager.h"

namespace ads {

// DockOverlayDropArea implementation
DockOverlayDropArea::DockOverlayDropArea(DockWidgetArea area, const wxRect& rect)
    : m_area(area)
    , m_rect(rect)
    , m_visible(true)
    , m_highlighted(false)
{
}

DockOverlayDropArea::~DockOverlayDropArea() {
}

// Event table
wxBEGIN_EVENT_TABLE(DockOverlay, wxFrame)
    EVT_PAINT(DockOverlay::onPaint)
    EVT_MOTION(DockOverlay::onMouseMove)
    EVT_LEAVE_WINDOW(DockOverlay::onMouseLeave)
    EVT_ERASE_BACKGROUND(DockOverlay::onEraseBackground)
    EVT_TIMER(wxID_ANY, DockOverlay::onRefreshTimer)
wxEND_EVENT_TABLE()

// DockOverlay implementation
DockOverlay::DockOverlay(wxWindow* parent, eMode mode)
    : wxFrame(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
             wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT | wxBORDER_NONE | wxSTAY_ON_TOP)
    , m_mode(mode)
    , m_targetWidget(nullptr)
    , m_allowedAreas(AllDockAreas)
    , m_lastHoveredArea(InvalidDockWidgetArea)
    , m_frameColor(wxColour(0, 122, 204))  // VS blue color
    , m_areaColor(wxColour(0, 122, 204, 255))  // VS blue, fully opaque
    , m_frameWidth(2)
    , m_transparency(51)
    , m_globalTransparency(51)
    , m_backgroundColor(wxColour(240, 240, 240, 51))
    , m_globalBackgroundColor(wxColour(200, 220, 240, 51))
    , m_borderColor(wxColour(0, 122, 204))
    , m_borderWidth(2)
    , m_dropAreaNormalBg(wxColour(255, 255, 255, 255))
    , m_dropAreaNormalBorder(wxColour(217, 217, 217))
    , m_dropAreaHighlightBg(wxColour(0, 122, 204, 255))
    , m_dropAreaHighlightBorder(wxColour(0, 122, 204))
    , m_dropAreaIconColor(wxColour(96, 96, 96))
    , m_dropAreaHighlightIconColor(wxColour(255, 255, 255))
    , m_cornerRadius(4)
    , m_refreshDelay(16)
    , m_optimizedRendering(false)
    , m_refreshTimer(nullptr)
    , m_pendingRefresh(false)
    , m_lastRefreshTime(0)
    , m_refreshCount(0)
    , m_isGlobalMode(false)
    , m_dragPreviewCallback(nullptr)
{
    // TEMPORARILY CHANGED FOR TESTING - SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundStyle(wxBG_STYLE_COLOUR);
    
    // Set opaque background color for testing
    SetBackgroundColour(wxColour(255, 255, 255, 255));  // Solid white background

    // Load configuration from config file
    loadConfiguration();

    // Set transparency based on configuration - 20% transparency
    // TEMPORARILY DISABLED FOR TESTING - SetTransparent(51);  // 20% transparency

    // Initialize refresh timer for debouncing
    m_refreshTimer = new wxTimer(this);

    // Create drop areas
    createDropAreas();

    // Set initial visibility based on allowed areas
    updateDropAreas();
}

DockOverlay::~DockOverlay() {
    if (m_refreshTimer) {
        m_refreshTimer->Stop();
        delete m_refreshTimer;
    }
}

DockWidgetArea DockOverlay::dropAreaUnderCursor() {
    wxPoint mousePos = wxGetMousePosition();
    wxPoint localPos = ScreenToClient(mousePos);

    wxLogDebug("DockOverlay::dropAreaUnderCursor - mouse pos: %d,%d local: %d,%d",
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
    bool needsRefresh = false;
    for (auto& dropArea : m_dropAreas) {
        bool shouldHighlight = std::find(areasToHighlight.begin(), areasToHighlight.end(), dropArea->area()) != areasToHighlight.end();
        bool wasHighlighted = dropArea->isHighlighted();

        if (shouldHighlight != wasHighlighted) {
            dropArea->setHighlighted(shouldHighlight);
            wxLogDebug("  Area %d: highlight changed %d -> %d", dropArea->area(), wasHighlighted, shouldHighlight);
            needsRefresh = true;
        }
    }

    // Refresh only if highlights changed
    if (needsRefresh) {
        wxLogDebug("DockOverlay::dropAreaUnderCursor - Highlights changed, refreshing");
        Refresh(); // Force immediate refresh for hover feedback
    }

    // Return the primary hovered area (first one found, if any)
    return areasToHighlight.empty() ? InvalidDockWidgetArea : areasToHighlight[0];
}

DockWidgetArea DockOverlay::showOverlay(wxWindow* target) {
    if (!target) {
        hideOverlay();
        return InvalidDockWidgetArea;
    }
    
    wxLogDebug("DockOverlay::showOverlay - target: %p", target);
    
    m_targetWidget = target;
    
    // Enable optimized rendering during drag operations
    optimizeRendering();
    
    updatePosition();
    
    wxLogDebug("DockOverlay position: %d,%d size: %dx%d", 
        GetPosition().x, GetPosition().y, GetSize().GetWidth(), GetSize().GetHeight());
    
    Show();
    Raise();

    // Make sure window is on top
    SetWindowStyleFlag(GetWindowStyleFlag() | wxSTAY_ON_TOP);

    // Additional z-order management for global mode
    if (m_isGlobalMode) {
        // In global mode, ensure we stay on top of all other windows
        SetFocus(); // This helps with z-order on some platforms
        wxLogDebug("DockOverlay::showOverlay - Global mode: ensuring topmost with focus");
    }
    
    // Use debounced refresh instead of immediate refresh
    requestRefresh();
    
    // Ensure minimum size
    if (GetSize().GetWidth() < 100 || GetSize().GetHeight() < 100) {
        SetSize(wxSize(200, 200));
        wxLogDebug("DockOverlay: Set minimum size to 200x200");
    }
    
    return dropAreaUnderCursor();
}

void DockOverlay::hideOverlay() {
    Hide();
    m_targetWidget = nullptr;
    
    // Clear highlights
    for (auto& dropArea : m_dropAreas) {
        dropArea->setHighlighted(false);
    }
}

void DockOverlay::updatePosition() {
    if (!m_targetWidget) {
        return;
    }
    
    wxRect rect = targetRect();
    wxLogDebug("DockOverlay::updatePosition - target rect: %d,%d %dx%d", 
        rect.x, rect.y, rect.width, rect.height);
        
    SetPosition(rect.GetPosition());
    SetSize(rect.GetSize());
    
    updateDropAreaPositions();
}

void DockOverlay::updateDropAreas() {
    for (auto& dropArea : m_dropAreas) {
        DockWidgetArea area = dropArea->area();
        bool allowed = (m_allowedAreas & area) == area;
        dropArea->setVisible(allowed);
    }
}

void DockOverlay::onPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);

    if (!m_targetWidget) {
        return;
    }

    // Optimized background rendering based on mode
    if (m_isGlobalMode) {
        // Global mode: more prominent background
        dc.SetBrush(wxBrush(m_globalBackgroundColor));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(GetClientRect());

        // Add subtle border for global mode
        dc.SetPen(wxPen(wxColour(m_borderColor.Red(), m_borderColor.Green(), m_borderColor.Blue(), 200), m_borderWidth));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(wxRect(0, 0, GetSize().GetWidth() - 1, GetSize().GetHeight() - 1));
    } else {
        // Normal mode: subtle background
        dc.SetBrush(wxBrush(m_backgroundColor));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(GetClientRect());
    }

    wxLogDebug("DockOverlay::onPaint - size: %dx%d, global mode: %d",
               GetSize().GetWidth(), GetSize().GetHeight(), m_isGlobalMode);

    // Draw drop areas with VS-style appearance (without direction indicators)
    paintDropAreas(dc);

    // Draw additional hints in global mode
    if (m_isGlobalMode) {
        drawGlobalModeHints(dc);
    }

    // Draw direction indicators on top of everything else
    drawDirectionIndicators(dc);
}

void DockOverlay::onMouseMove(wxMouseEvent& event) {
    wxPoint pos = event.GetPosition();

    // In global mode, ensure we stay on top during mouse interaction
    if (m_isGlobalMode) {
        Raise(); // Re-raise to ensure topmost during interaction
    }

    // Use the same logic as dropAreaUnderCursor for consistency
    DockWidgetArea hoveredArea = dropAreaUnderCursor();

    // Handle drag preview callback
    if (m_dragPreviewCallback && hoveredArea != InvalidDockWidgetArea) {
        wxRect targetRect = getPreviewRect(hoveredArea);
        wxLogDebug("DockOverlay::onMouseMove - hoveredArea=%d, targetRect=(%d,%d,%d,%d)",
                  hoveredArea, targetRect.x, targetRect.y, targetRect.width, targetRect.height);
        if (!targetRect.IsEmpty()) {
            wxLogDebug("Calling drag preview callback with size %dx%d", targetRect.GetWidth(), targetRect.GetHeight());
            m_dragPreviewCallback(hoveredArea, targetRect.GetSize());
        } else {
            wxLogDebug("Target rect is empty, not calling callback");
        }
    } else if (m_dragPreviewCallback && hoveredArea == InvalidDockWidgetArea) {
        // Reset to default size when not hovering over any area
        wxLogDebug("Resetting drag preview to default size");
        m_dragPreviewCallback(InvalidDockWidgetArea, wxSize());
    } else {
        wxLogDebug("No drag preview callback set");
    }
}

void DockOverlay::onMouseLeave(wxMouseEvent& event) {
    // Clear all highlights
    for (auto& dropArea : m_dropAreas) {
        dropArea->setHighlighted(false);
    }
    
    m_lastHoveredArea = InvalidDockWidgetArea;
    Refresh(); // Force immediate refresh for hover feedback
}

void DockOverlay::createDropAreas() {
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

void DockOverlay::updateDropAreaPositions() {
    wxSize size = GetClientSize();
    int dropSize = 32;  // Larger size for VS-style indicators
    int margin = 8;     // Slightly larger margin for better spacing

    // Update positions for each drop area
    for (size_t i = 0; i < m_dropAreas.size(); ++i) {
        DockWidgetArea area = m_dropAreas[i]->area();
        bool visible = m_dropAreas[i]->isVisible();
        wxRect rect = areaRect(area);

        // Create new drop area with updated rect but preserve visibility
        m_dropAreas[i] = std::make_unique<DockOverlayDropArea>(area, rect);
        m_dropAreas[i]->setVisible(visible);
    }

    // Make sure allowed areas are visible
    updateDropAreas();
}

void DockOverlay::paintDropAreas(wxDC& dc) {
    wxLogDebug("DockOverlay::paintDropAreas - %d areas", (int)m_dropAreas.size());
    
    for (const auto& dropArea : m_dropAreas) {
        wxLogDebug("  Area %d visible: %d", dropArea->area(), dropArea->isVisible());
        if (dropArea->isVisible()) {
            paintDropIndicator(dc, *dropArea);
        }
    }
}

void DockOverlay::paintDropIndicator(wxDC& dc, const DockOverlayDropArea& dropArea) {
    wxRect rect = dropArea.rect();
    DockWidgetArea area = dropArea.area();

    wxLogDebug("DockOverlay::paintDropIndicator - Area %d: rect(%d,%d,%d,%d) highlighted:%d", 
               area, rect.x, rect.y, rect.width, rect.height, dropArea.isHighlighted());

    // Force opaque rendering by drawing a solid white background first
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(wxColour(255, 255, 255, 255)));  // Solid white background
    dc.DrawRoundedRectangle(rect, m_cornerRadius);
    
    // Use configured colors - Force fully opaque colors for buttons
    wxColour normalBg = wxColour(m_dropAreaNormalBg.Red(), m_dropAreaNormalBg.Green(), m_dropAreaNormalBg.Blue(), 255);
    wxColour normalBorder = wxColour(m_dropAreaNormalBorder.Red(), m_dropAreaNormalBorder.Green(), m_dropAreaNormalBorder.Blue(), 255);
    wxColour highlightBg = wxColour(m_dropAreaHighlightBg.Red(), m_dropAreaHighlightBg.Green(), m_dropAreaHighlightBg.Blue(), 255);
    wxColour highlightBorder = wxColour(m_dropAreaHighlightBorder.Red(), m_dropAreaHighlightBorder.Green(), m_dropAreaHighlightBorder.Blue(), 255);
    wxColour iconColor = wxColour(m_dropAreaIconColor.Red(), m_dropAreaIconColor.Green(), m_dropAreaIconColor.Blue(), 255);
    wxColour highlightIconColor = wxColour(m_dropAreaHighlightIconColor.Red(), m_dropAreaHighlightIconColor.Green(), m_dropAreaHighlightIconColor.Blue(), 255);

    // Draw the indicator button with configured appearance - Force opaque rendering
    if (dropArea.isHighlighted()) {
        // Highlighted state - use red border for better visual feedback
        wxLogDebug("DockOverlay::paintDropIndicator - Drawing highlighted area %d with RED border", area);
        dc.SetPen(wxPen(wxColour(255, 0, 0, 255), m_borderWidth + 1));  // Red border, fully opaque
        dc.SetBrush(wxBrush(wxColour(highlightBg.Red(), highlightBg.Green(), highlightBg.Blue(), 255)));  // Force opaque
    } else {
        // Normal state
        wxLogDebug("DockOverlay::paintDropIndicator - Drawing normal area %d", area);
        dc.SetPen(wxPen(wxColour(normalBorder.Red(), normalBorder.Green(), normalBorder.Blue(), 255), 1));  // Force opaque
        dc.SetBrush(wxBrush(wxColour(normalBg.Red(), normalBg.Green(), normalBg.Blue(), 255)));  // Force opaque
    }

    // Draw rounded rectangle with configured corner radius
    dc.DrawRoundedRectangle(rect, m_cornerRadius);

    // Draw subtle inner shadow for depth
    if (!dropArea.isHighlighted()) {
        wxRect innerRect = rect;
        innerRect.Deflate(1);
        dc.SetPen(wxPen(wxColour(240, 240, 240, 255), 1));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRoundedRectangle(innerRect, m_cornerRadius - 1);
    }

    // Draw the icon - Force opaque icon color
    wxColour currentIconColor = dropArea.isHighlighted() ? 
        wxColour(highlightIconColor.Red(), highlightIconColor.Green(), highlightIconColor.Blue(), 255) : 
        wxColour(iconColor.Red(), iconColor.Green(), iconColor.Blue(), 255);
    drawAreaIcon(dc, rect, area, currentIconColor);

    // Note: Direction indicators are now drawn separately in drawDirectionIndicators()
}

void DockOverlay::drawPreviewArea(wxDC& dc, DockWidgetArea area, bool isDirectionIndicator) {
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
        // Strong contrast colors for target area preview - 40% transparency
        previewBorder = wxColour(255, 0, 0, 255);     // Bright red border, fully opaque
        previewFill = wxColour(255, 0, 0, 255);       // Bright red fill, fully opaque
    }

    // Draw preview rectangle with enhanced visibility
    dc.SetPen(wxPen(previewBorder, 3));  // Thicker border for direction indicators
    dc.SetBrush(wxBrush(previewFill));
    dc.DrawRectangle(previewRect);

    // Add a subtle inner highlight for depth
    wxRect innerRect = previewRect;
    innerRect.Deflate(2);
    dc.SetPen(wxPen(wxColour(255, 255, 255, 255), 1));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(innerRect);

    // Add direction arrow for better clarity
    // Removed: Red arrows are no longer needed for direction indicators
    // if (isDirectionIndicator) {
    //     drawDirectionArrow(dc, previewRect, area);
    // }
}

void DockOverlay::drawAreaIcon(wxDC& dc, const wxRect& rect, DockWidgetArea area, const wxColour& color) {
    // Calculate icon drawing area (centered in rect) - VS style proportions
    int iconSize = 16;  // Slightly larger for better visibility
    int arrowSize = 6;  // Arrow head size
    int lineWidth = 2;  // Slightly thicker lines for VS style

    wxPoint center(rect.x + rect.width / 2, rect.y + rect.height / 2);

    // Check if this area is currently hovered for red feedback
    bool isHovered = false;
    for (const auto& dropArea : m_dropAreas) {
        if (dropArea->area() == area && dropArea->isHighlighted()) {
            isHovered = true;
            break;
        }
    }

    // Use red color for hovered areas, otherwise use the provided color - Force opaque
    wxColour iconColor = isHovered ? wxColour(255, 0, 0, 255) : wxColour(color.Red(), color.Green(), color.Blue(), 255);

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

wxRect DockOverlay::dropIndicatorRect(DockWidgetArea area) const {
    return areaRect(area);
}

wxRect DockOverlay::targetRect() const {
    if (!m_targetWidget) {
        return wxRect();
    }
    
    wxPoint pos = m_targetWidget->GetScreenPosition();
    wxSize size = m_targetWidget->GetSize();
    return wxRect(pos, size);
}

wxRect DockOverlay::areaRect(DockWidgetArea area) const {
    wxSize size = GetClientSize();
    int dropSize = 32;  // Match the size from updateDropAreaPositions
    int margin = 8;     // Match the margin from updateDropAreaPositions

    // Different layouts for different overlay modes - VS style positioning
    if (m_mode == ModeDockAreaOverlay) {
        // For DockArea overlay: indicators positioned like VS - more spread out
        int centerX = size.GetWidth() / 2;
        int centerY = size.GetHeight() / 2;
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
            return wxRect((size.GetWidth() - dropSize) / 2, margin, dropSize, dropSize);
        case BottomDockWidgetArea:
            return wxRect((size.GetWidth() - dropSize) / 2, size.GetHeight() - margin - dropSize, dropSize, dropSize);
        case LeftDockWidgetArea:
            return wxRect(margin, (size.GetHeight() - dropSize) / 2, dropSize, dropSize);
        case RightDockWidgetArea:
            return wxRect(size.GetWidth() - margin - dropSize, (size.GetHeight() - dropSize) / 2, dropSize, dropSize);
        case CenterDockWidgetArea:
            return wxRect((size.GetWidth() - dropSize) / 2, (size.GetHeight() - dropSize) / 2, dropSize, dropSize);
        default:
            return wxRect();
        }
    }
}

wxBitmap DockOverlay::createDropIndicatorBitmap(DockWidgetArea area, int size) {
    wxBitmap bitmap(size, size);
    wxMemoryDC dc(bitmap);
    
    // Clear background
    dc.SetBackground(*wxWHITE_BRUSH);
    dc.Clear();
    
    // Draw indicator
    dc.SetPen(wxPen(m_frameColor, 2));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    
    // Draw based on area
    int center = size / 2;
    switch (area) {
    case TopDockWidgetArea:
    case BottomDockWidgetArea:
    case LeftDockWidgetArea:
    case RightDockWidgetArea:
        dc.DrawRectangle(5, 5, size - 10, size - 10);
        break;
    case CenterDockWidgetArea:
        dc.DrawCircle(center, center, center - 5);
        break;
    }
    
    dc.SelectObject(wxNullBitmap);
    return bitmap;
}

// DockOverlayCross implementation
wxBEGIN_EVENT_TABLE(DockOverlayCross, wxWindow)
    EVT_PAINT(DockOverlayCross::onPaint)
    EVT_MOTION(DockOverlayCross::onMouseMove)
wxEND_EVENT_TABLE()

DockOverlayCross::DockOverlayCross(DockOverlay* overlay)
    : wxWindow(overlay, wxID_ANY)
    , m_overlay(overlay)
    , m_iconSize(40)
    , m_iconColor(wxColour(58, 135, 173))
    , m_hoveredArea(InvalidDockWidgetArea)
{
    // TEMPORARILY CHANGED FOR TESTING - SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundStyle(wxBG_STYLE_COLOUR);
    SetSize(m_iconSize * 5, m_iconSize * 5);
}

DockOverlayCross::~DockOverlayCross() {
}

void DockOverlayCross::updatePosition() {
    if (!m_overlay->targetWidget()) {
        return;
    }
    
    wxSize parentSize = GetParent()->GetSize();
    wxSize mySize = GetSize();
    
    SetPosition(wxPoint(
        (parentSize.GetWidth() - mySize.GetWidth()) / 2,
        (parentSize.GetHeight() - mySize.GetHeight()) / 2
    ));
}

DockWidgetArea DockOverlayCross::cursorLocation() const {
    wxPoint mousePos = wxGetMousePosition();
    wxPoint localPos = ScreenToClient(mousePos);
    
    // Check each area
    if (areaRect(TopDockWidgetArea).Contains(localPos)) {
        return TopDockWidgetArea;
    }
    if (areaRect(BottomDockWidgetArea).Contains(localPos)) {
        return BottomDockWidgetArea;
    }
    if (areaRect(LeftDockWidgetArea).Contains(localPos)) {
        return LeftDockWidgetArea;
    }
    if (areaRect(RightDockWidgetArea).Contains(localPos)) {
        return RightDockWidgetArea;
    }
    if (areaRect(CenterDockWidgetArea).Contains(localPos)) {
        return CenterDockWidgetArea;
    }
    
    return InvalidDockWidgetArea;
}

void DockOverlayCross::onPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    
    drawCrossIcon(dc);
}

void DockOverlayCross::onMouseMove(wxMouseEvent& event) {
    DockWidgetArea newArea = cursorLocation();
    
    if (newArea != m_hoveredArea) {
        DockWidgetArea oldArea = m_hoveredArea;  // Store the old area before updating
        m_hoveredArea = newArea;
        
        wxLogDebug("DockOverlayCross::onMouseMove - Area changed from %d to %d", oldArea, newArea);
        
        // Only refresh the affected areas instead of the whole window
        wxRect oldRect = areaRect(oldArea);
        wxRect newRect = areaRect(newArea);
        RefreshRect(oldRect.Union(newRect), false);
    }
}

void DockOverlayCross::drawCrossIcon(wxDC& dc) {
    // Draw center
    drawAreaIndicator(dc, CenterDockWidgetArea);
    
    // Draw sides
    drawAreaIndicator(dc, TopDockWidgetArea);
    drawAreaIndicator(dc, BottomDockWidgetArea);
    drawAreaIndicator(dc, LeftDockWidgetArea);
    drawAreaIndicator(dc, RightDockWidgetArea);
}

void DockOverlayCross::drawAreaIndicator(wxDC& dc, DockWidgetArea area) {
    wxRect rect = areaRect(area);
    
    // Force opaque rendering by drawing a solid white background first
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(wxColour(255, 255, 255, 255)));  // Solid white background
    dc.DrawRectangle(rect);
    
    if (m_hoveredArea == area) {
        // Use red color for hovered areas
        wxLogDebug("DockOverlayCross::drawAreaIndicator - Drawing RED hovered area %d", area);
        dc.SetPen(wxPen(wxColour(255, 0, 0, 255), 3));  // Red border, thicker for better visibility
        dc.SetBrush(wxBrush(wxColour(255, 0, 0, 255)));  // Red fill, fully opaque
    } else {
        dc.SetPen(wxPen(wxColour(m_iconColor.Red(), m_iconColor.Green(), m_iconColor.Blue(), 255), 1));
        dc.SetBrush(wxBrush(wxColour(255, 255, 255, 255)));  // White fill for non-hovered areas
    }
    
    dc.DrawRectangle(rect);
}

wxRect DockOverlayCross::areaRect(DockWidgetArea area) const {
    wxSize size = GetSize();
    int center = size.GetWidth() / 2;
    
    switch (area) {
    case TopDockWidgetArea:
        return wxRect(center - m_iconSize/2, 0, m_iconSize, m_iconSize);
    case BottomDockWidgetArea:
        return wxRect(center - m_iconSize/2, size.GetHeight() - m_iconSize, m_iconSize, m_iconSize);
    case LeftDockWidgetArea:
        return wxRect(0, center - m_iconSize/2, m_iconSize, m_iconSize);
    case RightDockWidgetArea:
        return wxRect(size.GetWidth() - m_iconSize, center - m_iconSize/2, m_iconSize, m_iconSize);
    case CenterDockWidgetArea:
        return wxRect(center - m_iconSize/2, center - m_iconSize/2, m_iconSize, m_iconSize);
    default:
        return wxRect();
    }
}

// Global docking mode implementation
void DockOverlay::updateGlobalMode() {
    if (m_isGlobalMode) {
        // In global mode, show all areas more prominently
        // TEMPORARILY DISABLED FOR TESTING - SetTransparent(m_globalTransparency); // Use configured global transparency

        // Enable all areas in global mode
        m_allowedAreas = AllDockAreas;
        updateDropAreas();

        // Adjust colors for global mode
        m_frameColor = m_borderColor; // Use configured border color
        m_areaColor = wxColour(m_borderColor.Red(), m_borderColor.Green(), m_borderColor.Blue(), 255); // Fully opaque

        // Ensure overlay stays on top in global mode
        SetWindowStyleFlag(GetWindowStyleFlag() | wxSTAY_ON_TOP);
        Raise();
        SetFocus(); // Ensure focus for proper z-order

        wxLogDebug("DockOverlay: Enabled global docking mode - ensuring topmost");
    } else {
        // Normal mode
        // TEMPORARILY DISABLED FOR TESTING - SetTransparent(m_transparency); // Use configured normal transparency

        // Reset to default areas
        m_allowedAreas = AllDockAreas;

        // Reset colors
        m_frameColor = m_borderColor;
        m_areaColor = wxColour(m_borderColor.Red(), m_borderColor.Green(), m_borderColor.Blue(), 255);

        wxLogDebug("DockOverlay: Disabled global docking mode");
    }

    updateDropAreaPositions();
    requestRefresh();
}

void DockOverlay::showDragHints(DockWidget* draggedWidget) {
    if (!draggedWidget) return;

    wxLogDebug("DockOverlay::showDragHints - widget: %p", draggedWidget);

    // Show contextual hints based on widget type and current layout
    // For now, just update the overlay to show all available areas
    updateDropAreas();
    requestRefresh();
}

void DockOverlay::updateDragHints() {
    // Update hints based on current drag state
    // This could include showing preview areas, highlighting valid drop zones, etc.
    requestRefresh();
}

// Global mode hints drawing
void DockOverlay::drawGlobalModeHints(wxDC& dc) {
    wxRect clientRect = GetClientRect();

    // Draw screen edge indicators for global docking
    wxColour edgeColor(0, 122, 204, 255);  // VS blue, fully opaque
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
    for (const auto& dropArea : m_dropAreas) {
        if (dropArea->area() == CenterDockWidgetArea && dropArea->isVisible()) {
            wxRect centerRect = dropArea->rect();
            // Inflate center area slightly in global mode for better visibility
            centerRect.Inflate(2);

            // Draw enhanced center indicator
            dc.SetPen(wxPen(wxColour(0, 122, 204, 255), 2));
            dc.SetBrush(wxBrush(wxColour(0, 122, 204, 255)));
            dc.DrawRoundedRectangle(centerRect, 6);

            break;
        }
    }

    // Draw text hints for global docking
    drawGlobalModeTextHints(dc);
}

void DockOverlay::drawGlobalModeTextHints(wxDC& dc) {
    if (!m_targetWidget) return;

    wxRect clientRect = GetClientRect();
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
    dc.SetBrush(wxBrush(wxColour(255, 255, 255, 255)));
    dc.SetPen(wxPen(wxColour(200, 200, 200), 1));
    dc.DrawRoundedRectangle(textBgRect, 3);

    // Draw the hint text
    dc.DrawText(hintText, textX, textY);
}

wxRect DockOverlay::getPreviewRect(DockWidgetArea area) const {
    if (!m_targetWidget) {
        return wxRect();
    }
    
    wxRect clientRect = GetClientRect();
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

// Performance optimization methods
void DockOverlay::optimizeRendering() {
    // Reduce rendering frequency for better performance
    if (m_optimizedRendering) {
        return;
    }

    m_optimizedRendering = true;

    // Disable unnecessary features during drag operations
    // TEMPORARILY CHANGED FOR TESTING - SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundStyle(wxBG_STYLE_COLOUR);

    // Optimize double buffering
    if (GetParent() != nullptr) {
        // Use parent's DC for better performance
        // This reduces the overhead of creating new DCs
    }

    // Cache drop area geometries for better performance
    updateDropAreaGeometryCache();
}

void DockOverlay::setRenderingOptimization(bool enabled) {
    m_optimizedRendering = enabled;
}

void DockOverlay::updateDropAreaGeometryCache() {
    // Cache drop area geometries to avoid repeated calculations
    m_cachedGeometries.clear();

    wxSize size = GetSize();
    int dropSize = 32;  // Match VS-style size
    int margin = 8;     // Match VS-style margin

    // Cache center area
    m_cachedGeometries[CenterDockWidgetArea] = wxRect(
        (size.GetWidth() - dropSize) / 2,
        (size.GetHeight() - dropSize) / 2,
        dropSize,
        dropSize
    );

    // Cache side areas
    m_cachedGeometries[TopDockWidgetArea] = wxRect(
        (size.GetWidth() - dropSize) / 2,
        margin,
        dropSize,
        dropSize
    );

    m_cachedGeometries[BottomDockWidgetArea] = wxRect(
        (size.GetWidth() - dropSize) / 2,
        size.GetHeight() - margin - dropSize,
        dropSize,
        dropSize
    );

    m_cachedGeometries[LeftDockWidgetArea] = wxRect(
        margin,
        (size.GetHeight() - dropSize) / 2,
        dropSize,
        dropSize
    );

    m_cachedGeometries[RightDockWidgetArea] = wxRect(
        size.GetWidth() - margin - dropSize,
        (size.GetHeight() - dropSize) / 2,
        dropSize,
        dropSize
    );
}

// Enhanced debounce mechanism implementation
void DockOverlay::requestRefresh() {
    wxLongLong currentTime = wxGetLocalTimeMillis();

    // Check if we're refreshing too frequently
    if (m_lastRefreshTime > 0) {
        wxLongLong timeSinceLastRefresh = currentTime - m_lastRefreshTime;
        if (timeSinceLastRefresh < (1000 / MAX_REFRESHES_PER_SECOND)) {
            // Too frequent, schedule for later
            if (!m_pendingRefresh) {
                m_pendingRefresh = true;
                int delay = (1000 / MAX_REFRESHES_PER_SECOND) - timeSinceLastRefresh.GetValue();
                m_refreshTimer->Start(std::max(delay, 1), wxTIMER_ONE_SHOT);
            }
            return;
        }
    }

    // Normal refresh scheduling
    if (!m_pendingRefresh) {
        m_pendingRefresh = true;
        m_refreshTimer->Start(REFRESH_DEBOUNCE_MS, wxTIMER_ONE_SHOT);
    }
}

void DockOverlay::onRefreshTimer(wxTimerEvent& event) {
    if (m_pendingRefresh) {
        m_pendingRefresh = false;

        // Check if we should actually refresh (performance optimization)
        if (shouldRefreshNow()) {
            Refresh();
            Update();
            m_lastRefreshTime = wxGetLocalTimeMillis();
            m_refreshCount++;
        }
    }
}

bool DockOverlay::shouldRefreshNow() const {
    // Skip refresh if overlay is not visible
    if (!IsShown()) {
        return false;
    }

    // Skip refresh if no target widget
    if (!m_targetWidget) {
        return false;
    }

    // Always refresh in global mode for better responsiveness
    if (m_isGlobalMode) {
        return true;
    }

    // Throttle normal mode refreshes
    wxLongLong currentTime = wxGetLocalTimeMillis();
    if (m_lastRefreshTime > 0) {
        wxLongLong timeSinceLastRefresh = currentTime - m_lastRefreshTime;
        // Allow refresh if enough time has passed
        return timeSinceLastRefresh >= REFRESH_DEBOUNCE_MS;
    }

    return true;
}

// Configuration methods implementation
void DockOverlay::loadConfiguration() {
    // Load configuration from ThemeManager
    auto& themeManager = ThemeManager::getInstance();
    
    // Load transparency settings with fallback defaults (20% transparency)
    int overlayTransparency = themeManager.getInt("OverlayTransparency");
    m_transparency = (overlayTransparency > 0) ? overlayTransparency : 51;
    
    int globalTransparency = themeManager.getInt("GlobalModeTransparency");
    m_globalTransparency = (globalTransparency > 0) ? globalTransparency : 51;
    
    // Load background colors with fallback defaults (20% transparency)
    wxColour bgColor = themeManager.getColour("BackgroundColour");
    m_backgroundColor = bgColor.IsOk() ? bgColor : wxColour(240, 240, 240, 51);
    
    wxColour globalBgColor = themeManager.getColour("GlobalBackgroundColour");
    m_globalBackgroundColor = globalBgColor.IsOk() ? globalBgColor : wxColour(200, 220, 240, 51);
    
    // Load border settings with fallback defaults
    wxColour borderColor = themeManager.getColour("BorderColour");
    m_borderColor = borderColor.IsOk() ? borderColor : wxColour(0, 122, 204);
    
    int borderWidth = themeManager.getInt("BorderWidth");
    m_borderWidth = (borderWidth > 0) ? borderWidth : 2;
    
    // Load drop area colors with fallback defaults (20% transparency)
    wxColour normalBg = themeManager.getColour("DropAreaNormalBgColour");
    m_dropAreaNormalBg = normalBg.IsOk() ? normalBg : wxColour(255, 255, 255, 255);
    
    wxColour normalBorder = themeManager.getColour("DropAreaNormalBorderColour");
    m_dropAreaNormalBorder = normalBorder.IsOk() ? normalBorder : wxColour(217, 217, 217);
    
    wxColour highlightBg = themeManager.getColour("DropAreaHighlightBgColour");
    m_dropAreaHighlightBg = highlightBg.IsOk() ? highlightBg : wxColour(0, 122, 204, 255);
    
    wxColour highlightBorder = themeManager.getColour("DropAreaHighlightBorderColour");
    m_dropAreaHighlightBorder = highlightBorder.IsOk() ? highlightBorder : wxColour(0, 122, 204);
    
    // Load icon colors with fallback defaults
    wxColour iconColor = themeManager.getColour("DropAreaIconColour");
    m_dropAreaIconColor = iconColor.IsOk() ? iconColor : wxColour(96, 96, 96);
    
    wxColour highlightIconColor = themeManager.getColour("DropAreaHighlightIconColour");
    m_dropAreaHighlightIconColor = highlightIconColor.IsOk() ? highlightIconColor : wxColour(255, 255, 255);
    
    // Load other settings with fallback defaults
    int cornerRadius = themeManager.getInt("CornerRadius");
    m_cornerRadius = (cornerRadius > 0) ? cornerRadius : 4;
    
    int refreshDelay = themeManager.getInt("RefreshDelay");
    m_refreshDelay = (refreshDelay > 0) ? refreshDelay : 16;
    
    // Update frame color and area color to match border color
    m_frameColor = m_borderColor;
    m_areaColor = wxColour(m_borderColor.Red(), m_borderColor.Green(), m_borderColor.Blue(), 255);
    
    // Debug output
    wxLogDebug("DockOverlay configuration loaded - Transparency: %d, Global: %d, BorderWidth: %d, CornerRadius: %d", 
               m_transparency, m_globalTransparency, m_borderWidth, m_cornerRadius);
}

void DockOverlay::setTransparency(int transparency) {
    m_transparency = transparency;
    // TEMPORARILY DISABLED FOR TESTING - SetTransparent(transparency);
}

void DockOverlay::setBackgroundColor(const wxColour& color) {
    m_backgroundColor = color;
}

void DockOverlay::setGlobalBackgroundColor(const wxColour& color) {
    m_globalBackgroundColor = color;
}

void DockOverlay::setBorderColor(const wxColour& color) {
    m_borderColor = color;
    m_frameColor = color;
    m_areaColor = wxColour(color.Red(), color.Green(), color.Blue(), 255);
}

void DockOverlay::setBorderWidth(int width) {
    m_borderWidth = width;
}

void DockOverlay::setCornerRadius(int radius) {
    m_cornerRadius = radius;
}

void DockOverlay::setRefreshDelay(int delayMs) {
    m_refreshDelay = delayMs;
}

// Draw direction indicators on top of everything else
void DockOverlay::drawDirectionIndicators(wxDC& dc) {
    // Only draw direction indicators for highlighted drop areas
    for (const auto& dropArea : m_dropAreas) {
        if (dropArea->isVisible() && dropArea->isHighlighted()) {
            drawPreviewArea(dc, dropArea->area(), true);  // Direction indicator preview
        }
    }
}

// Draw direction arrow in the preview area
void DockOverlay::drawDirectionArrow(wxDC& dc, const wxRect& rect, DockWidgetArea area) {
    wxPoint center(rect.x + rect.width / 2, rect.y + rect.height / 2);
    int arrowSize = 8;
    int lineWidth = 3;
    
    // Check if this area is currently hovered for red feedback
    bool isHovered = false;
    for (const auto& dropArea : m_dropAreas) {
        if (dropArea->area() == area && dropArea->isHighlighted()) {
            isHovered = true;
            break;
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

bool DockOverlay::isMouseOverIcon(const wxPoint& mousePos, const wxRect& buttonRect, DockWidgetArea area) {
    // For direction indicators, we want to respond immediately when mouse enters the button area
    // Use the entire button rectangle for easier detection
    return buttonRect.Contains(mousePos);
}

} // namespace ads

