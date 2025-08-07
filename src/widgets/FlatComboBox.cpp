#include "widgets/FlatComboBox.h"
#include <wx/combobox.h>

wxDEFINE_EVENT(wxEVT_FLAT_COMBO_BOX_SELECTION_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_COMBO_BOX_DROPDOWN_OPENED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_COMBO_BOX_DROPDOWN_CLOSED, wxCommandEvent);

BEGIN_EVENT_TABLE(FlatComboBox, wxControl)
    EVT_PAINT(FlatComboBox::OnPaint)
    EVT_SIZE(FlatComboBox::OnSize)
    EVT_LEFT_DOWN(FlatComboBox::OnMouseDown)
    EVT_MOTION(FlatComboBox::OnMouseMove)
    EVT_LEAVE_WINDOW(FlatComboBox::OnMouseLeave)
    EVT_ENTER_WINDOW(FlatComboBox::OnMouseEnter)
    EVT_SET_FOCUS(FlatComboBox::OnFocus)
    EVT_KILL_FOCUS(FlatComboBox::OnKillFocus)
    EVT_KEY_DOWN(FlatComboBox::OnKeyDown)
    EVT_CHAR(FlatComboBox::OnChar)
END_EVENT_TABLE()

FlatComboBox::FlatComboBox(wxWindow* parent, wxWindowID id, const wxString& value,
                           const wxPoint& pos, const wxSize& size, ComboBoxStyle style, long style_flags)
    : wxControl(parent, id, pos, size, style_flags | wxBORDER_NONE)
    , m_comboBoxStyle(style)
    , m_state(ComboBoxState::DEFAULT_STATE)
    , m_enabled(true)
    , m_editable(false)
    , m_borderWidth(DEFAULT_BORDER_WIDTH)
    , m_cornerRadius(DEFAULT_CORNER_RADIUS)
    , m_padding(DEFAULT_PADDING)
    , m_dropdownButtonWidth(DEFAULT_DROPDOWN_BUTTON_WIDTH)
    , m_maxVisibleItems(DEFAULT_MAX_VISIBLE_ITEMS)
    , m_dropdownWidth(DEFAULT_DROPDOWN_WIDTH)
    , m_isFocused(false)
    , m_isHovered(false)
    , m_isPressed(false)
    , m_dropdownShown(false)
    , m_dropdownButtonHovered(false)
    , m_popup(nullptr)
{
    // Initialize default colors based on style
    InitializeDefaultColors();
    
    // Set default size if not specified
    if (size == wxDefaultSize) {
        SetInitialSize(DoGetBestSize());
    }
}

FlatComboBox::~FlatComboBox()
{
    if (m_popup) {
        m_popup->Destroy();
        m_popup = nullptr;
    }
}

void FlatComboBox::InitializeDefaultColors()
{
    switch (m_comboBoxStyle) {
        case ComboBoxStyle::DEFAULT_STYLE:
            m_backgroundColor = wxColour(255, 255, 255);
            m_focusedColor = wxColour(255, 255, 255);
            m_textColor = wxColour(0, 0, 0);
            m_borderColor = wxColour(200, 200, 200);
            m_dropdownBackgroundColor = wxColour(255, 255, 255);
            m_dropdownBorderColor = wxColour(200, 200, 200);
            break;
            
        case ComboBoxStyle::EDITABLE:
            m_backgroundColor = wxColour(255, 255, 255);
            m_focusedColor = wxColour(255, 255, 255);
            m_textColor = wxColour(0, 0, 0);
            m_borderColor = wxColour(200, 200, 200);
            m_dropdownBackgroundColor = wxColour(255, 255, 255);
            m_dropdownBorderColor = wxColour(200, 200, 200);
            break;
            
        case ComboBoxStyle::SEARCH:
            m_backgroundColor = wxColour(240, 240, 240);
            m_focusedColor = wxColour(255, 255, 255);
            m_textColor = wxColour(0, 0, 0);
            m_borderColor = wxColour(200, 200, 200);
            m_dropdownBackgroundColor = wxColour(255, 255, 255);
            m_dropdownBorderColor = wxColour(200, 200, 200);
            break;
    }
}

void FlatComboBox::SetComboBoxStyle(ComboBoxStyle style)
{
    m_comboBoxStyle = style;
    InitializeDefaultColors();
    this->Refresh();
}

void FlatComboBox::SetBackgroundColor(const wxColour& color)
{
    m_backgroundColor = color;
    this->Refresh();
}

void FlatComboBox::SetTextColor(const wxColour& color)
{
    m_textColor = color;
    this->Refresh();
}

void FlatComboBox::SetBorderColor(const wxColour& color)
{
    m_borderColor = color;
    this->Refresh();
}

void FlatComboBox::SetBorderWidth(int width)
{
    m_borderWidth = width;
    this->Refresh();
}

void FlatComboBox::SetCornerRadius(int radius)
{
    m_cornerRadius = radius;
    this->Refresh();
}

void FlatComboBox::SetPadding(int horizontal, int vertical)
{
    m_padding = horizontal; // Use single padding value
    this->Refresh();
}

