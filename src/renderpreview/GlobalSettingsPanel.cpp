#include "renderpreview/GlobalSettingsPanel.h"
#include "renderpreview/RenderPreviewDialog.h"
#include "renderpreview/PreviewCanvas.h"
#include "renderpreview/LightingPanel.h"
#include "renderpreview/AntiAliasingPanel.h"
#include "renderpreview/RenderingModePanel.h"
#include "renderpreview/BackgroundStylePanel.h"
#include "renderpreview/AntiAliasingManager.h"
#include "renderpreview/RenderingManager.h"
#include "renderpreview/BackgroundManager.h"
#include "config/ConfigManager.h"
#include "config/LightingConfig.h"
#include "config/FontManager.h"
#include "logger/Logger.h"
#include <wx/msgdlg.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

BEGIN_EVENT_TABLE(GlobalSettingsPanel, wxPanel)
END_EVENT_TABLE()

GlobalSettingsPanel::GlobalSettingsPanel(wxWindow* parent, RenderPreviewDialog* dialog, wxWindowID id)
    : wxPanel(parent, id)
    , m_parentDialog(dialog)
    , m_autoApply(false)
    , m_hasUnsavedChanges(false)
    , m_antiAliasingManager(nullptr)
    , m_renderingManager(nullptr)
    , m_backgroundManager(nullptr)
{
    LOG_INF_S("GlobalSettingsPanel::GlobalSettingsPanel: Initializing");
    
    FontManager& fontManager = FontManager::getInstance();
    fontManager.initialize();
    
    createUI();
    bindEvents();
    loadSettings();
    
    fontManager.applyFontToWindowAndChildren(this, "Default");
    
    applySpecificFonts();
    
    validatePresets();
    
    testPresetFunctionality();
    
    LOG_INF_S("GlobalSettingsPanel::GlobalSettingsPanel: Initialized successfully");
}

GlobalSettingsPanel::~GlobalSettingsPanel()
{
    LOG_INF_S("GlobalSettingsPanel::~GlobalSettingsPanel: Destroying");
}

void GlobalSettingsPanel::createUI()
{
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    m_notebook = new wxNotebook(this, wxID_ANY);
    
    m_lightingPanel = new LightingPanel(m_notebook, m_parentDialog);
    m_antiAliasingPanel = new AntiAliasingPanel(m_notebook, m_parentDialog);
    m_renderingModePanel = new RenderingModePanel(m_notebook, m_parentDialog);
    m_backgroundStylePanel = new BackgroundStylePanel(m_notebook, m_parentDialog);
    
    // Set parameter change callbacks for all panels
    m_lightingPanel->setParameterChangeCallback([this]() { markAsChanged(); });
    m_antiAliasingPanel->setParameterChangeCallback([this]() { markAsChanged(); });
    m_renderingModePanel->setParameterChangeCallback([this]() { markAsChanged(); });
    m_backgroundStylePanel->setParameterChangeCallback([this]() { markAsChanged(); });
    
    m_notebook->AddPage(m_lightingPanel, "Lighting");
    m_notebook->AddPage(m_antiAliasingPanel, "Anti-aliasing");
    m_notebook->AddPage(m_renderingModePanel, "Rendering Mode");
    m_notebook->AddPage(m_backgroundStylePanel, "Background Style");
    
    mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 4);
    
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_globalAutoApplyCheckBox = new wxCheckBox(this, wxID_ANY, wxT("Auto"));
    buttonSizer->Add(m_globalAutoApplyCheckBox, 0, wxALL | wxALIGN_CENTER_VERTICAL, 4);
    
    m_globalApplyButton = new wxButton(this, wxID_APPLY, wxT("Preview"));
    buttonSizer->Add(m_globalApplyButton, 0, wxALL, 4);

    m_mainApplyButton = new wxButton(this, wxID_ANY, wxT("Main Apply"));
    buttonSizer->Add(m_mainApplyButton, 0, wxALL, 4);
    
    m_globalSaveButton = new wxButton(this, wxID_SAVE, wxT("Save"));
    buttonSizer->Add(m_globalSaveButton, 0, wxALL, 4);
    
    m_globalResetButton = new wxButton(this, wxID_RESET, wxT("Reset"));
    buttonSizer->Add(m_globalResetButton, 0, wxALL, 4);
    
    // Removed Undo/Redo buttons
    
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 4);
    
    SetSizer(mainSizer);
    
    // Apply fonts to all panels
    applySpecificFonts();
    if (m_lightingPanel) m_lightingPanel->applyFonts();
    if (m_antiAliasingPanel) m_antiAliasingPanel->applyFonts();
    if (m_renderingModePanel) m_renderingModePanel->applyFonts();
    if (m_backgroundStylePanel) m_backgroundStylePanel->applyFonts();
}

