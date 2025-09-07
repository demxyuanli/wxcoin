#include "widgets/FlatCheckBox.h"
#include <wx/checkbox.h>
#include "config/FontManager.h"
#include <wx/graphics.h>
#include "config/ThemeManager.h"

wxDEFINE_EVENT(wxEVT_FLAT_CHECK_BOX_CLICKED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_CHECK_BOX_STATE_CHANGED, wxCommandEvent);

BEGIN_EVENT_TABLE(FlatCheckBox, wxControl)
EVT_PAINT(FlatCheckBox::OnPaint)
EVT_SIZE(FlatCheckBox::OnSize)
EVT_LEFT_DOWN(FlatCheckBox::OnMouseDown)
EVT_LEFT_UP(FlatCheckBox::OnMouseUp)
EVT_MOTION(FlatCheckBox::OnMouseMove)
EVT_LEAVE_WINDOW(FlatCheckBox::OnMouseLeave)
EVT_ENTER_WINDOW(FlatCheckBox::OnMouseEnter)
EVT_KEY_UP(FlatCheckBox::OnKeyUp)
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

	// Add a theme change listener
	ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
		InitializeDefaultColors();
		Refresh();
		});
}

FlatCheckBox::~FlatCheckBox()
{
}

void FlatCheckBox::InitializeDefaultColors()
{
	// Fluent Design System inspired colors from ThemeManager
	m_backgroundColor = CFG_COLOUR("SecondaryBackgroundColour");
	m_hoverColor = CFG_COLOUR("HomespaceHoverBgColour");
	m_checkedColor = CFG_COLOUR("AccentColour");
	m_textColor = CFG_COLOUR("PrimaryTextColour");
	m_borderColor = CFG_COLOUR("ButtonBorderColour");
	m_disabledColor = CFG_COLOUR("PanelDisabledBgColour");
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
	wxGraphicsContext* gc = wxGraphicsContext::Create(dc);

	if (gc) {
		// Draw background
		DrawBackground(*gc);

		// Draw checkbox
		DrawCheckBox(*gc);

		// Draw text
		DrawText(*gc);

		delete gc;
	}
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

void FlatCheckBox::OnMouseUp(wxMouseEvent& event)
{
	if (!m_enabled || !m_isPressed) return;
	m_isPressed = false;

	wxPoint mousePos = event.GetPosition();
	if (GetClientRect().Contains(mousePos)) {
		ToggleValue();
		SendCheckBoxEvent();
	}
	Refresh();
}

void FlatCheckBox::OnKeyUp(wxKeyEvent& event)
{
	if (!m_enabled) return;
	if (event.GetKeyCode() == WXK_SPACE) {
		ToggleValue();
		SendCheckBoxEvent();
	}
	event.Skip();
}

void FlatCheckBox::ToggleValue()
{
	if (m_triState) {
		// In tri-state, cycle through unchecked -> checked -> partially checked
		if (m_partiallyChecked) {
			m_partiallyChecked = false;
			m_checked = false;
		}
		else if (m_checked) {
			m_partiallyChecked = true;
			m_checked = true;
		}
		else {
			m_checked = true;
		}
	}
	else {
		m_checked = !m_checked;
	}
	Refresh();
}

void FlatCheckBox::SendCheckBoxEvent()
{
	wxCommandEvent event(wxEVT_FLAT_CHECK_BOX_STATE_CHANGED, GetId());
	event.SetEventObject(this);
	event.SetInt(m_checked ? (m_partiallyChecked ? wxCHK_UNDETERMINED : wxCHK_CHECKED) : wxCHK_UNCHECKED);
	ProcessWindowEvent(event);
}

// Helper functions
void FlatCheckBox::DrawBackground(wxGraphicsContext& gc)
{
	wxRect rect = this->GetClientRect();
	wxColour bgColor = GetCurrentBackgroundColor();

	gc.SetBrush(wxBrush(bgColor));
	gc.SetPen(wxPen(bgColor));
	DrawRoundedRectangle(gc, rect, m_cornerRadius);
}

void FlatCheckBox::DrawCheckBox(wxGraphicsContext& gc)
{
	wxRect checkBoxRect = GetCheckBoxRect();
	if (checkBoxRect.IsEmpty()) return;

	// Draw checkbox background
	wxColour bgColor = m_checked ? m_checkedColor : GetCurrentBackgroundColor();
	gc.SetBrush(wxBrush(bgColor));
	gc.SetPen(wxPen(GetCurrentBorderColor(), m_borderWidth));

	if (m_checkBoxStyle == CheckBoxStyle::RADIO) {
		// Draw radio button (circle)
		int centerX = checkBoxRect.x + checkBoxRect.width / 2;
		int centerY = checkBoxRect.y + checkBoxRect.height / 2;
		int radius = wxMin(checkBoxRect.width, checkBoxRect.height) / 2 - 2;

		// Draw outer circle
		gc.DrawEllipse(centerX - radius, centerY - radius, 2 * radius, 2 * radius);

		// Draw inner circle if checked
		if (m_checked) {
			gc.SetBrush(wxBrush(m_checkedColor));
			gc.DrawEllipse(centerX - (radius - 4), centerY - (radius - 4), 2 * (radius - 4), 2 * (radius - 4));
		}
	}
	else if (m_checkBoxStyle == CheckBoxStyle::SWITCH) {
		// Draw switch
		DrawSwitch(gc, checkBoxRect);
	}
	else {
		// Draw regular checkbox
		DrawRoundedRectangle(gc, checkBoxRect, m_cornerRadius);

		// Draw check mark if checked
		if (m_checked) {
			DrawCheckMark(gc, checkBoxRect);
		}
	}
}

void FlatCheckBox::DrawSwitch(wxGraphicsContext& gc, const wxRect& rect)
{
	// Draw switch track
	wxColour trackColor = m_checked ? m_checkedColor : wxColour(200, 200, 200);
	gc.SetBrush(wxBrush(trackColor));
	gc.SetPen(wxPen(trackColor));

	int trackHeight = rect.height;
	int trackWidth = rect.width;
	int trackY = rect.y;
	int trackX = rect.x;

	// Draw rounded rectangle for track
	DrawRoundedRectangle(gc, wxRect(trackX, trackY, trackWidth, trackHeight), trackHeight / 2);

	// Draw thumb
	int thumbSize = trackHeight - 4;
	int thumbY = trackY + 2;
	int thumbX = m_checked ? (trackX + trackWidth - thumbSize - 2) : (trackX + 2);

	gc.SetBrush(wxBrush(wxColour(255, 255, 255)));
	gc.SetPen(wxPen(wxColour(200, 200, 200)));
	gc.DrawEllipse(thumbX, thumbY, thumbSize, thumbSize);
}

void FlatCheckBox::DrawCheckMark(wxGraphicsContext& gc, const wxRect& rect)
{
	gc.SetPen(wxPen(wxColour(255, 255, 255), 2));

	int centerX = rect.x + rect.width / 2;
	int centerY = rect.y + rect.height / 2;
	int size = wxMin(rect.width, rect.height) / 4;

	// Draw check mark
	wxPoint2DDouble points[3];
	points[0] = wxPoint2DDouble(centerX - size, centerY);
	points[1] = wxPoint2DDouble(centerX - size / 2, centerY + size / 2);
	points[2] = wxPoint2DDouble(centerX + size, centerY - size / 2);

	gc.StrokeLines(3, points);
}

void FlatCheckBox::DrawRoundedRectangle(wxGraphicsContext& gc, const wxRect& rect, int radius)
{
	if (radius <= 0) {
		gc.DrawRectangle(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
		return;
	}
	gc.DrawRoundedRectangle(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight(), radius);
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
		}
		catch (...) {
			// If font manager is not available, use default font
			wxFont defaultFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
			SetFont(defaultFont);
		}
	}
}

void FlatCheckBox::DrawText(wxGraphicsContext& gc)
{
	wxRect textRect = GetTextRect();
	if (textRect.IsEmpty() || m_label.IsEmpty()) return;

	// Set the font for drawing
	wxFont currentFont = GetFont();
	if (currentFont.IsOk()) {
		gc.SetFont(currentFont, GetCurrentTextColor());
	}
	else {
		gc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT), GetCurrentTextColor());
	}

	gc.DrawText(m_label, textRect.x, textRect.y);
}