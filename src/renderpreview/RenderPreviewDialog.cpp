#include "renderpreview/RenderPreviewDialog.h"
#include "renderpreview/PreviewCanvas.h"
#include "renderpreview/GlobalSettingsPanel.h"
#include "renderpreview/ObjectSettingsPanel.h"
#include "renderpreview/ConfigValidator.h"
#include "renderpreview/UndoManager.h"
#include "config/ConfigManager.h"
#include "config/FontManager.h"
#include "logger/Logger.h"
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/colour.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/filedlg.h>
#include <wx/colordlg.h>
#include <wx/listbox.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <wx/scrolwin.h>
#include <wx/msgdlg.h>
#include <wx/grid.h>
#include <wx/font.h>
#include <fstream>
#include <sstream>

// Event table removed - using Bind() method instead

RenderPreviewDialog::RenderPreviewDialog(wxWindow* parent)
	: FramelessModalPopup(parent, wxT("Render Preview System"), wxSize(1200, 700))
	, m_currentLightIndex(-1)
	, m_undoManager(std::make_unique<UndoManager>())
	, m_validationEnabled(true)
{
	LOG_INF_S("RenderPreviewDialog::RenderPreviewDialog: Initializing");

	// Set up title bar with icon
	SetTitleIcon("render", wxSize(20, 20));
	ShowTitleIcon(true);

	// Initialize font manager
	FontManager& fontManager = FontManager::getInstance();
	fontManager.initialize();

	createUI();
	loadConfiguration();

	// Apply fonts to the entire dialog and its children
	fontManager.applyFontToWindowAndChildren(this, "Default");

	// Set fixed size to ensure proper layout
	SetSize(1200, 700);
	SetMinSize(wxSize(1200, 700));

	// Center the dialog on parent window
	if (parent) {
		CenterOnParent();
	}

	// Bind close event
	Bind(wxEVT_CLOSE_WINDOW, &RenderPreviewDialog::OnClose, this);

	LOG_INF_S("RenderPreviewDialog::RenderPreviewDialog: Initialized successfully");
}

void RenderPreviewDialog::createUI()
{
	auto* mainSizer = new wxBoxSizer(wxVERTICAL);

	// Top section: Configuration and Preview
	auto* topSizer = new wxBoxSizer(wxHORIZONTAL);

	// Left panel: Configuration tabs (fixed 450px width)
	auto* leftPanel = new wxPanel(m_contentPanel, wxID_ANY, wxDefaultPosition, wxSize(450, -1), wxBORDER_SUNKEN);
	auto* configNotebook = new wxNotebook(leftPanel, wxID_ANY);

	// Create panel instances
	m_globalSettingsPanel = new GlobalSettingsPanel(configNotebook, this);
	m_objectSettingsPanel = new ObjectSettingsPanel(configNotebook);

	// Add panels to notebook with clear categorization
	configNotebook->AddPage(m_globalSettingsPanel, wxT("Global Settings"));
	configNotebook->AddPage(m_objectSettingsPanel, wxT("Object Settings"));

	auto* leftSizer = new wxBoxSizer(wxVERTICAL);
	leftSizer->Add(configNotebook, 1, wxEXPAND | wxALL, 4);
	leftPanel->SetSizer(leftSizer);

	// Right panel: Render preview canvas (adaptive width)
	m_renderCanvas = new PreviewCanvas(m_contentPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize);

	// Set up manager references after canvas is created
	if (m_renderCanvas) {
		m_globalSettingsPanel->setAntiAliasingManager(m_renderCanvas->getAntiAliasingManager());
		m_globalSettingsPanel->setRenderingManager(m_renderCanvas->getRenderingManager());
		m_globalSettingsPanel->setBackgroundManager(m_renderCanvas->getBackgroundManager());

		// IMPORTANT: Set ObjectManager reference AFTER canvas is fully initialized
		if (m_objectSettingsPanel) {
			// Use CallAfter to ensure canvas is fully constructed
			CallAfter([this]() {
				ObjectManager* objectManager = m_renderCanvas->getObjectManager();
				if (objectManager) {
					m_objectSettingsPanel->setObjectManager(objectManager);
					m_objectSettingsPanel->setPreviewCanvas(m_renderCanvas);
					LOG_INF_S("RenderPreviewDialog: ObjectManager and PreviewCanvas connected to ObjectSettingsPanel");
				}
				else {
					LOG_ERR_S("RenderPreviewDialog: Failed to get ObjectManager from canvas");
				}
				});
		}
	}

	topSizer->Add(leftPanel, 0, wxEXPAND | wxALL, 2);
	topSizer->Add(m_renderCanvas, 1, wxEXPAND | wxALL, 2);

	mainSizer->Add(topSizer, 1, wxEXPAND | wxALL, 2);

	// Only global dialog buttons at the bottom
	auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

	buttonSizer->AddStretchSpacer();

	// Global dialog buttons (right side)
	auto* dialogButtonSizer = new wxBoxSizer(wxHORIZONTAL);

	m_helpButton = new wxButton(m_contentPanel, wxID_HELP, wxT("Help"));
	m_helpButton->Bind(wxEVT_BUTTON, &RenderPreviewDialog::OnHelp, this);
	dialogButtonSizer->Add(m_helpButton, 0, wxALL, 2);

	m_closeButton = new wxButton(m_contentPanel, wxID_CLOSE, wxT("Close"));
	m_closeButton->Bind(wxEVT_BUTTON, &RenderPreviewDialog::OnCloseButton, this);
	dialogButtonSizer->Add(m_closeButton, 0, wxALL, 2);

	buttonSizer->Add(dialogButtonSizer, 0, wxALL, 2);

	mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 2);

	m_contentPanel->SetSizer(mainSizer);
}

