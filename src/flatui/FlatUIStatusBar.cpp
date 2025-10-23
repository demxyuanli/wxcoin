#include "flatui/FlatUIStatusBar.h"
#include "logger/AsyncLogger.h"

wxBEGIN_EVENT_TABLE(FlatUIStatusBar, FlatUIThemeAware)
EVT_PAINT(FlatUIStatusBar::OnPaint)
EVT_SIZE(FlatUIStatusBar::OnSize)
wxEND_EVENT_TABLE()

FlatUIStatusBar::FlatUIStatusBar(wxWindow* parent, wxWindowID id)
	: FlatUIThemeAware(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
{
	SetFieldsCount(1);
	SetMinSize(wxSize(-1, 24));
	UpdateThemeValues();

	// Create flat progress bar; keep hidden by default
	m_progress = new FlatProgressBar(this, wxID_ANY, 0, 0, 100, wxDefaultPosition, wxSize(140, 16));
	m_progress->SetShowPercentage(true);
	m_progress->SetShowValue(false); // Don't show raw value, only percentage
	m_progress->SetCornerRadius(8); // Make it more rounded
	m_progress->SetBorderWidth(1); // Add border for better visibility
	// Set more visible colors
	m_progress->SetBackgroundColor(wxColour(240, 240, 240)); // Light gray background
	m_progress->SetProgressColor(wxColour(0, 120, 215)); // Blue progress color
	m_progress->SetBorderColor(wxColour(200, 200, 200)); // Gray border
	m_progress->SetTextColor(wxColour(0, 0, 0)); // Black text
	m_progress->Hide();
}

void FlatUIStatusBar::SetFieldsCount(int count) {
	m_fields.resize(count);
	m_fieldWidths.resize(count, -1);
	Refresh();
}

void FlatUIStatusBar::SetStatusText(const wxString& text, int field) {
	if (field >= 0 && field < (int)m_fields.size()) {
		m_fields[field] = text;
		Refresh();
	}
}

void FlatUIStatusBar::SetFieldText(const wxString& text, int field) {
	SetStatusText(text, field);
}

void FlatUIStatusBar::OnThemeChanged() {
	UpdateThemeValues();
	Refresh();
}

void FlatUIStatusBar::UpdateThemeValues() {
	m_bgColour = GetThemeColour("StatusBarBgColour");
	m_textColour = GetThemeColour("StatusBarTextColour");
	m_borderColour = GetThemeColour("StatusBarBorderColour");
	m_font = GetThemeFont();
}

void FlatUIStatusBar::OnPaint(wxPaintEvent& evt) {
	wxPaintDC dc(this);
	dc.SetBackground(wxBrush(m_bgColour));
	dc.Clear();
	dc.SetFont(m_font);
	dc.SetTextForeground(m_textColour);

	int w, h;
	GetClientSize(&w, &h);

	int fieldCount = m_fields.size();
	int x = 4;
	int fieldWidth = w / (fieldCount > 0 ? fieldCount : 1);

	for (int i = 0; i < fieldCount; ++i) {
		wxRect rect(x, 0, fieldWidth, h);
		dc.DrawText(m_fields[i], rect.x + 4, rect.y + (h - dc.GetCharHeight()) / 2);
		x += fieldWidth;
		if (i < fieldCount - 1) {
			dc.SetPen(wxPen(m_borderColour));
			dc.DrawLine(x - 1, 2, x - 1, h - 2);
		}
	}

	LayoutChildren();
}

void FlatUIStatusBar::OnSize(wxSizeEvent& evt) {
	LayoutChildren();
	evt.Skip();
}

void FlatUIStatusBar::LayoutChildren() {
	if (!m_progress) return;
	if (!m_progress->IsShown()) return;
	int w, h; GetClientSize(&w, &h);
	int fieldCount = std::max(1, (int)m_fields.size());
	int fieldWidth = w / fieldCount;
	// Place gauge in rightmost field, centered vertically and with some padding
	wxRect rightRect((fieldCount - 1) * fieldWidth, 0, fieldWidth, h);
	int gw = std::min(180, std::max(100, rightRect.width - 16));
	int gh = std::min(18, std::max(12, h - 8));
	int gx = rightRect.x + (rightRect.width - gw) - 8; // right aligned with padding
	int gy = rightRect.y + (rightRect.height - gh) / 2;
	m_progress->SetSize(gw, gh);
	m_progress->Move(gx, gy);
}

void FlatUIStatusBar::EnableProgressGauge(bool enable) {
	if (!m_progress) return;
	if (enable) {
		m_progress->Show();
		m_progress->SetValue(0); // Reset to 0
		m_progress->Refresh();
		LOG_INF_S_ASYNC("Progress gauge enabled");
	} else {
		m_progress->Hide();
		LOG_INF_S_ASYNC("Progress gauge disabled");
	}
	LayoutChildren();
	Refresh();
}

void FlatUIStatusBar::SetGaugeRange(int range) {
	if (m_progress) m_progress->SetRange(0, range);
}

void FlatUIStatusBar::SetGaugeValue(int value) {
	if (m_progress) {
		m_progress->SetValue(value);
		// Force refresh to ensure progress is visible
		m_progress->Refresh();
	}
}

void FlatUIStatusBar::SetGaugeIndeterminate(bool indeterminate) {
	if (m_progress) {
		if (indeterminate) {
			// Set to indeterminate style with animation
			m_progress->SetProgressBarStyle(FlatProgressBar::ProgressBarStyle::INDETERMINATE);
			m_progress->SetAnimated(true);
			m_progress->SetValue(0); // Reset value for indeterminate mode
		} else {
			// Set back to default style
			m_progress->SetProgressBarStyle(FlatProgressBar::ProgressBarStyle::DEFAULT_STYLE);
			m_progress->SetAnimated(false);
		}
		m_progress->Refresh();
	}
}