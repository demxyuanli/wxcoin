#ifndef FLAT_WIDGETS_EXAMPLE_DIALOG_H
#define FLAT_WIDGETS_EXAMPLE_DIALOG_H

#include <wx/dialog.h>
#include <wx/sizer.h>
#include "widgets/FlatWidgetsExample.h"

class FlatWidgetsExampleDialog : public wxDialog
{
public:
	FlatWidgetsExampleDialog(wxWindow* parent, const wxString& title = "Flat Widgets Example");
	virtual ~FlatWidgetsExampleDialog() = default;

private:
	FlatWidgetsExample* m_examplePanel;

	void CreateControls();
	void LayoutDialog();
};

#endif // FLAT_WIDGETS_EXAMPLE_DIALOG_H
