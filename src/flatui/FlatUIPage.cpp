#include "flatui/FlatUIPage.h"
#include "flatui/FlatUIEventManager.h"
#include "config/SvgIconManager.h"
#include "flatui/FlatUIPanel.h"
#include "flatui/FlatUIBar.h"
#include "logger/Logger.h"
#include <wx/dcbuffer.h>
#include "config/ThemeManager.h"

FlatUIPage::FlatUIPage(wxWindow* parent, const wxString& label)
	: wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
	m_label(label),
	m_isActive(false)
{
	SetFont(CFG_DEFAULTFONT());
	SetDoubleBuffered(true);
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	wxColour bg = CFG_COLOUR("ActBarBackgroundColour");
	SetBackgroundColour(bg);
	m_backgroundColour = bg;

	// Register theme change listener
	ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
		RefreshTheme();
		});

#ifdef __WXMSW__
	HWND hwnd = (HWND)GetHandle();
	if (hwnd) {
		long exStyle = ::GetWindowLong(hwnd, GWL_EXSTYLE);
		::SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_COMPOSITED);
	}
#endif

	m_sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(m_sizer);

	FlatUIEventManager::getInstance().bindPageEvents(this);

	Bind(wxEVT_PAINT, &FlatUIPage::OnPaint, this);
	Bind(wxEVT_SIZE, &FlatUIPage::OnSize, this);


}

FlatUIPage::~FlatUIPage()
{
	// Remove theme change listener
	ThemeManager::getInstance().removeThemeChangeListener(this);

	for (auto panel : m_panels)
		delete panel;
}

// Update layout method to position pin control
void FlatUIPage::UpdateLayout()
{
	// This method is now empty but might be used for other layout updates later.
}

void FlatUIPage::OnPaint(wxPaintEvent& evt)
{
	wxAutoBufferedPaintDC dc(this);
	wxSize size = GetSize();

	dc.SetBackground(m_backgroundColour);

	dc.Clear();

	// dc.SetPen(wxPen(CFG_COLOUR("PanelBorderColour"), CFG_INT("PanelBorderWidth")));

	//dc.DrawLine(2, 0, size.GetWidth()-2, 0);

	// dc.DrawLine(2, size.GetHeight() - 1, size.GetWidth()-2, size.GetHeight() - 1);

	// Ribbon style: Page typically doesn't have its own prominent border distinct from the active tab
	// or panel borders within it. A very subtle bottom line might be okay.
	// For now, let's remove the explicit border drawing here, relying on panel borders if any.

	// dc.SetPen(wxPen(FLATUI_DEFAULT_BG_COLOUR, 1)); // Was drawing top, left, bottom in bg color
	// dc.DrawLine(0, 0, size.GetWidth(), 0); // Top
	// dc.DrawLine(0, 0, 0, size.GetHeight()); // Left
	// dc.DrawLine(0, size.GetHeight() - 1, size.GetWidth(), size.GetHeight() - 1); // Bottom
	// dc.SetPen(wxPen(FLATUI_DEFAULT_BORDER_COLOUR, 1)); // Was drawing right border
	// dc.DrawLine(size.GetWidth() - 1, 0, size.GetWidth() - 1, size.GetHeight()); // Right

	// Ribbon style: Remove the page label drawing from within the page itself.
	// The label is shown on the tab in FlatUIBar.
	// dc.SetTextForeground(*wxBLACK);
	// dc.SetFont(wxFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	// dc.DrawText(GetLabel(), 0, 0);

	// LOG_DBG("Page painted: " + GetLabel().ToStdString() +
	//     ", Size: (" + std::to_string(size.GetWidth()) +
	//     ", " + std::to_string(size.GetHeight()) + ")",
	//     "FlatUIPage::OnPaint");

	evt.Skip();
}

void FlatUIPage::OnSize(wxSizeEvent& evt)
{
	wxSize newSize = evt.GetSize();

	// LOG_DBG("Page resized: " + GetLabel().ToStdString() +
	//     ", New Size: (" + std::to_string(newSize.GetWidth()) +
	//     ", " + std::to_string(newSize.GetHeight()) + ")",
	//     "FlatUIPage::OnSize");

	if (m_sizer) {
		wxEventBlocker blocker(this, wxEVT_SIZE);
		RecalculatePageHeight();
		Layout();
	}

	Refresh(false);
	evt.Skip();
}

void FlatUIPage::RecalculatePageHeight()
{
	static bool isRecalculating = false;
	if (isRecalculating)
		return;

	isRecalculating = true;

	Freeze();

	if (m_panels.empty()) {
		wxSize defaultSize(100, 60);
		SetMinSize(defaultSize);
		if (m_sizer) {
			m_sizer->SetDimension(0, 0, defaultSize.GetWidth(), defaultSize.GetHeight());
		}
		//LOG_INF_S("RecalculatePageHeight: Empty page " + GetLabel().ToStdString() + ", Set default size: (100,100)", "FlatUIPage");
		isRecalculating = false;
		Thaw();
		return;
	}

	bool wasHidden = !IsShown();
	if (wasHidden)
		Show();

	int maxHeight = 0;
	int totalWidth = 0;

	for (auto panel : m_panels) {
		if (!panel) continue;

		panel->UpdatePanelSize();
		wxSize panelBestSize = panel->GetBestSize();
		totalWidth += panelBestSize.GetWidth();
		maxHeight = wxMax(maxHeight, panelBestSize.GetHeight());

	}

	wxSize newMinSize(totalWidth, maxHeight);
	SetMinSize(newMinSize);

	if (m_sizer) {
		m_sizer->SetDimension(0, 0, totalWidth, maxHeight);
	}


	if (wasHidden)
		Hide();

	wxWindow* parent = GetParent();
	if (parent) {
		parent->Layout();
	}

	isRecalculating = false;
	Thaw();
}

void FlatUIPage::AddPanel(FlatUIPanel* panel)
{
	Freeze();
	m_panels.push_back(panel);

	wxBoxSizer* boxSizer = dynamic_cast<wxBoxSizer*>(GetSizer());
	if (!boxSizer) {
		boxSizer = new wxBoxSizer(wxHORIZONTAL);
		SetSizer(boxSizer);
	}

	panel->UpdatePanelSize();
	wxSize minSize = panel->GetBestSize();
	panel->SetMinSize(minSize);

	boxSizer->Add(panel, 0, wxALL, 1);

	wxSize pageSizeForLog = GetSize();
	wxSize panelSizeForLog = panel->GetSize();


	panel->Show();
	RecalculatePageHeight();
	Layout();
	Refresh(false);

	Thaw();
}

void FlatUIPage::InitializeLayout()
{
	Freeze();
	wxEventBlocker blocker(this, wxEVT_SIZE);
	for (auto panel : m_panels) {
		panel->UpdatePanelSize();
	}
	RecalculatePageHeight();
	Layout();
	Thaw();

}

void FlatUIPage::RefreshTheme()
{
	// Update all theme-based colors and settings
	wxColour bg = CFG_COLOUR("ActBarBackgroundColour");
	m_backgroundColour = bg;

	// Update control properties
	SetFont(CFG_DEFAULTFONT());
	SetBackgroundColour(bg);

	// Refresh all child panels
	for (auto panel : m_panels) {
		if (panel) {
			panel->Refresh(true);
			panel->Update();
		}
	}

	// Force refresh
	Refresh(true);
	Update();
}