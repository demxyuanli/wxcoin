#include "widgets/FlatSwitch.h"
#include <wx/dcclient.h>

wxDEFINE_EVENT(wxEVT_FLAT_SWITCH_TOGGLED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_SWITCH_STATE_CHANGED, wxCommandEvent);

BEGIN_EVENT_TABLE(FlatSwitch, wxControl)
    EVT_PAINT(FlatSwitch::OnPaint)
    EVT_SIZE(FlatSwitch::OnSize)
    EVT_LEFT_DOWN(FlatSwitch::OnMouseDown)
    EVT_LEFT_UP(FlatSwitch::OnMouseUp)
    EVT_MOTION(FlatSwitch::OnMouseMove)
    EVT_LEAVE_WINDOW(FlatSwitch::OnMouseLeave)
    EVT_ENTER_WINDOW(FlatSwitch::OnMouseEnter)
END_EVENT_TABLE()

FlatSwitch::FlatSwitch(wxWindow* parent, wxWindowID id, bool value,
                       const wxPoint& pos, const wxSize& size, SwitchStyle style, long style_flags)
    : wxControl(parent, id, pos, size, style_flags | wxBORDER_NONE)
    , m_switchStyle(style)
    , m_state(SwitchState::DEFAULT_STATE)
    , m_enabled(true)
    , m_value(value)
    , m_isPressed(false)
    , m_isHovered(false)
    , m_borderWidth(DEFAULT_BORDER_WIDTH)
    , m_cornerRadius(DEFAULT_CORNER_RADIUS)
    , m_thumbSize(wxSize(DEFAULT_THUMB_SIZE, DEFAULT_THUMB_SIZE))
    , m_trackHeight(DEFAULT_TRACK_HEIGHT)
    , m_animationDuration(DEFAULT_ANIMATION_DURATION)
{
    // Initialize default colors based on style
    InitializeDefaultColors();
    
    // Set default size if not specified
    if (size == wxDefaultSize) {
        SetInitialSize(DoGetBestSize());
    }
}

FlatSwitch::~FlatSwitch()
{
}

void FlatSwitch::InitializeDefaultColors()
{
    // Fluent Design System inspired colors for switches (based on PyQt-Fluent-Widgets)
    switch (m_switchStyle) {
        case SwitchStyle::DEFAULT_STYLE:
            m_backgroundColor = wxColour(200, 200, 200);  // Light gray when unchecked
            m_hoverColor = wxColour(190, 190, 190);       // Slightly darker on hover
            m_checkedColor = wxColour(32, 167, 232);       // Fluent Blue when checked
            m_thumbColor = wxColour(255, 255, 255);       // White thumb
            m_textColor = wxColour(32, 32, 32);           // Dark gray text
            m_borderColor = wxColour(180, 180, 180);      // Light gray border
            break;
            
        case SwitchStyle::ROUND:
            m_backgroundColor = wxColour(200, 200, 200);  // Light gray when unchecked
            m_hoverColor = wxColour(190, 190, 190);       // Slightly darker on hover
            m_checkedColor = wxColour(32, 167, 232);       // Fluent Blue when checked
            m_thumbColor = wxColour(255, 255, 255);       // White thumb
            m_textColor = wxColour(32, 32, 32);           // Dark gray text
            m_borderColor = wxColour(180, 180, 180);      // Light gray border
            break;
            
        case SwitchStyle::SQUARE:
            m_backgroundColor = wxColour(200, 200, 200);  // Light gray when unchecked
            m_hoverColor = wxColour(190, 190, 190);       // Slightly darker on hover
            m_checkedColor = wxColour(32, 167, 232);       // Fluent Blue when checked
            m_thumbColor = wxColour(255, 255, 255);       // White thumb
            m_textColor = wxColour(32, 32, 32);           // Dark gray text
            m_borderColor = wxColour(180, 180, 180);      // Light gray border
            break;
    }
}

void FlatSwitch::SetSwitchStyle(SwitchStyle style)
{
    m_switchStyle = style;
    InitializeDefaultColors();
    this->Refresh();
}

FlatSwitch::SwitchStyle FlatSwitch::GetSwitchStyle() const
{
    return m_switchStyle;
}

void FlatSwitch::SetLabel(const wxString& label)
{
    m_label = label;
    Refresh();
}

wxString FlatSwitch::GetLabel() const
{
    return m_label;
}

void FlatSwitch::SetValue(bool value)
{
    if (m_value != value) {
        m_value = value;
        Refresh();
        
        // Send state changed event
        wxCommandEvent event(wxEVT_FLAT_SWITCH_STATE_CHANGED, GetId());
        event.SetInt(m_value ? 1 : 0);
        ProcessWindowEvent(event);
    }
}

bool FlatSwitch::GetValue() const
{
    return m_value;
}

void FlatSwitch::SetBackgroundColor(const wxColour& color)
{
    m_backgroundColor = color;
    Refresh();
}

wxColour FlatSwitch::GetBackgroundColor() const
{
    return m_backgroundColor;
}

