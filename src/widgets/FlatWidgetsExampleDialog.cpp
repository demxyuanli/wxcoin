#include "widgets/FlatWidgetsExampleDialog.h"
#include <wx/button.h>
#include <wx/stattext.h>

FlatWidgetsExampleDialog::FlatWidgetsExampleDialog(wxWindow* parent, const wxString& title)
	: wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(1200, 700),
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX)
{
	wxLogMessage("FlatWidgetsExampleDialog constructor started");

	CreateControls();
	wxLogMessage("FlatWidgetsExampleDialog controls created");

	LayoutDialog();
	wxLogMessage("FlatWidgetsExampleDialog layout completed");

	// Center the dialog on the parent
	if (parent) {
		CentreOnParent();
		wxLogMessage("FlatWidgetsExampleDialog centered on parent");
	}
	else {
		Centre();
		wxLogMessage("FlatWidgetsExampleDialog centered on screen");
	}

	wxLogMessage("FlatWidgetsExampleDialog constructor completed");
}

void FlatWidgetsExampleDialog::CreateControls()
{
	// Create the example panel
	m_examplePanel = new FlatWidgetsExample(this);
}

void FlatWidgetsExampleDialog::LayoutDialog()
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	// Add a title
	wxStaticText* titleText = new wxStaticText(this, wxID_ANY, "Flat Widgets Example - PyQt-Fluent-Widgets Style");
	wxFont titleFont = titleText->GetFont();
	titleFont.SetPointSize(12);
	titleFont.SetWeight(wxFONTWEIGHT_BOLD);
	titleText->SetFont(titleFont);

	mainSizer->Add(titleText, 0, wxALIGN_CENTER | wxALL, 10);

	// Add the example panel
	mainSizer->Add(m_examplePanel, 1, wxEXPAND | wxALL, 10);

	// Add a close button
	wxButton* closeButton = new wxButton(this, wxID_CLOSE, "Close");
	closeButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		Close();
		});

	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->AddStretchSpacer();
	buttonSizer->Add(closeButton, 0, wxALL, 5);

	mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);

	SetSizer(mainSizer);

	// Set minimum size
	SetMinSize(wxSize(800, 500));
}