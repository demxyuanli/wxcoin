#include "renderpreview/BackgroundStylePanel.h"
#include "renderpreview/RenderPreviewDialog.h"
#include "renderpreview/BackgroundManager.h"
#include "config/FontManager.h"
#include "logger/Logger.h"
#include <wx/colordlg.h>
#include <wx/filedlg.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/grid.h>
#include <wx/filename.h>
#include <string>
#include <sstream>

BEGIN_EVENT_TABLE(BackgroundStylePanel, wxPanel)
EVT_CHOICE(wxID_ANY, BackgroundStylePanel::onBackgroundStyleChanged)
EVT_BUTTON(wxID_ANY, BackgroundStylePanel::onBackgroundColorButton)
EVT_BUTTON(wxID_ANY, BackgroundStylePanel::onGradientTopColorButton)
EVT_BUTTON(wxID_ANY, BackgroundStylePanel::onGradientBottomColorButton)
EVT_BUTTON(wxID_ANY, BackgroundStylePanel::onBackgroundImageButton)
EVT_SLIDER(wxID_ANY, BackgroundStylePanel::onBackgroundImageOpacityChanged)
EVT_CHOICE(wxID_ANY, BackgroundStylePanel::onBackgroundImageFitChanged)
EVT_CHECKBOX(wxID_ANY, BackgroundStylePanel::onBackgroundImageMaintainAspectChanged)
END_EVENT_TABLE()

BackgroundStylePanel::BackgroundStylePanel(wxWindow* parent, RenderPreviewDialog* dialog)
	: wxPanel(parent, wxID_ANY), m_parentDialog(dialog), m_backgroundManager(nullptr)
{
	createUI();
	bindEvents();
	updateControlStates();

	// Initialize with default values
	if (m_backgroundStyleChoice) {
		m_backgroundStyleChoice->SetSelection(0);
	}
	if (m_backgroundImageFitChoice) {
		m_backgroundImageFitChoice->SetSelection(0);
	}
	if (m_backgroundImageOpacitySlider) {
		m_backgroundImageOpacitySlider->SetValue(100);
	}
	if (m_backgroundImageMaintainAspectCheckBox) {
		m_backgroundImageMaintainAspectCheckBox->SetValue(true);
	}

	LOG_INF_S("BackgroundStylePanel: Initialized with default settings");
}

BackgroundStylePanel::~BackgroundStylePanel()
{
}

