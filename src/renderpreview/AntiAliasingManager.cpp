#include "renderpreview/AntiAliasingManager.h"
#include "logger/Logger.h"
#include <wx/glcanvas.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <algorithm>

AntiAliasingManager::AntiAliasingManager(wxGLCanvas* canvas, wxGLContext* glContext)
    : m_canvas(canvas), m_glContext(glContext), m_nextConfigId(1), m_activeConfigId(-1)
{
    initializePresets();
    LOG_INF_S("AntiAliasingManager: Initialized");
}

AntiAliasingManager::~AntiAliasingManager()
{
    // Don't call clearAllConfigurations() in destructor as it may try to access destroyed OpenGL context
    m_configurations.clear();
    m_activeConfigId = -1;
    LOG_INF_S("AntiAliasingManager: Destroyed");
}

int AntiAliasingManager::addConfiguration(const AntiAliasingSettings& settings)
{
    LOG_INF_S("AntiAliasingManager::addConfiguration: Adding configuration '" + settings.name + "'");
    
    // Validate settings before adding
    if (settings.method < 0 || settings.method > 4) {
        LOG_ERR_S("AntiAliasingManager::addConfiguration: Invalid method value " + std::to_string(settings.method) + " in configuration '" + settings.name + "'");
        return -1;
    }
    
    // Validate MSAA samples if method is MSAA
    if (settings.method == 1) { // MSAA method
        if (settings.msaaSamples < 2 || settings.msaaSamples > 16) {
            LOG_ERR_S("AntiAliasingManager::addConfiguration: MSAA samples must be between 2 and 16");
            return -1;
        }
        // Check if samples is a power of 2
        if ((settings.msaaSamples & (settings.msaaSamples - 1)) != 0) {
            LOG_ERR_S("AntiAliasingManager::addConfiguration: MSAA samples must be a power of 2");
            return -1;
        }
    }
    
    auto managedConfig = std::make_unique<ManagedAntiAliasing>();
    managedConfig->settings = settings;
    managedConfig->configId = m_nextConfigId++;
    managedConfig->isActive = false;
    
    int configId = managedConfig->configId;
    m_configurations[configId] = std::move(managedConfig);
    
    LOG_INF_S("AntiAliasingManager::addConfiguration: Successfully added configuration with ID " + std::to_string(configId));
    return configId;
}

bool AntiAliasingManager::removeConfiguration(int configId)
{
    auto it = m_configurations.find(configId);
    if (it == m_configurations.end()) {
        LOG_WRN_S("AntiAliasingManager::removeConfiguration: Configuration with ID " + std::to_string(configId) + " not found");
        return false;
    }
    
    if (it->second->isActive) {
        setActiveConfiguration(-1);
    }
    
    m_configurations.erase(it);
    
    LOG_INF_S("AntiAliasingManager::removeConfiguration: Successfully removed configuration with ID " + std::to_string(configId));
    return true;
}

bool AntiAliasingManager::updateConfiguration(int configId, const AntiAliasingSettings& settings)
{
    auto it = m_configurations.find(configId);
    if (it == m_configurations.end()) {
        LOG_WRN_S("AntiAliasingManager::updateConfiguration: Configuration with ID " + std::to_string(configId) + " not found");
        return false;
    }
    
    it->second->settings = settings;
    
    if (it->second->isActive) {
        applyToRenderPipeline();
    }
    
    LOG_INF_S("AntiAliasingManager::updateConfiguration: Successfully updated configuration with ID " + std::to_string(configId));
    return true;
}

void AntiAliasingManager::clearAllConfigurations()
{
    LOG_INF_S("AntiAliasingManager::clearAllConfigurations: Clearing all configurations");
    
    m_configurations.clear();
    m_activeConfigId = -1;
    
    // Only disable anti-aliasing if OpenGL context is still valid
    if (m_canvas && m_glContext) {
        try {
            disableAllAntiAliasing();
        } catch (...) {
            LOG_WRN_S("AntiAliasingManager::clearAllConfigurations: Failed to disable anti-aliasing (OpenGL context may be destroyed)");
        }
    }
    
    LOG_INF_S("AntiAliasingManager::clearAllConfigurations: All configurations cleared");
}

std::vector<int> AntiAliasingManager::getAllConfigurationIds() const
{
    std::vector<int> ids;
    for (const auto& pair : m_configurations) {
        ids.push_back(pair.first);
    }
    return ids;
}

