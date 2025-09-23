#include "widgets/FlatRadioButton.h"
#include "config/FontManager.h"
#include "config/ThemeManager.h"
#include <wx/graphics.h>
#include <wx/dcbuffer.h>

wxDEFINE_EVENT(wxEVT_FLAT_RADIO_BUTTON_CLICKED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_RADIO_BUTTON_STATE_CHANGED, wxCommandEvent);

BEGIN_EVENT_TABLE(FlatRadioButton, wxControl)
EVT_PAINT(FlatRadioButton::OnPaint)
EVT_SIZE(FlatRadioButton::OnSize)
EVT_LEFT_DOWN(FlatRadioButton::OnMouseDown)
EVT_LEFT_UP(FlatRadioButton::OnMouseUp)
EVT_MOTION(FlatRadioButton::OnMouseMove)
EVT_LEAVE_WINDOW(FlatRadioButton::OnMouseLeave)
EVT_ENTER_WINDOW(FlatRadioButton::OnMouseEnter)
EVT_KEY_UP(FlatRadioButton::OnKeyUp)
EVT_SET_FOCUS(FlatRadioButton::OnFocus)
EVT_KILL_FOCUS(FlatRadioButton::OnKillFocus)
END_EVENT_TABLE()

FlatRadioButton::FlatRadioButton(wxWindow* parent, wxWindowID id, const wxString& label,
	const wxPoint& pos, const wxSize& size, long style_flags)
	: wxControl(parent, id, pos, size, style_flags | wxBORDER_NONE)
	, m_label(label)
	, m_enabled(true)
	, m_checked(false)
	, m_borderWidth(DEFAULT_BORDER_WIDTH)
	, m_radioButtonSize(wxSize(DEFAULT_RADIO_BUTTON_SIZE, DEFAULT_RADIO_BUTTON_SIZE))
	, m_labelSpacing(DEFAULT_LABEL_SPACING)
	, m_useConfigFont(true)
	, m_isHovered(false)
	, m_isPressed(false)
	, m_hasFocus(false)
	, m_cachedGraphicsContext(nullptr)
	, m_lastPaintSize(0, 0)
	, m_needsRedraw(true)
{
	InitializeDefaultColors();
	ReloadFontFromConfig();

	if (size == wxDefaultSize) {
		SetInitialSize(DoGetBestSize());
	}

	ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
		InitializeDefaultColors();
		m_needsRedraw = true;
	Refresh();
		});
}

FlatRadioButton::~FlatRadioButton()
{
	// Clean up cached graphics context
	if (m_cachedGraphicsContext) {
		delete m_cachedGraphicsContext;
		m_cachedGraphicsContext = nullptr;
	}
}

void FlatRadioButton::InitializeDefaultColors()
{
	m_backgroundColor = CFG_COLOUR("SecondaryBackgroundColour");
	m_hoverColor = CFG_COLOUR("HomespaceHoverBgColour");
	m_checkedColor = CFG_COLOUR("AccentColour");
	m_textColor = CFG_COLOUR("PrimaryTextColour");
	m_borderColor = CFG_COLOUR("ButtonBorderColour");
	m_disabledColor = CFG_COLOUR("PanelDisabledBgColour");
}

void FlatRadioButton::SetLabel(const wxString& label)
{
	m_label = label;
	m_needsRedraw = true;
	Refresh();
}

void FlatRadioButton::SetValue(bool checked)
{
	if (m_checked != checked) {
		m_checked = checked;
		m_needsRedraw = true;
	Refresh();
	}
}

bool FlatRadioButton::Enable(bool enabled)
{
	m_enabled = enabled;
	wxControl::Enable(enabled);
	m_needsRedraw = true;
	Refresh();
	return true;
}

wxSize FlatRadioButton::DoGetBestSize() const
{
	wxSize textSize = GetTextExtent(m_label);
	int width = m_radioButtonSize.GetWidth() + m_labelSpacing + textSize.GetWidth() + 2 * m_borderWidth;
	int height = wxMax(m_radioButtonSize.GetHeight(), textSize.GetHeight()) + 2 * m_borderWidth;
	return wxSize(width, height);
}