void BackgroundStylePanel::createUI()
{
	wxBoxSizer* backgroundSizer = new wxBoxSizer(wxVERTICAL);

	// Background Style Section
	wxStaticBoxSizer* styleBoxSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Background Style");
	m_backgroundStyleChoice = new wxChoice(this, wxID_ANY);
	m_backgroundStyleChoice->Append("Solid Color");
	m_backgroundStyleChoice->Append("Gradient");
	m_backgroundStyleChoice->Append("Image");
	m_backgroundStyleChoice->SetSelection(0);
	styleBoxSizer->Add(m_backgroundStyleChoice, 0, wxEXPAND | wxALL, 4);
	backgroundSizer->Add(styleBoxSizer, 0, wxEXPAND | wxALL, 4);

	// Preset Backgrounds Section
	wxStaticBoxSizer* presetBoxSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Preset Backgrounds");

	// Create a grid sizer for preset buttons (2 columns)
	wxGridSizer* presetGridSizer = new wxGridSizer(2, 3, 4, 4);

	// Environment preset button (Sky blue)
	m_environmentPresetButton = new wxButton(this, wxID_ANY, "Environment", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	m_environmentPresetButton->SetBackgroundColour(wxColour(135, 206, 235));
	m_environmentPresetButton->SetForegroundColour(wxColour(0, 0, 0));
	presetGridSizer->Add(m_environmentPresetButton, 0, wxEXPAND | wxALL, 2);

	// Studio preset button (Light blue)
	m_studioPresetButton = new wxButton(this, wxID_ANY, "Studio", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	m_studioPresetButton->SetBackgroundColour(wxColour(240, 248, 255));
	m_studioPresetButton->SetForegroundColour(wxColour(0, 0, 0));
	presetGridSizer->Add(m_studioPresetButton, 0, wxEXPAND | wxALL, 2);

	// Outdoor preset button (Light yellow)
	m_outdoorPresetButton = new wxButton(this, wxID_ANY, "Outdoor", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	m_outdoorPresetButton->SetBackgroundColour(wxColour(255, 255, 224));
	m_outdoorPresetButton->SetForegroundColour(wxColour(0, 0, 0));
	presetGridSizer->Add(m_outdoorPresetButton, 0, wxEXPAND | wxALL, 2);

	// Industrial preset button (Light gray)
	m_industrialPresetButton = new wxButton(this, wxID_ANY, "Industrial", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	m_industrialPresetButton->SetBackgroundColour(wxColour(245, 245, 245));
	m_industrialPresetButton->SetForegroundColour(wxColour(0, 0, 0));
	presetGridSizer->Add(m_industrialPresetButton, 0, wxEXPAND | wxALL, 2);

	// CAD preset button (Light cream)
	m_cadPresetButton = new wxButton(this, wxID_ANY, "CAD", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	m_cadPresetButton->SetBackgroundColour(wxColour(255, 248, 220));
	m_cadPresetButton->SetForegroundColour(wxColour(0, 0, 0));
	presetGridSizer->Add(m_cadPresetButton, 0, wxEXPAND | wxALL, 2);

	// Dark preset button (Dark gray)
	m_darkPresetButton = new wxButton(this, wxID_ANY, "Dark", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	m_darkPresetButton->SetBackgroundColour(wxColour(40, 40, 40));
	m_darkPresetButton->SetForegroundColour(wxColour(255, 255, 255));
	presetGridSizer->Add(m_darkPresetButton, 0, wxEXPAND | wxALL, 2);

	presetBoxSizer->Add(presetGridSizer, 0, wxEXPAND | wxALL, 4);
	backgroundSizer->Add(presetBoxSizer, 0, wxEXPAND | wxALL, 4);

	// Color Settings Section
	wxStaticBoxSizer* colorBoxSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Color Settings");

	// Background color button with color display
	wxBoxSizer* backgroundColorSizer = new wxBoxSizer(wxHORIZONTAL);
	m_backgroundColorButton = new wxButton(this, wxID_ANY, "Background Color", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	m_backgroundColorButton->SetBackgroundColour(wxColour(173, 204, 255)); // Default light blue
	m_backgroundColorButton->SetForegroundColour(wxColour(0, 0, 0));
	backgroundColorSizer->Add(m_backgroundColorButton, 1, wxEXPAND | wxALL, 2);
	colorBoxSizer->Add(backgroundColorSizer, 0, wxEXPAND | wxALL, 4);

	// Gradient colors
	wxBoxSizer* gradientTopSizer = new wxBoxSizer(wxHORIZONTAL);
	m_gradientTopColorButton = new wxButton(this, wxID_ANY, "Gradient Top", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	m_gradientTopColorButton->SetBackgroundColour(wxColour(200, 220, 255));
	m_gradientTopColorButton->SetForegroundColour(wxColour(0, 0, 0));
	gradientTopSizer->Add(m_gradientTopColorButton, 1, wxEXPAND | wxALL, 2);
	colorBoxSizer->Add(gradientTopSizer, 0, wxEXPAND | wxALL, 4);

	wxBoxSizer* gradientBottomSizer = new wxBoxSizer(wxHORIZONTAL);
	m_gradientBottomColorButton = new wxButton(this, wxID_ANY, "Gradient Bottom", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	m_gradientBottomColorButton->SetBackgroundColour(wxColour(150, 180, 255));
	m_gradientBottomColorButton->SetForegroundColour(wxColour(255, 255, 255));
	gradientBottomSizer->Add(m_gradientBottomColorButton, 1, wxEXPAND | wxALL, 2);
	colorBoxSizer->Add(gradientBottomSizer, 0, wxEXPAND | wxALL, 4);

	backgroundSizer->Add(colorBoxSizer, 0, wxEXPAND | wxALL, 4);

	// Image Settings Section
	wxStaticBoxSizer* imageBoxSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Image Settings");
	m_backgroundImageButton = new wxButton(this, wxID_ANY, "Choose Image");
	imageBoxSizer->Add(m_backgroundImageButton, 0, wxEXPAND | wxALL, 4);
	m_backgroundImagePathLabel = new wxStaticText(this, wxID_ANY, "No image selected");
	imageBoxSizer->Add(m_backgroundImagePathLabel, 0, wxEXPAND | wxALL, 4);
	m_backgroundImageOpacitySlider = new wxSlider(this, wxID_ANY, 100, 0, 100, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL | wxSL_LABELS);
	imageBoxSizer->Add(m_backgroundImageOpacitySlider, 0, wxEXPAND | wxALL, 4);
	m_backgroundImageFitChoice = new wxChoice(this, wxID_ANY);
	m_backgroundImageFitChoice->Append("Fill");
	m_backgroundImageFitChoice->Append("Fit");
	m_backgroundImageFitChoice->Append("Stretch");
	m_backgroundImageFitChoice->SetSelection(0);
	imageBoxSizer->Add(m_backgroundImageFitChoice, 0, wxEXPAND | wxALL, 4);
	m_backgroundImageMaintainAspectCheckBox = new wxCheckBox(this, wxID_ANY, "Maintain Aspect Ratio");
	imageBoxSizer->Add(m_backgroundImageMaintainAspectCheckBox, 0, wxEXPAND | wxALL, 4);
	backgroundSizer->Add(imageBoxSizer, 0, wxEXPAND | wxALL, 4);

	SetSizer(backgroundSizer);
}

void BackgroundStylePanel::bindEvents()
{
	m_backgroundStyleChoice->Bind(wxEVT_CHOICE, &BackgroundStylePanel::onBackgroundStyleChanged, this);
	m_backgroundColorButton->Bind(wxEVT_BUTTON, &BackgroundStylePanel::onBackgroundColorButton, this);
	m_gradientTopColorButton->Bind(wxEVT_BUTTON, &BackgroundStylePanel::onGradientTopColorButton, this);
	m_gradientBottomColorButton->Bind(wxEVT_BUTTON, &BackgroundStylePanel::onGradientBottomColorButton, this);
	m_backgroundImageButton->Bind(wxEVT_BUTTON, &BackgroundStylePanel::onBackgroundImageButton, this);
	m_backgroundImageOpacitySlider->Bind(wxEVT_SLIDER, &BackgroundStylePanel::onBackgroundImageOpacityChanged, this);
	m_backgroundImageFitChoice->Bind(wxEVT_CHOICE, &BackgroundStylePanel::onBackgroundImageFitChanged, this);
	m_backgroundImageMaintainAspectCheckBox->Bind(wxEVT_CHECKBOX, &BackgroundStylePanel::onBackgroundImageMaintainAspectChanged, this);

	// Bind preset button events
	m_environmentPresetButton->Bind(wxEVT_BUTTON, &BackgroundStylePanel::onEnvironmentPresetButton, this);
	m_studioPresetButton->Bind(wxEVT_BUTTON, &BackgroundStylePanel::onStudioPresetButton, this);
	m_outdoorPresetButton->Bind(wxEVT_BUTTON, &BackgroundStylePanel::onOutdoorPresetButton, this);
	m_industrialPresetButton->Bind(wxEVT_BUTTON, &BackgroundStylePanel::onIndustrialPresetButton, this);
	m_cadPresetButton->Bind(wxEVT_BUTTON, &BackgroundStylePanel::onCadPresetButton, this);
	m_darkPresetButton->Bind(wxEVT_BUTTON, &BackgroundStylePanel::onDarkPresetButton, this);
}

void BackgroundStylePanel::updateControlStates()
{
	int selection = m_backgroundStyleChoice->GetSelection();

	// Enable/disable controls based on selection
	// 0 = Solid Color, 1 = Gradient, 2 = Image, 3 = Environment, 4 = Studio, 5 = Outdoor, 6 = Industrial, 7 = CAD, 8 = Dark
	bool isSolidColor = (selection == 0);
	bool isGradient = (selection == 1);
	bool isImage = (selection == 2);
	bool isPreset = (selection >= 3 && selection <= 8); // Environment, Studio, Outdoor, Industrial, CAD, Dark

	m_backgroundColorButton->Enable(isSolidColor);
	m_gradientTopColorButton->Enable(isGradient);
	m_gradientBottomColorButton->Enable(isGradient);
	m_backgroundImageButton->Enable(isImage);
	m_backgroundImageOpacitySlider->Enable(isImage);
	m_backgroundImageFitChoice->Enable(isImage);
	m_backgroundImageMaintainAspectCheckBox->Enable(isImage);
}

void BackgroundStylePanel::notifyParameterChanged()
{
	if (m_parameterChangeCallback) {
		m_parameterChangeCallback();
	}
}

void BackgroundStylePanel::onBackgroundStyleChanged(wxCommandEvent& event)
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		int activeId = m_backgroundManager->getActiveConfigurationId();
		int selection = m_backgroundStyleChoice->GetSelection();
		m_backgroundManager->setStyle(activeId, selection);

		// Set background color based on selection
		wxColour presetColor;
		switch (selection) {
		case 0: // Solid Color
			presetColor = wxColour(173, 204, 255); // Default light blue
			break;
		case 1: // Gradient
			presetColor = wxColour(200, 220, 255); // Light blue
			break;
		case 2: // Image
			presetColor = wxColour(173, 204, 255); // Default light blue
			break;
		case 3: // Environment
			presetColor = wxColour(135, 206, 235); // Sky blue
			break;
		case 4: // Studio
			presetColor = wxColour(240, 248, 255); // Light blue
			break;
		case 5: // Outdoor
			presetColor = wxColour(255, 255, 224); // Light yellow
			break;
		case 6: // Industrial
			presetColor = wxColour(245, 245, 245); // Light gray
			break;
		case 7: // CAD
			presetColor = wxColour(255, 248, 220); // Light cream
			break;
		case 8: // Dark
			presetColor = wxColour(40, 40, 40); // Dark gray
			break;
		default:
			presetColor = wxColour(173, 204, 255); // Default light blue
			break;
		}
		m_backgroundManager->setBackgroundColor(activeId, presetColor);

		// Apply the background changes immediately
		m_backgroundManager->renderBackground();

		notifyParameterChanged();

		std::ostringstream oss;
		oss << selection;
		LOG_INF_S("BackgroundStylePanel::onBackgroundStyleChanged: Applied background style " + oss.str());
	}
	if (m_parentDialog) {
		m_parentDialog->applyGlobalSettingsToCanvas();
	}
}

void BackgroundStylePanel::onBackgroundColorButton(wxCommandEvent& event)
{
	wxColourDialog dialog(this);
	if (dialog.ShowModal() == wxID_OK) {
		if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
			int activeId = m_backgroundManager->getActiveConfigurationId();
			wxColour selectedColor = dialog.GetColourData().GetColour();
			m_backgroundManager->setBackgroundColor(activeId, selectedColor);
			m_backgroundColorButton->SetBackgroundColour(selectedColor);
			m_backgroundColorButton->SetForegroundColour(wxColour(0, 0, 0));
			m_backgroundManager->renderBackground();

			notifyParameterChanged();

			LOG_INF_S("BackgroundStylePanel::onBackgroundColorButton: Applied background color");
		}
		if (m_parentDialog) {
			m_parentDialog->applyGlobalSettingsToCanvas();
		}
	}
}

void BackgroundStylePanel::onGradientTopColorButton(wxCommandEvent& event)
{
	wxColourDialog dialog(this);
	if (dialog.ShowModal() == wxID_OK) {
		if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
			int activeId = m_backgroundManager->getActiveConfigurationId();
			wxColour selectedColor = dialog.GetColourData().GetColour();
			m_backgroundManager->setGradientTopColor(activeId, selectedColor);
			m_gradientTopColorButton->SetBackgroundColour(selectedColor);
			m_gradientTopColorButton->SetForegroundColour(wxColour(0, 0, 0));
			m_backgroundManager->renderBackground();

			notifyParameterChanged();

			LOG_INF_S("BackgroundStylePanel::onGradientTopColorButton: Applied gradient top color");
		}
		if (m_parentDialog) {
			m_parentDialog->applyGlobalSettingsToCanvas();
		}
	}
}

void BackgroundStylePanel::onGradientBottomColorButton(wxCommandEvent& event)
{
	wxColourDialog dialog(this);
	if (dialog.ShowModal() == wxID_OK) {
		if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
			int activeId = m_backgroundManager->getActiveConfigurationId();
			wxColour selectedColor = dialog.GetColourData().GetColour();
			m_backgroundManager->setGradientBottomColor(activeId, selectedColor);
			m_gradientBottomColorButton->SetBackgroundColour(selectedColor);
			m_gradientBottomColorButton->SetForegroundColour(wxColour(255, 255, 255));
			m_backgroundManager->renderBackground();

			notifyParameterChanged();

			LOG_INF_S("BackgroundStylePanel::onGradientBottomColorButton: Applied gradient bottom color");
		}
		if (m_parentDialog) {
			m_parentDialog->applyGlobalSettingsToCanvas();
		}
	}
}

void BackgroundStylePanel::onBackgroundImageButton(wxCommandEvent& event)
{
	wxFileDialog openFileDialog(this, _("Open image file"), "", "",
		"Image files (*.png;*.jpg;*.jpeg;*.bmp;*.tiff)|*.png;*.jpg;*.jpeg;*.bmp;*.tiff|All files (*.*)|*.*",
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (openFileDialog.ShowModal() == wxID_OK) {
		if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
			int activeId = m_backgroundManager->getActiveConfigurationId();

			// Get the selected file path and normalize it
			wxString filePath = openFileDialog.GetPath();
			wxFileName fileName(filePath);
			fileName.MakeAbsolute();

			// Convert to UTF-8 string for storage
			std::string normalizedPath = fileName.GetFullPath().ToUTF8().data();

			m_backgroundManager->setImagePath(activeId, normalizedPath);
			m_backgroundManager->setImageEnabled(activeId, true);
			m_backgroundImagePathLabel->SetLabel(fileName.GetFullName());
			m_backgroundManager->renderBackground();

			notifyParameterChanged();

			LOG_INF_S("BackgroundStylePanel::onBackgroundImageButton: Applied background image: " + normalizedPath);
		}
		if (m_parentDialog) {
			m_parentDialog->applyGlobalSettingsToCanvas();
		}
	}
}

void BackgroundStylePanel::onBackgroundImageOpacityChanged(wxCommandEvent& event)
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		int activeId = m_backgroundManager->getActiveConfigurationId();
		m_backgroundManager->setImageOpacity(activeId, m_backgroundImageOpacitySlider->GetValue() / 100.0f);

		// Apply the background changes immediately
		m_backgroundManager->renderBackground();

		notifyParameterChanged();

		std::ostringstream oss;
		oss << m_backgroundImageOpacitySlider->GetValue();
		LOG_INF_S("BackgroundStylePanel::onBackgroundImageOpacityChanged: Applied image opacity " + oss.str());
	}
	if (m_parentDialog) {
		m_parentDialog->applyGlobalSettingsToCanvas();
	}
}

void BackgroundStylePanel::onBackgroundImageFitChanged(wxCommandEvent& event)
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		int activeId = m_backgroundManager->getActiveConfigurationId();
		m_backgroundManager->setImageFit(activeId, m_backgroundImageFitChoice->GetSelection());

		// Apply the background changes immediately
		m_backgroundManager->renderBackground();

		notifyParameterChanged();

		std::ostringstream oss;
		oss << m_backgroundImageFitChoice->GetSelection();
		LOG_INF_S("BackgroundStylePanel::onBackgroundImageFitChanged: Applied image fit " + oss.str());
	}
	if (m_parentDialog) {
		m_parentDialog->applyGlobalSettingsToCanvas();
	}
}

