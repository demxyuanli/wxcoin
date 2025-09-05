#include "widgets/FlatProgressBar.h"
#include <wx/dcclient.h>
#include <wx/dc.h>
#include <cmath>
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
	, m_textFollowProgress(false)
	, m_circularSize(DEFAULT_CIRCULAR_SIZE)
	, m_circularThickness(DEFAULT_CIRCULAR_THICKNESS)
	, m_showCircularText(true)
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
	// Choose drawing method based on style
	switch (m_progressBarStyle) {
	case ProgressBarStyle::MODERN_LINEAR:
		DrawModernLinearProgress(dc);
		break;
	case ProgressBarStyle::MODERN_CIRCULAR:
		DrawModernCircularProgress(dc);
		break;
	case ProgressBarStyle::CIRCULAR:
		DrawCircularProgress(dc);
		break;
	case ProgressBarStyle::INDETERMINATE:
		DrawIndeterminateProgress(dc);
		break;
	case ProgressBarStyle::STRIPED:
		DrawStripedProgress(dc);
		break;
	case ProgressBarStyle::DEFAULT_STYLE:
	default:
		DrawDefaultProgress(dc);
		break;
	}
}

// Modern style property implementations
void FlatProgressBar::SetTextFollowProgress(bool follow)
{
	m_textFollowProgress = follow;
	Refresh();
}

bool FlatProgressBar::IsTextFollowProgress() const
{
	return m_textFollowProgress;
}

void FlatProgressBar::SetCircularSize(int size)
{
	m_circularSize = size;
	Refresh();
}

int FlatProgressBar::GetCircularSize() const
{
	return m_circularSize;
}

void FlatProgressBar::SetCircularThickness(int thickness)
{
	m_circularThickness = thickness;
	Refresh();
}

int FlatProgressBar::GetCircularThickness() const
{
	return m_circularThickness;
}

void FlatProgressBar::SetShowCircularText(bool show)
{
	m_showCircularText = show;
	Refresh();
}

bool FlatProgressBar::IsShowCircularText() const
{
	return m_showCircularText;
}

