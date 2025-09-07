#ifndef FLAT_WIDGETS_BUTTONS_PANEL_H
#define FLAT_WIDGETS_BUTTONS_PANEL_H

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include "widgets/FlatButton.h"

class FlatWidgetsButtonsPanel : public wxPanel
{
public:
	FlatWidgetsButtonsPanel(wxWindow* parent);
	virtual ~FlatWidgetsButtonsPanel() = default;

private:
	void CreateControls();
	void LayoutPanel();
	void BindEvents();

	// Scrolled window
	wxScrolledWindow* m_scrolledWindow;

	// Button controls
	FlatButton* m_primaryButton;
	FlatButton* m_secondaryButton;
	FlatButton* m_outlineButton;
	FlatButton* m_textButton;
	FlatButton* m_iconButton;

	// Event handlers
	void OnPrimaryButtonClicked(wxCommandEvent& event);
	void OnSecondaryButtonClicked(wxCommandEvent& event);
	void OnOutlineButtonClicked(wxCommandEvent& event);
	void OnTextButtonClicked(wxCommandEvent& event);
	void OnIconButtonClicked(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();
};

#endif // FLAT_WIDGETS_BUTTONS_PANEL_H
