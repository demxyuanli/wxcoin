#include "widgets/FlatLineEdit.h"
#include <wx/textctrl.h>
#include "config/FontManager.h"
#include "config/ThemeManager.h"

wxDEFINE_EVENT(wxEVT_FLAT_LINE_EDIT_TEXT_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_LINE_EDIT_FOCUS_GAINED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_LINE_EDIT_FOCUS_LOST, wxCommandEvent);

BEGIN_EVENT_TABLE(FlatLineEdit, wxControl)
EVT_PAINT(FlatLineEdit::OnPaint)
EVT_SIZE(FlatLineEdit::OnSize)
EVT_LEFT_DOWN(FlatLineEdit::OnMouseDown)
EVT_MOTION(FlatLineEdit::OnMouseMove)
EVT_LEAVE_WINDOW(FlatLineEdit::OnMouseLeave)
EVT_ENTER_WINDOW(FlatLineEdit::OnMouseEnter)
EVT_SET_FOCUS(FlatLineEdit::OnFocusGained)
EVT_KILL_FOCUS(FlatLineEdit::OnFocusLost)
EVT_KEY_DOWN(FlatLineEdit::OnKeyDown)
EVT_CHAR(FlatLineEdit::OnChar)
END_EVENT_TABLE()

FlatLineEdit::FlatLineEdit(wxWindow* parent, wxWindowID id, const wxString& value,
	const wxPoint& pos, const wxSize& size, LineEditStyle style, long style_flags)
	: wxControl(parent, id, pos, size, style_flags | wxBORDER_NONE)
	, m_value(value)
	, m_lineEditStyle(style)
	, m_state(LineEditState::NORMAL)
	, m_enabled(true)
	, m_hasError(false)
	, m_clearable(false)
	, m_passwordMode(false)
	, m_passwordVisible(false)
	, m_placeholderText("")
	, m_borderWidth(DEFAULT_BORDER_WIDTH)
	, m_cornerRadius(DEFAULT_CORNER_RADIUS)
	, m_padding(DEFAULT_PADDING)
	, m_iconSpacing(DEFAULT_ICON_SPACING)
	, m_horizontalPadding(DEFAULT_PADDING)
	, m_verticalPadding(DEFAULT_PADDING)
	, m_isFocused(false)
	, m_isHovered(false)
	, m_isPressed(false)
	, m_showClearButton(false)
	, m_selectionStart(0)
	, m_selectionEnd(0)
	, m_hasSelection(false)
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

	// Theme change listener
	ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
		InitializeDefaultColors();
		Refresh();
		});
}

FlatLineEdit::~FlatLineEdit()
{
	ThemeManager::getInstance().removeThemeChangeListener(this);
}

void FlatLineEdit::InitializeDefaultColors()
{
	// Fluent Design System inspired colors using ThemeManager
	m_backgroundColor = CFG_COLOUR("TextCtrlBgColour");
	m_hoverColor = CFG_COLOUR("HomespaceHoverBgColour");
	m_focusedColor = CFG_COLOUR("TextCtrlBgColour");
	m_textColor = CFG_COLOUR("PrimaryTextColour");
	m_borderColor = CFG_COLOUR("ButtonBorderColour");
	m_focusBorderColor = CFG_COLOUR("AccentColour");
	m_placeholderColor = CFG_COLOUR("PlaceholderTextColour");
	// Error state colors
	m_errorColor = CFG_COLOUR("ErrorTextColour");
	m_errorBorderColor = CFG_COLOUR("ErrorTextColour");
}

void FlatLineEdit::SetPlaceholderText(const wxString& placeholder)
{
	m_placeholderText = placeholder;
	// Note: wxTextCtrl doesn't have built-in placeholder support in older versions
	// This would need to be implemented with custom drawing
}

void FlatLineEdit::SetClearable(bool clearable)
{
	m_clearable = clearable;
}

