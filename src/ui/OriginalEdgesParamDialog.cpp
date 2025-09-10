#include "OriginalEdgesParamDialog.h"
#include "logger/Logger.h"

OriginalEdgesParamDialog::OriginalEdgesParamDialog(wxWindow* parent)
	: wxDialog(parent, wxID_ANY, "Original Edges Parameters", wxDefaultPosition, wxSize(400, 300))
{
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	
	// Sampling Density
	wxStaticText* densityLabel = new wxStaticText(this, wxID_ANY, "Sampling Density:");
	m_samplingDensity = new wxSpinCtrlDouble(this, wxID_ANY, "80.0", wxDefaultPosition, wxSize(100, -1), 
		wxSP_ARROW_KEYS, 10.0, 200.0, 80.0, 10.0);
	m_samplingDensity->SetToolTip("Higher values = more detailed curves, lower values = faster rendering");
	
	wxBoxSizer* densitySizer = new wxBoxSizer(wxHORIZONTAL);
	densitySizer->Add(densityLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	densitySizer->Add(m_samplingDensity, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	mainSizer->Add(densitySizer, 0, wxEXPAND | wxALL, 5);
	
	// Minimum Length
	wxStaticText* minLengthLabel = new wxStaticText(this, wxID_ANY, "Minimum Length:");
	m_minLength = new wxSpinCtrlDouble(this, wxID_ANY, "0.01", wxDefaultPosition, wxSize(100, -1), 
		wxSP_ARROW_KEYS, 0.001, 1.0, 0.01, 0.001);
	m_minLength->SetToolTip("Edges shorter than this will be filtered out");
	
	wxBoxSizer* minLengthSizer = new wxBoxSizer(wxHORIZONTAL);
	minLengthSizer->Add(minLengthLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	minLengthSizer->Add(m_minLength, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	mainSizer->Add(minLengthSizer, 0, wxEXPAND | wxALL, 5);
	
	// Show Lines Only
	m_showLinesOnly = new wxCheckBox(this, wxID_ANY, "Show Lines Only");
	m_showLinesOnly->SetToolTip("Only show straight line edges, skip curved edges");
	mainSizer->Add(m_showLinesOnly, 0, wxALL, 5);
	
	// Edge Color
	wxStaticText* colorLabel = new wxStaticText(this, wxID_ANY, "Edge Color:");
	m_colorPicker = new wxColourPickerCtrl(this, wxID_ANY, wxColour(255, 255, 255), 
		wxDefaultPosition, wxSize(100, -1));
	m_colorPicker->SetToolTip("Color for original edges");
	
	wxBoxSizer* colorSizer = new wxBoxSizer(wxHORIZONTAL);
	colorSizer->Add(colorLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	colorSizer->Add(m_colorPicker, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	mainSizer->Add(colorSizer, 0, wxEXPAND | wxALL, 5);
	
	// Edge Width
	wxStaticText* widthLabel = new wxStaticText(this, wxID_ANY, "Edge Width:");
	m_edgeWidth = new wxSpinCtrlDouble(this, wxID_ANY, "1.0", wxDefaultPosition, wxSize(100, -1), 
		wxSP_ARROW_KEYS, 0.1, 10.0, 1.0, 0.1);
	m_edgeWidth->SetToolTip("Line width for original edges");
	
	wxBoxSizer* widthSizer = new wxBoxSizer(wxHORIZONTAL);
	widthSizer->Add(widthLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	widthSizer->Add(m_edgeWidth, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	mainSizer->Add(widthSizer, 0, wxEXPAND | wxALL, 5);
	
	// Buttons
	wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton* okButton = new wxButton(this, wxID_OK, "OK");
	wxButton* cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
	
	buttonSizer->Add(okButton, 0, wxALL, 5);
	buttonSizer->Add(cancelButton, 0, wxALL, 5);
	mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);
	
	SetSizer(mainSizer);
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
