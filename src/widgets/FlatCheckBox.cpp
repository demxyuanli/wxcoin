#include "widgets/FlatCheckBox.h"
#include <wx/checkbox.h>
#include "config/FontManager.h"

wxDEFINE_EVENT(wxEVT_FLAT_CHECK_BOX_CLICKED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_CHECK_BOX_STATE_CHANGED, wxCommandEvent);

BEGIN_EVENT_TABLE(FlatCheckBox, wxControl)
    EVT_PAINT(FlatCheckBox::OnPaint)
    EVT_SIZE(FlatCheckBox::OnSize)
    EVT_LEFT_DOWN(FlatCheckBox::OnMouseDown)
    EVT_MOTION(FlatCheckBox::OnMouseMove)
    EVT_LEAVE_WINDOW(FlatCheckBox::OnMouseLeave)
    EVT_ENTER_WINDOW(FlatCheckBox::OnMouseEnter)
    EVT_SET_FOCUS(FlatCheckBox::OnFocus)
    EVT_KILL_FOCUS(FlatCheckBox::OnKillFocus)
END_EVENT_TABLE()

FlatCheckBox::FlatCheckBox(wxWindow* parent, wxWindowID id, const wxString& label,
                           const wxPoint& pos, const wxSize& size, CheckBoxStyle style, long style_flags)
    : wxControl(parent, id, pos, size, style_flags | wxBORDER_NONE)
    , m_label(label)
    , m_checkBoxStyle(style)
    , m_state(CheckBoxState::UNCHECKED)
    , m_enabled(true)
    , m_checked(false)
    , m_triState(false)
    , m_partiallyChecked(false)
    , m_borderWidth(DEFAULT_BORDER_WIDTH)
    , m_cornerRadius(DEFAULT_CORNER_RADIUS)
    , m_checkBoxSize(wxSize(16, 16))
    , m_labelSpacing(DEFAULT_LABEL_SPACING)
    , m_useConfigFont(true)
{
    // Initialize default colors based on style
    InitializeDefaultColors();
    
    // Initialize font from configuration
    ReloadFontFromConfig();
    
    // Set default size if not specified
    if (size == wxDefaultSize) {
        SetInitialSize(DoGetBestSize());
    }
}

FlatCheckBox::~FlatCheckBox()
{
}

void FlatCheckBox::InitializeDefaultColors()
{
    // Fluent Design System inspired colors for checkboxes
    switch (m_checkBoxStyle) {
        case CheckBoxStyle::DEFAULT_STYLE:
            m_backgroundColor = wxColour(255, 255, 255);  // White background
            m_hoverColor = wxColour(248, 248, 248);       // Very light gray on hover
            m_checkedColor = wxColour(0, 120, 215);       // Fluent Blue when checked
            m_textColor = wxColour(32, 32, 32);           // Dark gray text
            m_borderColor = wxColour(200, 200, 200);      // Light gray border
            break;
            
        case CheckBoxStyle::SWITCH:
            m_backgroundColor = wxColour(200, 200, 200);  // Light gray when unchecked
            m_hoverColor = wxColour(190, 190, 190);       // Slightly darker on hover
            m_checkedColor = wxColour(0, 120, 215);       // Fluent Blue when checked
            m_textColor = wxColour(32, 32, 32);           // Dark gray text
            m_borderColor = wxColour(180, 180, 180);      // Light gray border
            break;
            
        case CheckBoxStyle::RADIO:
            m_backgroundColor = wxColour(255, 255, 255);  // White background
            m_hoverColor = wxColour(248, 248, 248);       // Very light gray on hover
            m_checkedColor = wxColour(0, 120, 215);       // Fluent Blue when checked
            m_textColor = wxColour(32, 32, 32);           // Dark gray text
            m_borderColor = wxColour(200, 200, 200);      // Light gray border
            break;
    }
}

void FlatCheckBox::SetCheckBoxStyle(CheckBoxStyle style)
{
    m_checkBoxStyle = style;
    InitializeDefaultColors();
    this->Refresh();
}

