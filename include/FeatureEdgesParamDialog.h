#pragma once

#include "widgets/FramelessModalPopup.h"
#include <wx/clrpicker.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/choice.h>

class FeatureEdgesParamDialog : public FramelessModalPopup {
public:
	explicit FeatureEdgesParamDialog(wxWindow* parent);

	double getAngle() const;
	double getMinLength() const;
	bool getOnlyConvex() const;
	bool getOnlyConcave() const;
	wxColour getEdgeColor() const;
	double getEdgeWidth() const;
	int getEdgeStyle() const;
	bool getEdgesOnly() const;

private:
	wxTextCtrl* m_angle{ nullptr };
	wxTextCtrl* m_minLength{ nullptr };
	wxCheckBox* m_onlyConvex{ nullptr };
	wxCheckBox* m_onlyConcave{ nullptr };
	wxColourPickerCtrl* m_colorPicker{ nullptr };
	wxSpinCtrlDouble* m_edgeWidth{ nullptr };
	wxChoice* m_edgeStyle{ nullptr };
	wxCheckBox* m_edgesOnly{ nullptr };
};
