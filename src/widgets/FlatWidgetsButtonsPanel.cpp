#include "widgets/FlatWidgetsButtonsPanel.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/scrolwin.h>
#include <wx/artprov.h>
#include <wx/msgdlg.h>

wxBEGIN_EVENT_TABLE(FlatWidgetsButtonsPanel, wxPanel)
EVT_BUTTON(wxID_ANY, FlatWidgetsButtonsPanel::OnPrimaryButtonClicked)
wxEND_EVENT_TABLE()

FlatWidgetsButtonsPanel::FlatWidgetsButtonsPanel(wxWindow* parent)
	: wxPanel(parent, wxID_ANY)
{
	SetBackgroundColour(wxColour(250, 250, 250));
	
	CreateControls();
	LayoutPanel();
	BindEvents();
}

void FlatWidgetsButtonsPanel::CreateControls()
{
	// Create scrolled window first
	m_scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
	m_scrolledWindow->SetScrollRate(10, 10);
	m_scrolledWindow->SetBackgroundColour(wxColour(250, 250, 250));
	
	// Create FlatButton examples on the scrolled window
	m_primaryButton = new FlatButton(m_scrolledWindow, wxID_ANY, "Primary Button",
		wxDefaultPosition, wxDefaultSize,
		FlatButton::ButtonStyle::PRIMARY);
	m_secondaryButton = new FlatButton(m_scrolledWindow, wxID_ANY, "Secondary Button",
		wxDefaultPosition, wxDefaultSize,
		FlatButton::ButtonStyle::SECONDARY);
	m_outlineButton = new FlatButton(m_scrolledWindow, wxID_ANY, "Outline Button",
		wxDefaultPosition, wxDefaultSize,
		FlatButton::ButtonStyle::OUTLINE);
	m_textButton = new FlatButton(m_scrolledWindow, wxID_ANY, "Text Button",
		wxDefaultPosition, wxDefaultSize,
		FlatButton::ButtonStyle::TEXT);
	m_iconButton = new FlatButton(m_scrolledWindow, wxID_ANY, "",
		wxDefaultPosition, wxSize(40, 40),
		FlatButton::ButtonStyle::ICON_ONLY);
	m_iconButton->SetIcon(wxArtProvider::GetIcon(wxART_INFORMATION, wxART_BUTTON, wxSize(16, 16)));
}

void FlatWidgetsButtonsPanel::LayoutPanel()
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* scrolledSizer = new wxBoxSizer(wxVERTICAL);

	// Buttons section
	wxStaticBoxSizer* buttonSizer = new wxStaticBoxSizer(wxVERTICAL, m_scrolledWindow, "FlatButton Examples");
	wxBoxSizer* buttonRow1 = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* buttonRow2 = new wxBoxSizer(wxHORIZONTAL);

	buttonRow1->Add(m_primaryButton, 0, wxALL, 5);
	buttonRow1->Add(m_secondaryButton, 0, wxALL, 5);
	buttonRow1->Add(m_outlineButton, 0, wxALL, 5);
	buttonRow2->Add(m_textButton, 0, wxALL, 5);
	buttonRow2->Add(m_iconButton, 0, wxALL, 5);

	buttonSizer->Add(buttonRow1, 0, wxEXPAND);
	buttonSizer->Add(buttonRow2, 0, wxEXPAND);

	scrolledSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);

	m_scrolledWindow->SetSizer(scrolledSizer);
	mainSizer->Add(m_scrolledWindow, 1, wxEXPAND);

	SetSizer(mainSizer);
}

void FlatWidgetsButtonsPanel::BindEvents()
{
	m_primaryButton->Bind(wxEVT_BUTTON, &FlatWidgetsButtonsPanel::OnPrimaryButtonClicked, this);
	m_secondaryButton->Bind(wxEVT_BUTTON, &FlatWidgetsButtonsPanel::OnSecondaryButtonClicked, this);
	m_outlineButton->Bind(wxEVT_BUTTON, &FlatWidgetsButtonsPanel::OnOutlineButtonClicked, this);
	m_textButton->Bind(wxEVT_BUTTON, &FlatWidgetsButtonsPanel::OnTextButtonClicked, this);
	m_iconButton->Bind(wxEVT_BUTTON, &FlatWidgetsButtonsPanel::OnIconButtonClicked, this);
}

void FlatWidgetsButtonsPanel::OnPrimaryButtonClicked(wxCommandEvent& event)
{
	wxMessageBox("Primary button clicked!", "FlatWidgetsButtonsPanel", wxOK | wxICON_INFORMATION);
}

void FlatWidgetsButtonsPanel::OnSecondaryButtonClicked(wxCommandEvent& event)
{
	wxMessageBox("Secondary button clicked!", "FlatWidgetsButtonsPanel", wxOK | wxICON_INFORMATION);
}

void FlatWidgetsButtonsPanel::OnOutlineButtonClicked(wxCommandEvent& event)
{
	wxMessageBox("Outline button clicked!", "FlatWidgetsButtonsPanel", wxOK | wxICON_INFORMATION);
}

void FlatWidgetsButtonsPanel::OnTextButtonClicked(wxCommandEvent& event)
{
	wxMessageBox("Text button clicked!", "FlatWidgetsButtonsPanel", wxOK | wxICON_INFORMATION);
}

void FlatWidgetsButtonsPanel::OnIconButtonClicked(wxCommandEvent& event)
{
	wxMessageBox("Icon button clicked!", "FlatWidgetsButtonsPanel", wxOK | wxICON_INFORMATION);
}