void GlobalSettingsPanel::bindEvents()
{
    m_globalAutoApplyCheckBox->Bind(wxEVT_CHECKBOX, &GlobalSettingsPanel::OnGlobalAutoApply, this);
    m_globalApplyButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::OnGlobalApply, this);
    m_mainApplyButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::OnMainApply, this);
    m_globalSaveButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::OnGlobalSave, this);
    m_globalResetButton->Bind(wxEVT_BUTTON, &GlobalSettingsPanel::OnGlobalReset, this);
}

void GlobalSettingsPanel::applySpecificFonts()
{
    FontManager& fontManager = FontManager::getInstance();
    
    // Apply fonts to buttons
    if (m_globalApplyButton) m_globalApplyButton->SetFont(fontManager.getButtonFont());
    if (m_globalSaveButton) m_globalSaveButton->SetFont(fontManager.getButtonFont());
    if (m_globalResetButton) m_globalResetButton->SetFont(fontManager.getButtonFont());
    if (m_mainApplyButton) m_mainApplyButton->SetFont(fontManager.getButtonFont());
    
    // Apply fonts to checkbox
    if (m_globalAutoApplyCheckBox) m_globalAutoApplyCheckBox->SetFont(fontManager.getLabelFont());
    
    // Apply fonts to notebook tabs
    if (m_notebook) {
        m_notebook->SetFont(fontManager.getDefaultFont());
    }
    
    // Apply fonts to child panels
    if (m_lightingPanel) {
        fontManager.applyFontToWindowAndChildren(m_lightingPanel, "Default");
    }
    if (m_antiAliasingPanel) {
        fontManager.applyFontToWindowAndChildren(m_antiAliasingPanel, "Default");
    }
    if (m_renderingModePanel) {
        fontManager.applyFontToWindowAndChildren(m_renderingModePanel, "Default");
    }
    if (m_backgroundStylePanel) {
        fontManager.applyFontToWindowAndChildren(m_backgroundStylePanel, "Default");
    }
}

std::vector<RenderLightSettings> GlobalSettingsPanel::getLights() const
{
    return m_lightingPanel->getLights();
}

void GlobalSettingsPanel::setLights(const std::vector<RenderLightSettings>& lights)
{
    m_lightingPanel->setLights(lights);
}

int GlobalSettingsPanel::getAntiAliasingMethod() const
{
    return m_antiAliasingPanel->getAntiAliasingMethod();
}

int GlobalSettingsPanel::getMSAASamples() const
{
    return m_antiAliasingPanel->getMSAASamples();
}

bool GlobalSettingsPanel::isFXAAEnabled() const
{
    return m_antiAliasingPanel->isFXAAEnabled();
}

void GlobalSettingsPanel::setAntiAliasingMethod(int method)
{
    m_antiAliasingPanel->setAntiAliasingMethod(method);
}

void GlobalSettingsPanel::setMSAASamples(int samples)
{
    m_antiAliasingPanel->setMSAASamples(samples);
}

void GlobalSettingsPanel::setFXAAEnabled(bool enabled)
{
    m_antiAliasingPanel->setFXAAEnabled(enabled);
}

int GlobalSettingsPanel::getRenderingMode() const
{
    return m_renderingModePanel->getRenderingMode();
}

void GlobalSettingsPanel::setRenderingMode(int mode)
{
    m_renderingModePanel->setRenderingMode(mode);
}

int GlobalSettingsPanel::getBackgroundStyle() const
{
    return m_backgroundStylePanel->getBackgroundStyle();
}

wxColour GlobalSettingsPanel::getBackgroundColor() const
{
    return m_backgroundStylePanel->getBackgroundColor();
}

wxColour GlobalSettingsPanel::getGradientTopColor() const
{
    return m_backgroundStylePanel->getGradientTopColor();
}

wxColour GlobalSettingsPanel::getGradientBottomColor() const
{
    return m_backgroundStylePanel->getGradientBottomColor();
}

std::string GlobalSettingsPanel::getBackgroundImagePath() const
{
    return m_backgroundStylePanel->getBackgroundImagePath();
}

bool GlobalSettingsPanel::isBackgroundImageEnabled() const
{
    return m_backgroundStylePanel->isBackgroundImageEnabled();
}