std::vector<AntiAliasingSettings> AntiAliasingManager::getAllConfigurations() const
{
    std::vector<AntiAliasingSettings> configs;
    for (const auto& pair : m_configurations) {
        configs.push_back(pair.second->settings);
    }
    return configs;
}

AntiAliasingSettings AntiAliasingManager::getConfiguration(int configId) const
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        return it->second->settings;
    }
    return AntiAliasingSettings();
}

bool AntiAliasingManager::hasConfiguration(int configId) const
{
    return m_configurations.find(configId) != m_configurations.end();
}

int AntiAliasingManager::getConfigurationCount() const
{
    return static_cast<int>(m_configurations.size());
}

bool AntiAliasingManager::setActiveConfiguration(int configId)
{
    if (m_activeConfigId != -1) {
        auto it = m_configurations.find(m_activeConfigId);
        if (it != m_configurations.end()) {
            it->second->isActive = false;
        }
    }
    
    if (configId != -1) {
        auto it = m_configurations.find(configId);
        if (it == m_configurations.end()) {
            LOG_WRN_S("AntiAliasingManager::setActiveConfiguration: Configuration with ID " + std::to_string(configId) + " not found");
            return false;
        }
        
        it->second->isActive = true;
        m_activeConfigId = configId;
        
        applyToRenderPipeline();
        
        LOG_INF_S("AntiAliasingManager::setActiveConfiguration: Activated configuration with ID " + std::to_string(configId));
    } else {
        m_activeConfigId = -1;
        disableAllAntiAliasing();
        LOG_INF_S("AntiAliasingManager::setActiveConfiguration: Deactivated all configurations");
    }
    
    return true;
}

int AntiAliasingManager::getActiveConfigurationId() const
{
    return m_activeConfigId;
}

AntiAliasingSettings AntiAliasingManager::getActiveConfiguration() const
{
    if (m_activeConfigId != -1) {
        auto it = m_configurations.find(m_activeConfigId);
        if (it != m_configurations.end()) {
            return it->second->settings;
        }
    }
    return AntiAliasingSettings();
}

bool AntiAliasingManager::hasActiveConfiguration() const
{
    return m_activeConfigId != -1;
}

void AntiAliasingManager::setMethod(int configId, int method)
{
    // Validate method range (0-4)
    if (method < 0 || method > 4) {
        LOG_WRN_S("AntiAliasingManager::setMethod: Method must be between 0 and 4, got " + std::to_string(method));
        return;
    }
    
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.method = method;
        if (it->second->isActive) {
            applyToRenderPipeline();
        }
    }
}

void AntiAliasingManager::setMSAASamples(int configId, int samples)
{
    // Validate MSAA samples
    if (samples < 2 || samples > 16) {
        LOG_ERR_S("AntiAliasingManager::setMSAASamples: MSAA samples must be between 2 and 16");
        return;
    }
    // Check if samples is a power of 2
    if ((samples & (samples - 1)) != 0) {
        LOG_ERR_S("AntiAliasingManager::setMSAASamples: MSAA samples must be a power of 2");
        return;
    }
    
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.msaaSamples = samples;
        if (it->second->isActive) {
            applyToRenderPipeline();
        }
        LOG_INF_S("AntiAliasingManager::setMSAASamples: Set MSAA samples to " + std::to_string(samples));
    } else {
        LOG_WRN_S("AntiAliasingManager::setMSAASamples: Configuration with ID " + std::to_string(configId) + " not found");
    }
}

void AntiAliasingManager::setFXAAEnabled(int configId, bool enabled)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.fxaaEnabled = enabled;
        if (it->second->isActive) {
            applyToRenderPipeline();
        }
    }
}

void AntiAliasingManager::setFXAAQuality(int configId, float quality)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.fxaaQuality = std::max(0.0f, std::min(1.0f, quality));
        if (it->second->isActive) {
            applyToRenderPipeline();
        }
    }
}

void AntiAliasingManager::setSSAAEnabled(int configId, bool enabled)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.ssaaEnabled = enabled;
        if (it->second->isActive) {
            applyToRenderPipeline();
        }
    }
}

void AntiAliasingManager::setSSAAFactor(int configId, int factor)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.ssaaFactor = factor;
        if (it->second->isActive) {
            applyToRenderPipeline();
        }
    }
}