void FlatRadioButton::OnPaint(wxPaintEvent& event)
{
	wxAutoBufferedPaintDC dc(this);
	wxSize size = GetClientSize();

	// Check if we need to recreate graphics context due to size change
	bool sizeChanged = (size != m_lastPaintSize);
	bool needNewContext = !m_cachedGraphicsContext || sizeChanged;

	if (needNewContext) {
		// Clean up old context
		if (m_cachedGraphicsContext) {
			delete m_cachedGraphicsContext;
			m_cachedGraphicsContext = nullptr;
		}

		// Create new graphics context
		m_cachedGraphicsContext = wxGraphicsContext::Create(dc);
		if (!m_cachedGraphicsContext) {
			event.Skip();
			return;
		}

		m_lastPaintSize = size;
		m_needsRedraw = true;
	}

	// Skip drawing if nothing changed and we have a cached context
	if (!m_needsRedraw && m_cachedGraphicsContext) {
		event.Skip();
		return;
	}

	// Use cached graphics context
	wxGraphicsContext* gc = m_cachedGraphicsContext;

	DrawBackground(*gc);
	DrawRadioButton(*gc);
	DrawText(*gc);

	// Mark that we've completed drawing
	m_needsRedraw = false;

	// Note: Don't delete gc here as it's cached
}

void FlatRadioButton::OnSize(wxSizeEvent& event)
{
	m_needsRedraw = true;
	m_needsRedraw = true;
	Refresh();
	event.Skip();
}

void FlatRadioButton::OnMouseDown(wxMouseEvent& event)
{
	if (!m_enabled) return;
	m_isPressed = true;
	SetFocus();
	m_needsRedraw = true;
	Refresh();
	event.Skip();
}

void FlatRadioButton::OnMouseUp(wxMouseEvent& event)
{
	if (!m_enabled || !m_isPressed) return;
	m_isPressed = false;

	wxPoint mousePos = event.GetPosition();
	if (GetClientRect().Contains(mousePos)) {
		if (!m_checked) {
			m_checked = true;
			SendRadioButtonEvent();

			// Uncheck other radio buttons in the same group
			wxWindow* parent = GetParent();
			if (parent) {
				for (wxWindow* child : parent->GetChildren()) {
					FlatRadioButton* other = dynamic_cast<FlatRadioButton*>(child);
					if (other && other != this) {
						other->SetValue(false);
					}
				}
			}
		}
	}
	m_needsRedraw = true;
	Refresh();
}

void FlatRadioButton::OnMouseMove(wxMouseEvent& event)
{
	if (!m_enabled) return;
	bool wasHovered = m_isHovered;
	m_isHovered = true;
	if (!wasHovered) {
		m_needsRedraw = true;
	Refresh();
	}
	event.Skip();
}

void FlatRadioButton::OnMouseLeave(wxMouseEvent& event)
{
	if (!m_enabled) return;
	m_isHovered = false;
	m_needsRedraw = true;
	Refresh();
	event.Skip();
}

void FlatRadioButton::OnMouseEnter(wxMouseEvent& event)
{
	if (!m_enabled) return;
	m_isHovered = true;
	m_needsRedraw = true;
	Refresh();
	event.Skip();
}

void FlatRadioButton::OnKeyUp(wxKeyEvent& event)
{
	if (!m_enabled) return;
	if (event.GetKeyCode() == WXK_SPACE) {
		if (!m_checked) {
			m_checked = true;
			SendRadioButtonEvent();
		}
	}
	event.Skip();
}

void FlatRadioButton::OnFocus(wxFocusEvent& event)
{
	m_hasFocus = true;
	m_needsRedraw = true;
	Refresh();
	event.Skip();
}

void FlatRadioButton::OnKillFocus(wxFocusEvent& event)
{
	m_hasFocus = false;
	m_needsRedraw = true;
	Refresh();
	event.Skip();
}

void FlatRadioButton::DrawBackground(wxGraphicsContext& gc)
{
	wxRect rect = GetClientRect();
	gc.SetBrush(wxBrush(GetCurrentBackgroundColor()));
	gc.SetPen(wxPen(GetCurrentBackgroundColor()));
	gc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);
}

