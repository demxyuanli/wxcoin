#ifndef FLAT_WIDGETS_SELECTION_PANEL_H
#define FLAT_WIDGETS_SELECTION_PANEL_H

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include "widgets/FlatCheckBox.h"
#include "widgets/FlatRadioButton.h"
#include "widgets/FlatSwitch.h"
#include "widgets/FlatSlider.h"

class FlatWidgetsSelectionPanel : public wxPanel
{
public:
	FlatWidgetsSelectionPanel(wxWindow* parent);
	virtual ~FlatWidgetsSelectionPanel() = default;

private:
	void CreateControls();
	void LayoutPanel();
	void BindEvents();

	// Scrolled window
	wxScrolledWindow* m_scrolledWindow;

	// Selection controls
	FlatCheckBox* m_normalCheckBox;
	FlatCheckBox* m_switchCheckBox;
	FlatCheckBox* m_radioCheckBox;

	FlatRadioButton* m_radioButton1;
	FlatRadioButton* m_radioButton2;
	FlatRadioButton* m_radioButton3;

	FlatSwitch* m_normalSwitch;
	FlatSwitch* m_roundSwitch;
	FlatSwitch* m_squareSwitch;

	FlatSlider* m_normalSlider;
	FlatSlider* m_progressSlider;
	FlatSlider* m_verticalSlider;

	// Event handlers
	void OnCheckBoxClicked(wxCommandEvent& event);
	void OnRadioButtonClicked(wxCommandEvent& event);
	void OnSwitchToggled(wxCommandEvent& event);
	void OnSliderValueChanged(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();
};

#endif // FLAT_WIDGETS_SELECTION_PANEL_H
