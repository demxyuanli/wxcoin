#include "widgets/FlatSlider.h"
#include <wx/slider.h>

wxDEFINE_EVENT(wxEVT_FLAT_SLIDER_VALUE_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_SLIDER_THUMB_DRAGGED, wxCommandEvent);

BEGIN_EVENT_TABLE(FlatSlider, wxControl)
    EVT_PAINT(FlatSlider::OnPaint)
    EVT_SIZE(FlatSlider::OnSize)
    EVT_LEFT_DOWN(FlatSlider::OnMouseDown)
    EVT_MOTION(FlatSlider::OnMouseMove)
    EVT_LEAVE_WINDOW(FlatSlider::OnMouseLeave)
    EVT_ENTER_WINDOW(FlatSlider::OnMouseEnter)
    EVT_SET_FOCUS(FlatSlider::OnFocus)
    EVT_KILL_FOCUS(FlatSlider::OnKillFocus)
END_EVENT_TABLE()

FlatSlider::FlatSlider(wxWindow* parent, wxWindowID id, int value, int minValue, int maxValue,
                       const wxPoint& pos, const wxSize& size, SliderStyle style, long style_flags)
    : wxControl(parent, id, pos, size, style_flags | wxBORDER_NONE)
    , m_sliderStyle(style)
    , m_state(SliderState::DEFAULT_STATE)
    , m_enabled(true)
    , m_showValue(false)
    , m_showTicks(false)
    , m_borderWidth(DEFAULT_BORDER_WIDTH)
    , m_cornerRadius(DEFAULT_CORNER_RADIUS)
    , m_thumbSize(wxSize(DEFAULT_THUMB_SIZE, DEFAULT_THUMB_SIZE))
    , m_trackHeight(DEFAULT_TRACK_HEIGHT)
    , m_value(value)
    , m_minValue(minValue)
    , m_maxValue(maxValue)
{
    // Initialize default colors based on style
    InitializeDefaultColors();
    
    // Set default size if not specified
    if (size == wxDefaultSize) {
        SetInitialSize(DoGetBestSize());
    }
}

FlatSlider::~FlatSlider()
{
}

void FlatSlider::InitializeDefaultColors()
{
    // Fluent Design System inspired colors for sliders (based on PyQt-Fluent-Widgets)
    switch (m_sliderStyle) {
        case SliderStyle::NORMAL:
            m_backgroundColor = wxColour(243, 243, 243);  // Light gray background
            m_hoverColor = wxColour(235, 235, 235);       // Slightly darker on hover
            m_progressColor = wxColour(200, 200, 200);    // Medium gray track
            m_thumbColor = wxColour(32, 167, 232);         // Fluent Blue thumb
            m_textColor = wxColour(32, 32, 32);           // Dark gray text
            m_borderColor = wxColour(180, 180, 180);      // Light gray border
            break;
            
        case SliderStyle::PROGRESS:
            m_backgroundColor = wxColour(243, 243, 243);  // Light gray background
            m_hoverColor = wxColour(235, 235, 235);       // Slightly darker on hover
            m_progressColor = wxColour(200, 200, 200);    // Medium gray track
            m_thumbColor = wxColour(32, 167, 232);         // Fluent Blue thumb
            m_textColor = wxColour(32, 32, 32);           // Dark gray text
            m_borderColor = wxColour(180, 180, 180);      // Light gray border
            break;
            
        case SliderStyle::VERTICAL:
            m_backgroundColor = wxColour(243, 243, 243);  // Light gray background
            m_hoverColor = wxColour(235, 235, 235);       // Slightly darker on hover
            m_progressColor = wxColour(200, 200, 200);    // Medium gray track
            m_thumbColor = wxColour(32, 167, 232);         // Fluent Blue thumb
            m_textColor = wxColour(32, 32, 32);           // Dark gray text
            m_borderColor = wxColour(180, 180, 180);      // Light gray border
            break;
    }
}

void FlatSlider::SetSliderStyle(SliderStyle style)
{
    m_sliderStyle = style;
    InitializeDefaultColors();
    this->Refresh();
}

void FlatSlider::SetLabel(const wxString& label)
{
    m_label = label;
    this->Refresh();
}

void FlatSlider::SetValue(int value)
{
    m_value = value;
    this->Refresh();
}

void FlatSlider::SetRange(int minValue, int maxValue)
{
    m_minValue = minValue;
    m_maxValue = maxValue;
    this->Refresh();
}

void FlatSlider::GetRange(int& minValue, int& maxValue) const
{
    minValue = m_minValue;
    maxValue = m_maxValue;
}

void FlatSlider::SetShowValue(bool showValue)
{
    m_showValue = showValue;
    this->Refresh();
}

void FlatSlider::SetShowTicks(bool showTicks)
{
    m_showTicks = showTicks;
    this->Refresh();
}

void FlatSlider::SetBackgroundColor(const wxColour& color)
{
    m_backgroundColor = color;
    this->Refresh();
}

