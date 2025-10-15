#include "WireframeParamDialog.h"
#include <wx/stattext.h>
#include <wx/statbox.h>

WireframeParamDialog::WireframeParamDialog(wxWindow* parent)
	: FramelessModalPopup(parent, "Wireframe Parameters", wxSize(400, 250))
{
    // Set up title bar with icon
    SetTitleIcon("edit", wxSize(20, 20));
    ShowTitleIcon(true);

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Wireframe Appearance section
    wxStaticBox* appearanceBox = new wxStaticBox(m_contentPanel, wxID_ANY, "Wireframe Appearance");
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

    // Show only new checkbox
    m_showOnlyNew = new wxCheckBox(m_contentPanel, wxID_ANY, "Show only new edges");
    appearanceContentSizer->Add(m_showOnlyNew, 0, wxLEFT | wxRIGHT | wxBOTTOM, 10);

    appearanceSizer->Add(appearanceContentSizer, 1, wxEXPAND);

    // Add section to main sizer
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
    SetMinSize(wxSize(wxMax(minSize.GetWidth(), 400), wxMax(minSize.GetHeight(), 250)));
    SetSize(wxSize(400, 250));
}

wxColour WireframeParamDialog::getEdgeColor() const {
	return m_colorPicker ? m_colorPicker->GetColour() : *wxBLACK;
}

double WireframeParamDialog::getEdgeWidth() const {
	return m_edgeWidth ? m_edgeWidth->GetValue() : 1.0;
}

int WireframeParamDialog::getEdgeStyle() const {
	return m_edgeStyle ? m_edgeStyle->GetSelection() : 0;
}

bool WireframeParamDialog::getShowOnlyNew() const {
	return m_showOnlyNew && m_showOnlyNew->GetValue();
}