// Global dialog event handlers

// Global dialog event handlers
void RenderPreviewDialog::OnCloseButton(wxCommandEvent& event)
{
	EndModal(wxID_CLOSE);
}

void RenderPreviewDialog::OnHelp(wxCommandEvent& event)
{
	wxString helpMessage = wxT("Render Preview System Help\n\n");
	helpMessage += wxT("Global Settings:\n");
	helpMessage += wxT("- Auto: Automatically apply global settings changes\n");
	helpMessage += wxT("- Apply: Apply current global settings to preview\n");
	helpMessage += wxT("- Save: Save global settings to configuration file\n");
	helpMessage += wxT("- Reset: Reset global settings to defaults\n");
	helpMessage += wxT("- Undo/Redo: Navigate through global settings history\n\n");
	helpMessage += wxT("Object Settings:\n");
	helpMessage += wxT("- Auto: Automatically apply object settings changes\n");
	helpMessage += wxT("- Apply: Apply current object settings to preview\n");
	helpMessage += wxT("- Save: Save object settings to configuration file\n");
	helpMessage += wxT("- Reset: Reset object settings to defaults\n");
	helpMessage += wxT("- Undo/Redo: Navigate through object settings history\n\n");
	helpMessage += wxT("Global Buttons:\n");
	helpMessage += wxT("- Help: Show this help message\n");
	helpMessage += wxT("- Close: Close the dialog");

	wxMessageBox(helpMessage, wxT("Help"), wxOK | wxICON_INFORMATION);
}

void RenderPreviewDialog::saveConfiguration()
{
	try {
		auto& configManager = ConfigManager::getInstance();

		// Save global settings
		if (m_globalSettingsPanel) {
			m_globalSettingsPanel->saveSettings();
		}

		// Save object settings
		if (m_objectSettingsPanel) {
			m_objectSettingsPanel->saveSettings();
		}

		configManager.save();
		LOG_INF_S("RenderPreviewDialog::saveConfiguration: Configuration saved successfully");
	}
	catch (const std::exception& e) {
		LOG_ERR_S("RenderPreviewDialog::saveConfiguration: Failed to save configuration: " + std::string(e.what()));
	}
}