float GlobalSettingsPanel::getBackgroundImageOpacity() const
{
    return m_backgroundStylePanel->getBackgroundImageOpacity();
}

int GlobalSettingsPanel::getBackgroundImageFit() const
{
    return m_backgroundStylePanel->getBackgroundImageFit();
}

bool GlobalSettingsPanel::isBackgroundImageMaintainAspect() const
{
    return m_backgroundStylePanel->isBackgroundImageMaintainAspect();
}

void GlobalSettingsPanel::setAntiAliasingManager(AntiAliasingManager* manager)
{
    m_antiAliasingManager = manager;
    m_antiAliasingPanel->setAntiAliasingManager(manager);
}

void GlobalSettingsPanel::setRenderingManager(RenderingManager* manager)
{
    m_renderingManager = manager;
    m_renderingModePanel->setRenderingManager(manager);
}

void GlobalSettingsPanel::setBackgroundManager(BackgroundManager* manager)
{
    m_backgroundManager = manager;
    m_backgroundStylePanel->setBackgroundManager(manager);
}

void GlobalSettingsPanel::setAutoApply(bool enabled)
{
    m_autoApply = enabled;
    m_globalAutoApplyCheckBox->SetValue(enabled);
}

void GlobalSettingsPanel::markAsChanged()
{
    m_hasUnsavedChanges = true;
    if (m_autoApply) {
        applySettingsToCanvas();
    }
}

void GlobalSettingsPanel::markAsSaved()
{
    m_hasUnsavedChanges = false;
}

void GlobalSettingsPanel::applySettingsToCanvas()
{
    if (m_parentDialog && m_parentDialog->getRenderCanvas()) {
        auto canvas = m_parentDialog->getRenderCanvas();
        canvas->updateMultiLighting(getLights());
        canvas->updateAntiAliasing(getAntiAliasingMethod(), getMSAASamples(), isFXAAEnabled());
        canvas->render(true);
        canvas->Refresh();
        canvas->Update();
        LOG_INF_S("GlobalSettingsPanel::applySettingsToCanvas: Settings automatically applied");
    }
}

void GlobalSettingsPanel::loadSettings()
{
    m_lightingPanel->loadSettings();
    m_antiAliasingPanel->loadSettings();
    m_renderingModePanel->loadSettings();
}

void GlobalSettingsPanel::saveSettings()
{
    m_lightingPanel->saveSettings();
    m_antiAliasingPanel->saveSettings();
    m_renderingModePanel->saveSettings();
}

void GlobalSettingsPanel::resetToDefaults()
{
    m_lightingPanel->resetToDefaults();
    m_antiAliasingPanel->resetToDefaults();
    m_renderingModePanel->resetToDefaults();
}

void GlobalSettingsPanel::OnGlobalApply(wxCommandEvent& event)
{
    applySettingsToCanvas();
    wxMessageBox(wxT("Global settings applied to preview successfully!"), wxT("Apply Global"), wxOK | wxICON_INFORMATION);
    LOG_INF_S("GlobalSettingsPanel::OnGlobalApply: Global settings applied");
}

void GlobalSettingsPanel::OnGlobalSave(wxCommandEvent& event)
{
    saveSettings();
    markAsSaved();
    wxMessageBox(wxT("Global settings saved successfully!"), wxT("Save Global"), wxOK | wxICON_INFORMATION);
    LOG_INF_S("GlobalSettingsPanel::OnGlobalSave: Global settings saved");
}

void GlobalSettingsPanel::OnGlobalReset(wxCommandEvent& event)
{
    resetToDefaults();
    wxMessageBox(wxT("Global settings reset to defaults!"), wxT("Reset Global"), wxOK | wxICON_INFORMATION);
    LOG_INF_S("GlobalSettingsPanel::OnGlobalReset: Global settings reset");
}

