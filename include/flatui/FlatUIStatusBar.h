#ifndef FLATUI_STATUS_BAR_H
#define FLATUI_STATUS_BAR_H

#include <wx/wx.h>
#include "flatui/FlatUIThemeAware.h"
#include <vector>
#include "widgets/FlatProgressBar.h"

class FlatUIStatusBar : public FlatUIThemeAware
{
public:
	FlatUIStatusBar(wxWindow* parent, wxWindowID id = wxID_ANY);

	void SetStatusText(const wxString& text, int field = 0);
	void SetFieldsCount(int count);
	void SetFieldText(const wxString& text, int field);

	// Progress bar API (rightmost field)
	void EnableProgressGauge(bool enable);
	void SetGaugeRange(int range);
	void SetGaugeValue(int value);

	virtual void OnThemeChanged() override;

protected:
	void OnPaint(wxPaintEvent& evt);
	void OnSize(wxSizeEvent& evt);
	void UpdateThemeValues() override;

private:
	std::vector<wxString> m_fields;
	std::vector<int> m_fieldWidths;
	wxColour m_bgColour, m_textColour, m_borderColour;
	wxFont m_font;
	FlatProgressBar* m_progress = nullptr;
	void LayoutChildren();

	wxDECLARE_EVENT_TABLE();
};

#endif // FLATUI_STATUS_BAR_H 