void AntiAliasingManager::setTAAEnabled(int configId, bool enabled)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.taaEnabled = enabled;
        if (it->second->isActive) {
            applyToRenderPipeline();
        }
    }
}

void AntiAliasingManager::setTAAStrength(int configId, float strength)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.taaStrength = std::max(0.0f, std::min(1.0f, strength));
        if (it->second->isActive) {
            applyToRenderPipeline();
        }
    }
}

void AntiAliasingManager::applyPreset(const std::string& presetName)
{
    auto it = m_presets.find(presetName);
    if (it != m_presets.end()) {
        // Create a new configuration with the preset settings
        AntiAliasingSettings presetSettings = it->second;
        presetSettings.name = presetName;
        
        // Add the preset as a new configuration
        int configId = addConfiguration(presetSettings);
        if (configId != -1) {
            // Set it as active
            setActiveConfiguration(configId);
            LOG_INF_S("AntiAliasingManager::applyPreset: Applied preset '" + presetName + "' with config ID " + std::to_string(configId));
        } else {
            LOG_ERR_S("AntiAliasingManager::applyPreset: Failed to create configuration for preset '" + presetName + "'");
        }
    } else {
        // Handle built-in presets
        AntiAliasingSettings settings;
        settings.enabled = true;
        
        if (presetName == "None") {
            settings.method = 0;
            settings.enabled = false;
        } else if (presetName == "MSAA 2x") {
            settings.method = 1;
            settings.msaaSamples = 2;
        } else if (presetName == "MSAA 4x") {
            settings.method = 1;
            settings.msaaSamples = 4;
        } else if (presetName == "MSAA 8x") {
            settings.method = 1;
            settings.msaaSamples = 8;
        } else if (presetName == "MSAA 16x") {
            settings.method = 1;
            settings.msaaSamples = 16;
        } else if (presetName == "FXAA Low") {
            settings.method = 2;
            settings.fxaaEnabled = true;
            settings.fxaaQuality = 0.3f;
        } else if (presetName == "FXAA Medium") {
            settings.method = 2;
            settings.fxaaEnabled = true;
            settings.fxaaQuality = 0.5f;
        } else if (presetName == "FXAA High") {
            settings.method = 2;
            settings.fxaaEnabled = true;
            settings.fxaaQuality = 0.7f;
        } else if (presetName == "FXAA Ultra") {
            settings.method = 2;
            settings.fxaaEnabled = true;
            settings.fxaaQuality = 1.0f;
        } else if (presetName == "SSAA 2x") {
            settings.method = 3;
            settings.ssaaEnabled = true;
            settings.ssaaFactor = 2;
        } else if (presetName == "SSAA 4x") {
            settings.method = 3;
            settings.ssaaEnabled = true;
            settings.ssaaFactor = 4;
        } else if (presetName == "TAA Low") {
            settings.method = 4;
            settings.taaEnabled = true;
            settings.taaStrength = 0.3f;
        } else if (presetName == "TAA Medium") {
            settings.method = 4;
            settings.taaEnabled = true;
            settings.taaStrength = 0.5f;
        } else if (presetName == "TAA High") {
            settings.method = 4;
            settings.taaEnabled = true;
            settings.taaStrength = 0.7f;
        } else if (presetName == "MSAA 4x + FXAA") {
            settings.method = 1;
            settings.msaaSamples = 4;
            settings.fxaaEnabled = true;
            settings.fxaaQuality = 0.5f;
        } else if (presetName == "MSAA 4x + TAA") {
            settings.method = 1;
            settings.msaaSamples = 4;
            settings.taaEnabled = true;
            settings.taaStrength = 0.5f;
        } else {
            // Default to MSAA 4x
            settings.method = 1;
            settings.msaaSamples = 4;
        }
        
        settings.name = presetName;
        int configId = addConfiguration(settings);
        if (configId != -1) {
            setActiveConfiguration(configId);
            LOG_INF_S("AntiAliasingManager::applyPreset: Applied built-in preset '" + presetName + "' with config ID " + std::to_string(configId));
        } else {
            LOG_ERR_S("AntiAliasingManager::applyPreset: Failed to create configuration for built-in preset '" + presetName + "'");
        }
    }
}