void BackgroundStylePanel::onBackgroundImageMaintainAspectChanged(wxCommandEvent& event)
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		int activeId = m_backgroundManager->getActiveConfigurationId();
		m_backgroundManager->setImageMaintainAspect(activeId, m_backgroundImageMaintainAspectCheckBox->GetValue());

		// Apply the background changes immediately
		m_backgroundManager->renderBackground();

		notifyParameterChanged();

		std::ostringstream oss;
		oss << m_backgroundImageMaintainAspectCheckBox->GetValue();
		LOG_INF_S("BackgroundStylePanel::onBackgroundImageMaintainAspectChanged: Applied image maintain aspect " + oss.str());
	}
	if (m_parentDialog) {
		m_parentDialog->applyGlobalSettingsToCanvas();
	}
}

void BackgroundStylePanel::updateButtonColors()
{
	if (!m_backgroundManager || !m_backgroundManager->hasActiveConfiguration()) {
		return;
	}

	BackgroundSettings settings = m_backgroundManager->getActiveConfiguration();

	// Update background color button
	if (m_backgroundColorButton) {
		m_backgroundColorButton->SetBackgroundColour(settings.backgroundColor);
		// Set foreground color based on background brightness
		int brightness = (settings.backgroundColor.Red() + settings.backgroundColor.Green() + settings.backgroundColor.Blue()) / 3;
		m_backgroundColorButton->SetForegroundColour(brightness > 128 ? wxColour(0, 0, 0) : wxColour(255, 255, 255));
		m_backgroundColorButton->Refresh();
	}

	// Update gradient color buttons
	if (m_gradientTopColorButton) {
		m_gradientTopColorButton->SetBackgroundColour(settings.gradientTopColor);
		int brightness = (settings.gradientTopColor.Red() + settings.gradientTopColor.Green() + settings.gradientTopColor.Blue()) / 3;
		m_gradientTopColorButton->SetForegroundColour(brightness > 128 ? wxColour(0, 0, 0) : wxColour(255, 255, 255));
		m_gradientTopColorButton->Refresh();
	}

	if (m_gradientBottomColorButton) {
		m_gradientBottomColorButton->SetBackgroundColour(settings.gradientBottomColor);
		int brightness = (settings.gradientBottomColor.Red() + settings.gradientBottomColor.Green() + settings.gradientBottomColor.Blue()) / 3;
		m_gradientBottomColorButton->SetForegroundColour(brightness > 128 ? wxColour(0, 0, 0) : wxColour(255, 255, 255));
		m_gradientBottomColorButton->Refresh();
	}
}