void FlatCheckBox::SetLabel(const wxString& label)
{
    m_label = label;
    this->Refresh();
}

void FlatCheckBox::SetValue(bool value)
{
    m_checked = value;
    this->Refresh();
}

void FlatCheckBox::SetTriState(bool triState)
{
    m_triState = triState;
}

void FlatCheckBox::SetBackgroundColor(const wxColour& color)
{
    m_backgroundColor = color;
    this->Refresh();
}

void FlatCheckBox::SetTextColor(const wxColour& color)
{
    m_textColor = color;
    this->Refresh();
}

void FlatCheckBox::SetBorderColor(const wxColour& color)
{
    m_borderColor = color;
    this->Refresh();
}

void FlatCheckBox::SetBorderWidth(int width)
{
    m_borderWidth = width;
    this->Refresh();
}

void FlatCheckBox::SetCornerRadius(int radius)
{
    m_cornerRadius = radius;
    this->Refresh();
}

void FlatCheckBox::SetCheckBoxSize(const wxSize& size)
{
    m_checkBoxSize = size;
    this->Refresh();
}

void FlatCheckBox::SetLabelSpacing(int spacing)
{
    m_labelSpacing = spacing;
    this->Refresh();
}

bool FlatCheckBox::Enable(bool enabled)
{
    m_enabled = enabled;
    wxControl::Enable(enabled);
    this->Refresh();
    return true;
}

// Missing function implementations
wxSize FlatCheckBox::DoGetBestSize() const
{
    wxSize textSize = GetTextExtent(m_label);
    int width = m_checkBoxSize.GetWidth() + m_labelSpacing + textSize.GetWidth() + 2 * m_borderWidth;
    int height = wxMax(m_checkBoxSize.GetHeight(), textSize.GetHeight()) + 2 * m_borderWidth;
    
    return wxSize(width, height);
}

void FlatCheckBox::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    wxRect rect = this->GetClientRect();
    
    // Draw background
    DrawBackground(dc);
    
    // Draw checkbox
    DrawCheckBox(dc);
    
    // Draw text
    DrawText(dc);
}

void FlatCheckBox::OnSize(wxSizeEvent& event)
{
    this->Refresh();
    event.Skip();
}

void FlatCheckBox::OnMouseDown(wxMouseEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    m_isPressed = true;
    this->SetFocus();
    this->Refresh();
    event.Skip();
}

void FlatCheckBox::OnMouseMove(wxMouseEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    bool wasHovered = m_isHovered;
    m_isHovered = true;
    
    if (!wasHovered) {
        this->Refresh();
    }
    
    event.Skip();
}

void FlatCheckBox::OnMouseLeave(wxMouseEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    m_isHovered = false;
    this->Refresh();
    event.Skip();
}

void FlatCheckBox::OnMouseEnter(wxMouseEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    m_isHovered = true;
    this->Refresh();
    event.Skip();
}

void FlatCheckBox::OnFocus(wxFocusEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    m_hasFocus = true;
    UpdateState(CheckBoxState::HOVERED);
    this->Refresh();
    event.Skip();
}

void FlatCheckBox::OnKillFocus(wxFocusEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    m_hasFocus = false;
    m_isPressed = false;
    UpdateState(CheckBoxState::UNCHECKED);
    this->Refresh();
    event.Skip();
}

// Helper functions
void FlatCheckBox::DrawBackground(wxDC& dc)
{
    wxRect rect = this->GetClientRect();
    wxColour bgColor = GetCurrentBackgroundColor();
    
    dc.SetBrush(wxBrush(bgColor));
    dc.SetPen(wxPen(bgColor));
    DrawRoundedRectangle(dc, rect, m_cornerRadius);
}

void FlatCheckBox::DrawCheckBox(wxDC& dc)
{
    wxRect checkBoxRect = GetCheckBoxRect();
    if (checkBoxRect.IsEmpty()) return;
    
    // Draw checkbox background
    wxColour bgColor = m_checked ? m_checkedColor : GetCurrentBackgroundColor();
    dc.SetBrush(wxBrush(bgColor));
    dc.SetPen(wxPen(GetCurrentBorderColor(), m_borderWidth));
    DrawRoundedRectangle(dc, checkBoxRect, m_cornerRadius);
    
    // Draw check mark if checked
    if (m_checked) {
        DrawCheckMark(dc, checkBoxRect);
    }
}

