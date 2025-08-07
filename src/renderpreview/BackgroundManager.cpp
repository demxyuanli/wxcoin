#include "renderpreview/BackgroundManager.h"
#include "renderpreview/PreviewCanvas.h"
#include "renderpreview/RenderingManager.h"
#include "config/ConfigManager.h"
#include "logger/Logger.h"
#include <wx/filename.h>
#include <wx/image.h>
#include <GL/gl.h>
#include <GL/glext.h>

BackgroundManager::BackgroundManager(PreviewCanvas* canvas)
    : m_canvas(canvas)
    , m_nextConfigId(1)
    , m_activeConfigId(-1)
{
    initializePresets();
    
    // Create default configuration
    BackgroundSettings defaultSettings;
    defaultSettings.style = 0; // Solid color
    defaultSettings.backgroundColor = wxColour(173, 204, 255); // Light blue
    defaultSettings.gradientTopColor = wxColour(200, 220, 255);
    defaultSettings.gradientBottomColor = wxColour(150, 180, 255);
    defaultSettings.name = "Default Background";
    
    int configId = addConfiguration(defaultSettings);
    if (configId != -1) {
        setActiveConfiguration(configId);
        
        // Apply the default configuration to RenderingManager
        if (m_canvas && m_canvas->getRenderingManager()) {
            RenderingManager* renderingManager = m_canvas->getRenderingManager();
            if (renderingManager->hasActiveConfiguration()) {
                int activeConfigId = renderingManager->getActiveConfigurationId();
                RenderingSettings renderingSettings = renderingManager->getConfiguration(activeConfigId);
                
                // Update background settings
                renderingSettings.backgroundStyle = defaultSettings.style;
                renderingSettings.backgroundColor = defaultSettings.backgroundColor;
                renderingSettings.gradientTopColor = defaultSettings.gradientTopColor;
                renderingSettings.gradientBottomColor = defaultSettings.gradientBottomColor;
                
                renderingManager->updateConfiguration(activeConfigId, renderingSettings);
            } else {
                // Create a new configuration if none exists
                RenderingSettings newSettings;
                newSettings.backgroundStyle = defaultSettings.style;
                newSettings.backgroundColor = defaultSettings.backgroundColor;
                newSettings.gradientTopColor = defaultSettings.gradientTopColor;
                newSettings.gradientBottomColor = defaultSettings.gradientBottomColor;
                
                int newConfigId = renderingManager->addConfiguration(newSettings);
                if (newConfigId != -1) {
                    renderingManager->setActiveConfiguration(newConfigId);
                }
            }
        }
        
        LOG_INF_S("BackgroundManager: Initialized with default configuration " + std::to_string(configId));
    } else {
        LOG_ERR_S("BackgroundManager: Failed to create default configuration");
    }
    
    LOG_INF_S("BackgroundManager: Initialized with canvas " + std::to_string(reinterpret_cast<uintptr_t>(canvas)));
}

BackgroundManager::~BackgroundManager()
{
    clearAllConfigurations();
    LOG_INF_S("BackgroundManager: Destroyed");
}

int BackgroundManager::addConfiguration(const BackgroundSettings& settings)
{
    auto managedBackground = std::make_unique<ManagedBackground>();
    managedBackground->settings = settings;
    managedBackground->configId = m_nextConfigId;
    managedBackground->isActive = false;
    
    m_configurations[m_nextConfigId] = std::move(managedBackground);
    int configId = m_nextConfigId;
    m_nextConfigId++;
    
    LOG_INF_S("BackgroundManager::addConfiguration: Added configuration " + std::to_string(configId) + " '" + settings.name + "'");
    return configId;
}

bool BackgroundManager::removeConfiguration(int configId)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        if (it->second->isActive) {
            m_activeConfigId = -1;
        }
        m_configurations.erase(it);
        LOG_INF_S("BackgroundManager::removeConfiguration: Removed configuration " + std::to_string(configId));
        return true;
    }
    LOG_WRN_S("BackgroundManager::removeConfiguration: Configuration " + std::to_string(configId) + " not found");
    return false;
}

bool BackgroundManager::updateConfiguration(int configId, const BackgroundSettings& settings)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings = settings;
        LOG_INF_S("BackgroundManager::updateConfiguration: Updated configuration " + std::to_string(configId));
        return true;
    }
    LOG_WRN_S("BackgroundManager::updateConfiguration: Configuration " + std::to_string(configId) + " not found");
    return false;
}