void RenderPreviewDialog::loadConfiguration()
{
	try {
		auto& configManager = ConfigManager::getInstance();

		// Load global settings
		if (m_globalSettingsPanel) {
			m_globalSettingsPanel->loadSettings();
		}

		// Load object settings
		if (m_objectSettingsPanel) {
			m_objectSettingsPanel->loadSettings();
		}

		LOG_INF_S("RenderPreviewDialog::loadConfiguration: Configuration loaded successfully");

		// Initialize light management if not already done
		if (m_lights.empty()) {
			RenderLightSettings defaultLight;
			m_lights.push_back(defaultLight);
			updateLightList();
		}

		// Apply loaded configuration to the preview canvas immediately
		applyLoadedConfigurationToCanvas();
	}
	catch (const std::exception& e) {
		LOG_ERR_S("RenderPreviewDialog::loadConfiguration: Failed to load configuration: " + std::string(e.what()));
	}
}

void RenderPreviewDialog::resetToDefaults()
{
	// Reset global settings
	if (m_globalSettingsPanel) {
		m_globalSettingsPanel->resetToDefaults();
	}

	// Reset object settings
	if (m_objectSettingsPanel) {
		m_objectSettingsPanel->resetToDefaults();
	}

	LOG_INF_S("RenderPreviewDialog::resetToDefaults: Configuration reset to defaults");
}

// Global settings methods
void RenderPreviewDialog::applyGlobalSettingsToCanvas()
{
	if (!m_globalSettingsPanel || !m_renderCanvas) return;

	// Validate settings if validation is enabled
	if (m_validationEnabled) {
		if (!validateCurrentSettings()) {
			return;
		}
	}

	// Get lighting settings from global panel
	auto lights = m_globalSettingsPanel->getLights();

	// Apply anti-aliasing settings
	int antiAliasingMethod = m_globalSettingsPanel->getAntiAliasingMethod();
	int msaaSamples = m_globalSettingsPanel->getMSAASamples();
	bool fxaaEnabled = m_globalSettingsPanel->isFXAAEnabled();

	// Apply rendering mode
	int renderingMode = m_globalSettingsPanel->getRenderingMode();

	// Get background settings from global panel
	int backgroundStyle = m_globalSettingsPanel->getBackgroundStyle();
	wxColour backgroundColor = m_globalSettingsPanel->getBackgroundColor();
	wxColour gradientTopColor = m_globalSettingsPanel->getGradientTopColor();
	wxColour gradientBottomColor = m_globalSettingsPanel->getGradientBottomColor();
	std::string backgroundImagePath = m_globalSettingsPanel->getBackgroundImagePath();
	bool backgroundImageEnabled = m_globalSettingsPanel->isBackgroundImageEnabled();
	float backgroundImageOpacity = m_globalSettingsPanel->getBackgroundImageOpacity();
	int backgroundImageFit = m_globalSettingsPanel->getBackgroundImageFit();
	bool backgroundImageMaintainAspect = m_globalSettingsPanel->isBackgroundImageMaintainAspect();

	// Apply to canvas using update methods
	if (!lights.empty()) {
		// Use multi-light support
		m_renderCanvas->updateMultiLighting(lights);
	}
	m_renderCanvas->updateAntiAliasing(antiAliasingMethod, msaaSamples, fxaaEnabled);
	m_renderCanvas->updateRenderingMode(renderingMode);

	// Apply background settings to rendering manager
	if (m_renderCanvas->getRenderingManager()) {
		RenderingManager* renderingManager = m_renderCanvas->getRenderingManager();
		if (renderingManager->hasActiveConfiguration()) {
			int activeConfigId = renderingManager->getActiveConfigurationId();
			RenderingSettings settings = renderingManager->getConfiguration(activeConfigId);

			// Update background settings
			settings.backgroundStyle = backgroundStyle;
			settings.backgroundColor = backgroundColor;
			settings.gradientTopColor = gradientTopColor;
			settings.gradientBottomColor = gradientBottomColor;
			settings.backgroundImagePath = backgroundImagePath;
			settings.backgroundImageEnabled = backgroundImageEnabled;
			settings.backgroundImageOpacity = backgroundImageOpacity;
			settings.backgroundImageFit = backgroundImageFit;
			settings.backgroundImageMaintainAspect = backgroundImageMaintainAspect;

			// Update configuration and apply
			renderingManager->updateConfiguration(activeConfigId, settings);
			renderingManager->setupRenderingState();

			LOG_INF_S("RenderPreviewDialog::applyGlobalSettingsToCanvas: Applied background settings");
		}
	}

	// Update background configuration from BackgroundManager
	if (m_renderCanvas) {
		m_renderCanvas->updateBackgroundFromConfig();
		m_renderCanvas->render(true);
		m_renderCanvas->Refresh();
		m_renderCanvas->Update();
	}

	LOG_INF_S("RenderPreviewDialog::applyGlobalSettingsToCanvas: Applied global settings to canvas");
}

