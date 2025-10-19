#pragma once

#include "widgets/FramelessModalPopup.h"
#include "edges/ModularEdgeComponent.h"
#include <wx/clrpicker.h>
#include <wx/spinctrl.h>
#include <wx/checkbox.h>
#include <wx/choice.h>

class OriginalEdgesParamDialog : public FramelessModalPopup {
public:
	explicit OriginalEdgesParamDialog(wxWindow* parent);

	double getSamplingDensity() const;
	double getMinLength() const;
	bool getShowLinesOnly() const;
	wxColour getEdgeColor() const;
	double getEdgeWidth() const;
	bool getHighlightIntersectionNodes() const;
	wxColour getIntersectionNodeColor() const;
	double getIntersectionNodeSize() const;
	IntersectionNodeShape getIntersectionNodeShape() const;

private:
	wxSpinCtrlDouble* m_samplingDensity{ nullptr };
	wxSpinCtrlDouble* m_minLength{ nullptr };
	wxCheckBox* m_showLinesOnly{ nullptr };
	wxColourPickerCtrl* m_colorPicker{ nullptr };
	wxSpinCtrlDouble* m_edgeWidth{ nullptr };
	wxCheckBox* m_highlightIntersectionNodes{ nullptr };
	wxColourPickerCtrl* m_intersectionNodeColorPicker{ nullptr };
	wxSpinCtrlDouble* m_intersectionNodeSize{ nullptr };
	wxChoice* m_intersectionNodeShape{ nullptr };
};