void BackgroundManager::clearAllConfigurations()
{
    m_configurations.clear();
    m_activeConfigId = -1;
    m_nextConfigId = 1;
    LOG_INF_S("BackgroundManager::clearAllConfigurations: Cleared all configurations");
}

std::vector<int> BackgroundManager::getAllConfigurationIds() const
{
    std::vector<int> ids;
    ids.reserve(m_configurations.size());
    for (const auto& pair : m_configurations) {
        ids.push_back(pair.first);
    }
    return ids;
}

std::vector<BackgroundSettings> BackgroundManager::getAllConfigurations() const
{
    std::vector<BackgroundSettings> configs;
    configs.reserve(m_configurations.size());
    for (const auto& pair : m_configurations) {
        configs.push_back(pair.second->settings);
    }
    return configs;
}

BackgroundSettings BackgroundManager::getConfiguration(int configId) const
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        return it->second->settings;
    }
    return BackgroundSettings();
}

bool BackgroundManager::hasConfiguration(int configId) const
{
    return m_configurations.find(configId) != m_configurations.end();
}

int BackgroundManager::getConfigurationCount() const
{
    return static_cast<int>(m_configurations.size());
}

bool BackgroundManager::setActiveConfiguration(int configId)
{
    if (configId == -1) {
        m_activeConfigId = -1;
        LOG_INF_S("BackgroundManager::setActiveConfiguration: Cleared active configuration");
        return true;
    }
    
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        // Deactivate current active configuration
        if (m_activeConfigId != -1) {
            auto activeIt = m_configurations.find(m_activeConfigId);
            if (activeIt != m_configurations.end()) {
                activeIt->second->isActive = false;
            }
        }
        
        // Activate new configuration
        it->second->isActive = true;
        m_activeConfigId = configId;
        
        LOG_INF_S("BackgroundManager::setActiveConfiguration: Set active configuration " + std::to_string(configId));
        return true;
    }
    
    LOG_WRN_S("BackgroundManager::setActiveConfiguration: Configuration " + std::to_string(configId) + " not found");
    return false;
}

int BackgroundManager::getActiveConfigurationId() const
{
    return m_activeConfigId;
}

BackgroundSettings BackgroundManager::getActiveConfiguration() const
{
    if (m_activeConfigId != -1) {
        auto it = m_configurations.find(m_activeConfigId);
        if (it != m_configurations.end()) {
            return it->second->settings;
        }
    }
    return BackgroundSettings();
}

bool BackgroundManager::hasActiveConfiguration() const
{
    return m_activeConfigId != -1 && m_configurations.find(m_activeConfigId) != m_configurations.end();
}

void BackgroundManager::setStyle(int configId, int style)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.style = style;
        LOG_INF_S("BackgroundManager::setStyle: Set style " + std::to_string(style) + " for configuration " + std::to_string(configId));
    }
}

void BackgroundManager::setBackgroundColor(int configId, const wxColour& color)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.backgroundColor = color;
        LOG_INF_S("BackgroundManager::setBackgroundColor: Set background color for configuration " + std::to_string(configId));
    }
}

void BackgroundManager::setGradientTopColor(int configId, const wxColour& color)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.gradientTopColor = color;
        LOG_INF_S("BackgroundManager::setGradientTopColor: Set gradient top color for configuration " + std::to_string(configId));
    }
}

void BackgroundManager::setGradientBottomColor(int configId, const wxColour& color)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.gradientBottomColor = color;
        LOG_INF_S("BackgroundManager::setGradientBottomColor: Set gradient bottom color for configuration " + std::to_string(configId));
    }
}

void BackgroundManager::setImagePath(int configId, const std::string& path)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.imagePath = path;
        it->second->settings.imageEnabled = !path.empty();
        LOG_INF_S("BackgroundManager::setImagePath: Set image path for configuration " + std::to_string(configId));
    }
}

void BackgroundManager::setImageEnabled(int configId, bool enabled)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.imageEnabled = enabled;
        LOG_INF_S("BackgroundManager::setImageEnabled: Set image enabled " + std::to_string(enabled) + " for configuration " + std::to_string(configId));
    }
}

void BackgroundManager::setImageOpacity(int configId, float opacity)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.imageOpacity = opacity;
        LOG_INF_S("BackgroundManager::setImageOpacity: Set image opacity " + std::to_string(opacity) + " for configuration " + std::to_string(configId));
    }
}

void BackgroundManager::setImageFit(int configId, int fit)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.imageFit = fit;
        LOG_INF_S("BackgroundManager::setImageFit: Set image fit " + std::to_string(fit) + " for configuration " + std::to_string(configId));
    }
}