void FlatSwitch::SetTextColor(const wxColour& color)
{
    m_textColor = color;
    Refresh();
}

wxColour FlatSwitch::GetTextColor() const
{
    return m_textColor;
}

void FlatSwitch::SetBorderColor(const wxColour& color)
{
    m_borderColor = color;
    Refresh();
}

wxColour FlatSwitch::GetBorderColor() const
{
    return m_borderColor;
}

void FlatSwitch::SetBorderWidth(int width)
{
    m_borderWidth = width;
    Refresh();
}

int FlatSwitch::GetBorderWidth() const
{
    return m_borderWidth;
}

void FlatSwitch::SetCornerRadius(int radius)
{
    m_cornerRadius = radius;
    Refresh();
}

int FlatSwitch::GetCornerRadius() const
{
    return m_cornerRadius;
}

void FlatSwitch::SetThumbSize(const wxSize& size)
{
    m_thumbSize = size;
    this->Refresh();
}

void FlatSwitch::SetTrackHeight(int height)
{
    m_trackHeight = height;
    this->Refresh();
}

int FlatSwitch::GetTrackHeight() const
{
    return m_trackHeight;
}

void FlatSwitch::SetAnimationDuration(int duration)
{
    m_animationDuration = duration;
}

int FlatSwitch::GetAnimationDuration() const
{
    return m_animationDuration;
}

bool FlatSwitch::Enable(bool enabled)
{
    m_enabled = enabled;
    wxControl::Enable(enabled);
    this->Refresh();
    return true;
}

bool FlatSwitch::IsEnabled() const
{
    return m_enabled;
}

// Missing function implementations
wxSize FlatSwitch::DoGetBestSize() const
{
    int width = m_thumbSize.GetWidth() * 2 + 2 * m_borderWidth;
    int height = m_thumbSize.GetHeight() + 2 * m_borderWidth;
    
    if (!m_label.IsEmpty()) {
        wxSize textSize = GetTextExtent(m_label);
        width += textSize.GetWidth() + 5;
        height = wxMax(height, textSize.GetHeight());
    }
    
    return wxSize(width, height);
}

void FlatSwitch::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);
    DrawSwitch(dc);
}

void FlatSwitch::OnSize(wxSizeEvent& event)
{
    Refresh();
    event.Skip();
}

void FlatSwitch::OnMouseDown(wxMouseEvent& event)
{
    if (m_enabled) {
        m_isPressed = true;
        Refresh();
    }
    event.Skip();
}

void FlatSwitch::OnMouseUp(wxMouseEvent& event)
{
    if (m_enabled && m_isPressed) {
        m_isPressed = false;
        SetValue(!m_value);
        
        // Send toggle event
        wxCommandEvent toggleEvent(wxEVT_FLAT_SWITCH_TOGGLED, this->GetId());
        toggleEvent.SetInt(m_value ? 1 : 0);
        this->ProcessWindowEvent(toggleEvent);
        
        Refresh();
    }
    event.Skip();
}

void FlatSwitch::OnMouseMove(wxMouseEvent& event)
{
    if (m_enabled) {
        bool wasHovered = m_isHovered;
        m_isHovered = true;
        
        if (!wasHovered) {
            Refresh();
        }
    }
    event.Skip();
}

void FlatSwitch::OnMouseLeave(wxMouseEvent& event)
{
    if (m_enabled) {
        m_isHovered = false;
        Refresh();
    }
    event.Skip();
}

void FlatSwitch::OnMouseEnter(wxMouseEvent& event)
{
    if (m_enabled) {
        m_isHovered = true;
        Refresh();
    }
    event.Skip();
}

void FlatSwitch::DrawSwitch(wxDC& dc)
{
    wxRect rect = this->GetClientRect();
    
    // Draw track
    wxColour trackColor = m_value ? m_checkedColor : m_backgroundColor;
    if (m_isHovered) {
        trackColor = m_value ? m_checkedColor : m_hoverColor;
    }
    
    dc.SetBrush(wxBrush(trackColor));
    dc.SetPen(wxPen(m_borderColor, m_borderWidth));
    dc.DrawRoundedRectangle(rect, m_cornerRadius);
    
    // Draw thumb
    int thumbX = m_value ? (rect.width - m_thumbSize.GetWidth() - m_borderWidth) : m_borderWidth;
    wxRect thumbRect(thumbX, m_borderWidth, m_thumbSize.GetWidth(), rect.height - 2 * m_borderWidth);
    
    dc.SetBrush(wxBrush(m_thumbColor));
    dc.SetPen(wxPen(m_borderColor, 1));
    dc.DrawRoundedRectangle(thumbRect, m_cornerRadius);
    
    // Draw label if provided
    if (!m_label.IsEmpty()) {
        dc.SetTextForeground(m_textColor);
        dc.SetFont(this->GetFont());
        
        wxSize textSize = dc.GetTextExtent(m_label);
        int x = rect.width + 5; // Position label to the right of the switch
        int y = (rect.height - textSize.y) / 2;
        
        dc.DrawText(m_label, x, y);
    }
}
