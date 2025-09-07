#include "widgets/FlatWidgetsProgressPanel.h"
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/scrolwin.h>
#include <wx/msgdlg.h>

wxBEGIN_EVENT_TABLE(FlatWidgetsProgressPanel, wxPanel)
EVT_BUTTON(wxID_ANY, FlatWidgetsProgressPanel::OnStartButtonClicked)
EVT_TIMER(wxID_ANY, FlatWidgetsProgressPanel::OnAnimationTimer)
wxEND_EVENT_TABLE()

FlatWidgetsProgressPanel::FlatWidgetsProgressPanel(wxWindow* parent)
	: wxPanel(parent, wxID_ANY),
	m_animationTimer(nullptr),
	m_animationValue(0),
	m_animationDirection(true),
	m_isAnimating(false)
{
	SetBackgroundColour(wxColour(250, 250, 250));
	
	CreateControls();
	LayoutPanel();
	BindEvents();
}

FlatWidgetsProgressPanel::~FlatWidgetsProgressPanel()
{
	if (m_animationTimer) {
		m_animationTimer->Stop();
		delete m_animationTimer;
	}
}

void FlatWidgetsProgressPanel::CreateControls()
{
	// Create scrolled window first
	m_scrolledWindow = new wxScrolledWindow(this, wxID_ANY);
	m_scrolledWindow->SetScrollRate(10, 10);
	m_scrolledWindow->SetBackgroundColour(wxColour(250, 250, 250));
	
	// Create FlatProgressBar examples on the scrolled window
	m_normalProgressBar = new FlatProgressBar(m_scrolledWindow, wxID_ANY, 60, 0, 100,
		wxDefaultPosition, wxSize(200, 20),
		FlatProgressBar::ProgressBarStyle::DEFAULT_STYLE);
	m_normalProgressBar->SetShowPercentage(true);
	m_normalProgressBar->SetShowValue(true);

	m_indeterminateProgressBar = new FlatProgressBar(m_scrolledWindow, wxID_ANY, 0, 0, 100,
		wxDefaultPosition, wxSize(200, 20),
		FlatProgressBar::ProgressBarStyle::INDETERMINATE);
	m_indeterminateProgressBar->SetShowLabel(true);
	m_indeterminateProgressBar->SetLabel("Loading...");
	m_indeterminateProgressBar->SetAnimated(true);

	m_stripedProgressBar = new FlatProgressBar(m_scrolledWindow, wxID_ANY, 80, 0, 100,
		wxDefaultPosition, wxSize(200, 20),
		FlatProgressBar::ProgressBarStyle::STRIPED);
	m_stripedProgressBar->SetShowPercentage(true);

	// Create modern style progress bars on the scrolled window
	m_modernLinearProgressBar = new FlatProgressBar(m_scrolledWindow, wxID_ANY, 45, 0, 100,
		wxDefaultPosition, wxSize(250, 25),
		FlatProgressBar::ProgressBarStyle::MODERN_LINEAR);
	m_modernLinearProgressBar->SetShowPercentage(true);
	m_modernLinearProgressBar->SetTextFollowProgress(true);
	m_modernLinearProgressBar->SetCornerRadius(12);

	m_modernCircularProgressBar = new FlatProgressBar(m_scrolledWindow, wxID_ANY, 75, 0, 100,
		wxDefaultPosition, wxSize(100, 100),
		FlatProgressBar::ProgressBarStyle::MODERN_CIRCULAR);
	m_modernCircularProgressBar->SetShowPercentage(true);
	m_modernCircularProgressBar->SetShowCircularText(true);
	m_modernCircularProgressBar->SetCircularSize(80);
	m_modernCircularProgressBar->SetCircularThickness(8);

	// Create control buttons on the scrolled window
	m_startButton = new wxButton(m_scrolledWindow, wxID_ANY, "Start Animation");
	m_stopButton = new wxButton(m_scrolledWindow, wxID_ANY, "Stop Animation");
	m_resetButton = new wxButton(m_scrolledWindow, wxID_ANY, "Reset");

	// Create animation timer
	m_animationTimer = new wxTimer(this, wxID_ANY);
}

