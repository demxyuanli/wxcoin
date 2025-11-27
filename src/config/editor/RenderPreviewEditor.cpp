#include "config/editor/RenderPreviewEditor.h"
#include "renderpreview/PreviewCanvas.h"
#include "renderpreview/GlobalSettingsPanel.h"
#include "renderpreview/ObjectSettingsPanel.h"
#include "config/ConfigManager.h"
#include "logger/Logger.h"
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/notebook.h>

RenderPreviewEditor::RenderPreviewEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId)
    : ConfigCategoryEditor(parent, configManager, categoryId)
    , m_renderCanvas(nullptr)
    , m_notebook(nullptr)
    , m_globalSettingsPanel(nullptr)
    , m_objectSettingsPanel(nullptr)
    , m_currentLightIndex(-1)
    , m_undoManager(std::make_unique<UndoManager>())
    , m_validationEnabled(true)
{
    createUI();
}

RenderPreviewEditor::~RenderPreviewEditor() {
}

void RenderPreviewEditor::createUI() {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Top section: Configuration and Preview
    wxBoxSizer* topSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left panel: Configuration tabs (fixed 450px width)
    wxPanel* leftPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(400, -1), wxBORDER_SUNKEN);
    m_notebook = new wxNotebook(leftPanel, wxID_ANY);
    
    // Create panel instances
    m_globalSettingsPanel = new GlobalSettingsPanel(m_notebook, nullptr);
    m_objectSettingsPanel = new ObjectSettingsPanel(m_notebook);
    
    // Add panels to notebook
    m_notebook->AddPage(m_globalSettingsPanel, "Global Settings");
    m_notebook->AddPage(m_objectSettingsPanel, "Object Settings");
    
    wxBoxSizer* leftSizer = new wxBoxSizer(wxVERTICAL);
    leftSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 4);
    leftPanel->SetSizer(leftSizer);
    
    // Right panel: Render preview canvas (adaptive width)
    m_renderCanvas = new PreviewCanvas(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
    
    // Set up manager references after canvas is created
    if (m_renderCanvas) {
        m_globalSettingsPanel->setAntiAliasingManager(m_renderCanvas->getAntiAliasingManager());
        m_globalSettingsPanel->setRenderingManager(m_renderCanvas->getRenderingManager());
        m_globalSettingsPanel->setBackgroundManager(m_renderCanvas->getBackgroundManager());
        
        // Set ObjectManager reference
        if (m_objectSettingsPanel) {
            ObjectManager* objectManager = m_renderCanvas->getObjectManager();
            if (objectManager) {
                m_objectSettingsPanel->setObjectManager(objectManager);
                m_objectSettingsPanel->setPreviewCanvas(m_renderCanvas);
            }
        }
    }
    
    topSizer->Add(leftPanel, 0, wxEXPAND | wxALL, 2);
    topSizer->Add(m_renderCanvas, 1, wxEXPAND | wxALL, 2);
    
    mainSizer->Add(topSizer, 1, wxEXPAND | wxALL, 2);
    
    SetSizer(mainSizer);
    FitInside();
    
    // Load configuration
    loadConfiguration();
}

void RenderPreviewEditor::loadConfig() {
    // UI is already created in createUI()
    // Just refresh the values
    loadConfiguration();
}

void RenderPreviewEditor::saveConfig() {
    saveConfiguration();
    if (m_changeCallback) {
        m_changeCallback();
    }
}

void RenderPreviewEditor::resetConfig() {
    resetToDefaults();
    if (m_changeCallback) {
        m_changeCallback();
    }
}

void RenderPreviewEditor::saveConfiguration() {
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
    } catch (const std::exception& e) {
        LOG_ERR("RenderPreviewEditor: Failed to save configuration: " + std::string(e.what()), "RenderPreviewEditor");
    }
}

void RenderPreviewEditor::loadConfiguration() {
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
        
        
        // Apply loaded configuration to the preview canvas immediately
        applyLoadedConfigurationToCanvas();
    } catch (const std::exception& e) {
        LOG_ERR("RenderPreviewEditor: Failed to load configuration: " + std::string(e.what()), "RenderPreviewEditor");
    }
}

void RenderPreviewEditor::resetToDefaults() {
    // Reset global settings
    if (m_globalSettingsPanel) {
        m_globalSettingsPanel->resetToDefaults();
    }
    
    // Reset object settings
    if (m_objectSettingsPanel) {
        m_objectSettingsPanel->resetToDefaults();
    }
    
}

void RenderPreviewEditor::applyGlobalSettingsToCanvas() {
    if (!m_globalSettingsPanel || !m_renderCanvas) return;
    
    // Validate settings if validation is enabled
    if (m_validationEnabled) {
        // Validation would go here if needed
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
        }
    }
    
    // Update background configuration from BackgroundManager
    if (m_renderCanvas) {
        m_renderCanvas->updateBackgroundFromConfig();
        m_renderCanvas->render(true);
        m_renderCanvas->Refresh();
        m_renderCanvas->Update();
    }
}

void RenderPreviewEditor::applyObjectSettingsToCanvas() {
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

void RenderPreviewEditor::applyLoadedConfigurationToCanvas() {
    
    if (!m_renderCanvas) {
        LOG_ERR("RenderPreviewEditor: No render canvas available", "RenderPreviewEditor");
        return;
    }
    
    // Apply global settings to canvas
    applyGlobalSettingsToCanvas();
    
    // Apply object settings to canvas
    applyObjectSettingsToCanvas();
    
}

