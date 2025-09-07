#include "flatui/FlatUITitledPanel.h"
#include <wx/bmpbuttn.h>
#include <wx/menu.h>
#include "config/SvgIconManager.h"

#ifndef SVG_ICON
#define SVG_ICON(name, size) SvgIconManager::GetInstance().GetBitmapBundle(name, size).GetBitmap(size)
#endif

class FlatUITitledPanelMenuButton : public wxBitmapButton {
public:
	FlatUITitledPanelMenuButton(wxWindow* parent)
		: wxBitmapButton(parent, wxID_ANY, SVG_ICON("menu", wxSize(8, 8)),
			wxDefaultPosition, wxSize(16, 16), wxBORDER_NONE)
	{
		SetToolTip(wxT("Menu"));
		SetBackgroundColour(parent->GetBackgroundColour());
		SetWindowStyleFlag(wxBORDER_NONE);
	}
};
class FlatUITitledPanelCollapseButton : public wxBitmapButton {
public:
	FlatUITitledPanelCollapseButton(wxWindow* parent)
		: wxBitmapButton(parent, wxID_ANY, SVG_ICON("collapse", wxSize(8, 8)),
			wxDefaultPosition, wxSize(16, 16), wxBORDER_NONE)
	{
		SetToolTip(wxT("Collapse"));
		SetBackgroundColour(parent->GetBackgroundColour());
		SetWindowStyleFlag(wxBORDER_NONE);
	}
};
class FlatUITitledPanelMaximizeButton : public wxBitmapButton {
public:
	FlatUITitledPanelMaximizeButton(wxWindow* parent)
		: wxBitmapButton(parent, wxID_ANY, SVG_ICON("max", wxSize(8, 8)),
			wxDefaultPosition, wxSize(16, 16), wxBORDER_NONE)
	{
		SetToolTip(wxT("Maximize"));
		SetBackgroundColour(parent->GetBackgroundColour());
		SetWindowStyleFlag(wxBORDER_NONE);
	}
};

FlatUITitledPanel::FlatUITitledPanel(wxWindow* parent, const wxString& title, long style)
	: FlatUIThemeAware(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, style)
{
	wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

	m_titleBar = new wxPanel(this);
	m_titleBar->SetMinSize(wxSize(-1, 16));
	wxBoxSizer* titleBarSizer = new wxBoxSizer(wxHORIZONTAL);
	m_titleLabel = new wxStaticText(m_titleBar, wxID_ANY, title);
	m_titleLabel->SetForegroundColour(GetThemeColour("TitledPanelHeaderTextColour"));
	m_titleLabel->SetFont(GetThemeFont());
	titleBarSizer->Add(m_titleLabel, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
	m_toolBarSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBitmapButton* menuBtn = new FlatUITitledPanelMenuButton(m_titleBar);
	wxBitmapButton* collapseBtn = new FlatUITitledPanelCollapseButton(m_titleBar);
	wxBitmapButton* maximizeBtn = new FlatUITitledPanelMaximizeButton(m_titleBar);
	m_toolBarSizer->Add(menuBtn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 2);
	m_toolBarSizer->Add(collapseBtn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 2);
	m_toolBarSizer->Add(maximizeBtn, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 2);
	titleBarSizer->AddStretchSpacer(1);
	titleBarSizer->Add(m_toolBarSizer, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	m_titleBar->SetSizer(titleBarSizer);
	rootSizer->Add(m_titleBar, 0, wxEXPAND | wxALL, 0);

	m_mainSizer = new wxBoxSizer(wxVERTICAL);
	rootSizer->Add(m_mainSizer, 1, wxEXPAND | wxALL, 0);
	SetDoubleBuffered(true);
	SetSizer(rootSizer);
	Layout();
}

FlatUITitledPanel::~FlatUITitledPanel() {}

void FlatUITitledPanel::SetTitle(const wxString& title) {
	if (m_titleLabel) m_titleLabel->SetLabel(title);
}

void FlatUITitledPanel::AddToolButton(wxWindow* button) {
	if (m_toolBarSizer && button) {
		m_toolBarSizer->Add(button, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 2);
		m_toolBarSizer->Layout();
		m_titleBar->Layout();
	}
}

void FlatUITitledPanel::ClearToolButtons() {
	if (m_toolBarSizer) {
		m_toolBarSizer->Clear(true);
		m_toolBarSizer->Layout();
		m_titleBar->Layout();
	}
}

void FlatUITitledPanel::OnThemeChanged() {
	SetBackgroundColour(GetThemeColour("TitledPanelBgColour"));
	if (m_titleBar) m_titleBar->SetBackgroundColour(GetThemeColour("TitledPanelHeaderColour"));
	if (m_titleLabel) {
		m_titleLabel->SetForegroundColour(GetThemeColour("TitledPanelHeaderTextColour"));
		m_titleLabel->SetFont(GetThemeFont());
	}
	for (auto child : m_titleBar->GetChildren()) {
		auto btn = wxDynamicCast(child, wxBitmapButton);
		if (btn) btn->SetBackgroundColour(m_titleBar->GetBackgroundColour());
	}
	Refresh();
	Update();
}

void FlatUITitledPanel::UpdateThemeValues() {
	OnThemeChanged();
}