void FlatWidgetsProgressPanel::LayoutPanel()
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* scrolledSizer = new wxBoxSizer(wxVERTICAL);

	// Control buttons section
	wxStaticBoxSizer* controlSizer = new wxStaticBoxSizer(wxVERTICAL, m_scrolledWindow, "Animation Controls");
	wxBoxSizer* buttonRow = new wxBoxSizer(wxHORIZONTAL);

	buttonRow->Add(m_startButton, 0, wxALL, 5);
	buttonRow->Add(m_stopButton, 0, wxALL, 5);
	buttonRow->Add(m_resetButton, 0, wxALL, 5);

	controlSizer->Add(buttonRow, 0, wxEXPAND);

	// ProgressBar section
	wxStaticBoxSizer* progressBarSizer = new wxStaticBoxSizer(wxVERTICAL, m_scrolledWindow, "FlatProgressBar Examples");
	wxBoxSizer* progressBarRow = new wxBoxSizer(wxVERTICAL);

	progressBarRow->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, "Normal Progress Bar:"), 0, wxBOTTOM, 2);
	progressBarRow->Add(m_normalProgressBar, 0, wxEXPAND | wxBOTTOM, 10);
	progressBarRow->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, "Indeterminate Progress Bar:"), 0, wxBOTTOM, 2);
	progressBarRow->Add(m_indeterminateProgressBar, 0, wxEXPAND | wxBOTTOM, 10);
	progressBarRow->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, "Striped Progress Bar:"), 0, wxBOTTOM, 2);
	progressBarRow->Add(m_stripedProgressBar, 0, wxEXPAND | wxBOTTOM, 10);
	
	// Modern style progress bars
	progressBarRow->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, "Modern Linear Progress Bar (Text Following):"), 0, wxBOTTOM, 2);
	progressBarRow->Add(m_modernLinearProgressBar, 0, wxEXPAND | wxBOTTOM, 10);
	
	wxBoxSizer* circularRow = new wxBoxSizer(wxHORIZONTAL);
	circularRow->Add(new wxStaticText(m_scrolledWindow, wxID_ANY, "Modern Circular Progress Bar:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
	circularRow->Add(m_modernCircularProgressBar, 0, wxALIGN_CENTER_VERTICAL);
	progressBarRow->Add(circularRow, 0, wxEXPAND | wxBOTTOM, 10);

	progressBarSizer->Add(progressBarRow, 0, wxEXPAND);

	scrolledSizer->Add(controlSizer, 0, wxEXPAND | wxALL, 10);
	scrolledSizer->Add(progressBarSizer, 0, wxEXPAND | wxALL, 10);

	m_scrolledWindow->SetSizer(scrolledSizer);
	mainSizer->Add(m_scrolledWindow, 1, wxEXPAND);

	SetSizer(mainSizer);
}

void FlatWidgetsProgressPanel::BindEvents()
{
	m_startButton->Bind(wxEVT_BUTTON, &FlatWidgetsProgressPanel::OnStartButtonClicked, this);
	m_stopButton->Bind(wxEVT_BUTTON, &FlatWidgetsProgressPanel::OnStopButtonClicked, this);
	m_resetButton->Bind(wxEVT_BUTTON, &FlatWidgetsProgressPanel::OnResetButtonClicked, this);
	m_animationTimer->Bind(wxEVT_TIMER, &FlatWidgetsProgressPanel::OnAnimationTimer, this);
}

void FlatWidgetsProgressPanel::OnStartButtonClicked(wxCommandEvent& event)
{
	StartAnimation();
}

void FlatWidgetsProgressPanel::OnStopButtonClicked(wxCommandEvent& event)
{
	StopAnimation();
}

void FlatWidgetsProgressPanel::OnResetButtonClicked(wxCommandEvent& event)
{
	StopAnimation();
	m_animationValue = 0;
	m_animationDirection = true;
	
	// Reset all progress bars
	m_normalProgressBar->SetValue(0);
	m_stripedProgressBar->SetValue(0);
	m_modernLinearProgressBar->SetValue(0);
	m_modernCircularProgressBar->SetValue(0);
	
	// Refresh all progress bars
	m_normalProgressBar->Refresh();
	m_stripedProgressBar->Refresh();
	m_modernLinearProgressBar->Refresh();
	m_modernCircularProgressBar->Refresh();
}

void FlatWidgetsProgressPanel::OnAnimationTimer(wxTimerEvent& event)
{
	UpdateProgressAnimation();
}

void FlatWidgetsProgressPanel::UpdateProgressAnimation()
{
	if (!m_isAnimating) return;

	// Update animation value
	if (m_animationDirection) {
		m_animationValue += 2;
		if (m_animationValue >= 100) {
			m_animationValue = 100;
			m_animationDirection = false;
		}
	}
	else {
		m_animationValue -= 2;
		if (m_animationValue <= 0) {
			m_animationValue = 0;
			m_animationDirection = true;
		}
	}

	// Update progress bars
	m_normalProgressBar->SetValue(m_animationValue);
	m_stripedProgressBar->SetValue(m_animationValue);
	m_modernLinearProgressBar->SetValue(m_animationValue);
	m_modernCircularProgressBar->SetValue(m_animationValue);

	// Refresh progress bars
	m_normalProgressBar->Refresh();
	m_stripedProgressBar->Refresh();
	m_modernLinearProgressBar->Refresh();
	m_modernCircularProgressBar->Refresh();
}

void FlatWidgetsProgressPanel::StartAnimation()
{
	if (!m_isAnimating) {
		m_isAnimating = true;
		m_animationTimer->Start(50); // Update every 50ms
		m_startButton->Enable(false);
		m_stopButton->Enable(true);
	}
}

void FlatWidgetsProgressPanel::StopAnimation()
{
	if (m_isAnimating) {
		m_isAnimating = false;
		m_animationTimer->Stop();
		m_startButton->Enable(true);
		m_stopButton->Enable(false);
	}
}