void FlatRadioButton::DrawRadioButton(wxGraphicsContext& gc)
{
	wxRect radioRect = GetRadioButtonRect();
	if (radioRect.IsEmpty()) return;

	gc.SetPen(wxPen(GetCurrentBorderColor(), m_borderWidth));
	gc.SetBrush(wxBrush(m_checked ? m_checkedColor : GetCurrentBackgroundColor()));

	int centerX = radioRect.x + radioRect.width / 2;
	int centerY = radioRect.y + radioRect.height / 2;
	int radius = wxMin(radioRect.width, radioRect.height) / 2 - 2;

	gc.DrawEllipse(centerX - radius, centerY - radius, 2 * radius, 2 * radius);

	if (m_checked) {
		gc.SetBrush(wxBrush(m_checkedColor));
		gc.DrawEllipse(centerX - (radius - 4), centerY - (radius - 4), 2 * (radius - 4), 2 * (radius - 4));
	}
}

void FlatRadioButton::DrawText(wxGraphicsContext& gc)
{
	wxRect textRect = GetTextRect();
	if (textRect.IsEmpty() || m_label.IsEmpty()) return;

	wxFont currentFont = GetFont();
	if (currentFont.IsOk()) {
		gc.SetFont(currentFont, GetCurrentTextColor());
	}
	else {
		gc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT), GetCurrentTextColor());
	}
	gc.DrawText(m_label, textRect.x, textRect.y);
}

void FlatRadioButton::SendRadioButtonEvent()
{
	wxCommandEvent event(wxEVT_FLAT_RADIO_BUTTON_STATE_CHANGED, GetId());
	event.SetEventObject(this);
	event.SetInt(m_checked);
	ProcessWindowEvent(event);
}

wxRect FlatRadioButton::GetRadioButtonRect() const
{
	wxRect clientRect = GetClientRect();
	return wxRect(
		clientRect.x + m_borderWidth,
		(clientRect.height - m_radioButtonSize.GetHeight()) / 2,
		m_radioButtonSize.GetWidth(),
		m_radioButtonSize.GetHeight()
	);
}

wxRect FlatRadioButton::GetTextRect() const
{
	wxRect clientRect = GetClientRect();
	wxRect radioRect = GetRadioButtonRect();
	return wxRect(
		radioRect.x + radioRect.width + m_labelSpacing,
		(clientRect.height - GetTextExtent(m_label).GetHeight()) / 2,
		clientRect.width - radioRect.width - m_labelSpacing - 2 * m_borderWidth,
		GetTextExtent(m_label).GetHeight()
	);
}

wxColour FlatRadioButton::GetCurrentBackgroundColor() const
{
	if (!m_enabled) return m_disabledColor;
	if (m_isHovered) return m_hoverColor;
	return m_backgroundColor;
}

wxColour FlatRadioButton::GetCurrentBorderColor() const
{
	if (!m_enabled) return wxColour(200, 200, 200);
	return m_borderColor;
}

wxColour FlatRadioButton::GetCurrentTextColor() const
{
	if (!m_enabled) return wxColour(128, 128, 128);
	return m_textColor;
}

void FlatRadioButton::SetCustomFont(const wxFont& font)
{
	m_customFont = font;
	m_useConfigFont = false;
	SetFont(font);
	InvalidateBestSize();
	m_needsRedraw = true;
	Refresh();
}

void FlatRadioButton::UseConfigFont(bool useConfig)
{
	m_useConfigFont = useConfig;
	if (useConfig) {
		ReloadFontFromConfig();
	}
}

void FlatRadioButton::ReloadFontFromConfig()
{
	if (m_useConfigFont) {
		try {
			FontManager& fontManager = FontManager::getInstance();
			wxFont configFont = fontManager.getLabelFont();
			if (configFont.IsOk()) {
				SetFont(configFont);
				InvalidateBestSize();
				m_needsRedraw = true;
	Refresh();
			}
		}
		catch (...) {
			wxFont defaultFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
			SetFont(defaultFont);
		}
	}
}