#include "docking/DockOverlay.h"
#include "docking/DockWidget.h"
#include "docking/DockArea.h"
#include "docking/DockContainerWidget.h"
#include <wx/dcbuffer.h>

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
wxEND_EVENT_TABLE()

// DockOverlay implementation
DockOverlay::DockOverlay(wxWindow* parent, eMode mode)
    : wxFrame(parent, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
             wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT | wxBORDER_NONE | wxSTAY_ON_TOP)
    , m_mode(mode)
    , m_targetWidget(nullptr)
    , m_allowedAreas(AllDockAreas)
    , m_lastHoveredArea(InvalidDockWidgetArea)
    , m_frameColor(wxColour(58, 135, 173))  // Blue color
    , m_areaColor(wxColour(58, 135, 173, 128))  // Semi-transparent blue
    , m_frameWidth(3)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetTransparent(200);  // Make overlay semi-transparent
    
    // Create drop areas
    createDropAreas();
}

DockOverlay::~DockOverlay() {
}

void DockOverlay::setAllowedAreas(int areas) {
    m_allowedAreas = areas;
    updateDropAreas();
}

DockWidgetArea DockOverlay::dropAreaUnderCursor() const {
    wxPoint mousePos = wxGetMousePosition();
    wxPoint localPos = ScreenToClient(mousePos);
    
    for (const auto& dropArea : m_dropAreas) {
        if (dropArea->isVisible() && dropArea->contains(localPos)) {
            return dropArea->area();
        }
    }
    
    return InvalidDockWidgetArea;
}

DockWidgetArea DockOverlay::showOverlay(wxWindow* target) {
    if (!target) {
        hideOverlay();
        return InvalidDockWidgetArea;
    }
    
    wxLogDebug("DockOverlay::showOverlay - target: %p", target);
    
    m_targetWidget = target;
    updatePosition();
    
    wxLogDebug("DockOverlay position: %d,%d size: %dx%d", 
        GetPosition().x, GetPosition().y, GetSize().GetWidth(), GetSize().GetHeight());
    
    Show();
    Raise();
    
    // Make sure window is on top
    SetWindowStyleFlag(GetWindowStyleFlag() | wxSTAY_ON_TOP);
    
    // Force a paint event
    Refresh();
    Update();
    
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
    
    // Fill with semi-transparent background for debugging
    dc.SetBrush(wxBrush(wxColour(100, 100, 255, 30)));  // Light blue, semi-transparent
    dc.SetPen(wxPen(wxColour(0, 0, 255), 2));
    dc.DrawRectangle(GetClientRect());
    
    if (!m_targetWidget) {
        return;
    }
    
    wxLogDebug("DockOverlay::onPaint - size: %dx%d", GetSize().GetWidth(), GetSize().GetHeight());
    
    // Draw drop areas
    paintDropAreas(dc);
}

void DockOverlay::onMouseMove(wxMouseEvent& event) {
    DockWidgetArea hoveredArea = InvalidDockWidgetArea;
    wxPoint pos = event.GetPosition();
    
    // Find hovered drop area
    for (auto& dropArea : m_dropAreas) {
        if (dropArea->isVisible() && dropArea->contains(pos)) {
            hoveredArea = dropArea->area();
            break;
        }
    }
    
    // Update highlights
    if (hoveredArea != m_lastHoveredArea) {
        for (auto& dropArea : m_dropAreas) {
            dropArea->setHighlighted(dropArea->area() == hoveredArea);
        }
        
        m_lastHoveredArea = hoveredArea;
        Refresh();
    }
}

void DockOverlay::onMouseLeave(wxMouseEvent& event) {
    // Clear all highlights
    for (auto& dropArea : m_dropAreas) {
        dropArea->setHighlighted(false);
    }
    
    m_lastHoveredArea = InvalidDockWidgetArea;
    Refresh();
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
    int dropSize = 40;  // Size of drop indicators
    int margin = 10;
    
    // Update positions for each drop area
    for (auto& dropArea : m_dropAreas) {
        wxRect rect = areaRect(dropArea->area());
        dropArea.reset(new DockOverlayDropArea(dropArea->area(), rect));
    }
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
    
    if (dropArea.isHighlighted()) {
        // Draw highlighted drop area
        dc.SetPen(wxPen(m_frameColor, m_frameWidth));
        dc.SetBrush(wxBrush(m_areaColor));
    } else {
        // Draw normal drop area
        dc.SetPen(wxPen(m_frameColor, 1));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
    }
    
    // Draw rectangle
    dc.DrawRectangle(rect);
    
    // Draw direction arrow
    wxPoint center(rect.x + rect.width / 2, rect.y + rect.height / 2);
    dc.SetPen(wxPen(m_frameColor, 2));
    
    switch (dropArea.area()) {
    case TopDockWidgetArea:
        dc.DrawLine(center.x, center.y + 10, center.x, center.y - 10);
        dc.DrawLine(center.x, center.y - 10, center.x - 5, center.y - 5);
        dc.DrawLine(center.x, center.y - 10, center.x + 5, center.y - 5);
        break;
    case BottomDockWidgetArea:
        dc.DrawLine(center.x, center.y - 10, center.x, center.y + 10);
        dc.DrawLine(center.x, center.y + 10, center.x - 5, center.y + 5);
        dc.DrawLine(center.x, center.y + 10, center.x + 5, center.y + 5);
        break;
    case LeftDockWidgetArea:
        dc.DrawLine(center.x + 10, center.y, center.x - 10, center.y);
        dc.DrawLine(center.x - 10, center.y, center.x - 5, center.y - 5);
        dc.DrawLine(center.x - 10, center.y, center.x - 5, center.y + 5);
        break;
    case RightDockWidgetArea:
        dc.DrawLine(center.x - 10, center.y, center.x + 10, center.y);
        dc.DrawLine(center.x + 10, center.y, center.x + 5, center.y - 5);
        dc.DrawLine(center.x + 10, center.y, center.x + 5, center.y + 5);
        break;
    case CenterDockWidgetArea:
        dc.DrawRectangle(center.x - 8, center.y - 8, 16, 16);
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
    int dropSize = 60;
    int margin = 20;
    
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
    SetBackgroundStyle(wxBG_STYLE_PAINT);
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
        m_hoveredArea = newArea;
        Refresh();
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
    
    if (m_hoveredArea == area) {
        dc.SetPen(wxPen(m_iconColor, 2));
        dc.SetBrush(wxBrush(m_iconColor));
    } else {
        dc.SetPen(wxPen(m_iconColor, 1));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
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

} // namespace ads
