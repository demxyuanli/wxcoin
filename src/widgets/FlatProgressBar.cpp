#include "widgets/FlatProgressBar.h"
#include <wx/dcclient.h>
#include "config/ThemeManager.h"

wxDEFINE_EVENT(wxEVT_FLAT_PROGRESS_BAR_VALUE_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_PROGRESS_BAR_COMPLETED, wxCommandEvent);

BEGIN_EVENT_TABLE(FlatProgressBar, wxControl)
EVT_PAINT(FlatProgressBar::OnPaint)
EVT_SIZE(FlatProgressBar::OnSize)
END_EVENT_TABLE()

FlatProgressBar::FlatProgressBar(wxWindow* parent, wxWindowID id, int value, int minValue, int maxValue,
	const wxPoint& pos, const wxSize& size, ProgressBarStyle style, long style_flags)
	: wxControl(parent, id, pos, size, style_flags | wxBORDER_NONE)
	, m_progressBarStyle(style)
	, m_state(ProgressBarState::DEFAULT_STATE)
	, m_enabled(true)
	, m_value(value)
	, m_minValue(minValue)
	, m_maxValue(maxValue)
	, m_showPercentage(false)
	, m_showValue(false)
	, m_showLabel(false)
	, m_striped(false)
	, m_animated(false)
	, m_borderWidth(DEFAULT_BORDER_WIDTH)
	, m_cornerRadius(DEFAULT_CORNER_RADIUS)
	, m_barHeight(DEFAULT_BAR_HEIGHT)
{
	// Initialize default colors based on style
	InitializeDefaultColors();

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

FlatProgressBar::~FlatProgressBar()
{
	ThemeManager::getInstance().removeThemeChangeListener(this);
}

void FlatProgressBar::InitializeDefaultColors()
{
	// Theme-based colors
	m_backgroundColor = CFG_COLOUR("SecondaryBackgroundColour");
	m_progressColor = CFG_COLOUR("AccentColour");
	m_textColor = CFG_COLOUR("PrimaryTextColour");
	m_borderColor = CFG_COLOUR("ButtonBorderColour");
}

void FlatProgressBar::SetProgressBarStyle(ProgressBarStyle style)
{
	m_progressBarStyle = style;
	InitializeDefaultColors();
	this->Refresh();
}

FlatProgressBar::ProgressBarStyle FlatProgressBar::GetProgressBarStyle() const
{
	return m_progressBarStyle;
}

void FlatProgressBar::SetLabel(const wxString& label)
{
	m_label = label;
	Refresh();
}

wxString FlatProgressBar::GetLabel() const
{
	return m_label;
}

void FlatProgressBar::SetValue(int value)
{
	if (value < m_minValue) value = m_minValue;
	if (value > m_maxValue) value = m_maxValue;

	if (m_value != value) {
		m_value = value;
		Refresh();

		// Send value changed event
		wxCommandEvent event(wxEVT_FLAT_PROGRESS_BAR_VALUE_CHANGED, this->GetId());
		event.SetInt(m_value);
		this->ProcessWindowEvent(event);

		// Send completed event if value reached max
		if (m_value >= m_maxValue) {
			wxCommandEvent completedEvent(wxEVT_FLAT_PROGRESS_BAR_COMPLETED, this->GetId());
			this->ProcessWindowEvent(completedEvent);
		}
	}
}

int FlatProgressBar::GetValue() const
{
	return m_value;
}

void FlatProgressBar::SetRange(int minValue, int maxValue)
{
	if (minValue < maxValue) {
		m_minValue = minValue;
		m_maxValue = maxValue;

		// Adjust current value if needed
		if (m_value < m_minValue) m_value = m_minValue;
		if (m_value > m_maxValue) m_value = m_maxValue;

		Refresh();
	}
}

void FlatProgressBar::GetRange(int& minValue, int& maxValue) const
{
	minValue = m_minValue;
	maxValue = m_maxValue;
}

void FlatProgressBar::SetShowPercentage(bool showPercentage)
{
	m_showPercentage = showPercentage;
	Refresh();
}

bool FlatProgressBar::IsShowPercentage() const
{
	return m_showPercentage;
}

void FlatProgressBar::SetShowValue(bool showValue)
{
	m_showValue = showValue;
	Refresh();
}

bool FlatProgressBar::IsShowValue() const
{
	return m_showValue;
}

void FlatProgressBar::SetShowLabel(bool showLabel)
{
	m_showLabel = showLabel;
	Refresh();
}

bool FlatProgressBar::IsShowLabel() const
{
	return m_showLabel;
}

void FlatProgressBar::SetStriped(bool striped)
{
	m_striped = striped;
	Refresh();
}

bool FlatProgressBar::IsStriped() const
{
	return m_striped;
}

void FlatProgressBar::SetAnimated(bool animated)
{
	m_animated = animated;
	Refresh();
}

bool FlatProgressBar::IsAnimated() const
{
	return m_animated;
}

void FlatProgressBar::SetBackgroundColor(const wxColour& color)
{
	m_backgroundColor = color;
	Refresh();
}

wxColour FlatProgressBar::GetBackgroundColor() const
{
	return m_backgroundColor;
}

void FlatProgressBar::SetProgressColor(const wxColour& color)
{
	m_progressColor = color;
	Refresh();
}

wxColour FlatProgressBar::GetProgressColor() const
{
	return m_progressColor;
}

void FlatProgressBar::SetTextColor(const wxColour& color)
{
	m_textColor = color;
	Refresh();
}

wxColour FlatProgressBar::GetTextColor() const
{
	return m_textColor;
}

void FlatProgressBar::SetBorderColor(const wxColour& color)
{
	m_borderColor = color;
	Refresh();
}

wxColour FlatProgressBar::GetBorderColor() const
{
	return m_borderColor;
}

void FlatProgressBar::SetBorderWidth(int width)
{
	m_borderWidth = width;
	Refresh();
}

int FlatProgressBar::GetBorderWidth() const
{
	return m_borderWidth;
}

void FlatProgressBar::SetCornerRadius(int radius)
{
	m_cornerRadius = radius;
	Refresh();
}

int FlatProgressBar::GetCornerRadius() const
{
	return m_cornerRadius;
}

void FlatProgressBar::SetBarHeight(int height)
{
	m_barHeight = height;
	Refresh();
}

int FlatProgressBar::GetBarHeight() const
{
	return m_barHeight;
}

void FlatProgressBar::SetEnabled(bool enabled)
{
	m_enabled = enabled;
	wxControl::Enable(enabled);
	Refresh();
}

bool FlatProgressBar::IsEnabled() const
{
	return m_enabled;
}

void FlatProgressBar::SetState(ProgressBarState state)
{
	if (m_state != state) {
		m_state = state;
		Refresh();
	}
}

// Missing function implementations
wxSize FlatProgressBar::DoGetBestSize() const
{
	int width = 200;  // Default width
	int height = m_barHeight + 2 * m_borderWidth;

	if (m_showLabel || !m_label.IsEmpty()) {
		wxSize textSize = GetTextExtent(m_label);
		height += textSize.GetHeight() + 5;
	}

	return wxSize(width, height);
}

void FlatProgressBar::OnPaint(wxPaintEvent& WXUNUSED(event))
{
	wxPaintDC dc(this);
	DrawProgressBar(dc);
}

void FlatProgressBar::OnSize(wxSizeEvent& event)
{
	Refresh();
	event.Skip();
}

void FlatProgressBar::DrawProgressBar(wxDC& dc)
{
	wxRect rect = this->GetClientRect();

	// Draw background
	dc.SetBrush(wxBrush(m_backgroundColor));
	dc.SetPen(wxPen(m_borderColor, m_borderWidth));
	dc.DrawRoundedRectangle(rect, m_cornerRadius);

	// Calculate progress width
	int progressWidth = 0;
	if (m_maxValue > m_minValue) {
		progressWidth = (int)((double)(m_value - m_minValue) / (m_maxValue - m_minValue) * rect.width);
	}

	if (progressWidth > 0) {
		// Draw progress bar
		wxRect progressRect = rect;
		progressRect.width = progressWidth;

		dc.SetBrush(wxBrush(m_progressColor));
		dc.SetPen(wxPen(m_progressColor, 1));
		dc.DrawRoundedRectangle(progressRect, m_cornerRadius);
	}

	// Draw text
	if (m_showLabel || m_showValue || m_showPercentage) {
		wxString text;
		if (m_showLabel && !m_label.IsEmpty()) {
			text = m_label;
		}
		else if (m_showValue) {
			text = wxString::Format("%d", m_value);
		}
		else if (m_showPercentage) {
			int percentage = 0;
			if (m_maxValue > m_minValue) {
				percentage = (int)((double)(m_value - m_minValue) / (m_maxValue - m_minValue) * 100);
			}
			text = wxString::Format("%d%%", percentage);
		}

		if (!text.IsEmpty()) {
			dc.SetTextForeground(m_textColor);
			dc.SetFont(this->GetFont());

			wxSize textSize = dc.GetTextExtent(text);
			int x = (rect.width - textSize.x) / 2;
			int y = (rect.height - textSize.y) / 2;

			dc.DrawText(text, x, y);
		}
	}
}