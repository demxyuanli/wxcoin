#include "widgets/FlatWidgetsExampleDialog.h"
#include "widgets/FlatWidgetsButtonsPanel.h"
#include "widgets/FlatWidgetsInputsPanel.h"
#include "widgets/FlatWidgetsSelectionPanel.h"
#include "widgets/FlatWidgetsProgressPanel.h"
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/notebook.h>
#include <wx/timer.h>

wxBEGIN_EVENT_TABLE(FlatWidgetsExampleDialog, wxDialog)
EVT_TIMER(wxID_ANY, FlatWidgetsExampleDialog::OnProgressTimer)
wxEND_EVENT_TABLE()

FlatWidgetsExampleDialog::FlatWidgetsExampleDialog(wxWindow* parent, const wxString& title)
	: wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(1200, 700),
		wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX),
	m_notebook(nullptr),
	m_progressTimer(nullptr)
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

FlatWidgetsExampleDialog::~FlatWidgetsExampleDialog()
{
	if (m_progressTimer) {
		m_progressTimer->Stop();
		delete m_progressTimer;
	}
}

void FlatWidgetsExampleDialog::CreateControls()
{
	// Create notebook for tabs
	m_notebook = new wxNotebook(this, wxID_ANY);
	
	// Create different category panels
	m_buttonsPanel = new FlatWidgetsButtonsPanel(m_notebook);
	m_inputsPanel = new FlatWidgetsInputsPanel(m_notebook);
	m_selectionPanel = new FlatWidgetsSelectionPanel(m_notebook);
	m_progressPanel = new FlatWidgetsProgressPanel(m_notebook);
	
	// Add panels to notebook
	m_notebook->AddPage(m_buttonsPanel, "Buttons", true);
	m_notebook->AddPage(m_inputsPanel, "Inputs", false);
	m_notebook->AddPage(m_selectionPanel, "Selection", false);
	m_notebook->AddPage(m_progressPanel, "Progress", false);
	
	// Create progress animation timer
	m_progressTimer = new wxTimer(this, wxID_ANY);
	Bind(wxEVT_TIMER, &FlatWidgetsExampleDialog::OnProgressTimer, this);
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

	// Add the notebook
	mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 10);

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

void FlatWidgetsExampleDialog::OnProgressTimer(wxTimerEvent& event)
{
	// Update progress bars with animation
	if (m_progressPanel) {
		m_progressPanel->UpdateProgressAnimation();
	}
}