void RenderPreviewDialog::updateLightList()
{
	// This method is now handled by GlobalSettingsPanel
	// The GlobalSettingsPanel manages its own light list
	LOG_INF_S("RenderPreviewDialog::updateLightList: Light list management moved to GlobalSettingsPanel");
}

void RenderPreviewDialog::onGlobalLightingChanged(wxCommandEvent& event)
{
	updateGlobalLighting();
}

void RenderPreviewDialog::onGlobalAntiAliasingChanged(wxCommandEvent& event)
{
	updateGlobalAntiAliasing();
}

void RenderPreviewDialog::onGlobalRenderingModeChanged(wxCommandEvent& event)
{
	updateGlobalRenderingMode();
}

void RenderPreviewDialog::onObjectMaterialChanged(wxCommandEvent& event)
{
	applyObjectSettingsToCanvas();
}

void RenderPreviewDialog::onObjectTextureChanged(wxCommandEvent& event)
{
	applyObjectSettingsToCanvas();
}

void RenderPreviewDialog::updateGlobalLighting()
{
	if (!m_renderCanvas || !m_globalSettingsPanel) return;

	// Apply lighting from global settings panel using multi-light support
	auto lights = m_globalSettingsPanel->getLights();
	m_renderCanvas->updateMultiLighting(lights);
}

void RenderPreviewDialog::updateGlobalAntiAliasing()
{
	if (!m_renderCanvas || !m_globalSettingsPanel) return;

	int method = m_globalSettingsPanel->getAntiAliasingMethod();
	int msaaSamples = m_globalSettingsPanel->getMSAASamples();
	bool fxaaEnabled = m_globalSettingsPanel->isFXAAEnabled();

	m_renderCanvas->updateAntiAliasing(method, msaaSamples, fxaaEnabled);
}

void RenderPreviewDialog::updateGlobalRenderingMode()
{
	if (!m_renderCanvas || !m_globalSettingsPanel) return;

	int mode = m_globalSettingsPanel->getRenderingMode();
	m_renderCanvas->updateRenderingMode(mode);
}

void RenderPreviewDialog::applyObjectSettingsToCanvas()
{
	if (!m_renderCanvas) return;

	// Apply material settings
	if (m_objectSettingsPanel) {
		float ambient = m_objectSettingsPanel->getAmbient();
		float diffuse = m_objectSettingsPanel->getDiffuse();
		float specular = m_objectSettingsPanel->getSpecular();
		float shininess = m_objectSettingsPanel->getShininess();
		float transparency = m_objectSettingsPanel->getTransparency();

		m_renderCanvas->updateMaterial(ambient, diffuse, specular, shininess, transparency);
	}

	// Apply texture settings
	if (m_objectSettingsPanel) {
		bool enabled = m_objectSettingsPanel->isTextureEnabled();
		int mode = m_objectSettingsPanel->getTextureMode();
		float scale = m_objectSettingsPanel->getTextureScale();

		m_renderCanvas->updateTexture(enabled, mode, scale);
	}

	if (m_renderCanvas) {
		m_renderCanvas->render(false);
	}
}

