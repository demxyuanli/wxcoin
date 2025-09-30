#include "widgets/FlatWidgetsExampleDialog.h"
#include "widgets/FlatWidgetsButtonsPanel.h"
#include "widgets/FlatWidgetsInputsPanel.h"
#include "widgets/FlatWidgetsSelectionPanel.h"
#include "widgets/FlatWidgetsProgressPanel.h"
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/notebook.h>
#include <wx/timer.h>

wxBEGIN_EVENT_TABLE(FlatWidgetsExampleDialog, FramelessModalPopup)
EVT_TIMER(wxID_ANY, FlatWidgetsExampleDialog::OnProgressTimer)
wxEND_EVENT_TABLE()

FlatWidgetsExampleDialog::FlatWidgetsExampleDialog(wxWindow* parent, const wxString& title)
	: FramelessModalPopup(parent, title, wxSize(1200, 700)),
	m_notebook(nullptr),
	m_progressTimer(nullptr)
{
	wxLogMessage("FlatWidgetsExampleDialog constructor started");

	// Set up title bar with icon
	SetTitleIcon("widgets", wxSize(20, 20));
	ShowTitleIcon(true);

	CreateControls();
	wxLogMessage("FlatWidgetsExampleDialog controls created");

	LayoutDialog();
	wxLogMessage("FlatWidgetsExampleDialog layout completed");

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
	m_progressTimer = new wxTimer(m_contentPanel, wxID_ANY);
	Bind(wxEVT_TIMER, &FlatWidgetsExampleDialog::OnProgressTimer, this);
}

void FlatWidgetsExampleDialog::LayoutDialog()
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	// Add a title
	wxStaticText* titleText = new wxStaticText(m_contentPanel, wxID_ANY, "Flat Widgets Example - PyQt-Fluent-Widgets Style");
	wxFont titleFont = titleText->GetFont();
	titleFont.SetPointSize(12);
	titleFont.SetWeight(wxFONTWEIGHT_BOLD);
	titleText->SetFont(titleFont);

	mainSizer->Add(titleText, 0, wxALIGN_CENTER | wxALL, 10);

	// Add the notebook
	mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 10);

	// Add a close button
	wxButton* closeButton = new wxButton(m_contentPanel, wxID_CLOSE, "Close");
	closeButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		Close();
		});

	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	buttonSizer->AddStretchSpacer();
	buttonSizer->Add(closeButton, 0, wxALL, 5);

	mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);

	m_contentPanel->SetSizer(mainSizer);
}

void FlatWidgetsExampleDialog::OnProgressTimer(wxTimerEvent& event)
{
	// Update progress bars with animation
	if (m_progressPanel) {
		m_progressPanel->UpdateProgressAnimation();
	}
}