#include "ui/OutlineSettingsDialog.h"
#include "ui/OutlinePreviewCanvas.h"
#include <wx/splitter.h>
#include <wx/statline.h>
#include <iomanip>
#include <sstream>

OutlineSettingsDialog::OutlineSettingsDialog(wxWindow* parent, const ImageOutlineParams& params)
	: FramelessModalPopup(parent, "Outline Settings", wxSize(1200, 800)), 
	  m_params(params) {
	
	// Set up title bar with icon
	SetTitleIcon("outline", wxSize(20, 20));
	ShowTitleIcon(true);
	
	// Initialize extended parameters with base parameters
	m_extParams.depthWeight = params.depthWeight;
	m_extParams.normalWeight = params.normalWeight;
	m_extParams.depthThreshold = params.depthThreshold;
	m_extParams.normalThreshold = params.normalThreshold;
	m_extParams.edgeIntensity = params.edgeIntensity;
	m_extParams.thickness = params.thickness;
	
	// Create main splitter
	wxSplitterWindow* splitter = new wxSplitterWindow(m_contentPanel, wxID_ANY);
	splitter->SetMinimumPaneSize(400);
	
	// Left panel - controls
	wxPanel* controlPanel = new wxPanel(splitter);
	wxBoxSizer* controlSizer = new wxBoxSizer(wxVERTICAL);
	
	// Title
	wxStaticText* title = new wxStaticText(controlPanel, wxID_ANY, "Outline Parameters");
	wxFont titleFont = title->GetFont();
	titleFont.SetPointSize(titleFont.GetPointSize() + 2);
	titleFont.SetWeight(wxFONTWEIGHT_BOLD);
	title->SetFont(titleFont);
	controlSizer->Add(title, 0, wxALL | wxALIGN_CENTER, 10);
	controlSizer->Add(new wxStaticLine(controlPanel), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
	
	// Helper lambda to create slider with label
	auto makeSlider = [&](const wxString& label, int min, int max, int value, 
	                     wxSlider** sliderPtr, wxStaticText** labelPtr) {
		wxBoxSizer* box = new wxBoxSizer(wxHORIZONTAL);
		
		// Parameter name
		wxStaticText* nameLabel = new wxStaticText(controlPanel, wxID_ANY, label);
		nameLabel->SetMinSize(wxSize(120, -1));
		box->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
		
		// Slider
		*sliderPtr = new wxSlider(controlPanel, wxID_ANY, value, min, max, 
		                         wxDefaultPosition, wxSize(200, -1));
		box->Add(*sliderPtr, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
		
		// Value label
		*labelPtr = new wxStaticText(controlPanel, wxID_ANY, wxString::Format("%.3f", value / 100.0));
		(*labelPtr)->SetMinSize(wxSize(50, -1));
		box->Add(*labelPtr, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
		
		controlSizer->Add(box, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
		
		// Bind slider event
		(*sliderPtr)->Bind(wxEVT_SLIDER, &OutlineSettingsDialog::onSliderChange, this);
	};
	
	// Create sliders
	controlSizer->AddSpacer(10);
	makeSlider("Depth Weight", 0, 200, int(params.depthWeight * 100), 
	          &m_depthW, &m_depthWLabel);
	makeSlider("Normal Weight", 0, 200, int(params.normalWeight * 100), 
	          &m_normalW, &m_normalWLabel);
	makeSlider("Depth Threshold", 0, 50, int(params.depthThreshold * 1000), 
	          &m_depthTh, &m_depthThLabel);
	makeSlider("Normal Threshold", 0, 200, int(params.normalThreshold * 100), 
	          &m_normalTh, &m_normalThLabel);
	makeSlider("Edge Intensity", 0, 200, int(params.edgeIntensity * 100), 
	          &m_intensity, &m_intensityLabel);
	makeSlider("Thickness", 10, 400, int(params.thickness * 100), 
	          &m_thickness, &m_thicknessLabel);
	
	// Color section
	controlSizer->AddSpacer(20);
	wxStaticText* colorTitle = new wxStaticText(controlPanel, wxID_ANY, "Color Settings");
	wxFont colorTitleFont = colorTitle->GetFont();
	colorTitleFont.SetWeight(wxFONTWEIGHT_BOLD);
	colorTitle->SetFont(colorTitleFont);
	controlSizer->Add(colorTitle, 0, wxALL | wxALIGN_CENTER, 10);
	controlSizer->Add(new wxStaticLine(controlPanel), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
	
	// Helper lambda to create color picker with label
	auto makeColorPicker = [&](const wxString& label, const wxColour& color, 
	                          wxColourPickerCtrl** pickerPtr) {
		wxBoxSizer* box = new wxBoxSizer(wxHORIZONTAL);
		
		// Label
		wxStaticText* nameLabel = new wxStaticText(controlPanel, wxID_ANY, label);
		nameLabel->SetMinSize(wxSize(120, -1));
		box->Add(nameLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
		
		// Color picker
		*pickerPtr = new wxColourPickerCtrl(controlPanel, wxID_ANY, color, 
		                                   wxDefaultPosition, wxSize(100, -1));
		box->Add(*pickerPtr, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
		
		controlSizer->Add(box, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
	};
	
	// Create color pickers
	makeColorPicker("Background", m_extParams.backgroundColor, &m_bgColorPicker);
	makeColorPicker("Outline Color", m_extParams.outlineColor, &m_outlineColorPicker);
	makeColorPicker("Hover Color", m_extParams.hoverColor, &m_hoverColorPicker);
	makeColorPicker("Geometry Color", m_extParams.geometryColor, &m_geomColorPicker);
	
	// Description text
	controlSizer->AddSpacer(20);
	wxStaticText* desc = new wxStaticText(controlPanel, wxID_ANY, 
		"Adjust parameters to control outline appearance.\n"
		"- Depth Weight: Sensitivity to depth changes\n"
		"- Normal Weight: Sensitivity to surface angle changes\n"
		"- Thresholds: Edge detection sensitivity\n"
		"- Intensity: Overall outline strength\n"
		"- Thickness: Line width");
	desc->Wrap(280);
	controlSizer->Add(desc, 0, wxALL, 10);
	
	// Buttons
	controlSizer->AddStretchSpacer();
	wxBoxSizer* btnSizer = new wxBoxSizer(wxHORIZONTAL);
	wxButton* resetBtn = new wxButton(controlPanel, wxID_ANY, "Reset");
	wxButton* okBtn = new wxButton(controlPanel, wxID_OK, "OK");
	wxButton* cancelBtn = new wxButton(controlPanel, wxID_CANCEL, "Cancel");
	
	resetBtn->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
		// Reset to default values
		ImageOutlineParams defaults;
		m_depthW->SetValue(int(defaults.depthWeight * 100));
		m_normalW->SetValue(int(defaults.normalWeight * 100));
		m_depthTh->SetValue(int(defaults.depthThreshold * 1000));
		m_normalTh->SetValue(int(defaults.normalThreshold * 100));
		m_intensity->SetValue(int(defaults.edgeIntensity * 100));
		m_thickness->SetValue(int(defaults.thickness * 100));
		updateLabels();
		updatePreview();
	});
	
	okBtn->Bind(wxEVT_BUTTON, &OutlineSettingsDialog::onOk, this);
	
	btnSizer->Add(resetBtn, 0, wxALL, 5);
	btnSizer->AddStretchSpacer();
	btnSizer->Add(okBtn, 0, wxALL, 5);
	btnSizer->Add(cancelBtn, 0, wxALL, 5);
	controlSizer->Add(btnSizer, 0, wxEXPAND | wxALL, 10);
	
	controlPanel->SetSizer(controlSizer);
	
	// Right panel - preview
	wxPanel* previewPanel = new wxPanel(splitter);
	wxBoxSizer* previewSizer = new wxBoxSizer(wxVERTICAL);
	
	// Preview title
	wxStaticText* previewTitle = new wxStaticText(previewPanel, wxID_ANY, "Preview");
	previewTitle->SetFont(titleFont);
	previewSizer->Add(previewTitle, 0, wxALL | wxALIGN_CENTER, 10);
	previewSizer->Add(new wxStaticLine(previewPanel), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
	
	// Preview canvas with larger minimum size
	m_previewCanvas = new OutlinePreviewCanvas(previewPanel, wxID_ANY, 
	                                          wxDefaultPosition, wxSize(600, 600));
	m_previewCanvas->SetMinSize(wxSize(500, 500));
	previewSizer->Add(m_previewCanvas, 1, wxEXPAND | wxALL, 10);
	
	// Preview instructions
	wxStaticText* instructions = new wxStaticText(previewPanel, wxID_ANY, 
		"Left click and drag to rotate");
	previewSizer->Add(instructions, 0, wxALL | wxALIGN_CENTER, 5);
	
	previewPanel->SetSizer(previewSizer);
	
	// Split the window with more space for controls
	splitter->SplitVertically(controlPanel, previewPanel, 450);
	
	// Main sizer
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(splitter, 1, wxEXPAND);
	m_contentPanel->SetSizer(mainSizer);
	
	// Bind color picker events
	m_bgColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &OutlineSettingsDialog::onColorChange, this);
	m_outlineColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &OutlineSettingsDialog::onColorChange, this);
	m_hoverColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &OutlineSettingsDialog::onColorChange, this);
	m_geomColorPicker->Bind(wxEVT_COLOURPICKER_CHANGED, &OutlineSettingsDialog::onColorChange, this);
	
	// Initialize preview with colors
	m_previewCanvas->setBackgroundColor(m_extParams.backgroundColor);
	m_previewCanvas->setOutlineColor(m_extParams.outlineColor);
	m_previewCanvas->setHoverColor(m_extParams.hoverColor);
	m_previewCanvas->setGeometryColor(m_extParams.geometryColor);
	
	// Initialize preview
	updateLabels();
	updatePreview();
	
	// Center dialog
	CenterOnParent();
}

void OutlineSettingsDialog::onSliderChange(wxCommandEvent& event) {
	updateLabels();
	updatePreview();
	event.Skip();
}

void OutlineSettingsDialog::onColorChange(wxColourPickerEvent& event) {
	// Update extended parameters
	if (event.GetEventObject() == m_bgColorPicker) {
		m_extParams.backgroundColor = m_bgColorPicker->GetColour();
		m_previewCanvas->setBackgroundColor(m_extParams.backgroundColor);
	} else if (event.GetEventObject() == m_outlineColorPicker) {
		m_extParams.outlineColor = m_outlineColorPicker->GetColour();
		m_previewCanvas->setOutlineColor(m_extParams.outlineColor);
	} else if (event.GetEventObject() == m_hoverColorPicker) {
		m_extParams.hoverColor = m_hoverColorPicker->GetColour();
		m_previewCanvas->setHoverColor(m_extParams.hoverColor);
	} else if (event.GetEventObject() == m_geomColorPicker) {
		m_extParams.geometryColor = m_geomColorPicker->GetColour();
		m_previewCanvas->setGeometryColor(m_extParams.geometryColor);
	}
	event.Skip();
}

void OutlineSettingsDialog::updateLabels() {
	// Update value labels with current slider values
	auto formatValue = [](double value, int precision = 3) {
		std::stringstream ss;
		ss << std::fixed << std::setprecision(precision) << value;
		return wxString(ss.str());
	};
	
	m_depthWLabel->SetLabel(formatValue(m_depthW->GetValue() / 100.0, 2));
	m_normalWLabel->SetLabel(formatValue(m_normalW->GetValue() / 100.0, 2));
	m_depthThLabel->SetLabel(formatValue(m_depthTh->GetValue() / 1000.0, 3));
	m_normalThLabel->SetLabel(formatValue(m_normalTh->GetValue() / 100.0, 2));
	m_intensityLabel->SetLabel(formatValue(m_intensity->GetValue() / 100.0, 2));
	m_thicknessLabel->SetLabel(formatValue(m_thickness->GetValue() / 100.0, 2));
}

void OutlineSettingsDialog::updatePreview() {
	// Update parameters from sliders
	m_params.depthWeight = m_depthW->GetValue() / 100.0f;
	m_params.normalWeight = m_normalW->GetValue() / 100.0f;
	m_params.depthThreshold = m_depthTh->GetValue() / 1000.0f;
	m_params.normalThreshold = m_normalTh->GetValue() / 100.0f;
	m_params.edgeIntensity = m_intensity->GetValue() / 100.0f;
	m_params.thickness = m_thickness->GetValue() / 100.0f;
	
	// Sync to extended parameters
	m_extParams.depthWeight = m_params.depthWeight;
	m_extParams.normalWeight = m_params.normalWeight;
	m_extParams.depthThreshold = m_params.depthThreshold;
	m_extParams.normalThreshold = m_params.normalThreshold;
	m_extParams.edgeIntensity = m_params.edgeIntensity;
	m_extParams.thickness = m_params.thickness;
	
	// Update preview
	if (m_previewCanvas) {
		m_previewCanvas->updateOutlineParams(m_params);
	}
}

void OutlineSettingsDialog::onOk(wxCommandEvent&) {
	// Parameters are already updated in m_params
	EndModal(wxID_OK);
}