void FlatComboBox::GetPadding(int& horizontal, int& vertical) const
{
    horizontal = m_padding;
    vertical = m_padding;
}

void FlatComboBox::SetEnabled(bool enabled)
{
    m_enabled = enabled;
    wxControl::Enable(enabled);
    this->Refresh();
}

// Missing function implementations
wxSize FlatComboBox::DoGetBestSize() const
{
    wxSize textSize = GetTextExtent(m_value.IsEmpty() ? "Sample Text" : m_value);
    int width = textSize.GetWidth() + 2 * m_padding + 2 * m_borderWidth + m_dropdownButtonWidth;
    int height = textSize.GetHeight() + 2 * m_padding + 2 * m_borderWidth;
    
    return wxSize(width, height);
}

void FlatComboBox::Append(const wxString& item, const wxBitmap& icon, const wxVariant& data)
{
    ComboBoxItem newItem(item, icon, data);
    m_items.push_back(newItem);
    this->Refresh();
}



void FlatComboBox::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    wxRect rect = this->GetClientRect();
    
    // Draw background
    DrawBackground(dc);
    
    // Draw border
    DrawBorder(dc);
    
    // Draw text
    DrawText(dc);
    
    // Draw dropdown button
    DrawDropdownButton(dc);
}

void FlatComboBox::OnSize(wxSizeEvent& event)
{
    this->Refresh();
    event.Skip();
}

void FlatComboBox::OnMouseDown(wxMouseEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    m_isPressed = true;
    this->SetFocus();
    
    // Check if dropdown button was clicked
    wxPoint pos = event.GetPosition();
    if (GetDropdownButtonRect().Contains(pos)) {
        HandleDropdownClick();
    }
    
    this->Refresh();
    event.Skip();
}

void FlatComboBox::OnMouseMove(wxMouseEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    bool wasHovered = m_isHovered;
    m_isHovered = true;
    
    // Check if dropdown button is hovered
    wxPoint pos = event.GetPosition();
    bool wasButtonHovered = m_dropdownButtonHovered;
    m_dropdownButtonHovered = GetDropdownButtonRect().Contains(pos);
    
    if (!wasHovered || wasButtonHovered != m_dropdownButtonHovered) {
        this->Refresh();
    }
    
    event.Skip();
}

void FlatComboBox::OnMouseLeave(wxMouseEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    m_isHovered = false;
    m_dropdownButtonHovered = false;
    this->Refresh();
    event.Skip();
}

void FlatComboBox::OnMouseEnter(wxMouseEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    m_isHovered = true;
    this->Refresh();
    event.Skip();
}

void FlatComboBox::OnFocus(wxFocusEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    m_isFocused = true;
    UpdateState(ComboBoxState::FOCUSED);
    this->Refresh();
    event.Skip();
}

void FlatComboBox::OnKillFocus(wxFocusEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    m_isFocused = false;
    m_isPressed = false;
    UpdateState(ComboBoxState::DEFAULT_STATE);
    this->Refresh();
    event.Skip();
}

void FlatComboBox::OnKeyDown(wxKeyEvent& event)
{
    if (!m_enabled) {
        event.Skip();
        return;
    }
    
    switch (event.GetKeyCode()) {
        case WXK_TAB:
            event.Skip();
            break;
        case WXK_RETURN:
        case WXK_SPACE:
            HandleDropdownClick();
            break;
        default:
            event.Skip();
            break;
    }
}

void FlatComboBox::OnChar(wxKeyEvent& event)
{
    if (!m_enabled || !m_editable) {
        event.Skip();
        return;
    }
    
    // Handle character input for editable combo box
    int keyCode = event.GetKeyCode();
    if (keyCode >= 32 && keyCode < 127) {
        wxString newValue = m_value + wxString((wxChar)keyCode);
        SetValue(newValue);
    }
    
    event.Skip();
}

// Helper functions
void FlatComboBox::DrawBackground(wxDC& dc)
{
    wxRect rect = this->GetClientRect();
    wxColour bgColor = GetCurrentBackgroundColor();
    
    dc.SetBrush(wxBrush(bgColor));
    dc.SetPen(wxPen(bgColor));
    DrawRoundedRectangle(dc, rect, m_cornerRadius);
}

void FlatComboBox::DrawBorder(wxDC& dc)
{
    wxRect rect = this->GetClientRect();
    wxColour borderColor = GetCurrentBorderColor();
    
    dc.SetBrush(wxBrush(*wxTRANSPARENT_BRUSH));
    dc.SetPen(wxPen(borderColor, m_borderWidth));
    DrawRoundedRectangle(dc, rect, m_cornerRadius);
}

void FlatComboBox::DrawText(wxDC& dc)
{
    wxRect textRect = GetTextRect();
    if (textRect.IsEmpty()) return;
    
    dc.SetTextForeground(GetCurrentTextColor());
    dc.SetFont(this->GetFont());
    
    wxString displayText = m_value;
    if (displayText.IsEmpty() && m_selection >= 0 && m_selection < (int)m_items.size()) {
        displayText = m_items[m_selection].text;
    }
    
    dc.DrawText(displayText, textRect.x, textRect.y);
}