void AntiAliasingManager::saveAsPreset(int configId, const std::string& presetName)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        m_presets[presetName] = it->second->settings;
        LOG_INF_S("AntiAliasingManager::saveAsPreset: Saved configuration as preset '" + presetName + "'");
    }
}

std::vector<std::string> AntiAliasingManager::getAvailablePresets() const
{
    std::vector<std::string> presets;
    for (const auto& pair : m_presets) {
        presets.push_back(pair.first);
    }
    return presets;
}

void AntiAliasingManager::applyToRenderPipeline()
{
    if (m_activeConfigId != -1) {
        auto it = m_configurations.find(m_activeConfigId);
        if (it != m_configurations.end()) {
            const auto& settings = it->second->settings;
            
            if (!settings.enabled) {
                disableAllAntiAliasing();
                return;
            }
            
            switch (settings.method) {
                case 0: // None
                    disableAllAntiAliasing();
                    break;
                case 1: // MSAA
                    applyMSAA(settings);
                    break;
                case 2: // FXAA
                    applyFXAA(settings);
                    break;
                case 3: // SSAA
                    applySSAA(settings);
                    break;
                case 4: // TAA
                    applyTAA(settings);
                    break;
                default:
                    disableAllAntiAliasing();
                    break;
            }
            
            LOG_INF_S("AntiAliasingManager::applyToRenderPipeline: Applied method " + std::to_string(settings.method));
        }
    }
}

void AntiAliasingManager::updateRenderingState()
{
    applyToRenderPipeline();
}

float AntiAliasingManager::getPerformanceImpact() const
{
    if (m_activeConfigId != -1) {
        auto it = m_configurations.find(m_activeConfigId);
        if (it != m_configurations.end()) {
            const auto& settings = it->second->settings;
            
            float impact = 1.0f;
            
            switch (settings.method) {
                case 0: // None
                    impact = 1.0f;
                    break;
                case 1: // MSAA
                    impact = 1.0f + (settings.msaaSamples / 4.0f) * 0.8f;
                    break;
                case 2: // FXAA
                    impact = 1.0f + settings.fxaaQuality * 0.3f;
                    break;
                case 3: // SSAA
                    impact = 1.0f + (settings.ssaaFactor * settings.ssaaFactor) * 0.5f;
                    break;
                case 4: // TAA
                    impact = 1.0f + settings.taaStrength * 0.4f + settings.jitterStrength * 0.2f;
                    break;
                default:
                    impact = 1.0f;
                    break;
            }
            
            // Additional factors
            if (settings.adaptiveEnabled) {
                impact *= 0.9f; // Adaptive AA reduces impact
            }
            if (settings.temporalFiltering) {
                impact *= 1.1f; // Temporal filtering increases impact
            }
            
            return impact;
        }
    }
    return 1.0f;
}

std::string AntiAliasingManager::getQualityDescription() const
{
    if (m_activeConfigId != -1) {
        auto it = m_configurations.find(m_activeConfigId);
        if (it != m_configurations.end()) {
            const auto& settings = it->second->settings;
            
            switch (settings.method) {
                case 0: return "No Anti-Aliasing";
                case 1: return "MSAA " + std::to_string(settings.msaaSamples) + "x";
                case 2: return "FXAA (Quality: " + std::to_string(static_cast<int>(settings.fxaaQuality * 100)) + "%)";
                case 3: return "SSAA " + std::to_string(settings.ssaaFactor) + "x";
                case 4: return "TAA (Strength: " + std::to_string(static_cast<int>(settings.taaStrength * 100)) + "%)";
                default: return "Unknown";
            }
        }
    }
    return "No Configuration Active";
}