void FlatCheckBox::DrawText(wxDC& dc)
{
    wxRect textRect = GetTextRect();
    if (textRect.IsEmpty() || m_label.IsEmpty()) return;
    
    // Set the font for drawing
    wxFont currentFont = GetFont();
    if (currentFont.IsOk()) {
        dc.SetFont(currentFont);
    }
    
    dc.SetTextForeground(GetCurrentTextColor());
    dc.DrawText(m_label, textRect.x, textRect.y);
}

void FlatCheckBox::DrawCheckMark(wxDC& dc, const wxRect& rect)
{
    dc.SetPen(wxPen(wxColour(255, 255, 255), 2));
    
    int centerX = rect.x + rect.width / 2;
    int centerY = rect.y + rect.height / 2;
    int size = wxMin(rect.width, rect.height) / 4;
    
    // Draw check mark
    wxPoint points[3];
    points[0] = wxPoint(centerX - size, centerY);
    points[1] = wxPoint(centerX - size/2, centerY + size/2);
    points[2] = wxPoint(centerX + size, centerY - size/2);
    
    dc.DrawLines(3, points);
}

void FlatCheckBox::DrawRoundedRectangle(wxDC& dc, const wxRect& rect, int radius)
{
    dc.DrawRoundedRectangle(rect, radius);
}

void FlatCheckBox::UpdateState(CheckBoxState newState)
{
    m_state = newState;
}

wxRect FlatCheckBox::GetCheckBoxRect() const
{
    wxRect clientRect = this->GetClientRect();
    return wxRect(
        clientRect.x + m_borderWidth,
        (clientRect.height - m_checkBoxSize.GetHeight()) / 2,
        m_checkBoxSize.GetWidth(),
        m_checkBoxSize.GetHeight()
    );
}

wxRect FlatCheckBox::GetTextRect() const
{
    wxRect clientRect = this->GetClientRect();
    wxRect checkBoxRect = GetCheckBoxRect();
    
    return wxRect(
        checkBoxRect.x + checkBoxRect.width + m_labelSpacing,
        (clientRect.height - GetTextExtent(m_label).GetHeight()) / 2,
        clientRect.width - checkBoxRect.width - m_labelSpacing - 2 * m_borderWidth,
        GetTextExtent(m_label).GetHeight()
    );
}

wxColour FlatCheckBox::GetCurrentBackgroundColor() const
{
    if (!m_enabled) return wxColour(240, 240, 240);
    if (m_isHovered) return m_hoverColor;
    return m_backgroundColor;
}

wxColour FlatCheckBox::GetCurrentBorderColor() const
{
    if (!m_enabled) return wxColour(200, 200, 200);
    return m_borderColor;
}

wxColour FlatCheckBox::GetCurrentTextColor() const
{
    if (!m_enabled) return wxColour(128, 128, 128);
    return m_textColor;
}

// Font configuration methods
void FlatCheckBox::SetCustomFont(const wxFont& font)
{
    m_customFont = font;
    m_useConfigFont = false;
    SetFont(font);
    InvalidateBestSize();
    Refresh();
}

void FlatCheckBox::UseConfigFont(bool useConfig)
{
    m_useConfigFont = useConfig;
    if (useConfig) {
        ReloadFontFromConfig();
    }
}

void FlatCheckBox::ReloadFontFromConfig()
{
    if (m_useConfigFont) {
        try {
            FontManager& fontManager = FontManager::getInstance();
            wxFont configFont = fontManager.getLabelFont();
            if (configFont.IsOk()) {
                SetFont(configFont);
                InvalidateBestSize();
                Refresh();
            }
        } catch (...) {
            // If font manager is not available, use default font
            wxFont defaultFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
            SetFont(defaultFont);
        }
    }
}