void FlatComboBox::DrawDropdownButton(wxDC& dc)
{
    wxRect buttonRect = GetDropdownButtonRect();
    if (buttonRect.IsEmpty()) return;
    
    // Draw dropdown arrow
    dc.SetPen(wxPen(GetCurrentTextColor(), 1));
    
    int centerX = buttonRect.x + buttonRect.width / 2;
    int centerY = buttonRect.y + buttonRect.height / 2;
    int arrowSize = 4;
    
    wxPoint points[3];
    points[0] = wxPoint(centerX - arrowSize, centerY - arrowSize);
    points[1] = wxPoint(centerX + arrowSize, centerY - arrowSize);
    points[2] = wxPoint(centerX, centerY + arrowSize);
    
    dc.DrawPolygon(3, points);
}

void FlatComboBox::DrawRoundedRectangle(wxDC& dc, const wxRect& rect, int radius)
{
    dc.DrawRoundedRectangle(rect, radius);
}

void FlatComboBox::UpdateState(ComboBoxState newState)
{
    m_state = newState;
}

void FlatComboBox::HandleDropdownClick()
{
    if (m_dropdownShown) {
        HideDropdown();
    } else {
        ShowDropdown();
    }
}

wxRect FlatComboBox::GetTextRect() const
{
    wxRect clientRect = this->GetClientRect();
    return wxRect(
        clientRect.x + m_padding + m_borderWidth,
        clientRect.y + m_padding + m_borderWidth,
        clientRect.width - 2 * (m_padding + m_borderWidth) - m_dropdownButtonWidth,
        clientRect.height - 2 * (m_padding + m_borderWidth)
    );
}

wxRect FlatComboBox::GetDropdownButtonRect() const
{
    wxRect clientRect = this->GetClientRect();
    return wxRect(
        clientRect.x + clientRect.width - m_dropdownButtonWidth - m_borderWidth,
        clientRect.y + m_borderWidth,
        m_dropdownButtonWidth,
        clientRect.height - 2 * m_borderWidth
    );
}

wxColour FlatComboBox::GetCurrentBackgroundColor() const
{
    if (!m_enabled) return wxColour(240, 240, 240);
    if (m_isFocused) return m_focusedColor;
    if (m_isHovered) return wxColour(250, 250, 250);
    return m_backgroundColor;
}

wxColour FlatComboBox::GetCurrentBorderColor() const
{
    if (!m_enabled) return wxColour(200, 200, 200);
    if (m_isFocused) return wxColour(0, 120, 215);
    return m_borderColor;
}

wxColour FlatComboBox::GetCurrentTextColor() const
{
    if (!m_enabled) return wxColour(128, 128, 128);
    return m_textColor;
}

void FlatComboBox::ShowDropdown()
{
    if (!m_dropdownShown) {
        m_dropdownShown = true;
        UpdateState(ComboBoxState::DROPDOWN_OPEN);
        this->Refresh();
        
        // Send dropdown opened event
        wxCommandEvent event(wxEVT_FLAT_COMBO_BOX_DROPDOWN_OPENED, this->GetId());
        this->ProcessWindowEvent(event);
    }
}

void FlatComboBox::HideDropdown()
{
    if (m_dropdownShown) {
        m_dropdownShown = false;
        UpdateState(ComboBoxState::DEFAULT_STATE);
        this->Refresh();
        
        // Send dropdown closed event
        wxCommandEvent event(wxEVT_FLAT_COMBO_BOX_DROPDOWN_CLOSED, this->GetId());
        this->ProcessWindowEvent(event);
    }
}

wxString FlatComboBox::GetValue() const
{
    // Return the current value - this would need to be implemented based on the actual data structure
    return wxEmptyString; // Placeholder implementation
}

void FlatComboBox::SetValue(const wxString& value)
{
    // Set the current value - this would need to be implemented based on the actual data structure
    this->Refresh();
}

void FlatComboBox::OnSelectionChanged(wxCommandEvent& event)
{
    // Send custom event
    wxCommandEvent customEvent(wxEVT_FLAT_COMBO_BOX_SELECTION_CHANGED, this->GetId());
    customEvent.SetString(GetValue());
    this->ProcessWindowEvent(customEvent);
    
    // Call base class handler
    event.Skip();
}

void FlatComboBox::OnDropdownOpened(wxCommandEvent& event)
{
    // Send custom event
    wxCommandEvent customEvent(wxEVT_FLAT_COMBO_BOX_DROPDOWN_OPENED, this->GetId());
    this->ProcessWindowEvent(customEvent);
    
    event.Skip();
}

void FlatComboBox::OnDropdownClosed(wxCommandEvent& event)
{
    // Send custom event
    wxCommandEvent customEvent(wxEVT_FLAT_COMBO_BOX_DROPDOWN_CLOSED, this->GetId());
    this->ProcessWindowEvent(customEvent);
    
    event.Skip();
}