void BackgroundStylePanel::onEnvironmentPresetButton(wxCommandEvent& event)
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		int activeId = m_backgroundManager->getActiveConfigurationId();
		m_backgroundManager->setStyle(activeId, 3); // Environment style
		m_backgroundManager->setBackgroundColor(activeId, wxColour(135, 206, 235)); // Sky blue
		m_backgroundManager->renderBackground();

		// Update UI
		m_backgroundStyleChoice->SetSelection(0); // Set to Solid Color
		updateControlStates();
		updateButtonColors();

		notifyParameterChanged();

		LOG_INF_S("BackgroundStylePanel::onEnvironmentPresetButton: Applied environment preset");
	}
	if (m_parentDialog) {
		m_parentDialog->applyGlobalSettingsToCanvas();
	}
}

void BackgroundStylePanel::onStudioPresetButton(wxCommandEvent& event)
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		int activeId = m_backgroundManager->getActiveConfigurationId();
		m_backgroundManager->setStyle(activeId, 4); // Studio style
		m_backgroundManager->setBackgroundColor(activeId, wxColour(240, 248, 255)); // Light blue
		m_backgroundManager->renderBackground();

		// Update UI
		m_backgroundStyleChoice->SetSelection(0); // Set to Solid Color
		updateControlStates();
		updateButtonColors();

		notifyParameterChanged();

		LOG_INF_S("BackgroundStylePanel::onStudioPresetButton: Applied studio preset");
	}
	if (m_parentDialog) {
		m_parentDialog->applyGlobalSettingsToCanvas();
	}
}