// Modern style drawing implementations
void FlatProgressBar::DrawModernLinearProgress(wxDC& dc)
{
	wxRect rect = this->GetClientRect();
	
	// Draw background with modern styling
	wxColour bgColor = m_backgroundColor;
	bgColor = bgColor.ChangeLightness(95); // Slightly lighter background
	dc.SetBrush(wxBrush(bgColor));
	dc.SetPen(wxPen(bgColor, 0)); // No border for modern look
	dc.DrawRoundedRectangle(rect, m_cornerRadius);
	
	// Calculate progress width
	int progressWidth = 0;
	if (m_maxValue > m_minValue) {
		progressWidth = (int)((double)(m_value - m_minValue) / (m_maxValue - m_minValue) * rect.width);
	}
	
	if (progressWidth > 0) {
		// Draw progress bar with gradient effect
		wxRect progressRect = rect;
		progressRect.width = progressWidth;
		
		// Create gradient effect
		wxColour startColor = m_progressColor;
		wxColour endColor = m_progressColor.ChangeLightness(110);
		
		// Draw gradient background
		for (int i = 0; i < progressRect.height; i++) {
			double ratio = (double)i / progressRect.height;
			wxColour gradientColor = wxColour(
				startColor.Red() + (int)((endColor.Red() - startColor.Red()) * ratio),
				startColor.Green() + (int)((endColor.Green() - startColor.Green()) * ratio),
				startColor.Blue() + (int)((endColor.Blue() - startColor.Blue()) * ratio)
			);
			dc.SetPen(wxPen(gradientColor, 1));
			dc.DrawLine(progressRect.x, progressRect.y + i, 
						progressRect.x + progressRect.width, progressRect.y + i);
		}
		
		// Draw text following progress
		if (m_textFollowProgress && (m_showPercentage || m_showValue || m_showLabel)) {
			DrawTextFollowingProgress(dc, progressRect);
		}
	}
	
	// Draw text in center if not following progress
	if (!m_textFollowProgress && (m_showPercentage || m_showValue || m_showLabel)) {
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

void FlatProgressBar::DrawModernCircularProgress(wxDC& dc)
{
	wxRect rect = this->GetClientRect();
	
	// Calculate center and radius
	int centerX = rect.x + rect.width / 2;
	int centerY = rect.y + rect.height / 2;
	int radius = wxMin(m_circularSize / 2, wxMin(rect.width, rect.height) / 2 - m_circularThickness);
	
	// Draw background circle
	wxColour bgColor = m_backgroundColor;
	bgColor = bgColor.ChangeLightness(95);
	dc.SetBrush(wxBrush(bgColor));
	dc.SetPen(wxPen(bgColor, m_circularThickness));
	dc.DrawCircle(centerX, centerY, radius);
	
	// Calculate progress angle
	double progressAngle = 0.0;
	if (m_maxValue > m_minValue) {
		progressAngle = (double)(m_value - m_minValue) / (m_maxValue - m_minValue) * 360.0;
	}
	
	if (progressAngle > 0) {
		// Draw progress arc
		dc.SetBrush(wxBrush(wxColour(0, 0, 0, 0))); // Transparent brush
		dc.SetPen(wxPen(m_progressColor, m_circularThickness));
		
		// Draw arc from top (0 degrees) clockwise
		wxPoint center(centerX, centerY);
		wxPoint startPoint(centerX, centerY - radius);
		
		// Create arc points
		std::vector<wxPoint> arcPoints;
		for (int i = 0; i <= (int)progressAngle; i += 2) {
			double angle = i * M_PI / 180.0;
			int x = centerX + (int)(radius * sin(angle));
			int y = centerY - (int)(radius * cos(angle));
			arcPoints.push_back(wxPoint(x, y));
		}
		
		// Draw arc segments
		for (size_t i = 1; i < arcPoints.size(); i++) {
			dc.DrawLine(arcPoints[i-1], arcPoints[i]);
		}
	}
	
	// Draw center text
	if (m_showCircularText && (m_showPercentage || m_showValue || m_showLabel)) {
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
			wxFont font = this->GetFont();
			font.SetPointSize(font.GetPointSize() + 2); // Slightly larger for circular
			dc.SetFont(font);
			
			wxSize textSize = dc.GetTextExtent(text);
			int x = centerX - textSize.x / 2;
			int y = centerY - textSize.y / 2;
			
			dc.DrawText(text, x, y);
		}
	}
}

void FlatProgressBar::DrawTextFollowingProgress(wxDC& dc, const wxRect& progressRect)
{
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
		dc.SetTextForeground(wxColour(255, 255, 255)); // White text for contrast
		dc.SetFont(this->GetFont());
		
		wxSize textSize = dc.GetTextExtent(text);
		
		// Position text at the end of progress bar
		int x = progressRect.x + progressRect.width - textSize.x - 8;
		int y = progressRect.y + (progressRect.height - textSize.y) / 2;
		
		// Ensure text doesn't go outside the progress bar
		if (x < progressRect.x + 8) {
			x = progressRect.x + 8;
		}
		
		// Draw text with shadow for better visibility
		dc.SetTextForeground(wxColour(0, 0, 0, 128));
		dc.DrawText(text, x + 1, y + 1);
		dc.SetTextForeground(wxColour(255, 255, 255));
		dc.DrawText(text, x, y);
	}
}

