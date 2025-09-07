#ifndef FLAT_WIDGETS_EXAMPLE_DIALOG_H
#define FLAT_WIDGETS_EXAMPLE_DIALOG_H

#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/notebook.h>
#include <wx/timer.h>
#include "widgets/FlatWidgetsExample.h"

// Forward declarations for category panels
class FlatWidgetsButtonsPanel;
class FlatWidgetsInputsPanel;
class FlatWidgetsSelectionPanel;
class FlatWidgetsProgressPanel;

class FlatWidgetsExampleDialog : public wxDialog
{
public:
	FlatWidgetsExampleDialog(wxWindow* parent, const wxString& title = "Flat Widgets Example");
	virtual ~FlatWidgetsExampleDialog();

private:
	wxNotebook* m_notebook;
	FlatWidgetsButtonsPanel* m_buttonsPanel;
	FlatWidgetsInputsPanel* m_inputsPanel;
	FlatWidgetsSelectionPanel* m_selectionPanel;
	FlatWidgetsProgressPanel* m_progressPanel;
	wxTimer* m_progressTimer;

	void CreateControls();
	void LayoutDialog();
	void OnProgressTimer(wxTimerEvent& event);
	
	wxDECLARE_EVENT_TABLE();
};

#endif // FLAT_WIDGETS_EXAMPLE_DIALOG_H