void BackgroundStylePanel::onOutdoorPresetButton(wxCommandEvent& event)
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		int activeId = m_backgroundManager->getActiveConfigurationId();
		m_backgroundManager->setStyle(activeId, 5); // Outdoor style
		m_backgroundManager->setBackgroundColor(activeId, wxColour(255, 255, 224)); // Light yellow
		m_backgroundManager->renderBackground();

		// Update UI
		m_backgroundStyleChoice->SetSelection(0); // Set to Solid Color
		updateControlStates();
		updateButtonColors();

		notifyParameterChanged();

		LOG_INF_S("BackgroundStylePanel::onOutdoorPresetButton: Applied outdoor preset");
	}
	if (m_parentDialog) {
		m_parentDialog->applyGlobalSettingsToCanvas();
	}
}

void BackgroundStylePanel::onIndustrialPresetButton(wxCommandEvent& event)
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		int activeId = m_backgroundManager->getActiveConfigurationId();
		m_backgroundManager->setStyle(activeId, 6); // Industrial style
		m_backgroundManager->setBackgroundColor(activeId, wxColour(245, 245, 245)); // Light gray
		m_backgroundManager->renderBackground();

		// Update UI
		m_backgroundStyleChoice->SetSelection(0); // Set to Solid Color
		updateControlStates();
		updateButtonColors();

		notifyParameterChanged();

		LOG_INF_S("BackgroundStylePanel::onIndustrialPresetButton: Applied industrial preset");
	}
	if (m_parentDialog) {
		m_parentDialog->applyGlobalSettingsToCanvas();
	}
}

