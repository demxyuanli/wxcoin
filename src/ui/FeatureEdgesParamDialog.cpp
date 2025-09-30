#include "FeatureEdgesParamDialog.h"
#include <wx/stattext.h>

FeatureEdgesParamDialog::FeatureEdgesParamDialog(wxWindow* parent)
	: FramelessModalPopup(parent, "Feature Edges Parameters", wxSize(360, 220))
{
    // Set up title bar with icon
    SetTitleIcon("edit", wxSize(20, 20));
    ShowTitleIcon(true);

    wxBoxSizer* top = new wxBoxSizer(wxVERTICAL);

    wxFlexGridSizer* grid = new wxFlexGridSizer(2, 8, 8);
    grid->AddGrowableCol(1, 1);

    grid->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Angle (deg):"), 0, wxALIGN_CENTER_VERTICAL);
    m_angle = new wxTextCtrl(m_contentPanel, wxID_ANY, "15.0");
    grid->Add(m_angle, 1, wxEXPAND);

    grid->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Min length:"), 0, wxALIGN_CENTER_VERTICAL);
    m_minLength = new wxTextCtrl(m_contentPanel, wxID_ANY, "0.005");
    grid->Add(m_minLength, 1, wxEXPAND);

    m_onlyConvex = new wxCheckBox(m_contentPanel, wxID_ANY, "Only convex");
    m_onlyConcave = new wxCheckBox(m_contentPanel, wxID_ANY, "Only concave");
    grid->Add(m_onlyConvex, 0, wxALIGN_LEFT);
    grid->Add(m_onlyConcave, 0, wxALIGN_LEFT);

    grid->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Edge color:"), 0, wxALIGN_CENTER_VERTICAL);
    m_colorPicker = new wxColourPickerCtrl(m_contentPanel, wxID_ANY, *wxBLACK);
    grid->Add(m_colorPicker, 1, wxEXPAND);

    grid->Add(new wxStaticText(m_contentPanel, wxID_ANY, "Edge width:"), 0, wxALIGN_CENTER_VERTICAL);
    m_edgeWidth = new wxSpinCtrlDouble(m_contentPanel, wxID_ANY);
    m_edgeWidth->SetRange(0.1, 10.0);
    m_edgeWidth->SetIncrement(0.1);
    m_edgeWidth->SetValue(1.0);
    grid->Add(m_edgeWidth, 1, wxEXPAND);

    m_edgesOnly = new wxCheckBox(m_contentPanel, wxID_ANY, "Show edges only (hide faces)");
    grid->Add(m_edgesOnly, 0, wxALIGN_LEFT);
    grid->AddSpacer(1);

    top->Add(grid, 1, wxALL | wxEXPAND, 12);

    wxStdDialogButtonSizer* btns = new wxStdDialogButtonSizer();
    btns->AddButton(new wxButton(m_contentPanel, wxID_OK));
    btns->AddButton(new wxButton(m_contentPanel, wxID_CANCEL));
    btns->Realize();
    top->Add(btns, 0, wxALL | wxALIGN_RIGHT, 10);

    m_contentPanel->SetSizerAndFit(top);
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

bool FeatureEdgesParamDialog::getEdgesOnly() const {
	return m_edgesOnly && m_edgesOnly->GetValue();
}