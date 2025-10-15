#pragma once

#include "widgets/FramelessModalPopup.h"
#include <wx/clrpicker.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/choice.h>

class WireframeParamDialog : public FramelessModalPopup {
public:
	explicit WireframeParamDialog(wxWindow* parent);

	wxColour getEdgeColor() const;
	double getEdgeWidth() const;
	int getEdgeStyle() const;
	bool getShowOnlyNew() const;

private:
	wxColourPickerCtrl* m_colorPicker{ nullptr };
	wxSpinCtrlDouble* m_edgeWidth{ nullptr };
	wxChoice* m_edgeStyle{ nullptr };
	wxCheckBox* m_showOnlyNew{ nullptr };
};
