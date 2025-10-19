#include "OriginalEdgesParamDialog.h"
#include "logger/Logger.h"

OriginalEdgesParamDialog::OriginalEdgesParamDialog(wxWindow* parent)
	: FramelessModalPopup(parent, "Original Edges Parameters", wxSize(400, 450))
{
	// Set up title bar with icon
	SetTitleIcon("line", wxSize(20, 20));
	ShowTitleIcon(true);

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	// Sampling Density
	wxStaticText* densityLabel = new wxStaticText(m_contentPanel, wxID_ANY, "Sampling Density:");
	m_samplingDensity = new wxSpinCtrlDouble(m_contentPanel, wxID_ANY, "80.0", wxDefaultPosition, wxSize(100, -1),
		wxSP_ARROW_KEYS, 10.0, 200.0, 80.0, 10.0);
	m_samplingDensity->SetToolTip("Higher values = more detailed curves, lower values = faster rendering");

	wxBoxSizer* densitySizer = new wxBoxSizer(wxHORIZONTAL);
	densitySizer->Add(densityLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	densitySizer->Add(m_samplingDensity, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	mainSizer->Add(densitySizer, 0, wxEXPAND | wxALL, 5);

	// Minimum Length
	wxStaticText* minLengthLabel = new wxStaticText(m_contentPanel, wxID_ANY, "Minimum Length:");
	m_minLength = new wxSpinCtrlDouble(m_contentPanel, wxID_ANY, "0.01", wxDefaultPosition, wxSize(100, -1),
		wxSP_ARROW_KEYS, 0.001, 1.0, 0.01, 0.001);
	m_minLength->SetToolTip("Edges shorter than this will be filtered out");

	wxBoxSizer* minLengthSizer = new wxBoxSizer(wxHORIZONTAL);
	minLengthSizer->Add(minLengthLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	minLengthSizer->Add(m_minLength, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	mainSizer->Add(minLengthSizer, 0, wxEXPAND | wxALL, 5);

	// Show Lines Only
	m_showLinesOnly = new wxCheckBox(m_contentPanel, wxID_ANY, "Show Lines Only");
	m_showLinesOnly->SetToolTip("Only show straight line edges, skip curved edges");
	mainSizer->Add(m_showLinesOnly, 0, wxALL, 5);

	// Edge Color
	wxStaticText* colorLabel = new wxStaticText(m_contentPanel, wxID_ANY, "Edge Color:");
	m_colorPicker = new wxColourPickerCtrl(m_contentPanel, wxID_ANY, wxColour(255, 255, 255),
		wxDefaultPosition, wxSize(100, -1));
	m_colorPicker->SetToolTip("Color for original edges");

	wxBoxSizer* colorSizer = new wxBoxSizer(wxHORIZONTAL);
	colorSizer->Add(colorLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	colorSizer->Add(m_colorPicker, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	mainSizer->Add(colorSizer, 0, wxEXPAND | wxALL, 5);

	// Edge Width
	wxStaticText* widthLabel = new wxStaticText(m_contentPanel, wxID_ANY, "Edge Width:");
	m_edgeWidth = new wxSpinCtrlDouble(m_contentPanel, wxID_ANY, "1.0", wxDefaultPosition, wxSize(100, -1),
		wxSP_ARROW_KEYS, 0.1, 10.0, 1.0, 0.1);
	m_edgeWidth->SetToolTip("Line width for original edges");

	wxBoxSizer* widthSizer = new wxBoxSizer(wxHORIZONTAL);
	widthSizer->Add(widthLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	widthSizer->Add(m_edgeWidth, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	mainSizer->Add(widthSizer, 0, wxEXPAND | wxALL, 5);

	// Highlight Intersection Nodes
	m_highlightIntersectionNodes = new wxCheckBox(m_contentPanel, wxID_ANY, "Highlight Intersection Nodes");
	m_highlightIntersectionNodes->SetToolTip("Show intersection points between edges as highlighted nodes");
	mainSizer->Add(m_highlightIntersectionNodes, 0, wxALL, 5);

	// Intersection Node Color
	wxStaticText* intersectionColorLabel = new wxStaticText(m_contentPanel, wxID_ANY, "Node Color:");
	m_intersectionNodeColorPicker = new wxColourPickerCtrl(m_contentPanel, wxID_ANY, wxColour(255, 0, 0),
		wxDefaultPosition, wxSize(100, -1));
	m_intersectionNodeColorPicker->SetToolTip("Color for intersection nodes");
	m_intersectionNodeColorPicker->Enable(false); // Initially disabled

	wxBoxSizer* intersectionColorSizer = new wxBoxSizer(wxHORIZONTAL);
	intersectionColorSizer->Add(intersectionColorLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	intersectionColorSizer->Add(m_intersectionNodeColorPicker, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	mainSizer->Add(intersectionColorSizer, 0, wxEXPAND | wxALL, 5);

	// Intersection Node Size
	wxStaticText* intersectionSizeLabel = new wxStaticText(m_contentPanel, wxID_ANY, "Node Size:");
	m_intersectionNodeSize = new wxSpinCtrlDouble(m_contentPanel, wxID_ANY, "3.0", wxDefaultPosition, wxSize(100, -1),
		wxSP_ARROW_KEYS, 1.0, 20.0, 3.0, 0.5);
	m_intersectionNodeSize->SetToolTip("Size of intersection nodes");
	m_intersectionNodeSize->Enable(false); // Initially disabled

	wxBoxSizer* intersectionSizeSizer = new wxBoxSizer(wxHORIZONTAL);
	intersectionSizeSizer->Add(intersectionSizeLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	intersectionSizeSizer->Add(m_intersectionNodeSize, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	mainSizer->Add(intersectionSizeSizer, 0, wxEXPAND | wxALL, 5);

	// Intersection Node Shape
	wxStaticText* intersectionShapeLabel = new wxStaticText(m_contentPanel, wxID_ANY, "Node Shape:");
	m_intersectionNodeShape = new wxChoice(m_contentPanel, wxID_ANY, wxDefaultPosition, wxSize(100, -1));
	m_intersectionNodeShape->Append("Point (Fastest)");
	m_intersectionNodeShape->Append("Cross");
	m_intersectionNodeShape->Append("Cube");
	m_intersectionNodeShape->Append("Sphere (Best Quality)");
	m_intersectionNodeShape->SetSelection(0); // Default to Point
	m_intersectionNodeShape->SetToolTip("Shape for intersection nodes - Point is fastest for many nodes");
	m_intersectionNodeShape->Enable(false); // Initially disabled

	wxBoxSizer* intersectionShapeSizer = new wxBoxSizer(wxHORIZONTAL);
	intersectionShapeSizer->Add(intersectionShapeLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	intersectionShapeSizer->Add(m_intersectionNodeShape, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	mainSizer->Add(intersectionShapeSizer, 0, wxEXPAND | wxALL, 5);

	// Connect checkbox event to enable/disable controls
	m_highlightIntersectionNodes->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event) {
		bool enabled = event.IsChecked();
		m_intersectionNodeColorPicker->Enable(enabled);
		m_intersectionNodeSize->Enable(enabled);
		m_intersectionNodeShape->Enable(enabled);
	});

	// Buttons
	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton* okButton = new wxButton(m_contentPanel, wxID_OK, "OK");
	wxButton* cancelButton = new wxButton(m_contentPanel, wxID_CANCEL, "Cancel");

	buttonSizer->Add(okButton, 0, wxALL, 5);
	buttonSizer->Add(cancelButton, 0, wxALL, 5);
	mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);

	m_contentPanel->SetSizer(mainSizer);
	Layout();
}

double OriginalEdgesParamDialog::getSamplingDensity() const {
	return m_samplingDensity->GetValue();
}

double OriginalEdgesParamDialog::getMinLength() const {
	return m_minLength->GetValue();
}

bool OriginalEdgesParamDialog::getShowLinesOnly() const {
	return m_showLinesOnly->GetValue();
}

wxColour OriginalEdgesParamDialog::getEdgeColor() const {
	return m_colorPicker->GetColour();
}

double OriginalEdgesParamDialog::getEdgeWidth() const {
	return m_edgeWidth->GetValue();
}

bool OriginalEdgesParamDialog::getHighlightIntersectionNodes() const {
	return m_highlightIntersectionNodes->GetValue();
}

wxColour OriginalEdgesParamDialog::getIntersectionNodeColor() const {
	return m_intersectionNodeColorPicker->GetColour();
}

double OriginalEdgesParamDialog::getIntersectionNodeSize() const {
	return m_intersectionNodeSize->GetValue();
}

IntersectionNodeShape OriginalEdgesParamDialog::getIntersectionNodeShape() const {
	int selection = m_intersectionNodeShape->GetSelection();
	switch (selection) {
		case 0: return IntersectionNodeShape::Point;
		case 1: return IntersectionNodeShape::Cross;
		case 2: return IntersectionNodeShape::Cube;
		case 3: return IntersectionNodeShape::Sphere;
		default: return IntersectionNodeShape::Point;
	}
}