void BackgroundManager::setImageMaintainAspect(int configId, bool maintain)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.imageMaintainAspect = maintain;
        LOG_INF_S("BackgroundManager::setImageMaintainAspect: Set image maintain aspect " + std::to_string(maintain) + " for configuration " + std::to_string(configId));
    }
}

void BackgroundManager::applyPreset(const std::string& presetName)
{
    auto it = m_presets.find(presetName);
    if (it != m_presets.end()) {
        // Create a new configuration with the preset settings
        BackgroundSettings presetSettings = it->second;
        presetSettings.name = presetName;
        
        // Add the preset as a new configuration
        int configId = addConfiguration(presetSettings);
        if (configId != -1) {
            // Set it as active
            setActiveConfiguration(configId);
            LOG_INF_S("BackgroundManager::applyPreset: Applied preset '" + presetName + "' with config ID " + std::to_string(configId));
        } else {
            LOG_ERR_S("BackgroundManager::applyPreset: Failed to create configuration for preset '" + presetName + "'");
        }
    } else {
        // Handle built-in presets
        BackgroundSettings settings;
        if (presetName == "Solid") {
            settings.style = 0;
            settings.backgroundColor = wxColour(173, 204, 255);
        } else if (presetName == "Gradient") {
            settings.style = 1;
            settings.gradientTopColor = wxColour(200, 220, 255);
            settings.gradientBottomColor = wxColour(150, 180, 255);
        } else if (presetName == "Environment") {
            settings.style = 3;
            settings.backgroundColor = wxColour(135, 206, 235);
        } else if (presetName == "Studio") {
            settings.style = 4;
            settings.backgroundColor = wxColour(240, 248, 255);
        } else if (presetName == "Outdoor") {
            settings.style = 5;
            settings.backgroundColor = wxColour(255, 255, 224);
        } else if (presetName == "Industrial") {
            settings.style = 6;
            settings.backgroundColor = wxColour(245, 245, 245);
        } else if (presetName == "CAD") {
            settings.style = 7;
            settings.backgroundColor = wxColour(255, 248, 220);
        } else if (presetName == "Dark") {
            settings.style = 8;
            settings.backgroundColor = wxColour(40, 40, 40);
        } else {
            LOG_WRN_S("BackgroundManager::applyPreset: Unknown preset '" + presetName + "'");
            return;
        }
        
        settings.name = presetName;
        int configId = addConfiguration(settings);
        if (configId != -1) {
            setActiveConfiguration(configId);
            LOG_INF_S("BackgroundManager::applyPreset: Applied built-in preset '" + presetName + "' with config ID " + std::to_string(configId));
        } else {
            LOG_ERR_S("BackgroundManager::applyPreset: Failed to create configuration for built-in preset '" + presetName + "'");
        }
    }
}

void BackgroundManager::saveAsPreset(int configId, const std::string& presetName)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        m_presets[presetName] = it->second->settings;
        LOG_INF_S("BackgroundManager::saveAsPreset: Saved configuration " + std::to_string(configId) + " as preset '" + presetName + "'");
    }
}

std::vector<std::string> BackgroundManager::getAvailablePresets() const
{
    std::vector<std::string> presets;
    presets.reserve(m_presets.size());
    for (const auto& pair : m_presets) {
        presets.push_back(pair.first);
    }
    return presets;
}