void BackgroundStylePanel::onCadPresetButton(wxCommandEvent& event)
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		int activeId = m_backgroundManager->getActiveConfigurationId();
		m_backgroundManager->setStyle(activeId, 7); // CAD style
		m_backgroundManager->setBackgroundColor(activeId, wxColour(255, 248, 220)); // Light cream
		m_backgroundManager->renderBackground();

		// Update UI
		m_backgroundStyleChoice->SetSelection(0); // Set to Solid Color
		updateControlStates();
		updateButtonColors();

		notifyParameterChanged();

		LOG_INF_S("BackgroundStylePanel::onCadPresetButton: Applied CAD preset");
	}
	if (m_parentDialog) {
		m_parentDialog->applyGlobalSettingsToCanvas();
	}
}

void BackgroundStylePanel::onDarkPresetButton(wxCommandEvent& event)
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		int activeId = m_backgroundManager->getActiveConfigurationId();
		m_backgroundManager->setStyle(activeId, 8); // Dark style
		m_backgroundManager->setBackgroundColor(activeId, wxColour(40, 40, 40)); // Dark gray
		m_backgroundManager->renderBackground();

		// Update UI
		m_backgroundStyleChoice->SetSelection(0); // Set to Solid Color
		updateControlStates();
		updateButtonColors();

		notifyParameterChanged();

		LOG_INF_S("BackgroundStylePanel::onDarkPresetButton: Applied dark preset");
	}
	if (m_parentDialog) {
		m_parentDialog->applyGlobalSettingsToCanvas();
	}
}

