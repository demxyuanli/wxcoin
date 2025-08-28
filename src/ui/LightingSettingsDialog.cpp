#include "LightingSettingsDialog.h"
#include "config/LightingConfig.h"
#include "SceneManager.h"
#include "Logger.h"
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/colour.h>
#include <wx/colordlg.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/valnum.h>
#include <wx/listbox.h>
#include <wx/slider.h>
#include <wx/scrolwin.h>
#include <wx/grid.h>
#include <wx/font.h>
#include <wx/msgdlg.h>
#include <sstream>

BEGIN_EVENT_TABLE(LightingSettingsDialog, wxDialog)
EVT_BUTTON(wxID_APPLY, LightingSettingsDialog::onApply)
EVT_BUTTON(wxID_OK, LightingSettingsDialog::onOK)
EVT_BUTTON(wxID_CANCEL, LightingSettingsDialog::onCancel)
EVT_BUTTON(wxID_RESET, LightingSettingsDialog::onReset)
END_EVENT_TABLE()

LightingSettingsDialog::LightingSettingsDialog(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxDialog(parent, id, title, pos, wxSize(800, 600), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	m_config(LightingConfig::getInstance()),
	m_currentLightIndex(-1)
{
	// Create main layout
	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

	// Create notebook for different lighting sections
	m_notebook = new wxNotebook(this, wxID_ANY);

	// Create pages
	createEnvironmentPage();
	m_notebook->AddPage(m_environmentPage, "Environment Lighting", true);

	createLightsPage();
	m_notebook->AddPage(m_lightsPage, "Light Management", false);

	createPresetsPage();
	m_notebook->AddPage(m_presetsPage, "Presets", false);

	mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 5);

	// Create buttons
	createButtons();
	mainSizer->Add(m_buttonSizer, 0, wxEXPAND | wxALL, 5);

	SetSizer(mainSizer);

	// Load current settings
	updateEnvironmentProperties();
	updateLightList();

	// Initialize current preset label
	m_currentPresetLabel->SetLabel("No preset applied");
}

LightingSettingsDialog::~LightingSettingsDialog()
{
}

void LightingSettingsDialog::createEnvironmentPage()
{
	m_environmentPage = new wxPanel(m_notebook);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	// Ambient lighting section
	wxStaticBoxSizer* ambientSizer = new wxStaticBoxSizer(wxVERTICAL, m_environmentPage, "Ambient Lighting");

	wxBoxSizer* ambientColorSizer = new wxBoxSizer(wxHORIZONTAL);
	ambientColorSizer->Add(new wxStaticText(m_environmentPage, wxID_ANY, "Color:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	m_ambientColorButton = new wxButton(m_environmentPage, wxID_ANY, "", wxDefaultPosition, wxSize(60, 25));
	ambientColorSizer->Add(m_ambientColorButton, 0, wxALL, 5);
	ambientSizer->Add(ambientColorSizer);

	wxBoxSizer* ambientIntensitySizer = new wxBoxSizer(wxHORIZONTAL);
	m_ambientIntensityLabel = new wxStaticText(m_environmentPage, wxID_ANY, "Intensity: 0.2");
	ambientIntensitySizer->Add(m_ambientIntensityLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	m_ambientIntensitySlider = new wxSlider(m_environmentPage, wxID_ANY, 20, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	ambientIntensitySizer->Add(m_ambientIntensitySlider, 1, wxEXPAND | wxALL, 5);
	ambientSizer->Add(ambientIntensitySizer);

	sizer->Add(ambientSizer, 0, wxEXPAND | wxALL, 5);

	m_environmentPage->SetSizer(sizer);

	// Bind events
	m_ambientColorButton->Bind(wxEVT_BUTTON, &LightingSettingsDialog::onColorButtonClicked, this);
	m_ambientIntensitySlider->Bind(wxEVT_SLIDER, &LightingSettingsDialog::onEnvironmentPropertyChanged, this);
}

void LightingSettingsDialog::createLightsPage()
{
	m_lightsPage = new wxPanel(m_notebook);
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

	// Left side - lights list
	wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
	leftSizer->Add(new wxStaticText(m_lightsPage, wxID_ANY, "Lights:"), 0, wxALL, 5);

	m_lightsList = new wxListBox(m_lightsPage, wxID_ANY, wxDefaultPosition, wxSize(150, 200));
	leftSizer->Add(m_lightsList, 1, wxEXPAND | wxALL, 5);

	wxBoxSizer* lightButtonSizer = new wxBoxSizer(wxHORIZONTAL);
	m_addLightButton = new wxButton(m_lightsPage, wxID_ANY, "Add Light");
	m_removeLightButton = new wxButton(m_lightsPage, wxID_ANY, "Remove Light");
	lightButtonSizer->Add(m_addLightButton, 1, wxEXPAND | wxALL, 5);
	lightButtonSizer->Add(m_removeLightButton, 1, wxEXPAND | wxALL, 5);
	leftSizer->Add(lightButtonSizer);

	sizer->Add(leftSizer, 0, wxEXPAND | wxALL, 5);

	// Right side - light properties
	wxBoxSizer* rightSizer = new wxBoxSizer(wxVERTICAL);
	rightSizer->Add(new wxStaticText(m_lightsPage, wxID_ANY, "Light Properties:"), 0, wxALL, 5);

	// Create a scrolled window for the properties
	wxScrolledWindow* scrollWindow = new wxScrolledWindow(m_lightsPage, wxID_ANY);
	scrollWindow->SetScrollRate(10, 10);
	wxBoxSizer* scrollSizer = new wxBoxSizer(wxVERTICAL);

	// Light name
	wxBoxSizer* nameSizer = new wxBoxSizer(wxHORIZONTAL);
	nameSizer->Add(new wxStaticText(scrollWindow, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	m_lightNameText = new wxTextCtrl(scrollWindow, wxID_ANY);
	nameSizer->Add(m_lightNameText, 1, wxEXPAND | wxALL, 5);
	scrollSizer->Add(nameSizer);

	// Light type
	wxBoxSizer* typeSizer = new wxBoxSizer(wxHORIZONTAL);
	typeSizer->Add(new wxStaticText(scrollWindow, wxID_ANY, "Type:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	m_lightTypeChoice = new wxChoice(scrollWindow, wxID_ANY);
	m_lightTypeChoice->Append("Directional");
	m_lightTypeChoice->Append("Point");
	m_lightTypeChoice->Append("Spot");
	typeSizer->Add(m_lightTypeChoice, 1, wxEXPAND | wxALL, 5);
	scrollSizer->Add(typeSizer);

	// Enabled checkbox
	m_lightEnabledCheck = new wxCheckBox(scrollWindow, wxID_ANY, "Enabled");
	scrollSizer->Add(m_lightEnabledCheck, 0, wxALL, 5);

	// Position
	wxStaticBoxSizer* positionSizer = new wxStaticBoxSizer(wxVERTICAL, scrollWindow, "Position");
	wxBoxSizer* posXSizer = new wxBoxSizer(wxHORIZONTAL);
	posXSizer->Add(new wxStaticText(scrollWindow, wxID_ANY, "X:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	m_positionXSpin = new wxSpinCtrlDouble(scrollWindow, wxID_ANY, "0.0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100.0, 100.0, 0.0, 0.1);
	posXSizer->Add(m_positionXSpin, 1, wxEXPAND | wxALL, 5);
	positionSizer->Add(posXSizer);

	wxBoxSizer* posYSizer = new wxBoxSizer(wxHORIZONTAL);
	posYSizer->Add(new wxStaticText(scrollWindow, wxID_ANY, "Y:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	m_positionYSpin = new wxSpinCtrlDouble(scrollWindow, wxID_ANY, "0.0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100.0, 100.0, 0.0, 0.1);
	posYSizer->Add(m_positionYSpin, 1, wxEXPAND | wxALL, 5);
	positionSizer->Add(posYSizer);

	wxBoxSizer* posZSizer = new wxBoxSizer(wxHORIZONTAL);
	posZSizer->Add(new wxStaticText(scrollWindow, wxID_ANY, "Z:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	m_positionZSpin = new wxSpinCtrlDouble(scrollWindow, wxID_ANY, "10.0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -100.0, 100.0, 10.0, 0.1);
	posZSizer->Add(m_positionZSpin, 1, wxEXPAND | wxALL, 5);
	positionSizer->Add(posZSizer);

	scrollSizer->Add(positionSizer, 0, wxEXPAND | wxALL, 5);

	// Direction
	wxStaticBoxSizer* directionSizer = new wxStaticBoxSizer(wxVERTICAL, scrollWindow, "Direction");
	wxBoxSizer* dirXSizer = new wxBoxSizer(wxHORIZONTAL);
	dirXSizer->Add(new wxStaticText(scrollWindow, wxID_ANY, "X:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	m_directionXSpin = new wxSpinCtrlDouble(scrollWindow, wxID_ANY, "0.0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -1.0, 1.0, 0.0, 0.1);
	dirXSizer->Add(m_directionXSpin, 1, wxEXPAND | wxALL, 5);
	directionSizer->Add(dirXSizer);

	wxBoxSizer* dirYSizer = new wxBoxSizer(wxHORIZONTAL);
	dirYSizer->Add(new wxStaticText(scrollWindow, wxID_ANY, "Y:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	m_directionYSpin = new wxSpinCtrlDouble(scrollWindow, wxID_ANY, "0.0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -1.0, 1.0, 0.0, 0.1);
	dirYSizer->Add(m_directionYSpin, 1, wxEXPAND | wxALL, 5);
	directionSizer->Add(dirYSizer);

	wxBoxSizer* dirZSizer = new wxBoxSizer(wxHORIZONTAL);
	dirZSizer->Add(new wxStaticText(scrollWindow, wxID_ANY, "Z:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	m_directionZSpin = new wxSpinCtrlDouble(scrollWindow, wxID_ANY, "-1.0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -1.0, 1.0, -1.0, 0.1);
	dirZSizer->Add(m_directionZSpin, 1, wxEXPAND | wxALL, 5);
	directionSizer->Add(dirZSizer);

	scrollSizer->Add(directionSizer, 0, wxEXPAND | wxALL, 5);

	// Color and intensity
	wxStaticBoxSizer* colorSizer = new wxStaticBoxSizer(wxVERTICAL, scrollWindow, "Color & Intensity");
	wxBoxSizer* colorButtonSizer = new wxBoxSizer(wxHORIZONTAL);
	colorButtonSizer->Add(new wxStaticText(scrollWindow, wxID_ANY, "Color:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	m_lightColorButton = new wxButton(scrollWindow, wxID_ANY, "", wxDefaultPosition, wxSize(60, 25));
	colorButtonSizer->Add(m_lightColorButton, 0, wxALL, 5);
	colorSizer->Add(colorButtonSizer);

	wxBoxSizer* intensitySizer = new wxBoxSizer(wxHORIZONTAL);
	m_lightIntensityLabel = new wxStaticText(scrollWindow, wxID_ANY, "Intensity: 1.0");
	intensitySizer->Add(m_lightIntensityLabel, 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	m_lightIntensitySlider = new wxSlider(scrollWindow, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	intensitySizer->Add(m_lightIntensitySlider, 1, wxEXPAND | wxALL, 5);
	colorSizer->Add(intensitySizer);

	scrollSizer->Add(colorSizer, 0, wxEXPAND | wxALL, 5);

	// Set up the scroll window
	scrollWindow->SetSizer(scrollSizer);
	rightSizer->Add(scrollWindow, 1, wxEXPAND | wxALL, 5);

	sizer->Add(rightSizer, 2, wxEXPAND | wxALL, 5);

	m_lightsPage->SetSizer(sizer);

	// Bind events
	m_lightsList->Bind(wxEVT_LISTBOX, &LightingSettingsDialog::onLightSelected, this);
	m_addLightButton->Bind(wxEVT_BUTTON, &LightingSettingsDialog::onAddLight, this);
	m_removeLightButton->Bind(wxEVT_BUTTON, &LightingSettingsDialog::onRemoveLight, this);
	m_lightColorButton->Bind(wxEVT_BUTTON, &LightingSettingsDialog::onColorButtonClicked, this);
	m_lightIntensitySlider->Bind(wxEVT_SLIDER, &LightingSettingsDialog::onLightPropertyChanged, this);

	// Bind property change events
	m_lightNameText->Bind(wxEVT_TEXT, &LightingSettingsDialog::onLightPropertyChanged, this);
	m_lightTypeChoice->Bind(wxEVT_CHOICE, &LightingSettingsDialog::onLightPropertyChanged, this);
	m_lightEnabledCheck->Bind(wxEVT_CHECKBOX, &LightingSettingsDialog::onLightPropertyChanged, this);
	m_positionXSpin->Bind(wxEVT_SPINCTRLDOUBLE, &LightingSettingsDialog::onLightPropertyChanged, this);
	m_positionYSpin->Bind(wxEVT_SPINCTRLDOUBLE, &LightingSettingsDialog::onLightPropertyChanged, this);
	m_positionZSpin->Bind(wxEVT_SPINCTRLDOUBLE, &LightingSettingsDialog::onLightPropertyChanged, this);
	m_directionXSpin->Bind(wxEVT_SPINCTRLDOUBLE, &LightingSettingsDialog::onLightPropertyChanged, this);
	m_directionYSpin->Bind(wxEVT_SPINCTRLDOUBLE, &LightingSettingsDialog::onLightPropertyChanged, this);
	m_directionZSpin->Bind(wxEVT_SPINCTRLDOUBLE, &LightingSettingsDialog::onLightPropertyChanged, this);
}

void LightingSettingsDialog::createPresetsPage()
{
	m_presetsPage = new wxPanel(m_notebook);
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	// Title
	wxStaticText* titleLabel = new wxStaticText(m_presetsPage, wxID_ANY, "Quick Lighting Presets", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
	titleLabel->SetFont(wxFont(14, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
	sizer->Add(titleLabel, 0, wxALIGN_CENTER | wxALL, 10);

	// Description
	wxStaticText* descLabel = new wxStaticText(m_presetsPage, wxID_ANY,
		"Click any preset button below to immediately apply the lighting setup to your scene.\n"
		"Each preset provides a different lighting atmosphere for your 3D models.",
		wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
	sizer->Add(descLabel, 0, wxALIGN_CENTER | wxALL, 10);

	// Create a grid of preset buttons
	wxGridSizer* gridSizer = new wxGridSizer(2, 4, 10, 10);

	// Studio Lighting
	wxButton* studioButton = new wxButton(m_presetsPage, wxID_ANY, "Studio\nLighting", wxDefaultPosition, wxSize(150, 80));
	studioButton->SetBackgroundColour(wxColour(240, 248, 255)); // Light blue
	studioButton->SetToolTip("Professional studio lighting with key, fill, and rim lights");
	gridSizer->Add(studioButton, 0, wxEXPAND);

	// Outdoor Lighting
	wxButton* outdoorButton = new wxButton(m_presetsPage, wxID_ANY, "Outdoor\nLighting", wxDefaultPosition, wxSize(150, 80));
	outdoorButton->SetBackgroundColour(wxColour(255, 255, 224)); // Light yellow
	outdoorButton->SetToolTip("Natural outdoor lighting with sun and sky illumination");
	gridSizer->Add(outdoorButton, 0, wxEXPAND);

	// Dramatic Lighting
	wxButton* dramaticButton = new wxButton(m_presetsPage, wxID_ANY, "Dramatic\nLighting", wxDefaultPosition, wxSize(150, 80));
	dramaticButton->SetBackgroundColour(wxColour(255, 228, 225)); // Light red
	dramaticButton->SetToolTip("Dramatic lighting with strong shadows and contrast");
	gridSizer->Add(dramaticButton, 0, wxEXPAND);

	// Warm Lighting
	wxButton* warmButton = new wxButton(m_presetsPage, wxID_ANY, "Warm\nLighting", wxDefaultPosition, wxSize(150, 80));
	warmButton->SetBackgroundColour(wxColour(255, 240, 245)); // Light pink
	warmButton->SetToolTip("Warm, cozy lighting with orange and yellow tones");
	gridSizer->Add(warmButton, 0, wxEXPAND);

	// Cool Lighting
	wxButton* coolButton = new wxButton(m_presetsPage, wxID_ANY, "Cool\nLighting", wxDefaultPosition, wxSize(150, 80));
	coolButton->SetBackgroundColour(wxColour(240, 255, 255)); // Light cyan
	coolButton->SetToolTip("Cool, blue-tinted lighting for a modern look");
	gridSizer->Add(coolButton, 0, wxEXPAND);

	// Minimal Lighting
	wxButton* minimalButton = new wxButton(m_presetsPage, wxID_ANY, "Minimal\nLighting", wxDefaultPosition, wxSize(150, 80));
	minimalButton->SetBackgroundColour(wxColour(245, 245, 245)); // Light gray
	minimalButton->SetToolTip("Simple, minimal lighting with subtle shadows");
	gridSizer->Add(minimalButton, 0, wxEXPAND);

	// FreeCAD Three-Light Model
	wxButton* freecadButton = new wxButton(m_presetsPage, wxID_ANY, "FreeCAD\nThree-Light", wxDefaultPosition, wxSize(150, 80));
	freecadButton->SetBackgroundColour(wxColour(230, 255, 230)); // Light green
	freecadButton->SetToolTip("Classic FreeCAD three-light model: main, fill, and back lights");
	gridSizer->Add(freecadButton, 0, wxEXPAND);

	// NavigationCube Lighting
	wxButton* navcubeButton = new wxButton(m_presetsPage, wxID_ANY, "Navigation\nCube", wxDefaultPosition, wxSize(150, 80));
	navcubeButton->SetBackgroundColour(wxColour(255, 230, 255)); // Light purple
	navcubeButton->SetToolTip("NavigationCube-style lighting with multiple directional lights");
	gridSizer->Add(navcubeButton, 0, wxEXPAND);

	sizer->Add(gridSizer, 0, wxALIGN_CENTER | wxALL, 20);

	// Add some spacing
	sizer->AddSpacer(20);

	// Current preset info
	wxStaticBoxSizer* infoSizer = new wxStaticBoxSizer(wxVERTICAL, m_presetsPage, "Current Preset Info");
	m_currentPresetLabel = new wxStaticText(m_presetsPage, wxID_ANY, "No preset applied");
	infoSizer->Add(m_currentPresetLabel, 0, wxALL, 5);
	sizer->Add(infoSizer, 0, wxEXPAND | wxALL, 10);

	m_presetsPage->SetSizer(sizer);

	// Bind events for preset buttons
	studioButton->Bind(wxEVT_BUTTON, &LightingSettingsDialog::onStudioPreset, this);
	outdoorButton->Bind(wxEVT_BUTTON, &LightingSettingsDialog::onOutdoorPreset, this);
	dramaticButton->Bind(wxEVT_BUTTON, &LightingSettingsDialog::onDramaticPreset, this);
	warmButton->Bind(wxEVT_BUTTON, &LightingSettingsDialog::onWarmPreset, this);
	coolButton->Bind(wxEVT_BUTTON, &LightingSettingsDialog::onCoolPreset, this);
	minimalButton->Bind(wxEVT_BUTTON, &LightingSettingsDialog::onMinimalPreset, this);
	freecadButton->Bind(wxEVT_BUTTON, &LightingSettingsDialog::onFreeCADPreset, this);
	navcubeButton->Bind(wxEVT_BUTTON, &LightingSettingsDialog::onNavcubePreset, this);
}

void LightingSettingsDialog::createButtons()
{
	m_buttonSizer = new wxBoxSizer(wxHORIZONTAL);

	m_applyButton = new wxButton(this, wxID_APPLY, "Apply");
	m_okButton = new wxButton(this, wxID_OK, "OK");
	m_cancelButton = new wxButton(this, wxID_CANCEL, "Cancel");
	m_resetButton = new wxButton(this, wxID_RESET, "Reset");

	m_buttonSizer->Add(m_applyButton, 0, wxALL, 5);
	m_buttonSizer->AddStretchSpacer();
	m_buttonSizer->Add(m_resetButton, 0, wxALL, 5);
	m_buttonSizer->Add(m_okButton, 0, wxALL, 5);
	m_buttonSizer->Add(m_cancelButton, 0, wxALL, 5);

	// Bind events
	m_applyButton->Bind(wxEVT_BUTTON, &LightingSettingsDialog::onApply, this);
	m_okButton->Bind(wxEVT_BUTTON, &LightingSettingsDialog::onOK, this);
	m_cancelButton->Bind(wxEVT_BUTTON, &LightingSettingsDialog::onCancel, this);
	m_resetButton->Bind(wxEVT_BUTTON, &LightingSettingsDialog::onReset, this);
}

void LightingSettingsDialog::updateEnvironmentProperties()
{
	// Update ambient color button
	Quantity_Color ambientColor = m_config.getEnvironmentSettings().ambientColor;
	updateColorButton(m_ambientColorButton, ambientColor);

	// Update ambient intensity
	double intensity = m_config.getEnvironmentSettings().ambientIntensity;
	m_ambientIntensitySlider->SetValue(static_cast<int>(intensity * 100));
	m_ambientIntensityLabel->SetLabel(wxString::Format("Intensity: %.1f", intensity));
}

void LightingSettingsDialog::updateLightList()
{
	m_lightsList->Clear();
	m_tempLights = m_config.getAllLights();

	for (const auto& light : m_tempLights) {
		m_lightsList->Append(light.name);
	}

	if (m_lightsList->GetCount() > 0) {
		m_lightsList->SetSelection(0);
		m_currentLightIndex = 0;
		updateLightProperties();
	}
	else {
		m_currentLightIndex = -1;
	}
}

void LightingSettingsDialog::updateLightProperties()
{
	if (m_currentLightIndex >= 0 && m_currentLightIndex < static_cast<int>(m_tempLights.size())) {
		const auto& light = m_tempLights[m_currentLightIndex];

		m_lightNameText->SetValue(light.name);

		// Set light type selection based on string type
		if (light.type == "directional") {
			m_lightTypeChoice->SetSelection(0);
		}
		else if (light.type == "point") {
			m_lightTypeChoice->SetSelection(1);
		}
		else if (light.type == "spot") {
			m_lightTypeChoice->SetSelection(2);
		}
		else {
			m_lightTypeChoice->SetSelection(0);
		}

		m_lightEnabledCheck->SetValue(light.enabled);

		m_positionXSpin->SetValue(light.positionX);
		m_positionYSpin->SetValue(light.positionY);
		m_positionZSpin->SetValue(light.positionZ);

		m_directionXSpin->SetValue(light.directionX);
		m_directionYSpin->SetValue(light.directionY);
		m_directionZSpin->SetValue(light.directionZ);

		updateColorButton(m_lightColorButton, light.color);
		m_lightIntensitySlider->SetValue(static_cast<int>(light.intensity * 100));
		m_lightIntensityLabel->SetLabel(wxString::Format("Intensity: %.1f", light.intensity));
	}
}

void LightingSettingsDialog::updateColorButton(wxButton* button, const Quantity_Color& color)
{
	wxColour wxColor = quantityColorToWxColour(color);
	button->SetBackgroundColour(wxColor);
	button->Refresh();
}

Quantity_Color LightingSettingsDialog::wxColourToQuantityColor(const wxColour& wxColor) const
{
	return Quantity_Color(wxColor.Red() / 255.0, wxColor.Green() / 255.0, wxColor.Blue() / 255.0, Quantity_TOC_RGB);
}

wxColour LightingSettingsDialog::quantityColorToWxColour(const Quantity_Color& color) const
{
	Standard_Real r, g, b;
	color.Values(r, g, b, Quantity_TOC_RGB);
	return wxColour(static_cast<unsigned char>(r * 255),
		static_cast<unsigned char>(g * 255),
		static_cast<unsigned char>(b * 255));
}

void LightingSettingsDialog::onLightSelected(wxCommandEvent& event)
{
	m_currentLightIndex = m_lightsList->GetSelection();
	updateLightProperties();
}

void LightingSettingsDialog::onAddLight(wxCommandEvent& event)
{
	LightSettings newLight;
	newLight.name = "New Light " + std::to_string(m_tempLights.size() + 1);
	newLight.type = "directional";
	newLight.enabled = true;
	newLight.positionX = 0.0;
	newLight.positionY = 0.0;
	newLight.positionZ = 10.0;
	newLight.directionX = 0.0;
	newLight.directionY = 0.0;
	newLight.directionZ = -1.0;
	newLight.color = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
	newLight.intensity = 1.0;

	m_tempLights.push_back(newLight);
	updateLightList();

	// Select the new light
	m_lightsList->SetSelection(m_tempLights.size() - 1);
	m_currentLightIndex = m_tempLights.size() - 1;
	updateLightProperties();
}

void LightingSettingsDialog::onRemoveLight(wxCommandEvent& event)
{
	if (m_currentLightIndex >= 0 && m_currentLightIndex < static_cast<int>(m_tempLights.size())) {
		m_tempLights.erase(m_tempLights.begin() + m_currentLightIndex);
		updateLightList();
	}
}

void LightingSettingsDialog::onLightPropertyChanged(wxCommandEvent& event)
{
	if (m_currentLightIndex >= 0 && m_currentLightIndex < static_cast<int>(m_tempLights.size())) {
		auto& light = m_tempLights[m_currentLightIndex];

		light.name = m_lightNameText->GetValue().ToStdString();

		// Set light type based on selection
		int selection = m_lightTypeChoice->GetSelection();
		if (selection == 0) {
			light.type = "directional";
		}
		else if (selection == 1) {
			light.type = "point";
		}
		else if (selection == 2) {
			light.type = "spot";
		}
		else {
			light.type = "directional";
		}

		light.enabled = m_lightEnabledCheck->GetValue();

		light.positionX = m_positionXSpin->GetValue();
		light.positionY = m_positionYSpin->GetValue();
		light.positionZ = m_positionZSpin->GetValue();

		light.directionX = m_directionXSpin->GetValue();
		light.directionY = m_directionYSpin->GetValue();
		light.directionZ = m_directionZSpin->GetValue();

		light.intensity = m_lightIntensitySlider->GetValue() / 100.0;
		m_lightIntensityLabel->SetLabel(wxString::Format("Intensity: %.1f", light.intensity));

		// Update the list item name
		m_lightsList->SetString(m_currentLightIndex, light.name);
	}
}

void LightingSettingsDialog::onEnvironmentPropertyChanged(wxCommandEvent& event)
{
	double intensity = m_ambientIntensitySlider->GetValue() / 100.0;
	m_ambientIntensityLabel->SetLabel(wxString::Format("Intensity: %.1f", intensity));
}

void LightingSettingsDialog::onColorButtonClicked(wxCommandEvent& event)
{
	wxButton* button = dynamic_cast<wxButton*>(event.GetEventObject());
	if (!button) return;

	wxColourData data;
	data.SetColour(button->GetBackgroundColour());

	wxColourDialog dialog(this, &data);
	if (dialog.ShowModal() == wxID_OK) {
		wxColour color = dialog.GetColourData().GetColour();
		button->SetBackgroundColour(color);
		button->Refresh();

		// Update the corresponding setting
		Quantity_Color quantityColor = wxColourToQuantityColor(color);

		if (button == m_ambientColorButton) {
			m_config.setEnvironmentAmbientColor(quantityColor);
		}
		else if (button == m_lightColorButton && m_currentLightIndex >= 0) {
			m_tempLights[m_currentLightIndex].color = quantityColor;
		}
	}
}

void LightingSettingsDialog::onPresetSelected(wxCommandEvent& event)
{
	// Preset selection logic can be implemented here
}

void LightingSettingsDialog::onApplyPreset(wxCommandEvent& event)
{
	int selection = m_presetChoice->GetSelection();
	if (selection != wxNOT_FOUND) {
		auto presetNames = m_config.getAvailablePresets();
		if (selection < static_cast<int>(presetNames.size())) {
			m_config.applyPreset(presetNames[selection]);
			updateEnvironmentProperties();
			updateLightList();
		}
	}
}

void LightingSettingsDialog::onApply(wxCommandEvent& event)
{
	applySettings();
	wxMessageBox("Lighting settings applied successfully!", "Success", wxOK | wxICON_INFORMATION);
}

void LightingSettingsDialog::onOK(wxCommandEvent& event)
{
	applySettings();
	saveSettings();
	EndModal(wxID_OK);
}

void LightingSettingsDialog::onCancel(wxCommandEvent& event)
{
	EndModal(wxID_CANCEL);
}

void LightingSettingsDialog::onReset(wxCommandEvent& event)
{
	m_config.resetToDefaults();
	updateEnvironmentProperties();
	updateLightList();
	wxMessageBox("Settings reset to defaults!", "Reset", wxOK | wxICON_INFORMATION);
}

void LightingSettingsDialog::applySettings()
{
	// Apply ambient settings
	double ambientIntensity = m_ambientIntensitySlider->GetValue() / 100.0;
	m_config.setEnvironmentAmbientIntensity(ambientIntensity);

	// Apply lights - update each light individually
	for (size_t i = 0; i < m_tempLights.size(); ++i) {
		if (i < m_config.getAllLights().size()) {
			m_config.updateLight(i, m_tempLights[i]);
		}
		else {
			m_config.addLight(m_tempLights[i]);
		}
	}

	// Remove extra lights if we have fewer temp lights
	while (m_config.getAllLights().size() > m_tempLights.size()) {
		m_config.removeLight(m_config.getAllLights().size() - 1);
	}

	// Apply to scene
	m_config.applySettingsToScene();
}

void LightingSettingsDialog::saveSettings()
{
	m_config.saveToFile();
}

void LightingSettingsDialog::onStudioPreset(wxCommandEvent& event)
{
	applyPresetAndUpdate("Studio", "Professional studio lighting with key, fill, and rim lights");
}

void LightingSettingsDialog::onOutdoorPreset(wxCommandEvent& event)
{
	applyPresetAndUpdate("Outdoor", "Natural outdoor lighting with sun and sky illumination");
}

void LightingSettingsDialog::onDramaticPreset(wxCommandEvent& event)
{
	applyPresetAndUpdate("Dramatic", "Dramatic lighting with strong shadows and contrast");
}

void LightingSettingsDialog::onWarmPreset(wxCommandEvent& event)
{
	applyPresetAndUpdate("Warm", "Warm, cozy lighting with orange and yellow tones");
}

void LightingSettingsDialog::onCoolPreset(wxCommandEvent& event)
{
	applyPresetAndUpdate("Cool", "Cool, blue-tinted lighting for a modern look");
}

void LightingSettingsDialog::onMinimalPreset(wxCommandEvent& event)
{
	applyPresetAndUpdate("Minimal", "Simple, minimal lighting with subtle shadows");
}

void LightingSettingsDialog::onFreeCADPreset(wxCommandEvent& event)
{
	applyPresetAndUpdate("FreeCAD", "Classic FreeCAD three-light model: main, fill, and back lights");
}

void LightingSettingsDialog::onNavcubePreset(wxCommandEvent& event)
{
	applyPresetAndUpdate("Navcube", "NavigationCube-style lighting with multiple directional lights");
}

void LightingSettingsDialog::applyPresetAndUpdate(const std::string& presetName, const std::string& description)
{
	// Apply the preset
	if (presetName == "Studio") {
		m_config.applyStudioPreset();
	}
	else if (presetName == "Outdoor") {
		m_config.applyOutdoorPreset();
	}
	else if (presetName == "Dramatic") {
		m_config.applyDramaticPreset();
	}
	else if (presetName == "Warm") {
		m_config.applyWarmPreset();
	}
	else if (presetName == "Cool") {
		m_config.applyCoolPreset();
	}
	else if (presetName == "Minimal") {
		m_config.applyMinimalPreset();
	}
	else if (presetName == "FreeCAD") {
		m_config.applyFreeCADThreeLightPreset();
	}
	else if (presetName == "Navcube") {
		m_config.applyNavigationCubePreset();
	}

	// Update the UI to reflect the new settings
	updateEnvironmentProperties();
	updateLightList();

	// Update the current preset label
	m_currentPresetLabel->SetLabel(wxString::Format("Current: %s\n%s", presetName, description));

	// Show feedback to user
	wxString message = wxString::Format("Applied %s preset!\n\n%s", presetName, description);
	wxMessageBox(message, "Preset Applied", wxOK | wxICON_INFORMATION);

	// Force immediate scene update
	m_config.applySettingsToScene();

	LOG_INF_S("Applied " + presetName + " lighting preset");
}