void BackgroundManager::applyToPreviewViewport()
{
    if (hasActiveConfiguration() && m_canvas) {
        BackgroundSettings activeSettings = getActiveConfiguration();
        
        // Update the RenderingManager with our background settings
        if (m_canvas->getRenderingManager()) {
            RenderingManager* renderingManager = m_canvas->getRenderingManager();
            if (renderingManager->hasActiveConfiguration()) {
                int activeConfigId = renderingManager->getActiveConfigurationId();
                RenderingSettings renderingSettings = renderingManager->getConfiguration(activeConfigId);
                
                // Update background settings
                renderingSettings.backgroundStyle = activeSettings.style;
                renderingSettings.backgroundColor = activeSettings.backgroundColor;
                renderingSettings.gradientTopColor = activeSettings.gradientTopColor;
                renderingSettings.gradientBottomColor = activeSettings.gradientBottomColor;
                renderingSettings.backgroundImagePath = activeSettings.imagePath;
                renderingSettings.backgroundImageEnabled = activeSettings.imageEnabled;
                renderingSettings.backgroundImageOpacity = activeSettings.imageOpacity;
                renderingSettings.backgroundImageFit = activeSettings.imageFit;
                renderingSettings.backgroundImageMaintainAspect = activeSettings.imageMaintainAspect;
                
                // Update the rendering manager configuration
                renderingManager->updateConfiguration(activeConfigId, renderingSettings);
                
                LOG_INF_S("BackgroundManager::applyToPreviewViewport: Updated RenderingManager with background settings");
            } else {
                // Create a new configuration if none exists
                RenderingSettings newSettings;
                newSettings.backgroundStyle = activeSettings.style;
                newSettings.backgroundColor = activeSettings.backgroundColor;
                newSettings.gradientTopColor = activeSettings.gradientTopColor;
                newSettings.gradientBottomColor = activeSettings.gradientBottomColor;
                newSettings.backgroundImagePath = activeSettings.imagePath;
                newSettings.backgroundImageEnabled = activeSettings.imageEnabled;
                newSettings.backgroundImageOpacity = activeSettings.imageOpacity;
                newSettings.backgroundImageFit = activeSettings.imageFit;
                newSettings.backgroundImageMaintainAspect = activeSettings.imageMaintainAspect;
                
                int newConfigId = renderingManager->addConfiguration(newSettings);
                if (newConfigId != -1) {
                    renderingManager->setActiveConfiguration(newConfigId);
                    LOG_INF_S("BackgroundManager::applyToPreviewViewport: Created new RenderingManager configuration " + std::to_string(newConfigId));
                }
            }
        }
        
        // Force a render to apply the background
        m_canvas->render(false);
        
        // Force canvas refresh
        m_canvas->Refresh();
        m_canvas->Update();
        
        LOG_INF_S("BackgroundManager::applyToPreviewViewport: Applied active configuration to preview viewport");
    } else {
        LOG_WRN_S("BackgroundManager::applyToPreviewViewport: No active configuration or canvas");
    }
}

void BackgroundManager::updatePreviewViewport()
{
    applyToPreviewViewport();
}

void BackgroundManager::renderBackground()
{
    if (!hasActiveConfiguration() || !m_canvas) {
        LOG_WRN_S("BackgroundManager::renderBackground: No active configuration or canvas");
        return;
    }
    
    BackgroundSettings settings = getActiveConfiguration();
    
    // Update the RenderingManager with our background settings
    if (m_canvas->getRenderingManager()) {
        RenderingManager* renderingManager = m_canvas->getRenderingManager();
        if (renderingManager->hasActiveConfiguration()) {
            int activeConfigId = renderingManager->getActiveConfigurationId();
            RenderingSettings renderingSettings = renderingManager->getConfiguration(activeConfigId);
            
            // Update background settings
            renderingSettings.backgroundStyle = settings.style;
            renderingSettings.backgroundColor = settings.backgroundColor;
            renderingSettings.gradientTopColor = settings.gradientTopColor;
            renderingSettings.gradientBottomColor = settings.gradientBottomColor;
            renderingSettings.backgroundImagePath = settings.imagePath;
            renderingSettings.backgroundImageEnabled = settings.imageEnabled;
            renderingSettings.backgroundImageOpacity = settings.imageOpacity;
            renderingSettings.backgroundImageFit = settings.imageFit;
            renderingSettings.backgroundImageMaintainAspect = settings.imageMaintainAspect;
            
            // Update the rendering manager configuration
            renderingManager->updateConfiguration(activeConfigId, renderingSettings);
            
            LOG_INF_S("BackgroundManager::renderBackground: Updated RenderingManager with background settings");
        } else {
            // Create a new configuration if none exists
            RenderingSettings newSettings;
            newSettings.backgroundStyle = settings.style;
            newSettings.backgroundColor = settings.backgroundColor;
            newSettings.gradientTopColor = settings.gradientTopColor;
            newSettings.gradientBottomColor = settings.gradientBottomColor;
            newSettings.backgroundImagePath = settings.imagePath;
            newSettings.backgroundImageEnabled = settings.imageEnabled;
            newSettings.backgroundImageOpacity = settings.imageOpacity;
            newSettings.backgroundImageFit = settings.imageFit;
            newSettings.backgroundImageMaintainAspect = settings.imageMaintainAspect;
            
            int newConfigId = renderingManager->addConfiguration(newSettings);
            if (newConfigId != -1) {
                renderingManager->setActiveConfiguration(newConfigId);
                LOG_INF_S("BackgroundManager::renderBackground: Created new RenderingManager configuration " + std::to_string(newConfigId));
            }
        }
    }
    
    // Force a render to apply the background
    if (m_canvas) {
        m_canvas->render(false);
        m_canvas->Refresh();
        m_canvas->Update();
        LOG_INF_S("BackgroundManager::renderBackground: Forced canvas render and refresh");
    }
}

