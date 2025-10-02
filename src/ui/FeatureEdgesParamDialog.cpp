#include "FeatureEdgesParamDialog.h"
#include <wx/stattext.h>
#include <wx/statbox.h>

FeatureEdgesParamDialog::FeatureEdgesParamDialog(wxWindow* parent)
	: FramelessModalPopup(parent, "Feature Edges Parameters", wxSize(450, 350))
{
    // Set up title bar with icon
    SetTitleIcon("edit", wxSize(20, 20));
    ShowTitleIcon(true);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Edge Detection Parameters section
    wxStaticBox* detectionBox = new wxStaticBox(m_contentPanel, wxID_ANY, "Edge Detection Parameters");
    wxStaticBoxSizer* detectionSizer = new wxStaticBoxSizer(detectionBox, wxVERTICAL);

    wxBoxSizer* detectionContentSizer = new wxBoxSizer(wxVERTICAL);

    // Angle and Min Length row
    wxFlexGridSizer* paramRow1 = new wxFlexGridSizer(4, 10, 15); // 4 columns, 10px hgap, 15px vgap
    paramRow1->AddGrowableCol(1, 1);
    paramRow1->AddGrowableCol(3, 1);

    paramRow1->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Angle (deg):"), 0, wxALIGN_CENTER_VERTICAL);
    m_angle = new wxTextCtrl(m_contentPanel, wxID_ANY, "15.0", wxDefaultPosition, wxSize(80, -1));
    paramRow1->Add(m_angle, 1, wxEXPAND);

    paramRow1->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Min length:"), 0, wxALIGN_CENTER_VERTICAL);
    m_minLength = new wxTextCtrl(m_contentPanel, wxID_ANY, "0.005", wxDefaultPosition, wxSize(80, -1));
    paramRow1->Add(m_minLength, 1, wxEXPAND);

    detectionContentSizer->Add(paramRow1, 0, wxEXPAND | wxALL, 10);

    // Convex/Concave checkboxes row
    wxBoxSizer* checkboxSizer = new wxBoxSizer(wxHORIZONTAL);
    m_onlyConvex = new wxCheckBox(m_contentPanel, wxID_ANY, "Only convex");
    m_onlyConcave = new wxCheckBox(m_contentPanel, wxID_ANY, "Only concave");

    checkboxSizer->Add(m_onlyConvex, 0, wxRIGHT, 20);
    checkboxSizer->Add(m_onlyConcave, 0);

    detectionContentSizer->Add(checkboxSizer, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    detectionSizer->Add(detectionContentSizer, 1, wxEXPAND);

    // Edge Appearance section
    wxStaticBox* appearanceBox = new wxStaticBox(m_contentPanel, wxID_ANY, "Edge Appearance");
    wxStaticBoxSizer* appearanceSizer = new wxStaticBoxSizer(appearanceBox, wxVERTICAL);

    wxBoxSizer* appearanceContentSizer = new wxBoxSizer(wxVERTICAL);

    // Color and Width row
    wxFlexGridSizer* appearanceRow1 = new wxFlexGridSizer(4, 10, 15);
    appearanceRow1->AddGrowableCol(1, 1);
    appearanceRow1->AddGrowableCol(3, 1);

    appearanceRow1->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Edge color:"), 0, wxALIGN_CENTER_VERTICAL);
    m_colorPicker = new wxColourPickerCtrl(m_contentPanel, wxID_ANY, *wxBLACK);
    appearanceRow1->Add(m_colorPicker, 1, wxEXPAND);

    appearanceRow1->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Edge width:"), 0, wxALIGN_CENTER_VERTICAL);
    m_edgeWidth = new wxSpinCtrlDouble(m_contentPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(80, -1));
    m_edgeWidth->SetRange(0.1, 10.0);
    m_edgeWidth->SetIncrement(0.1);
    m_edgeWidth->SetValue(1.0);
    appearanceRow1->Add(m_edgeWidth, 1, wxEXPAND);

    appearanceContentSizer->Add(appearanceRow1, 0, wxEXPAND | wxALL, 10);

    // Style row
    wxBoxSizer* styleSizer = new wxBoxSizer(wxHORIZONTAL);
    styleSizer->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Edge style:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    m_edgeStyle = new wxChoice(m_contentPanel, wxID_ANY);
    m_edgeStyle->Append("Solid");
    m_edgeStyle->Append("Dashed");
    m_edgeStyle->Append("Dotted");
    m_edgeStyle->Append("Dash-Dot");
    m_edgeStyle->SetSelection(0); // Default to Solid
    styleSizer->Add(m_edgeStyle, 1, wxEXPAND);

    appearanceContentSizer->Add(styleSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    // Edges only checkbox
    m_edgesOnly = new wxCheckBox(m_contentPanel, wxID_ANY, "Show edges only (hide faces)");
    appearanceContentSizer->Add(m_edgesOnly, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    appearanceSizer->Add(appearanceContentSizer, 1, wxEXPAND);

    // Add sections to main sizer
    mainSizer->Add(detectionSizer, 0, wxEXPAND | wxALL, 10);
    mainSizer->Add(appearanceSizer, 0, wxEXPAND | wxALL, 10);

    // Buttons section
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* okBtn = new wxButton(m_contentPanel, wxID_OK, "OK");
    wxButton* cancelBtn = new wxButton(m_contentPanel, wxID_CANCEL, "Cancel");

    okBtn->SetDefault();
    okBtn->SetMinSize(wxSize(80, 30));
    cancelBtn->SetMinSize(wxSize(80, 30));

    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(okBtn, 0, wxALL, 5);
    buttonSizer->Add(cancelBtn, 0, wxALL, 5);

    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);

    m_contentPanel->SetSizer(mainSizer);
    Layout();

    // Set minimum size to ensure buttons are visible
    wxSize minSize = mainSizer->GetMinSize();
    SetMinSize(wxSize(wxMax(minSize.GetWidth(), 450), wxMax(minSize.GetHeight(), 350)));
    SetSize(wxSize(450, 350));
}

double FeatureEdgesParamDialog::getAngle() const {
	double v = 15.0;
	m_angle->GetValue().ToDouble(&v);
	return v;
}

double FeatureEdgesParamDialog::getMinLength() const {
	double v = 0.005;
	m_minLength->GetValue().ToDouble(&v);
	return v;
}

bool FeatureEdgesParamDialog::getOnlyConvex() const {
	return m_onlyConvex && m_onlyConvex->GetValue();
}

bool FeatureEdgesParamDialog::getOnlyConcave() const {
	return m_onlyConcave && m_onlyConcave->GetValue();
}

wxColour FeatureEdgesParamDialog::getEdgeColor() const {
	return m_colorPicker ? m_colorPicker->GetColour() : *wxBLACK;
}

double FeatureEdgesParamDialog::getEdgeWidth() const {
	return m_edgeWidth ? m_edgeWidth->GetValue() : 1.0;
}

int FeatureEdgesParamDialog::getEdgeStyle() const {
	return m_edgeStyle ? m_edgeStyle->GetSelection() : 0;
}

bool FeatureEdgesParamDialog::getEdgesOnly() const {
	return m_edgesOnly && m_edgesOnly->GetValue();
}