// Missing method implementations
void FlatProgressBar::DrawStripes(wxDC& dc, const wxRect& rect)
{
	// Draw diagonal stripes
	wxColour stripeColor = m_progressColor.ChangeLightness(120);
	dc.SetPen(wxPen(stripeColor, 1));
	
	int stripeWidth = 8;
	int stripeSpacing = 4;
	
	for (int x = rect.x - stripeWidth; x < rect.x + rect.width + stripeWidth; x += stripeWidth + stripeSpacing) {
		// Draw diagonal stripe
		for (int i = 0; i < rect.height; i++) {
			int stripeX = x + i;
			if (stripeX >= rect.x && stripeX < rect.x + rect.width) {
				dc.DrawPoint(stripeX, rect.y + i);
			}
		}
	}
}

void FlatProgressBar::DrawCircularProgress(wxDC& dc)
{
	wxRect rect = this->GetClientRect();
	
	// Calculate center and radius
	int centerX = rect.x + rect.width / 2;
	int centerY = rect.y + rect.height / 2;
	int radius = wxMin(rect.width, rect.height) / 2 - 10;
	
	// Draw background circle
	dc.SetBrush(wxBrush(m_backgroundColor));
	dc.SetPen(wxPen(m_borderColor, 2));
	dc.DrawCircle(centerX, centerY, radius);
	
	// Calculate progress angle
	double progressAngle = 0.0;
	if (m_maxValue > m_minValue) {
		progressAngle = (double)(m_value - m_minValue) / (m_maxValue - m_minValue) * 360.0;
	}
	
	if (progressAngle > 0) {
		// Draw progress arc
		dc.SetBrush(wxBrush(wxColour(0, 0, 0, 0))); // Transparent brush
		dc.SetPen(wxPen(m_progressColor, 4));
		
		// Draw arc from top (0 degrees) clockwise
		wxPoint center(centerX, centerY);
		
		// Create arc points
		std::vector<wxPoint> arcPoints;
		for (int i = 0; i <= (int)progressAngle; i += 2) {
			double angle = i * M_PI / 180.0;
			int x = centerX + (int)(radius * sin(angle));
			int y = centerY - (int)(radius * cos(angle));
			arcPoints.push_back(wxPoint(x, y));
		}
		
		// Draw arc segments
		for (size_t i = 1; i < arcPoints.size(); i++) {
			dc.DrawLine(arcPoints[i-1], arcPoints[i]);
		}
	}
	
	// Draw center text
	if (m_showPercentage || m_showValue || m_showLabel) {
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
			wxFont font = this->GetFont();
			font.SetPointSize(font.GetPointSize() + 1);
			dc.SetFont(font);
			
			wxSize textSize = dc.GetTextExtent(text);
			int x = centerX - textSize.x / 2;
			int y = centerY - textSize.y / 2;
			
			dc.DrawText(text, x, y);
		}
	}
}

// Additional drawing methods for different styles
void FlatProgressBar::DrawDefaultProgress(wxDC& dc)
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

void FlatProgressBar::DrawIndeterminateProgress(wxDC& dc)
{
	wxRect rect = this->GetClientRect();

	// Draw background
	dc.SetBrush(wxBrush(m_backgroundColor));
	dc.SetPen(wxPen(m_borderColor, m_borderWidth));
	dc.DrawRoundedRectangle(rect, m_cornerRadius);

	// Draw animated indeterminate progress
	if (m_animated) {
		int barWidth = rect.width / 3;
		int x = (int)(m_animationProgress * (rect.width - barWidth));
		
		wxRect progressRect(x, rect.y, barWidth, rect.height);
		dc.SetBrush(wxBrush(m_progressColor));
		dc.SetPen(wxPen(m_progressColor, 1));
		dc.DrawRoundedRectangle(progressRect, m_cornerRadius);
	}
}

void FlatProgressBar::DrawStripedProgress(wxDC& dc)
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
		// Draw progress bar with stripes
		wxRect progressRect = rect;
		progressRect.width = progressWidth;

		dc.SetBrush(wxBrush(m_progressColor));
		dc.SetPen(wxPen(m_progressColor, 1));
		dc.DrawRoundedRectangle(progressRect, m_cornerRadius);

		// Draw stripes
		DrawStripes(dc, progressRect);
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