void GlobalSettingsPanel::OnMainApply(wxCommandEvent& event)
{
    // Apply preview-approved lighting to main viewport via LightingConfig
    try {
        LightingConfig& lightingConfig = LightingConfig::getInstance();

        // Clear existing lights in config
        auto& existingLights = lightingConfig.getAllLights();
        for (int i = static_cast<int>(existingLights.size()) - 1; i >= 0; --i) {
            lightingConfig.removeLight(i);
        }

        // Convert preview RenderLightSettings to main LightSettings
        std::vector<RenderLightSettings> previewLights = getLights();
        for (const auto& rl : previewLights) {
            LightSettings ls;
            ls.enabled = rl.enabled;
            ls.name = rl.name;
            ls.type = rl.type; // expects "directional" | "point" | "spot"
            ls.positionX = rl.positionX;
            ls.positionY = rl.positionY;
            ls.positionZ = rl.positionZ;
            ls.directionX = rl.directionX;
            ls.directionY = rl.directionY;
            ls.directionZ = rl.directionZ;
            ls.color = Quantity_Color(rl.color.Red() / 255.0, rl.color.Green() / 255.0, rl.color.Blue() / 255.0, Quantity_TOC_RGB);
            ls.intensity = rl.intensity;
            ls.spotAngle = rl.spotAngle;
            ls.spotExponent = rl.spotExponent;
            ls.constantAttenuation = rl.constantAttenuation;
            ls.linearAttenuation = rl.linearAttenuation;
            ls.quadraticAttenuation = rl.quadraticAttenuation;

            lightingConfig.addLight(ls);
        }

        // Persist and notify main scene to rebuild lighting
        lightingConfig.saveToFile();
        lightingConfig.applySettingsToScene();

        wxMessageBox(wxT("Applied preview lighting to main viewport"), wxT("Main Apply"), wxOK | wxICON_INFORMATION);
        LOG_INF_S("GlobalSettingsPanel::OnMainApply: Applied preview lighting to main viewport");
    } catch (const std::exception& e) {
        LOG_ERR_S(std::string("GlobalSettingsPanel::OnMainApply: Exception: ") + e.what());
        wxMessageBox(wxT("Failed to apply settings to main viewport"), wxT("Main Apply"), wxOK | wxICON_ERROR);
    }
}

void GlobalSettingsPanel::OnGlobalAutoApply(wxCommandEvent& event)
{
    m_autoApply = event.IsChecked();
    LOG_INF_S("GlobalSettingsPanel::OnGlobalAutoApply: Global auto apply " + std::string(m_autoApply ? "enabled" : "disabled"));
    
    // If auto apply is enabled and there are unsaved changes, apply them immediately
    if (m_autoApply && m_hasUnsavedChanges) {
        applySettingsToCanvas();
    }
}

void GlobalSettingsPanel::validatePresets()
{
    LOG_INF_S("GlobalSettingsPanel::validatePresets: Validating preset configurations");
    if (m_antiAliasingManager) {
        auto presets = m_antiAliasingManager->getAvailablePresets();
        LOG_INF_S("GlobalSettingsPanel::validatePresets: Found " + std::to_string(presets.size()) + " anti-aliasing presets");
        for (const auto& preset : presets) {
            LOG_INF_S("GlobalSettingsPanel::validatePresets: Anti-aliasing preset: " + preset);
        }
    }
    if (m_renderingManager) {
        auto presets = m_renderingManager->getAvailablePresets();
        LOG_INF_S("GlobalSettingsPanel::validatePresets: Found " + std::to_string(presets.size()) + " rendering presets");
        for (const auto& preset : presets) {
            LOG_INF_S("GlobalSettingsPanel::validatePresets: Rendering preset: " + preset);
        }
    }
    LOG_INF_S("GlobalSettingsPanel::validatePresets: Preset validation completed");
}

void GlobalSettingsPanel::testPresetFunctionality()
{
    LOG_INF_S("GlobalSettingsPanel::testPresetFunctionality: Testing preset functionality");
    if (m_antiAliasingManager) {
        std::vector<std::string> testPresets = {"MSAA 4x", "FXAA Medium", "CAD Standard", "Gaming Fast"};
        for (const auto& preset : testPresets) {
            LOG_INF_S("GlobalSettingsPanel::testPresetFunctionality: Testing anti-aliasing preset: " + preset);
            m_antiAliasingManager->applyPreset(preset);
            auto activeConfig = m_antiAliasingManager->getActiveConfiguration();
            LOG_INF_S("GlobalSettingsPanel::testPresetFunctionality: Applied preset name: " + activeConfig.name);
        }
    }
    if (m_renderingManager) {
        std::vector<std::string> testPresets = {"Balanced", "CAD Standard", "Gaming Fast", "Presentation Standard"};
        for (const auto& preset : testPresets) {
            LOG_INF_S("GlobalSettingsPanel::testPresetFunctionality: Testing rendering preset: " + preset);
            m_renderingManager->applyPreset(preset);
            auto activeConfig = m_renderingManager->getActiveConfiguration();
            LOG_INF_S("GlobalSettingsPanel::testPresetFunctionality: Applied preset name: " + activeConfig.name);
        }
    }
    LOG_INF_S("GlobalSettingsPanel::testPresetFunctionality: Preset functionality test completed");
}