void FlatLineEdit::SetPasswordMode(bool passwordMode)
{
	m_passwordMode = passwordMode;
	if (passwordMode) {
		SetWindowStyle(GetWindowStyle() | wxTE_PASSWORD);
	}
	else {
		SetWindowStyle(GetWindowStyle() & ~wxTE_PASSWORD);
	}
}

void FlatLineEdit::SetLineEditStyle(LineEditStyle style)
{
	m_lineEditStyle = style;
	InitializeDefaultColors();
	this->Refresh();
}

void FlatLineEdit::SetBackgroundColor(const wxColour& color)
{
	m_backgroundColor = color;
	this->Refresh();
}

void FlatLineEdit::SetTextColor(const wxColour& color)
{
	m_textColor = color;
	SetForegroundColour(color);
}

void FlatLineEdit::SetBorderColor(const wxColour& color)
{
	m_borderColor = color;
	this->Refresh();
}

void FlatLineEdit::SetBorderWidth(int width)
{
	m_borderWidth = width;
	this->Refresh();
}

void FlatLineEdit::SetCornerRadius(int radius)
{
	m_cornerRadius = radius;
	this->Refresh();
}

void FlatLineEdit::SetPadding(int horizontal, int vertical)
{
	m_horizontalPadding = horizontal;
	m_verticalPadding = vertical;
	this->Refresh();
}

void FlatLineEdit::GetPadding(int& horizontal, int& vertical) const
{
	horizontal = m_horizontalPadding;
	vertical = m_verticalPadding;
}

void FlatLineEdit::SetEnabled(bool enabled)
{
	m_enabled = enabled;
	wxControl::Enable(enabled);
	this->Refresh();
}

void FlatLineEdit::OnTextChanged(wxCommandEvent& event)
{
	// Send custom event
	wxCommandEvent customEvent(wxEVT_FLAT_LINE_EDIT_TEXT_CHANGED, this->GetId());
	customEvent.SetString(GetValue());
	this->ProcessWindowEvent(customEvent);

	event.Skip();
}

void FlatLineEdit::OnFocusGained(wxFocusEvent& event)
{
	m_state = LineEditState::FOCUSED;
	this->Refresh();

	// Send custom event
	wxCommandEvent customEvent(wxEVT_FLAT_LINE_EDIT_FOCUS_GAINED, this->GetId());
	this->ProcessWindowEvent(customEvent);

	event.Skip();
}

void FlatLineEdit::OnFocusLost(wxFocusEvent& event)
{
	m_state = LineEditState::NORMAL;
	this->Refresh();

	// Send custom event
	wxCommandEvent customEvent(wxEVT_FLAT_LINE_EDIT_FOCUS_LOST, this->GetId());
	this->ProcessWindowEvent(customEvent);

	event.Skip();
}

// Missing function implementations
wxSize FlatLineEdit::DoGetBestSize() const
{
	wxSize textSize = GetTextExtent(m_value.IsEmpty() ? m_placeholderText : m_value);
	int width = textSize.GetWidth() + 2 * m_horizontalPadding + 2 * m_borderWidth;
	int height = textSize.GetHeight() + 2 * m_verticalPadding + 2 * m_borderWidth;

	// Add space for icons if present
	if (m_leftIcon.IsOk()) {
		width += m_leftIcon.GetWidth() + m_iconSpacing;
	}
	if (m_rightIcon.IsOk() || m_clearable) {
		width += m_rightIcon.GetWidth() + m_iconSpacing;
	}

	return wxSize(width, height);
}

wxString FlatLineEdit::GetValue() const
{
	return m_value;
}

void FlatLineEdit::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxRect rect = this->GetClientRect();

	// Draw background
	DrawBackground(dc);

	// Draw border
	DrawBorder(dc);

	// Draw text or placeholder
	if (m_value.IsEmpty() && !m_isFocused) {
		DrawPlaceholder(dc);
	}
	else {
		DrawText(dc);
	}

	// Draw icons
	DrawIcons(dc);
}

void FlatLineEdit::OnSize(wxSizeEvent& event)
{
	UpdateTextRect();
	this->Refresh();
	event.Skip();
}