void FlatSlider::SetTextColor(const wxColour& color)
{
    m_textColor = color;
    this->Refresh();
}

void FlatSlider::SetBorderColor(const wxColour& color)
{
    m_borderColor = color;
    this->Refresh();
}

void FlatSlider::SetBorderWidth(int width)
{
    m_borderWidth = width;
    this->Refresh();
}

void FlatSlider::SetCornerRadius(int radius)
{
    m_cornerRadius = radius;
    this->Refresh();
}

void FlatSlider::SetThumbSize(const wxSize& size)
{
    m_thumbSize = size;
    this->Refresh();
}

void FlatSlider::SetTrackHeight(int height)
{
    m_trackHeight = height;
    this->Refresh();
}

bool FlatSlider::Enable(bool enabled)
{
    m_enabled = enabled;
    wxControl::Enable(enabled);
    this->Refresh();
    return true;
}

// Missing function implementations
wxSize FlatSlider::DoGetBestSize() const
{
    int width = 200;  // Default width
    int height = m_trackHeight + 2 * m_borderWidth + m_thumbSize.GetHeight();
    
    if (!m_label.IsEmpty()) {
        wxSize textSize = GetTextExtent(m_label);
        height += textSize.GetHeight() + 5;
    }
    
    return wxSize(width, height);
}

void FlatSlider::OnPaint(wxPaintEvent& WXUNUSED(event))
{
    wxPaintDC dc(this);
    wxRect rect = this->GetClientRect();
    
    // Draw background
    DrawBackground(dc);
    
    // Draw track
    DrawTrack(dc);
    
    // Draw thumb
    DrawThumb(dc);
    
    // Draw text if present
    if (!m_label.IsEmpty()) {
        DrawText(dc);
    }
}

void FlatSlider::OnSize(wxSizeEvent& event)
{
    this->Refresh();
    event.Skip();
}

void FlatSlider::OnMouseDown(wxMouseEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    m_isDragging = true;
    this->SetFocus();
    
    // Update value based on mouse position
    wxPoint pos = event.GetPosition();
    UpdateValueFromPosition(pos);
    
    this->Refresh();
    event.Skip();
}

void FlatSlider::OnMouseMove(wxMouseEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    bool wasHovered = m_isHovered;
    m_isHovered = true;
    
    if (m_isDragging) {
        // Update value based on mouse position
        wxPoint pos = event.GetPosition();
        UpdateValueFromPosition(pos);
    }
    
    if (!wasHovered) {
        this->Refresh();
    }
    
    event.Skip();
}

void FlatSlider::OnMouseLeave(wxMouseEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    m_isHovered = false;
    this->Refresh();
    event.Skip();
}

void FlatSlider::OnMouseEnter(wxMouseEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    m_isHovered = true;
    this->Refresh();
    event.Skip();
}

void FlatSlider::OnFocus(wxFocusEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    m_hasFocus = true;
    UpdateState(SliderState::FOCUSED);
    this->Refresh();
    event.Skip();
}

void FlatSlider::OnKillFocus(wxFocusEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    m_hasFocus = false;
    m_isDragging = false;
    UpdateState(SliderState::DEFAULT_STATE);
    this->Refresh();
    event.Skip();
}

// Helper functions
void FlatSlider::DrawBackground(wxDC& dc)
{
    wxRect rect = this->GetClientRect();
    wxColour bgColor = GetCurrentBackgroundColor();
    
    dc.SetBrush(wxBrush(bgColor));
    dc.SetPen(wxPen(bgColor));
    DrawRoundedRectangle(dc, rect, m_cornerRadius);
}

void FlatSlider::DrawTrack(wxDC& dc)
{
    wxRect trackRect = GetTrackRect();
    if (trackRect.IsEmpty()) return;
    
    // Draw track background
    dc.SetBrush(wxBrush(m_progressColor));
    dc.SetPen(wxPen(m_borderColor, 1));
    DrawRoundedRectangle(dc, trackRect, m_cornerRadius);
    
    // Draw progress
    if (m_maxValue > m_minValue) {
        double progress = (double)(m_value - m_minValue) / (m_maxValue - m_minValue);
        
        if (m_sliderStyle == SliderStyle::VERTICAL) {
            // Vertical slider - progress is height-based
            int progressHeight = (int)(trackRect.height * progress);
            
            if (progressHeight > 0) {
                wxRect progressRect = trackRect;
                progressRect.y = trackRect.y + trackRect.height - progressHeight;
                progressRect.height = progressHeight;
                
                dc.SetBrush(wxBrush(m_thumbColor));
                DrawRoundedRectangle(dc, progressRect, m_cornerRadius);
            }
        } else {
            // Horizontal slider - progress is width-based
            int progressWidth = (int)(trackRect.width * progress);
            
            if (progressWidth > 0) {
                wxRect progressRect = trackRect;
                progressRect.width = progressWidth;
                
                dc.SetBrush(wxBrush(m_thumbColor));
                DrawRoundedRectangle(dc, progressRect, m_cornerRadius);
            }
        }
    }
}