void BackgroundStylePanel::setBackgroundManager(BackgroundManager* manager)
{
	m_backgroundManager = manager;

	// Sync UI with current background settings
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		BackgroundSettings settings = m_backgroundManager->getActiveConfiguration();

		// Update UI controls
		if (m_backgroundStyleChoice) {
			m_backgroundStyleChoice->SetSelection(settings.style);
		}

		// Update control states based on current style
		updateControlStates();

		// Update image path label if image is set
		if (settings.imageEnabled && !settings.imagePath.empty() && m_backgroundImagePathLabel) {
			m_backgroundImagePathLabel->SetLabel(wxString::FromUTF8(settings.imagePath));
		}

		// Update image opacity slider
		if (m_backgroundImageOpacitySlider) {
			m_backgroundImageOpacitySlider->SetValue(static_cast<int>(settings.imageOpacity * 100));
		}

		// Update image fit choice
		if (m_backgroundImageFitChoice) {
			m_backgroundImageFitChoice->SetSelection(settings.imageFit);
		}

		// Update maintain aspect checkbox
		if (m_backgroundImageMaintainAspectCheckBox) {
			m_backgroundImageMaintainAspectCheckBox->SetValue(settings.imageMaintainAspect);
		}

		// Update button colors
		updateButtonColors();

		LOG_INF_S("BackgroundStylePanel::setBackgroundManager: Synced UI with background settings");
	}
}