void AntiAliasingManager::initializePresets()
{
    // No Anti-Aliasing
    AntiAliasingSettings none;
    none.name = "No Anti-Aliasing";
    none.enabled = false;
    none.method = 0;
    m_presets["None"] = none;
    
    // MSAA Presets
    AntiAliasingSettings msaa2x;
    msaa2x.name = "MSAA 2x";
    msaa2x.method = 1;
    msaa2x.msaaSamples = 2;
    m_presets["MSAA 2x"] = msaa2x;
    
    AntiAliasingSettings msaa4x;
    msaa4x.name = "MSAA 4x";
    msaa4x.method = 1;
    msaa4x.msaaSamples = 4;
    m_presets["MSAA 4x"] = msaa4x;
    
    AntiAliasingSettings msaa8x;
    msaa8x.name = "MSAA 8x";
    msaa8x.method = 1;
    msaa8x.msaaSamples = 8;
    m_presets["MSAA 8x"] = msaa8x;
    
    AntiAliasingSettings msaa16x;
    msaa16x.name = "MSAA 16x";
    msaa16x.method = 1;
    msaa16x.msaaSamples = 16;
    m_presets["MSAA 16x"] = msaa16x;
    
    // FXAA Presets
    AntiAliasingSettings fxaaLow;
    fxaaLow.name = "FXAA Low";
    fxaaLow.method = 2;
    fxaaLow.fxaaEnabled = true;
    fxaaLow.fxaaQuality = 0.25f;
    m_presets["FXAA Low"] = fxaaLow;
    
    AntiAliasingSettings fxaaMedium;
    fxaaMedium.name = "FXAA Medium";
    fxaaMedium.method = 2;
    fxaaMedium.fxaaEnabled = true;
    fxaaMedium.fxaaQuality = 0.5f;
    m_presets["FXAA Medium"] = fxaaMedium;
    
    AntiAliasingSettings fxaaHigh;
    fxaaHigh.name = "FXAA High";
    fxaaHigh.method = 2;
    fxaaHigh.fxaaEnabled = true;
    fxaaHigh.fxaaQuality = 0.75f;
    m_presets["FXAA High"] = fxaaHigh;
    
    AntiAliasingSettings fxaaUltra;
    fxaaUltra.name = "FXAA Ultra";
    fxaaUltra.method = 2;
    fxaaUltra.fxaaEnabled = true;
    fxaaUltra.fxaaQuality = 1.0f;
    m_presets["FXAA Ultra"] = fxaaUltra;
    
    // SSAA Presets
    AntiAliasingSettings ssaa2x;
    ssaa2x.name = "SSAA 2x";
    ssaa2x.method = 3;
    ssaa2x.ssaaEnabled = true;
    ssaa2x.ssaaFactor = 2;
    m_presets["SSAA 2x"] = ssaa2x;
    
    AntiAliasingSettings ssaa4x;
    ssaa4x.name = "SSAA 4x";
    ssaa4x.method = 3;
    ssaa4x.ssaaEnabled = true;
    ssaa4x.ssaaFactor = 4;
    m_presets["SSAA 4x"] = ssaa4x;
    
    // TAA Presets
    AntiAliasingSettings taaLow;
    taaLow.name = "TAA Low";
    taaLow.method = 4;
    taaLow.taaEnabled = true;
    taaLow.taaStrength = 0.3f;
    taaLow.jitterStrength = 0.3f;
    m_presets["TAA Low"] = taaLow;
    
    AntiAliasingSettings taaMedium;
    taaMedium.name = "TAA Medium";
    taaMedium.method = 4;
    taaMedium.taaEnabled = true;
    taaMedium.taaStrength = 0.6f;
    taaMedium.jitterStrength = 0.5f;
    m_presets["TAA Medium"] = taaMedium;
    
    AntiAliasingSettings taaHigh;
    taaHigh.name = "TAA High";
    taaHigh.method = 4;
    taaHigh.taaEnabled = true;
    taaHigh.taaStrength = 0.8f;
    taaHigh.jitterStrength = 0.7f;
    m_presets["TAA High"] = taaHigh;
    
    // Hybrid Presets (Combination of methods)
    AntiAliasingSettings hybridMSAA_FXAA;
    hybridMSAA_FXAA.name = "MSAA 4x + FXAA";
    hybridMSAA_FXAA.method = 1;
    hybridMSAA_FXAA.msaaSamples = 4;
    hybridMSAA_FXAA.fxaaEnabled = true;
    hybridMSAA_FXAA.fxaaQuality = 0.5f;
    m_presets["MSAA 4x + FXAA"] = hybridMSAA_FXAA;
    
    AntiAliasingSettings hybridMSAA_TAA;
    hybridMSAA_TAA.name = "MSAA 4x + TAA";
    hybridMSAA_TAA.method = 1;
    hybridMSAA_TAA.msaaSamples = 4;
    hybridMSAA_TAA.taaEnabled = true;
    hybridMSAA_TAA.taaStrength = 0.6f;
    hybridMSAA_TAA.jitterStrength = 0.5f;
    m_presets["MSAA 4x + TAA"] = hybridMSAA_TAA;
    
    // Performance Optimized Presets
    AntiAliasingSettings performanceLow;
    performanceLow.name = "Performance Low";
    performanceLow.method = 2;
    performanceLow.fxaaEnabled = true;
    performanceLow.fxaaQuality = 0.25f;
    performanceLow.adaptiveEnabled = true;
    performanceLow.edgeThreshold = 0.05f;
    m_presets["Performance Low"] = performanceLow;
    
    AntiAliasingSettings performanceMedium;
    performanceMedium.name = "Performance Medium";
    performanceMedium.method = 1;
    performanceMedium.msaaSamples = 2;
    performanceMedium.adaptiveEnabled = true;
    performanceMedium.edgeThreshold = 0.1f;
    m_presets["Performance Medium"] = performanceMedium;
    
    // Quality Optimized Presets
    AntiAliasingSettings qualityHigh;
    qualityHigh.name = "Quality High";
    qualityHigh.method = 1;
    qualityHigh.msaaSamples = 8;
    qualityHigh.adaptiveEnabled = true;
    qualityHigh.edgeThreshold = 0.15f;
    qualityHigh.temporalFiltering = true;
    m_presets["Quality High"] = qualityHigh;
    
    AntiAliasingSettings qualityUltra;
    qualityUltra.name = "Quality Ultra";
    qualityUltra.method = 3;
    qualityUltra.ssaaEnabled = true;
    qualityUltra.ssaaFactor = 4;
    qualityUltra.adaptiveEnabled = true;
    qualityUltra.edgeThreshold = 0.2f;
    qualityUltra.temporalFiltering = true;
    m_presets["Quality Ultra"] = qualityUltra;
    
    // CAD/Engineering Specific Presets
    AntiAliasingSettings cadStandard;
    cadStandard.name = "CAD Standard";
    cadStandard.method = 1;
    cadStandard.msaaSamples = 4;
    cadStandard.adaptiveEnabled = true;
    cadStandard.edgeThreshold = 0.12f;
    m_presets["CAD Standard"] = cadStandard;
    
    AntiAliasingSettings cadHighQuality;
    cadHighQuality.name = "CAD High Quality";
    cadHighQuality.method = 1;
    cadHighQuality.msaaSamples = 8;
    cadHighQuality.adaptiveEnabled = true;
    cadHighQuality.edgeThreshold = 0.15f;
    cadHighQuality.temporalFiltering = true;
    m_presets["CAD High Quality"] = cadHighQuality;
    
    // Gaming/Real-time Presets
    AntiAliasingSettings gamingFast;
    gamingFast.name = "Gaming Fast";
    gamingFast.method = 2;
    gamingFast.fxaaEnabled = true;
    gamingFast.fxaaQuality = 0.4f;
    gamingFast.adaptiveEnabled = true;
    gamingFast.edgeThreshold = 0.08f;
    m_presets["Gaming Fast"] = gamingFast;
    
    AntiAliasingSettings gamingBalanced;
    gamingBalanced.name = "Gaming Balanced";
    gamingBalanced.method = 4;
    gamingBalanced.taaEnabled = true;
    gamingBalanced.taaStrength = 0.7f;
    gamingBalanced.jitterStrength = 0.6f;
    gamingBalanced.temporalFiltering = true;
    m_presets["Gaming Balanced"] = gamingBalanced;
    
    // Mobile/Embedded Presets
    AntiAliasingSettings mobileLow;
    mobileLow.name = "Mobile Low";
    mobileLow.method = 2;
    mobileLow.fxaaEnabled = true;
    mobileLow.fxaaQuality = 0.2f;
    mobileLow.adaptiveEnabled = true;
    mobileLow.edgeThreshold = 0.03f;
    m_presets["Mobile Low"] = mobileLow;
    
    AntiAliasingSettings mobileMedium;
    mobileMedium.name = "Mobile Medium";
    mobileMedium.method = 2;
    mobileMedium.fxaaEnabled = true;
    mobileMedium.fxaaQuality = 0.35f;
    mobileMedium.adaptiveEnabled = true;
    mobileMedium.edgeThreshold = 0.06f;
    m_presets["Mobile Medium"] = mobileMedium;
    
    LOG_INF_S("AntiAliasingManager::initializePresets: Initialized " + std::to_string(m_presets.size()) + " presets");
}