void BackgroundManager::renderSolidBackground(const wxColour& color)
{
    // This method is no longer needed as we're using the PreviewCanvas's rendering system
    // The background color is handled by the RenderingManager
}

void BackgroundManager::renderGradientBackground(const wxColour& topColor, const wxColour& bottomColor)
{
    // This method is no longer needed as we're using the PreviewCanvas's rendering system
    // The gradient background is handled by the PreviewCanvas's renderGradientBackground method
}

void BackgroundManager::renderImageBackground(const std::string& imagePath, float opacity, int fit, bool maintainAspect)
{
    // This method is no longer needed as we're using the PreviewCanvas's rendering system
    // Image backgrounds will be implemented separately in the PreviewCanvas
}

void BackgroundManager::renderEnvironmentBackground()
{
    renderSolidBackground(wxColour(135, 206, 235)); // Sky blue
}

void BackgroundManager::renderStudioBackground()
{
    renderSolidBackground(wxColour(240, 248, 255)); // Light blue
}

void BackgroundManager::renderOutdoorBackground()
{
    renderSolidBackground(wxColour(255, 255, 224)); // Light yellow
}

void BackgroundManager::renderIndustrialBackground()
{
    renderSolidBackground(wxColour(245, 245, 245)); // Light gray
}

void BackgroundManager::renderCADBackground()
{
    renderSolidBackground(wxColour(255, 248, 220)); // Light cream
}

void BackgroundManager::renderDarkBackground()
{
    renderSolidBackground(wxColour(40, 40, 40)); // Dark gray
}

void BackgroundManager::loadFromRenderingSettings(const RenderingSettings& settings)
{
    BackgroundSettings bgSettings;
    bgSettings.style = settings.backgroundStyle;
    bgSettings.backgroundColor = settings.backgroundColor;
    bgSettings.gradientTopColor = settings.gradientTopColor;
    bgSettings.gradientBottomColor = settings.gradientBottomColor;
    bgSettings.imagePath = settings.backgroundImagePath;
    bgSettings.imageEnabled = settings.backgroundImageEnabled;
    bgSettings.imageOpacity = settings.backgroundImageOpacity;
    bgSettings.imageFit = settings.backgroundImageFit;
    bgSettings.imageMaintainAspect = settings.backgroundImageMaintainAspect;
    
    int configId = addConfiguration(bgSettings);
    if (configId != -1) {
        setActiveConfiguration(configId);
        LOG_INF_S("BackgroundManager::loadFromRenderingSettings: Loaded settings into configuration " + std::to_string(configId));
    }
}

void BackgroundManager::saveToRenderingSettings(RenderingSettings& settings) const
{
    if (hasActiveConfiguration()) {
        BackgroundSettings bgSettings = getActiveConfiguration();
        settings.backgroundStyle = bgSettings.style;
        settings.backgroundColor = bgSettings.backgroundColor;
        settings.gradientTopColor = bgSettings.gradientTopColor;
        settings.gradientBottomColor = bgSettings.gradientBottomColor;
        settings.backgroundImagePath = bgSettings.imagePath;
        settings.backgroundImageEnabled = bgSettings.imageEnabled;
        settings.backgroundImageOpacity = bgSettings.imageOpacity;
        settings.backgroundImageFit = bgSettings.imageFit;
        settings.backgroundImageMaintainAspect = bgSettings.imageMaintainAspect;
        
        LOG_INF_S("BackgroundManager::saveToRenderingSettings: Saved active configuration to rendering settings");
    }
}

void BackgroundManager::resetToDefaults()
{
    clearAllConfigurations();
    BackgroundSettings defaultSettings;
    defaultSettings.style = 0; // Solid color
    defaultSettings.backgroundColor = wxColour(173, 204, 255); // Light blue
    defaultSettings.gradientTopColor = wxColour(200, 220, 255);
    defaultSettings.gradientBottomColor = wxColour(150, 180, 255);
    defaultSettings.name = "Default Background";
    addConfiguration(defaultSettings);
    setActiveConfiguration(0);
    LOG_INF_S("BackgroundManager::resetToDefaults: Reset to default configuration");
}