void FlatLineEdit::OnMouseDown(wxMouseEvent& event)
{
	if (!m_enabled) {
		event.Skip();
		return;
	}

	m_isPressed = true;
	this->SetFocus();

	// Handle icon clicks
	wxPoint pos = event.GetPosition();
	HandleIconClick(pos);

	this->Refresh();
	event.Skip();
}

void FlatLineEdit::OnMouseMove(wxMouseEvent& event)
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

void FlatLineEdit::OnMouseLeave(wxMouseEvent& event)
{
	if (!m_enabled) {
		event.Skip();
		return;
	}

	m_isHovered = false;
	this->Refresh();
	event.Skip();
}

void FlatLineEdit::OnMouseEnter(wxMouseEvent& event)
{
	if (!m_enabled) {
		event.Skip();
		return;
	}

	m_isHovered = true;
	this->Refresh();
	event.Skip();
}

void FlatLineEdit::OnKeyDown(wxKeyEvent& event)
{
	if (!m_enabled) {
		event.Skip();
		return;
	}

	// Handle special keys
	switch (event.GetKeyCode()) {
	case WXK_TAB:
		event.Skip();
		break;
	case WXK_RETURN:
	{
		// Send text changed event
		wxCommandEvent textEvent(wxEVT_FLAT_LINE_EDIT_TEXT_CHANGED, this->GetId());
		textEvent.SetString(m_value);
		this->ProcessWindowEvent(textEvent);
	}
	break;
	default:
		event.Skip();
		break;
	}
}

void FlatLineEdit::OnChar(wxKeyEvent& event)
{
	if (!m_enabled) {
		event.Skip();
		return;
	}

	// Handle character input
	int keyCode = event.GetKeyCode();
	if (keyCode >= 32 && keyCode < 127) {
		wxString newValue = m_value + wxString((wxChar)keyCode);
		SetValue(newValue);
	}

	event.Skip();
}

// Helper functions
void FlatLineEdit::DrawBackground(wxDC& dc)
{
	wxRect rect = this->GetClientRect();
	wxColour bgColor = GetCurrentBackgroundColor();

	dc.SetBrush(wxBrush(bgColor));
	dc.SetPen(wxPen(bgColor));
	DrawRoundedRectangle(dc, rect, m_cornerRadius);
}

void FlatLineEdit::DrawBorder(wxDC& dc)
{
	wxRect rect = this->GetClientRect();
	wxColour borderColor = GetCurrentBorderColor();

	dc.SetBrush(wxBrush(*wxTRANSPARENT_BRUSH));
	dc.SetPen(wxPen(borderColor, m_borderWidth));
	DrawRoundedRectangle(dc, rect, m_cornerRadius);
}

void FlatLineEdit::DrawText(wxDC& dc)
{
	wxRect textRect = GetTextRect();
	if (textRect.IsEmpty()) return;

	// Set the font for drawing
	wxFont currentFont = GetFont();
	if (currentFont.IsOk()) {
		dc.SetFont(currentFont);
	}

	dc.SetTextForeground(GetCurrentTextColor());

	wxString displayText = m_value;
	if (m_passwordMode && !m_passwordVisible) {
		displayText = wxString('*', m_value.length());
	}

	dc.DrawText(displayText, textRect.x, textRect.y);
}

void FlatLineEdit::DrawPlaceholder(wxDC& dc)
{
	wxRect textRect = GetTextRect();
	if (textRect.IsEmpty() || m_placeholderText.IsEmpty()) return;

	// Set the font for drawing
	wxFont currentFont = GetFont();
	if (currentFont.IsOk()) {
		dc.SetFont(currentFont);
	}

	dc.SetTextForeground(m_placeholderColor);
	dc.DrawText(m_placeholderText, textRect.x, textRect.y);
}

void FlatLineEdit::DrawIcons(wxDC& dc)
{
	// Draw left icon
	if (m_leftIcon.IsOk()) {
		wxRect iconRect = GetLeftIconRect();
		dc.DrawBitmap(m_leftIcon, iconRect.x, iconRect.y);
	}

	// Draw right icon or clear button
	if (m_rightIcon.IsOk()) {
		wxRect iconRect = GetRightIconRect();
		dc.DrawBitmap(m_rightIcon, iconRect.x, iconRect.y);
	}
}

