#ifndef FLAT_WIDGETS_PROGRESS_PANEL_H
#define FLAT_WIDGETS_PROGRESS_PANEL_H

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <wx/timer.h>
#include <wx/button.h>
#include "widgets/FlatProgressBar.h"

class FlatWidgetsProgressPanel : public wxPanel
{
public:
	FlatWidgetsProgressPanel(wxWindow* parent);
	virtual ~FlatWidgetsProgressPanel();

	void UpdateProgressAnimation();
	void StartAnimation();
	void StopAnimation();

private:
	void CreateControls();
	void LayoutPanel();
	void BindEvents();

	// Scrolled window
	wxScrolledWindow* m_scrolledWindow;

	// Progress bar controls
	FlatProgressBar* m_normalProgressBar;
	FlatProgressBar* m_indeterminateProgressBar;
	FlatProgressBar* m_stripedProgressBar;
	FlatProgressBar* m_modernLinearProgressBar;
	FlatProgressBar* m_modernCircularProgressBar;

	// Control buttons
	wxButton* m_startButton;
	wxButton* m_stopButton;
	wxButton* m_resetButton;

	// Animation
	wxTimer* m_animationTimer;
	int m_animationValue;
	bool m_animationDirection; // true = increasing, false = decreasing
	bool m_isAnimating;

	// Event handlers
	void OnStartButtonClicked(wxCommandEvent& event);
	void OnStopButtonClicked(wxCommandEvent& event);
	void OnResetButtonClicked(wxCommandEvent& event);
	void OnAnimationTimer(wxTimerEvent& event);

	wxDECLARE_EVENT_TABLE();
};

#endif // FLAT_WIDGETS_PROGRESS_PANEL_H