float BackgroundManager::getPerformanceImpact() const
{
    if (hasActiveConfiguration()) {
        BackgroundSettings settings = getActiveConfiguration();
        switch (settings.style) {
            case 0: return 0.1f; // Solid
            case 1: return 0.2f; // Gradient
            case 2: return 0.5f; // Image
            case 3: case 4: case 5: case 6: case 7: case 8: return 0.1f; // Presets
            default: return 0.1f;
        }
    }
    return 0.1f;
}

std::string BackgroundManager::getQualityDescription() const
{
    if (hasActiveConfiguration()) {
        BackgroundSettings settings = getActiveConfiguration();
        switch (settings.style) {
            case 0: return "Solid Color Background";
            case 1: return "Gradient Background";
            case 2: return "Image Background";
            case 3: return "Environment Background";
            case 4: return "Studio Background";
            case 5: return "Outdoor Background";
            case 6: return "Industrial Background";
            case 7: return "CAD Background";
            case 8: return "Dark Background";
            default: return "Unknown Background";
        }
    }
    return "No Background";
}

void BackgroundManager::initializePresets()
{
    // Initialize built-in presets
    BackgroundSettings solidPreset;
    solidPreset.style = 0;
    solidPreset.backgroundColor = wxColour(173, 204, 255);
    solidPreset.name = "Solid";
    m_presets["Solid"] = solidPreset;
    
    BackgroundSettings gradientPreset;
    gradientPreset.style = 1;
    gradientPreset.gradientTopColor = wxColour(200, 220, 255);
    gradientPreset.gradientBottomColor = wxColour(150, 180, 255);
    gradientPreset.name = "Gradient";
    m_presets["Gradient"] = gradientPreset;
    
    BackgroundSettings environmentPreset;
    environmentPreset.style = 3;
    environmentPreset.backgroundColor = wxColour(135, 206, 235);
    environmentPreset.name = "Environment";
    m_presets["Environment"] = environmentPreset;
    
    BackgroundSettings studioPreset;
    studioPreset.style = 4;
    studioPreset.backgroundColor = wxColour(240, 248, 255);
    studioPreset.name = "Studio";
    m_presets["Studio"] = studioPreset;
    
    BackgroundSettings outdoorPreset;
    outdoorPreset.style = 5;
    outdoorPreset.backgroundColor = wxColour(255, 255, 224);
    outdoorPreset.name = "Outdoor";
    m_presets["Outdoor"] = outdoorPreset;
    
    BackgroundSettings industrialPreset;
    industrialPreset.style = 6;
    industrialPreset.backgroundColor = wxColour(245, 245, 245);
    industrialPreset.name = "Industrial";
    m_presets["Industrial"] = industrialPreset;
    
    BackgroundSettings cadPreset;
    cadPreset.style = 7;
    cadPreset.backgroundColor = wxColour(255, 248, 220);
    cadPreset.name = "CAD";
    m_presets["CAD"] = cadPreset;
    
    BackgroundSettings darkPreset;
    darkPreset.style = 8;
    darkPreset.backgroundColor = wxColour(40, 40, 40);
    darkPreset.name = "Dark";
    m_presets["Dark"] = darkPreset;
    
    LOG_INF_S("BackgroundManager::initializePresets: Initialized " + std::to_string(m_presets.size()) + " presets");
}

void BackgroundManager::setupOpenGLState(const BackgroundSettings& settings)
{
    if (!m_canvas) {
        return;
    }
    
    // Update the RenderingManager with our background settings
    if (m_canvas->getRenderingManager()) {
        RenderingSettings renderingSettings = m_canvas->getRenderingManager()->getActiveConfiguration();
        renderingSettings.backgroundStyle = settings.style;
        renderingSettings.backgroundColor = settings.backgroundColor;
        renderingSettings.gradientTopColor = settings.gradientTopColor;
        renderingSettings.gradientBottomColor = settings.gradientBottomColor;
        renderingSettings.backgroundImagePath = settings.imagePath;
        renderingSettings.backgroundImageEnabled = settings.imageEnabled;
        renderingSettings.backgroundImageOpacity = settings.imageOpacity;
        renderingSettings.backgroundImageFit = settings.imageFit;
        renderingSettings.backgroundImageMaintainAspect = settings.imageMaintainAspect;
        
        // Update the rendering manager
        int activeConfigId = m_canvas->getRenderingManager()->getActiveConfigurationId();
        if (activeConfigId != -1) {
            m_canvas->getRenderingManager()->updateConfiguration(activeConfigId, renderingSettings);
        }
    }
}