void FlatLineEdit::DrawRoundedRectangle(wxDC& dc, const wxRect& rect, int radius)
{
	// Simple rounded rectangle implementation
	dc.DrawRoundedRectangle(rect, radius);
}

void FlatLineEdit::UpdateState(LineEditState newState)
{
	m_state = newState;
}

void FlatLineEdit::UpdateTextRect()
{
	// Update text rectangle based on current size and padding
	wxRect clientRect = this->GetClientRect();
	m_textRect = wxRect(
		clientRect.x + m_horizontalPadding + m_borderWidth,
		clientRect.y + m_verticalPadding + m_borderWidth,
		clientRect.width - 2 * (m_horizontalPadding + m_borderWidth),
		clientRect.height - 2 * (m_verticalPadding + m_borderWidth)
	);
}

void FlatLineEdit::HandleIconClick(const wxPoint& pos)
{
	// Handle icon clicks
	if (m_clearable && GetRightIconRect().Contains(pos)) {
		Clear();
	}
}

wxRect FlatLineEdit::GetTextRect() const
{
	return m_textRect;
}

wxRect FlatLineEdit::GetLeftIconRect() const
{
	if (!m_leftIcon.IsOk()) return wxRect();

	wxRect clientRect = this->GetClientRect();
	return wxRect(
		clientRect.x + m_borderWidth,
		(clientRect.height - m_leftIcon.GetHeight()) / 2,
		m_leftIcon.GetWidth(),
		m_leftIcon.GetHeight()
	);
}

wxRect FlatLineEdit::GetRightIconRect() const
{
	if (!m_rightIcon.IsOk()) return wxRect();

	wxRect clientRect = this->GetClientRect();
	return wxRect(
		clientRect.x + clientRect.width - m_rightIcon.GetWidth() - m_borderWidth,
		(clientRect.height - m_rightIcon.GetHeight()) / 2,
		m_rightIcon.GetWidth(),
		m_rightIcon.GetHeight()
	);
}

wxColour FlatLineEdit::GetCurrentBackgroundColor() const
{
	if (!m_enabled) return wxColour(240, 240, 240);
	if (m_isFocused) return m_focusedColor;
	if (m_isHovered) return m_hoverColor;
	return m_backgroundColor;
}

wxColour FlatLineEdit::GetCurrentBorderColor() const
{
	if (!m_enabled) return wxColour(200, 200, 200);
	if (m_isFocused) return m_focusBorderColor;
	if (m_hasError) return wxColour(255, 0, 0);
	return m_borderColor;
}

wxColour FlatLineEdit::GetCurrentTextColor() const
{
	if (!m_enabled) return wxColour(128, 128, 128);
	return m_textColor;
}

void FlatLineEdit::SetValue(const wxString& value)
{
	m_value = value;
	this->Refresh();
}

void FlatLineEdit::Clear()
{
	m_value.Clear();
	this->Refresh();
}

// Font configuration methods
void FlatLineEdit::SetCustomFont(const wxFont& font)
{
	m_customFont = font;
	m_useConfigFont = false;
	SetFont(font);
	InvalidateBestSize();
	Refresh();
}

void FlatLineEdit::UseConfigFont(bool useConfig)
{
	m_useConfigFont = useConfig;
	if (useConfig) {
		ReloadFontFromConfig();
	}
}

void FlatLineEdit::ReloadFontFromConfig()
{
	if (m_useConfigFont) {
		try {
			FontManager& fontManager = FontManager::getInstance();
			wxFont configFont = fontManager.getTextCtrlFont();
			if (configFont.IsOk()) {
				SetFont(configFont);
				InvalidateBestSize();
				Refresh();
			}
		}
		catch (...) {
			// If font manager is not available, use default font
			wxFont defaultFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
			SetFont(defaultFont);
		}
	}
}