// New feature implementations

void RenderPreviewDialog::saveCurrentState(const std::string& description)
{
	if (m_undoManager) {
		auto snapshot = createSnapshot();
		m_undoManager->saveState(snapshot, description);
	}
}

void RenderPreviewDialog::applySnapshot(const ConfigSnapshot& snapshot)
{
	// Apply lighting settings
	if (m_globalSettingsPanel) {
		m_globalSettingsPanel->setLights(snapshot.lights);
	}

	// Apply anti-aliasing settings
	if (m_globalSettingsPanel) {
		m_globalSettingsPanel->setAntiAliasingMethod(snapshot.antiAliasingMethod);
		m_globalSettingsPanel->setMSAASamples(snapshot.msaaSamples);
		m_globalSettingsPanel->setFXAAEnabled(snapshot.fxaaEnabled);
	}

	// Apply rendering mode
	if (m_globalSettingsPanel) {
		m_globalSettingsPanel->setRenderingMode(snapshot.renderingMode);
	}

	// Apply object settings
	if (m_objectSettingsPanel) {
		// Note: These would need to be added to ObjectSettingsPanel
		// m_objectSettingsPanel->setAmbient(snapshot.materialAmbient);
		// m_objectSettingsPanel->setDiffuse(snapshot.materialDiffuse);
		// m_objectSettingsPanel->setSpecular(snapshot.materialSpecular);
		// m_objectSettingsPanel->setShininess(snapshot.materialShininess);
		// m_objectSettingsPanel->setTransparency(snapshot.materialTransparency);
		// m_objectSettingsPanel->setTextureEnabled(snapshot.textureEnabled);
		// m_objectSettingsPanel->setTextureMode(snapshot.textureMode);
		// m_objectSettingsPanel->setTextureScale(snapshot.textureScale);
	}

	// Apply to canvas
	applyGlobalSettingsToCanvas();
	applyObjectSettingsToCanvas();
}

ConfigSnapshot RenderPreviewDialog::createSnapshot() const
{
	ConfigSnapshot snapshot;

	// Get lighting settings
	if (m_globalSettingsPanel) {
		snapshot.lights = m_globalSettingsPanel->getLights();
		snapshot.antiAliasingMethod = m_globalSettingsPanel->getAntiAliasingMethod();
		snapshot.msaaSamples = m_globalSettingsPanel->getMSAASamples();
		snapshot.fxaaEnabled = m_globalSettingsPanel->isFXAAEnabled();
		snapshot.renderingMode = m_globalSettingsPanel->getRenderingMode();
	}

	// Get object settings
	if (m_objectSettingsPanel) {
		snapshot.materialAmbient = m_objectSettingsPanel->getAmbient();
		snapshot.materialDiffuse = m_objectSettingsPanel->getDiffuse();
		snapshot.materialSpecular = m_objectSettingsPanel->getSpecular();
		snapshot.materialShininess = m_objectSettingsPanel->getShininess();
		snapshot.materialTransparency = m_objectSettingsPanel->getTransparency();
		snapshot.textureEnabled = m_objectSettingsPanel->isTextureEnabled();
		snapshot.textureMode = m_objectSettingsPanel->getTextureMode();
		snapshot.textureScale = m_objectSettingsPanel->getTextureScale();
	}

	return snapshot;
}