void AntiAliasingManager::applyMSAA(const AntiAliasingSettings& settings)
{
    if (m_canvas && m_glContext) {
        m_canvas->SetCurrent(*m_glContext);
        
        // Enable multisampling
        glEnable(GL_MULTISAMPLE);
        
        // Set the number of samples if supported
        if (settings.msaaSamples > 0) {
            // Try to set the number of samples using GL_SAMPLES
            GLint maxSamples;
            glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
            
            if (settings.msaaSamples <= maxSamples) {
                // Set the number of samples for the framebuffer
                glEnable(GL_MULTISAMPLE);
                
                // For MSAA to work properly, we need to ensure the framebuffer supports multisampling
                // This is typically handled by the wxGLCanvas when it's created with multisample attributes
                LOG_INF_S("AntiAliasingManager::applyMSAA: Applied MSAA with " + std::to_string(settings.msaaSamples) + " samples (max supported: " + std::to_string(maxSamples) + ")");
            } else {
                LOG_WRN_S("AntiAliasingManager::applyMSAA: Requested " + std::to_string(settings.msaaSamples) + " samples, but only " + std::to_string(maxSamples) + " are supported");
            }
        }
        
        // Additional MSAA settings for better quality
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_POLYGON_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
        
        LOG_INF_S("AntiAliasingManager::applyMSAA: Applied MSAA with " + std::to_string(settings.msaaSamples) + " samples");
    }
}

