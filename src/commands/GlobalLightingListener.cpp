#include "GlobalLightingListener.h"
#include "renderpreview/PreviewCanvas.h"
#include "config/LightingConfig.h"
#include "logger/Logger.h"
#include <wx/colour.h>
#include <OpenCASCADE/Quantity_Color.hxx>

GlobalLightingListener::GlobalLightingListener(PreviewCanvas* previewCanvas)
    : m_previewCanvas(previewCanvas)
    , m_globalLightingConfig(&LightingConfig::getInstance())
    , m_isConnected(false)
{
    LOG_INF_S("GlobalLightingListener: Initialized");
}

GlobalLightingListener::~GlobalLightingListener()
{
    disconnectFromGlobalLighting();
    LOG_INF_S("GlobalLightingListener: Destroyed");
}

void GlobalLightingListener::connectToGlobalLighting()
{
    if (m_isConnected) {
        LOG_WRN_S("GlobalLightingListener: Already connected to global lighting");
        return;
    }

    // Create callback for settings changes
    m_settingsChangedCallback = [this]() {
        this->onGlobalLightingChanged();
    };

    // Register callback with global lighting config
    m_globalLightingConfig->addSettingsChangedCallback(m_settingsChangedCallback);
    m_isConnected = true;

    LOG_INF_S("GlobalLightingListener: Connected to global lighting configuration");
}

void GlobalLightingListener::disconnectFromGlobalLighting()
{
    if (!m_isConnected) {
        return;
    }

    // Note: LightingConfig doesn't have a removeCallback method, so we'll just mark as disconnected
    m_isConnected = false;
    m_settingsChangedCallback = nullptr;

    LOG_INF_S("GlobalLightingListener: Disconnected from global lighting configuration");
}

void GlobalLightingListener::applyGlobalSettingsToPreview()
{
    if (!m_previewCanvas) {
        LOG_WRN_S("GlobalLightingListener: No preview canvas available");
        return;
    }

    LOG_INF_S("GlobalLightingListener: Applying global settings to preview canvas");
    
    applyEnvironmentSettings();
    applyLightingSettings();
}

void GlobalLightingListener::setPreviewCanvas(PreviewCanvas* canvas)
{
    m_previewCanvas = canvas;
    LOG_INF_S("GlobalLightingListener: Preview canvas updated");
}

void GlobalLightingListener::onGlobalLightingChanged()
{
    if (!m_isConnected) {
        return;
    }

    LOG_INF_S("GlobalLightingListener: Global lighting settings changed, applying to preview");
    applyGlobalSettingsToPreview();
}

void GlobalLightingListener::applyEnvironmentSettings()
{
    if (!m_previewCanvas || !m_globalLightingConfig) {
        return;
    }

    const auto& envSettings = m_globalLightingConfig->getEnvironmentSettings();
    
    // Convert Quantity_Color to wxColour
    wxColour ambientColor(
        static_cast<unsigned char>(envSettings.ambientColor.Red() * 255),
        static_cast<unsigned char>(envSettings.ambientColor.Green() * 255),
        static_cast<unsigned char>(envSettings.ambientColor.Blue() * 255)
    );

    // Apply environment settings to preview canvas
    // Note: We'll use the existing updateLighting method with environment parameters
    float ambientIntensity = static_cast<float>(envSettings.ambientIntensity);
    float diffuseIntensity = 1.0f; // Default diffuse
    float specularIntensity = 0.5f; // Default specular
    float overallIntensity = 1.0f; // Default overall intensity

    m_previewCanvas->updateLighting(
        ambientIntensity,
        diffuseIntensity, 
        specularIntensity,
        ambientColor,
        overallIntensity
    );

    LOG_INF_S("GlobalLightingListener: Applied environment settings to preview canvas");
}

void GlobalLightingListener::applyLightingSettings()
{
    if (!m_previewCanvas || !m_globalLightingConfig) {
        return;
    }

    const auto& lights = m_globalLightingConfig->getAllLights();
    
    if (lights.empty()) {
        LOG_WRN_S("GlobalLightingListener: No lights available in global configuration");
        return;
    }

    // Convert global lights to RenderLightSettings format
    std::vector<RenderLightSettings> renderLights;
    
    for (const auto& globalLight : lights) {
        if (!globalLight.enabled) {
            continue;
        }

        RenderLightSettings renderLight;
        renderLight.name = globalLight.name;
        renderLight.type = globalLight.type;
        renderLight.enabled = globalLight.enabled;
        renderLight.positionX = globalLight.positionX;
        renderLight.positionY = globalLight.positionY;
        renderLight.positionZ = globalLight.positionZ;
        renderLight.directionX = globalLight.directionX;
        renderLight.directionY = globalLight.directionY;
        renderLight.directionZ = globalLight.directionZ;
        
        // Convert Quantity_Color to wxColour
        renderLight.color = wxColour(
            static_cast<unsigned char>(globalLight.color.Red() * 255),
            static_cast<unsigned char>(globalLight.color.Green() * 255),
            static_cast<unsigned char>(globalLight.color.Blue() * 255)
        );
        
        renderLight.intensity = static_cast<float>(globalLight.intensity);
        renderLight.spotAngle = static_cast<float>(globalLight.spotAngle);
        renderLight.spotExponent = static_cast<float>(globalLight.spotExponent);
        renderLight.constantAttenuation = static_cast<float>(globalLight.constantAttenuation);
        renderLight.linearAttenuation = static_cast<float>(globalLight.linearAttenuation);
        renderLight.quadraticAttenuation = static_cast<float>(globalLight.quadraticAttenuation);

        renderLights.push_back(renderLight);
    }

    // Apply lights to preview canvas using multi-light support
    if (!renderLights.empty()) {
        m_previewCanvas->updateMultiLighting(renderLights);
        LOG_INF_S("GlobalLightingListener: Applied " + std::to_string(renderLights.size()) + " lights to preview canvas");
    } else {
        LOG_WRN_S("GlobalLightingListener: No enabled lights to apply");
    }
} 