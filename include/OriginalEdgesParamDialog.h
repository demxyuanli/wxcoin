#pragma once

#include "widgets/FramelessModalPopup.h"
#include <wx/clrpicker.h>
#include <wx/spinctrl.h>
#include <wx/checkbox.h>

class OriginalEdgesParamDialog : public FramelessModalPopup {
public:
	explicit OriginalEdgesParamDialog(wxWindow* parent);

	double getSamplingDensity() const;
	double getMinLength() const;
	bool getShowLinesOnly() const;
	wxColour getEdgeColor() const;
	double getEdgeWidth() const;

private:
	wxSpinCtrlDouble* m_samplingDensity{ nullptr };
	wxSpinCtrlDouble* m_minLength{ nullptr };
	wxCheckBox* m_showLinesOnly{ nullptr };
	wxColourPickerCtrl* m_colorPicker{ nullptr };
	wxSpinCtrlDouble* m_edgeWidth{ nullptr };
};