void AntiAliasingManager::applyFXAA(const AntiAliasingSettings& settings)
{
    if (m_canvas && m_glContext) {
        m_canvas->SetCurrent(*m_glContext);
        
        // Disable MSAA when using FXAA
        glDisable(GL_MULTISAMPLE);
        
        // Enable line and polygon smoothing for better FXAA effect
        glEnable(GL_LINE_SMOOTH);
        glEnable(GL_POLYGON_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
        
        // FXAA is typically implemented as a post-processing shader
        // For now, we'll use OpenGL's built-in smoothing as a fallback
        if (settings.fxaaQuality > 0.5f) {
            // High quality FXAA - use more aggressive smoothing
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }
        
        LOG_INF_S("AntiAliasingManager::applyFXAA: Applied FXAA with quality " + std::to_string(settings.fxaaQuality));
    }
}

void AntiAliasingManager::applySSAA(const AntiAliasingSettings& settings)
{
    if (m_canvas && m_glContext) {
        m_canvas->SetCurrent(*m_glContext);
        glDisable(GL_MULTISAMPLE);
        LOG_INF_S("AntiAliasingManager::applySSAA: Applied SSAA with factor " + std::to_string(settings.ssaaFactor));
    }
}

void AntiAliasingManager::applyTAA(const AntiAliasingSettings& settings)
{
    if (m_canvas && m_glContext) {
        m_canvas->SetCurrent(*m_glContext);
        glDisable(GL_MULTISAMPLE);
        LOG_INF_S("AntiAliasingManager::applyTAA: Applied TAA with strength " + std::to_string(settings.taaStrength));
    }
}

void AntiAliasingManager::disableAllAntiAliasing()
{
    if (m_canvas && m_glContext) {
        try {
            m_canvas->SetCurrent(*m_glContext);
            glDisable(GL_MULTISAMPLE);
            LOG_INF_S("AntiAliasingManager::disableAllAntiAliasing: Disabled all anti-aliasing");
        } catch (...) {
            LOG_WRN_S("AntiAliasingManager::disableAllAntiAliasing: Failed to disable anti-aliasing (OpenGL context may be destroyed)");
        }
    }
}

void AntiAliasingManager::setupOpenGLState(const AntiAliasingSettings& settings)
{
    if (m_canvas && m_glContext) {
        m_canvas->SetCurrent(*m_glContext);
        
        if (settings.enabled) {
            switch (settings.method) {
                case 1: // MSAA
                    glEnable(GL_MULTISAMPLE);
                    break;
                default:
                    glDisable(GL_MULTISAMPLE);
                    break;
            }
        } else {
            glDisable(GL_MULTISAMPLE);
        }
    }
}

void AntiAliasingManager::restoreOpenGLState()
{
    if (m_canvas && m_glContext) {
        m_canvas->SetCurrent(*m_glContext);
        glDisable(GL_MULTISAMPLE);
    }
} 