int BackgroundStylePanel::getBackgroundStyle() const
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		return m_backgroundManager->getActiveConfiguration().style;
	}
	return 0;
}

wxColour BackgroundStylePanel::getBackgroundColor() const
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		return m_backgroundManager->getActiveConfiguration().backgroundColor;
	}
	return wxColour(0, 0, 0);
}

wxColour BackgroundStylePanel::getGradientTopColor() const
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		return m_backgroundManager->getActiveConfiguration().gradientTopColor;
	}
	return wxColour(0, 0, 0);
}

wxColour BackgroundStylePanel::getGradientBottomColor() const
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		return m_backgroundManager->getActiveConfiguration().gradientBottomColor;
	}
	return wxColour(0, 0, 0);
}

std::string BackgroundStylePanel::getBackgroundImagePath() const
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		return m_backgroundManager->getActiveConfiguration().imagePath;
	}
	return "";
}

bool BackgroundStylePanel::isBackgroundImageEnabled() const
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		return m_backgroundManager->getActiveConfiguration().imageEnabled;
	}
	return false;
}

float BackgroundStylePanel::getBackgroundImageOpacity() const
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		return m_backgroundManager->getActiveConfiguration().imageOpacity;
	}
	return 1.0f;
}

int BackgroundStylePanel::getBackgroundImageFit() const
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		return m_backgroundManager->getActiveConfiguration().imageFit;
	}
	return 0;
}

bool BackgroundStylePanel::isBackgroundImageMaintainAspect() const
{
	if (m_backgroundManager && m_backgroundManager->hasActiveConfiguration()) {
		return m_backgroundManager->getActiveConfiguration().imageMaintainAspect;
	}
	return true;
}

void BackgroundStylePanel::applyFonts()
{
	FontManager& fontManager = FontManager::getInstance();

	// Apply fonts to choice controls
	if (m_backgroundStyleChoice) m_backgroundStyleChoice->SetFont(fontManager.getChoiceFont());
	if (m_backgroundImageFitChoice) m_backgroundImageFitChoice->SetFont(fontManager.getChoiceFont());

	// Apply fonts to buttons
	if (m_backgroundColorButton) m_backgroundColorButton->SetFont(fontManager.getButtonFont());
	if (m_gradientTopColorButton) m_gradientTopColorButton->SetFont(fontManager.getButtonFont());
	if (m_gradientBottomColorButton) m_gradientBottomColorButton->SetFont(fontManager.getButtonFont());
	if (m_backgroundImageButton) m_backgroundImageButton->SetFont(fontManager.getButtonFont());

	// Apply fonts to preset buttons
	if (m_environmentPresetButton) m_environmentPresetButton->SetFont(fontManager.getButtonFont());
	if (m_studioPresetButton) m_studioPresetButton->SetFont(fontManager.getButtonFont());
	if (m_outdoorPresetButton) m_outdoorPresetButton->SetFont(fontManager.getButtonFont());
	if (m_industrialPresetButton) m_industrialPresetButton->SetFont(fontManager.getButtonFont());
	if (m_cadPresetButton) m_cadPresetButton->SetFont(fontManager.getButtonFont());
	if (m_darkPresetButton) m_darkPresetButton->SetFont(fontManager.getButtonFont());

	// Apply fonts to slider
	if (m_backgroundImageOpacitySlider) m_backgroundImageOpacitySlider->SetFont(fontManager.getLabelFont());

	// Apply fonts to checkbox
	if (m_backgroundImageMaintainAspectCheckBox) m_backgroundImageMaintainAspectCheckBox->SetFont(fontManager.getLabelFont());

	// Apply fonts to labels
	if (m_backgroundImagePathLabel) m_backgroundImagePathLabel->SetFont(fontManager.getLabelFont());

	// Apply fonts to all static texts in the panel
	wxWindowList& children = GetChildren();
	for (wxWindowList::iterator it = children.begin(); it != children.end(); ++it) {
		wxWindow* child = *it;
		if (child) {
			if (dynamic_cast<wxStaticText*>(child)) {
				child->SetFont(fontManager.getLabelFont());
			}
		}
	}
}