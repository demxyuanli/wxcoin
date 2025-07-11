#include "flatui/FlatUIPinButton.h"
#include "config/SvgIconManager.h"
#include "config/ThemeManager.h"
#include "logger/Logger.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>



// Define the custom event
wxDEFINE_EVENT(wxEVT_PIN_BUTTON_CLICKED, wxCommandEvent);

wxBEGIN_EVENT_TABLE(FlatUIPinButton, wxControl)
    EVT_PAINT(FlatUIPinButton::OnPaint)
    EVT_SIZE(FlatUIPinButton::OnSize)
    EVT_MOTION(FlatUIPinButton::OnMouseMove)
    EVT_LEAVE_WINDOW(FlatUIPinButton::OnMouseLeave)
    EVT_LEFT_DOWN(FlatUIPinButton::OnLeftDown)
wxEND_EVENT_TABLE()

FlatUIPinButton::FlatUIPinButton(wxWindow* parent, wxWindowID id,
                                 const wxPoint& pos, const wxSize& size, long style)
    : wxControl(parent, id, pos, size, style | wxBORDER_NONE),
      m_iconHover(false)
{
    SetName("FlatUIPinButton");
    SetDoubleBuffered(true);
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    
    // Set minimum size based on SVG or fallback to default
    wxSize bestSize = DoGetBestSize();
    SetMinSize(bestSize);
    SetSize(bestSize);
    
    LOG_INF("FlatUIPinButton created", "FlatUIPinButton");
}

FlatUIPinButton::~FlatUIPinButton()
{
    LOG_INF("FlatUIPinButton destroyed", "FlatUIPinButton");
}

wxSize FlatUIPinButton::DoGetBestSize() const
{
    try {
        auto& iconManager = SvgIconManager::GetInstance();
        if (iconManager.HasIcon("thumbtack")) {
            // Use a larger fixed size to ensure visibility
            int padding = 4; // 4px padding on each side
            return wxSize(12 + padding * 2, 12 + padding * 2); // 20x20 total
        }
    } catch (...) {
        // Fall through to default size
    }
    
    // Fallback to a larger default size
    return wxSize(CONTROL_WIDTH, CONTROL_HEIGHT);
}

void FlatUIPinButton::NotifyPinClicked()
{
    // Send custom event to parent
    wxCommandEvent event(wxEVT_PIN_BUTTON_CLICKED, GetId());
    event.SetEventObject(this);
    
    wxWindow* parent = GetParent();
    if (parent) {
        parent->GetEventHandler()->ProcessEvent(event);
    }
}

void FlatUIPinButton::OnPaint(wxPaintEvent& evt)
{
    wxAutoBufferedPaintDC dc(this);

    // Clear background
    dc.SetBackground(wxBrush(CFG_COLOUR("PinButtonBgColour")));
    dc.Clear();

    // Draw unpin icon
    DrawPinIcon(dc);
}

void FlatUIPinButton::OnSize(wxSizeEvent& evt)
{
    Refresh();
    evt.Skip();
}

void FlatUIPinButton::OnMouseMove(wxMouseEvent& evt)
{
    bool wasHover = m_iconHover;
    m_iconHover = true;
    
    if (m_iconHover != wasHover) {
        Refresh();
        SetCursor(wxCursor(wxCURSOR_HAND));
    }
    
    evt.Skip();
}

void FlatUIPinButton::OnMouseLeave(wxMouseEvent& evt)
{
    if (m_iconHover) {
        m_iconHover = false;
        Refresh();
        SetCursor(wxCursor(wxCURSOR_ARROW));
    }
    evt.Skip();
}

void FlatUIPinButton::OnLeftDown(wxMouseEvent& evt)
{
    NotifyPinClicked();
    // Don't skip - we handled this event
}

void FlatUIPinButton::DrawPinIcon(wxDC& dc)
{
    // Try to draw SVG icon first
    bool drewSvg = false;
    
    try {
        DrawSvgIcon(dc);
        drewSvg = true;
    } catch (...) {
        // Fall through to fallback drawing
    }
    
    if (!drewSvg) {
        DrawFallbackIcon(dc);
    }
}

void FlatUIPinButton::DrawSvgIcon(wxDC& dc)
{
    auto& iconManager = SvgIconManager::GetInstance();
    wxBitmap iconBitmap = iconManager.GetIconBitmap("thumbtack", wxSize(8, 8));

    if (iconBitmap.IsOk()) {
        wxRect clientRect = GetClientRect();
        // Center the icon in the control
        wxPoint iconPos = clientRect.GetPosition() +
            wxSize((clientRect.GetWidth() - iconBitmap.GetWidth()) / 2,
                (clientRect.GetHeight() - iconBitmap.GetHeight()) / 2);
        dc.DrawBitmap(iconBitmap, iconPos);
    }
}

void FlatUIPinButton::DrawFallbackIcon(wxDC& dc)
{
    wxRect clientRect = GetClientRect();
    wxPoint center = clientRect.GetPosition() + wxSize(clientRect.GetWidth() / 2, clientRect.GetHeight() / 2);

    wxColour iconColor = CFG_COLOUR("BarActiveTextColour");
    dc.SetPen(wxPen(iconColor, 2));
    dc.SetBrush(wxBrush(iconColor));

    // Draw up arrow icon
    wxPoint arrow[3];
    arrow[0] = wxPoint(center.x, center.y - 4);     // Top point
    arrow[1] = wxPoint(center.x - 4, center.y + 2); // Bottom left
    arrow[2] = wxPoint(center.x + 4, center.y + 2); // Bottom right

    dc.DrawPolygon(3, arrow);
} 