void FlatSlider::DrawThumb(wxDC& dc)
{
    wxRect thumbRect = GetThumbRect();
    if (thumbRect.IsEmpty()) return;
    
    dc.SetBrush(wxBrush(m_thumbColor));
    dc.SetPen(wxPen(m_borderColor, 1));
    DrawRoundedRectangle(dc, thumbRect, m_cornerRadius);
}

void FlatSlider::DrawText(wxDC& dc)
{
    if (m_label.IsEmpty()) return;
    
    wxRect clientRect = this->GetClientRect();
    dc.SetTextForeground(GetCurrentTextColor());
    dc.SetFont(this->GetFont());
    
    wxSize textSize = dc.GetTextExtent(m_label);
    int x = clientRect.x + m_borderWidth;
    int y = clientRect.y + clientRect.height - textSize.GetHeight() - m_borderWidth;
    
    dc.DrawText(m_label, x, y);
}

void FlatSlider::DrawRoundedRectangle(wxDC& dc, const wxRect& rect, int radius)
{
    dc.DrawRoundedRectangle(rect, radius);
}

void FlatSlider::UpdateState(SliderState newState)
{
    m_state = newState;
}

void FlatSlider::UpdateValueFromPosition(const wxPoint& pos)
{
    wxRect trackRect = GetTrackRect();
    if (trackRect.IsEmpty()) return;
    
    double ratio = 0.0;
    
    if (m_sliderStyle == SliderStyle::VERTICAL) {
        // Vertical slider - use Y coordinate
        int posInTrack = trackRect.y + trackRect.height - pos.y;
        ratio = (double)posInTrack / trackRect.height;
    } else {
        // Horizontal slider - use X coordinate
        int posInTrack = pos.x - trackRect.x;
        ratio = (double)posInTrack / trackRect.width;
    }
    
    ratio = wxMax(0.0, wxMin(1.0, ratio));
    
    int newValue = m_minValue + (int)(ratio * (m_maxValue - m_minValue));
    if (newValue != m_value) {
        m_value = newValue;
        
        // Send value changed event
        wxCommandEvent event(wxEVT_FLAT_SLIDER_VALUE_CHANGED, this->GetId());
        event.SetInt(m_value);
        this->ProcessWindowEvent(event);
        
        this->Refresh();
    }
}

wxRect FlatSlider::GetTrackRect() const
{
    wxRect clientRect = this->GetClientRect();
    
    if (m_sliderStyle == SliderStyle::VERTICAL) {
        // Vertical slider - track is vertical
        int trackX = (clientRect.width - m_trackHeight) / 2;
        
        return wxRect(
            trackX,
            clientRect.y + m_borderWidth + m_thumbSize.GetHeight() / 2,
            m_trackHeight,
            clientRect.height - 2 * m_borderWidth - m_thumbSize.GetHeight()
        );
    } else {
        // Horizontal slider - track is horizontal
        int trackY = (clientRect.height - m_trackHeight) / 2;
        
        return wxRect(
            clientRect.x + m_borderWidth + m_thumbSize.GetWidth() / 2,
            trackY,
            clientRect.width - 2 * m_borderWidth - m_thumbSize.GetWidth(),
            m_trackHeight
        );
    }
}

wxRect FlatSlider::GetThumbRect() const
{
    wxRect trackRect = GetTrackRect();
    if (trackRect.IsEmpty()) return wxRect();
    
    double ratio = (double)(m_value - m_minValue) / (m_maxValue - m_minValue);
    
    if (m_sliderStyle == SliderStyle::VERTICAL) {
        // Vertical slider - thumb position is Y-based
        int thumbY = trackRect.y + trackRect.height - (int)(ratio * trackRect.height) - m_thumbSize.GetHeight() / 2;
        int thumbX = (trackRect.x + trackRect.width / 2) - m_thumbSize.GetWidth() / 2;
        
        return wxRect(thumbX, thumbY, m_thumbSize.GetWidth(), m_thumbSize.GetHeight());
    } else {
        // Horizontal slider - thumb position is X-based
        int thumbX = trackRect.x + (int)(ratio * trackRect.width) - m_thumbSize.GetWidth() / 2;
        int thumbY = (trackRect.y + trackRect.height / 2) - m_thumbSize.GetHeight() / 2;
        
        return wxRect(thumbX, thumbY, m_thumbSize.GetWidth(), m_thumbSize.GetHeight());
    }
}

wxColour FlatSlider::GetCurrentBackgroundColor() const
{
    if (!m_enabled) return wxColour(240, 240, 240);
    if (m_isHovered) return m_hoverColor;
    return m_backgroundColor;
}

wxColour FlatSlider::GetCurrentTextColor() const
{
    if (!m_enabled) return wxColour(128, 128, 128);
    return m_textColor;
}
