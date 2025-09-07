#ifndef FLAT_WIDGETS_INPUTS_PANEL_H
#define FLAT_WIDGETS_INPUTS_PANEL_H

#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include "widgets/FlatLineEdit.h"
#include "widgets/FlatComboBox.h"

class FlatWidgetsInputsPanel : public wxPanel
{
public:
	FlatWidgetsInputsPanel(wxWindow* parent);
	virtual ~FlatWidgetsInputsPanel() = default;

private:
	void CreateControls();
	void LayoutPanel();
	void BindEvents();

	// Scrolled window
	wxScrolledWindow* m_scrolledWindow;

	// Input controls
	FlatLineEdit* m_normalLineEdit;
	FlatLineEdit* m_searchLineEdit;
	FlatLineEdit* m_passwordLineEdit;
	FlatLineEdit* m_clearableLineEdit;

	FlatComboBox* m_normalComboBox;
	FlatComboBox* m_editableComboBox;
	FlatComboBox* m_searchComboBox;

	// Event handlers
	void OnLineEditTextChanged(wxCommandEvent& event);
	void OnComboBoxSelectionChanged(wxCommandEvent& event);

	wxDECLARE_EVENT_TABLE();
};

#endif // FLAT_WIDGETS_INPUTS_PANEL_H
