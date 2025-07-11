#include "flatui/FlatUIUnpinButton.h"
#include "config/SvgIconManager.h"
#include "config/ThemeManager.h"
#include "logger/Logger.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>



// Define the custom event
wxDEFINE_EVENT(wxEVT_UNPIN_BUTTON_CLICKED, wxCommandEvent);

wxBEGIN_EVENT_TABLE(FlatUIUnpinButton, wxControl)
    EVT_PAINT(FlatUIUnpinButton::OnPaint)
    EVT_SIZE(FlatUIUnpinButton::OnSize)
    EVT_MOTION(FlatUIUnpinButton::OnMouseMove)
    EVT_LEAVE_WINDOW(FlatUIUnpinButton::OnMouseLeave)
    EVT_LEFT_DOWN(FlatUIUnpinButton::OnLeftDown)
wxEND_EVENT_TABLE()

FlatUIUnpinButton::FlatUIUnpinButton(wxWindow* parent, wxWindowID id,
                                     const wxPoint& pos, const wxSize& size, long style)
    : wxControl(parent, id, pos, size, style | wxBORDER_NONE),
      m_iconHover(false)
{
    SetName("FlatUIUnpinButton");
    SetDoubleBuffered(true);
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    
    // Set minimum size based on SVG or fallback to default
    wxSize bestSize = DoGetBestSize();
    SetMinSize(bestSize);
    SetSize(bestSize);
    
    LOG_INF("FlatUIUnpinButton created", "FlatUIUnpinButton");
}

FlatUIUnpinButton::~FlatUIUnpinButton()
{
    LOG_INF("FlatUIUnpinButton destroyed", "FlatUIUnpinButton");
}

wxSize FlatUIUnpinButton::DoGetBestSize() const
{
    try {
        auto& iconManager = SvgIconManager::GetInstance();
        if (iconManager.HasIcon("up")) {
            // Use a larger fixed size to ensure visibility
            int padding = 4; // 4px padding on each side
            return wxSize(12 + padding * 2, 12 + padding * 2); // 20x20 total
        }
    } catch (...) {
        // Fall through to default size
    }
    
    // Fallback to default size if SVG not available
    return wxSize(CONTROL_WIDTH, CONTROL_HEIGHT);
}

void FlatUIUnpinButton::NotifyUnpinClicked()
{
    // Send custom event to parent
    wxCommandEvent event(wxEVT_UNPIN_BUTTON_CLICKED, GetId());
    event.SetEventObject(this);
    
    wxWindow* parent = GetParent();
    if (parent) {
        parent->GetEventHandler()->ProcessEvent(event);
    }
}

void FlatUIUnpinButton::OnPaint(wxPaintEvent& evt)
{
    wxAutoBufferedPaintDC dc(this);

    // Clear background
    dc.SetBackground(wxBrush(CFG_COLOUR("UnpinButtonBgColour")));
    dc.Clear();

    // Draw unpin icon
    DrawUnpinIcon(dc);
}

void FlatUIUnpinButton::OnSize(wxSizeEvent& evt)
{
    Refresh();
    evt.Skip();
}

void FlatUIUnpinButton::OnMouseMove(wxMouseEvent& evt)
{
    bool wasHover = m_iconHover;
    m_iconHover = true;
    
    if (m_iconHover != wasHover) {
        Refresh();
        SetCursor(wxCursor(wxCURSOR_HAND));
    }
    
    evt.Skip();
}

void FlatUIUnpinButton::OnMouseLeave(wxMouseEvent& evt)
{
    if (m_iconHover) {
        m_iconHover = false;
        Refresh();
        SetCursor(wxCursor(wxCURSOR_ARROW));
    }
    evt.Skip();
}

void FlatUIUnpinButton::OnLeftDown(wxMouseEvent& evt)
{
    NotifyUnpinClicked();
    // Don't skip - we handled this event
}

void FlatUIUnpinButton::DrawUnpinIcon(wxDC& dc)
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

void FlatUIUnpinButton::DrawSvgIcon(wxDC& dc)
{
    auto& iconManager = SvgIconManager::GetInstance();
    wxBitmap iconBitmap = iconManager.GetIconBitmap("up", wxSize(8,8));
    
    if (iconBitmap.IsOk()) {
        wxRect clientRect = GetClientRect();
        // Center the icon in the control
        wxPoint iconPos = clientRect.GetPosition() + 
                         wxSize((clientRect.GetWidth() - iconBitmap.GetWidth())/2, 
                               (clientRect.GetHeight() - iconBitmap.GetHeight())/2);
        dc.DrawBitmap(iconBitmap, iconPos);
    }
}

void FlatUIUnpinButton::DrawFallbackIcon(wxDC& dc)
{
    wxRect clientRect = GetClientRect();
    wxPoint center = clientRect.GetPosition() + wxSize(clientRect.GetWidth()/2, clientRect.GetHeight()/2);
    
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