bool RenderPreviewDialog::validateCurrentSettings()
{
	if (!m_globalSettingsPanel) return false;

	// Validate lighting settings
	auto lights = m_globalSettingsPanel->getLights();
	auto lightValidation = ConfigValidator::validateLightSettings(lights);
	if (!lightValidation.isValid) {
		wxMessageBox(wxString(lightValidation.errorMessage), wxT("Validation Error"), wxOK | wxICON_ERROR);
		return false;
	}

	// Validate anti-aliasing settings
	int method = m_globalSettingsPanel->getAntiAliasingMethod();
	int msaaSamples = m_globalSettingsPanel->getMSAASamples();
	bool fxaaEnabled = m_globalSettingsPanel->isFXAAEnabled();
	auto aaValidation = ConfigValidator::validateAntiAliasingSettings(method, msaaSamples, fxaaEnabled);
	if (!aaValidation.isValid) {
		wxMessageBox(wxString(aaValidation.errorMessage), wxT("Validation Error"), wxOK | wxICON_ERROR);
		return false;
	}

	// Validate rendering mode
	int mode = m_globalSettingsPanel->getRenderingMode();
	auto modeValidation = ConfigValidator::validateRenderingMode(mode);
	if (!modeValidation.isValid) {
		wxMessageBox(wxString(modeValidation.errorMessage), wxT("Validation Error"), wxOK | wxICON_ERROR);
		return false;
	}

	// Validate object settings
	if (m_objectSettingsPanel) {
		float ambient = m_objectSettingsPanel->getAmbient();
		float diffuse = m_objectSettingsPanel->getDiffuse();
		float specular = m_objectSettingsPanel->getSpecular();
		float shininess = m_objectSettingsPanel->getShininess();
		float transparency = m_objectSettingsPanel->getTransparency();
		auto materialValidation = ConfigValidator::validateMaterialSettings(ambient, diffuse, specular, shininess, transparency);
		if (!materialValidation.isValid) {
			wxMessageBox(wxString(materialValidation.errorMessage), wxT("Validation Error"), wxOK | wxICON_ERROR);
			return false;
		}

		bool textureEnabled = m_objectSettingsPanel->isTextureEnabled();
		int textureMode = m_objectSettingsPanel->getTextureMode();
		float textureScale = m_objectSettingsPanel->getTextureScale();
		auto textureValidation = ConfigValidator::validateTextureSettings(textureEnabled, textureMode, textureScale);
		if (!textureValidation.isValid) {
			wxMessageBox(wxString(textureValidation.errorMessage), wxT("Validation Error"), wxOK | wxICON_ERROR);
			return false;
		}
	}

	return true;
}

// Legacy method for backward compatibility - now delegates to panels
void RenderPreviewDialog::setAutoApply(bool enabled)
{
	if (m_globalSettingsPanel) {
		m_globalSettingsPanel->setAutoApply(enabled);
	}
	if (m_objectSettingsPanel) {
		// TODO: Add setAutoApply method to ObjectSettingsPanel if needed
	}
}

void RenderPreviewDialog::setValidationEnabled(bool enabled)
{
	m_validationEnabled = enabled;
}

bool RenderPreviewDialog::shouldSaveOnClose()
{
	if (m_globalSettingsPanel && m_globalSettingsPanel->hasUnsavedChanges()) {
		int result = wxMessageBox(
			wxT("You have unsaved changes. Would you like to save them before closing?"),
			wxT("Save Changes"),
			wxYES_NO | wxCANCEL | wxICON_QUESTION
		);

		switch (result) {
		case wxYES:
			m_globalSettingsPanel->saveSettings();
			return true;
		case wxNO:
			return true;
		case wxCANCEL:
			return false;
		default:
			return false;
		}
	}
	return true;
}

void RenderPreviewDialog::OnClose(wxCloseEvent& event)
{
	if (shouldSaveOnClose()) {
		event.Skip();
	}
	else {
		event.Veto();
	}
}

void RenderPreviewDialog::applyLoadedConfigurationToCanvas()
{
	LOG_INF_S("RenderPreviewDialog::applyLoadedConfigurationToCanvas: Applying loaded configuration to preview canvas");

	if (!m_renderCanvas) {
		LOG_ERR_S("RenderPreviewDialog::applyLoadedConfigurationToCanvas: No render canvas available");
		return;
	}

	// Apply global settings to canvas
	applyGlobalSettingsToCanvas();

	// Apply object settings to canvas
	applyObjectSettingsToCanvas();

	LOG_INF_S("RenderPreviewDialog::applyLoadedConfigurationToCanvas: Configuration applied successfully");
}