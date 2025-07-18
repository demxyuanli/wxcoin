#include "config/LightingConfig.h"
#include "logger/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <wx/stdpaths.h>
#include <wx/filename.h>

LightingConfig& LightingConfig::getInstance()
{
    static LightingConfig instance;
    return instance;
}

LightingConfig::LightingConfig()
{
    // Initialize default lighting setup
    initializeDefaultLights();
    
    // Load configuration from file on startup
    loadFromFile();
}

std::string LightingConfig::getConfigFilePath() const
{
    // Save to local root directory
    wxString currentDir = wxGetCwd();
    wxFileName configFile(currentDir, "lighting_settings.ini");
    
    // Create directory if it doesn't exist
    if (!configFile.DirExists()) {
        configFile.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }
    
    return configFile.GetFullPath().ToStdString();
}

void LightingConfig::initializeDefaultLights()
{
    m_lights.clear();
    
    // Main directional light
    LightSettings mainLight;
    mainLight.name = "Main Light";
    mainLight.type = "directional";
    mainLight.directionX = 0.5;
    mainLight.directionY = 0.5;
    mainLight.directionZ = -1.0;
    mainLight.color = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
    mainLight.intensity = 1.0;
    m_lights.push_back(mainLight);
    
    // Fill light
    LightSettings fillLight;
    fillLight.name = "Fill Light";
    fillLight.type = "directional";
    fillLight.directionX = -0.3;
    fillLight.directionY = -0.3;
    fillLight.directionZ = -0.5;
    fillLight.color = Quantity_Color(0.9, 0.9, 1.0, Quantity_TOC_RGB);
    fillLight.intensity = 0.6;
    m_lights.push_back(fillLight);
    
    // Back light
    LightSettings backLight;
    backLight.name = "Back Light";
    backLight.type = "directional";
    backLight.directionX = 0.2;
    backLight.directionY = 0.2;
    backLight.directionZ = 0.8;
    backLight.color = Quantity_Color(1.0, 1.0, 0.9, Quantity_TOC_RGB);
    backLight.intensity = 0.5;
    m_lights.push_back(backLight);
    
    // Environment settings
    m_environmentSettings.name = "Environment";
    m_environmentSettings.type = "ambient";
    m_environmentSettings.ambientColor = Quantity_Color(0.4, 0.4, 0.4, Quantity_TOC_RGB);
    m_environmentSettings.ambientIntensity = 1.0;
}

bool LightingConfig::loadFromFile(const std::string& filename)
{
    std::string configPath = filename.empty() ? getConfigFilePath() : filename;
    std::ifstream file(configPath);
    
    if (!file.is_open()) {
        LOG_INF_S("LightingConfig: No config file found, using defaults: " + configPath);
        return false;
    }
    
    LOG_INF_S("LightingConfig: Loading configuration from: " + configPath);
    
    std::string line;
    std::string currentSection;
    int currentLightIndex = -1;
    
    while (std::getline(file, line)) {
        // Remove comments and trim whitespace
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        
        if (line.empty()) {
            continue;
        }
        
        // Check if this is a section header
        if (line[0] == '[' && line[line.length() - 1] == ']') {
            currentSection = line.substr(1, line.length() - 2);
            
            if (currentSection.substr(0, 5) == "Light") {
                currentLightIndex = std::stoi(currentSection.substr(5));
                if (currentLightIndex >= static_cast<int>(m_lights.size())) {
                    m_lights.resize(currentLightIndex + 1);
                }
            }
            continue;
        }
        
        // Parse key-value pairs
        size_t equalPos = line.find('=');
        if (equalPos == std::string::npos) {
            continue;
        }
        
        std::string key = line.substr(0, equalPos);
        std::string value = line.substr(equalPos + 1);
        
        // Trim whitespace from key and value
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        
        if (currentSection == "Environment") {
            if (key == "AmbientColor") {
                m_environmentSettings.ambientColor = stringToColor(value);
            } else if (key == "AmbientIntensity") {
                m_environmentSettings.ambientIntensity = std::stod(value);
            }
        } else if (currentLightIndex >= 0 && currentLightIndex < static_cast<int>(m_lights.size())) {
            LightSettings& light = m_lights[currentLightIndex];
            
            if (key == "Name") {
                light.name = value;
            } else if (key == "Type") {
                light.type = value;
            } else if (key == "Enabled") {
                light.enabled = stringToBool(value);
            } else if (key == "PositionX") {
                light.positionX = std::stod(value);
            } else if (key == "PositionY") {
                light.positionY = std::stod(value);
            } else if (key == "PositionZ") {
                light.positionZ = std::stod(value);
            } else if (key == "DirectionX") {
                light.directionX = std::stod(value);
            } else if (key == "DirectionY") {
                light.directionY = std::stod(value);
            } else if (key == "DirectionZ") {
                light.directionZ = std::stod(value);
            } else if (key == "Color") {
                light.color = stringToColor(value);
            } else if (key == "Intensity") {
                light.intensity = std::stod(value);
            } else if (key == "SpotAngle") {
                light.spotAngle = std::stod(value);
            } else if (key == "SpotExponent") {
                light.spotExponent = std::stod(value);
            } else if (key == "ConstantAttenuation") {
                light.constantAttenuation = std::stod(value);
            } else if (key == "LinearAttenuation") {
                light.linearAttenuation = std::stod(value);
            } else if (key == "QuadraticAttenuation") {
                light.quadraticAttenuation = std::stod(value);
            }
        }
    }
    
    file.close();
    LOG_INF_S("LightingConfig: Configuration loaded successfully");
    
    // Notify listeners of settings change
    notifySettingsChanged();
    
    return true;
}

bool LightingConfig::saveToFile(const std::string& filename) const
{
    std::string configPath = filename.empty() ? getConfigFilePath() : filename;
    std::ofstream file(configPath);
    
    if (!file.is_open()) {
        LOG_ERR_S("LightingConfig: Failed to save configuration to: " + configPath);
        return false;
    }
    
    LOG_INF_S("LightingConfig: Saving configuration to: " + configPath);
    
    // Save Environment settings
    file << "[Environment]\n";
    file << "AmbientColor=" << colorToString(m_environmentSettings.ambientColor) << "\n";
    file << "AmbientIntensity=" << m_environmentSettings.ambientIntensity << "\n\n";
    
    // Save each light
    for (size_t i = 0; i < m_lights.size(); ++i) {
        const LightSettings& light = m_lights[i];
        file << "[Light" << i << "]\n";
        file << "Name=" << light.name << "\n";
        file << "Type=" << light.type << "\n";
        file << "Enabled=" << boolToString(light.enabled) << "\n";
        file << "PositionX=" << light.positionX << "\n";
        file << "PositionY=" << light.positionY << "\n";
        file << "PositionZ=" << light.positionZ << "\n";
        file << "DirectionX=" << light.directionX << "\n";
        file << "DirectionY=" << light.directionY << "\n";
        file << "DirectionZ=" << light.directionZ << "\n";
        file << "Color=" << colorToString(light.color) << "\n";
        file << "Intensity=" << light.intensity << "\n";
        file << "SpotAngle=" << light.spotAngle << "\n";
        file << "SpotExponent=" << light.spotExponent << "\n";
        file << "ConstantAttenuation=" << light.constantAttenuation << "\n";
        file << "LinearAttenuation=" << light.linearAttenuation << "\n";
        file << "QuadraticAttenuation=" << light.quadraticAttenuation << "\n\n";
    }
    
    file.close();
    LOG_INF_S("LightingConfig: Configuration saved successfully");
    return true;
}

void LightingConfig::addLight(const LightSettings& light)
{
    m_lights.push_back(light);
    notifySettingsChanged();
}

void LightingConfig::removeLight(int index)
{
    if (index >= 0 && index < static_cast<int>(m_lights.size())) {
        m_lights.erase(m_lights.begin() + index);
        notifySettingsChanged();
    }
}

void LightingConfig::updateLight(int index, const LightSettings& light)
{
    if (index >= 0 && index < static_cast<int>(m_lights.size())) {
        m_lights[index] = light;
        notifySettingsChanged();
    }
}

const LightSettings& LightingConfig::getLight(int index) const
{
    static LightSettings defaultLight;
    if (index >= 0 && index < static_cast<int>(m_lights.size())) {
        return m_lights[index];
    }
    return defaultLight;
}

void LightingConfig::setEnvironmentSettings(const LightSettings& settings)
{
    m_environmentSettings = settings;
    notifySettingsChanged();
}

void LightingConfig::setLightEnabled(int index, bool enabled)
{
    if (index >= 0 && index < static_cast<int>(m_lights.size())) {
        m_lights[index].enabled = enabled;
        notifySettingsChanged();
    }
}

void LightingConfig::setLightName(int index, const std::string& name)
{
    if (index >= 0 && index < static_cast<int>(m_lights.size())) {
        m_lights[index].name = name;
        notifySettingsChanged();
    }
}

void LightingConfig::setLightType(int index, const std::string& type)
{
    if (index >= 0 && index < static_cast<int>(m_lights.size())) {
        m_lights[index].type = type;
        notifySettingsChanged();
    }
}

void LightingConfig::setLightPosition(int index, double x, double y, double z)
{
    if (index >= 0 && index < static_cast<int>(m_lights.size())) {
        m_lights[index].positionX = x;
        m_lights[index].positionY = y;
        m_lights[index].positionZ = z;
        notifySettingsChanged();
    }
}

void LightingConfig::setLightDirection(int index, double x, double y, double z)
{
    if (index >= 0 && index < static_cast<int>(m_lights.size())) {
        m_lights[index].directionX = x;
        m_lights[index].directionY = y;
        m_lights[index].directionZ = z;
        notifySettingsChanged();
    }
}

void LightingConfig::setLightColor(int index, const Quantity_Color& color)
{
    if (index >= 0 && index < static_cast<int>(m_lights.size())) {
        m_lights[index].color = color;
        notifySettingsChanged();
    }
}

void LightingConfig::setLightIntensity(int index, double intensity)
{
    if (index >= 0 && index < static_cast<int>(m_lights.size())) {
        m_lights[index].intensity = intensity;
        notifySettingsChanged();
    }
}

void LightingConfig::setLightSpotAngle(int index, double angle)
{
    if (index >= 0 && index < static_cast<int>(m_lights.size())) {
        m_lights[index].spotAngle = angle;
        notifySettingsChanged();
    }
}

void LightingConfig::setLightSpotExponent(int index, double exponent)
{
    if (index >= 0 && index < static_cast<int>(m_lights.size())) {
        m_lights[index].spotExponent = exponent;
        notifySettingsChanged();
    }
}

void LightingConfig::setLightAttenuation(int index, double constant, double linear, double quadratic)
{
    if (index >= 0 && index < static_cast<int>(m_lights.size())) {
        m_lights[index].constantAttenuation = constant;
        m_lights[index].linearAttenuation = linear;
        m_lights[index].quadraticAttenuation = quadratic;
        notifySettingsChanged();
    }
}

void LightingConfig::setEnvironmentAmbientColor(const Quantity_Color& color)
{
    m_environmentSettings.ambientColor = color;
    notifySettingsChanged();
}

void LightingConfig::setEnvironmentAmbientIntensity(double intensity)
{
    m_environmentSettings.ambientIntensity = intensity;
    notifySettingsChanged();
}

void LightingConfig::applyPreset(const std::string& presetName)
{
    if (presetName == "Studio") {
        m_lights.clear();
        
        // Key light
        LightSettings keyLight;
        keyLight.name = "Key Light";
        keyLight.type = "directional";
        keyLight.directionX = 0.7;
        keyLight.directionY = 0.3;
        keyLight.directionZ = -0.6;
        keyLight.color = Quantity_Color(1.0, 0.95, 0.9, Quantity_TOC_RGB);
        keyLight.intensity = 1.0;
        m_lights.push_back(keyLight);
        
        // Fill light
        LightSettings fillLight;
        fillLight.name = "Fill Light";
        fillLight.type = "directional";
        fillLight.directionX = -0.4;
        fillLight.directionY = 0.2;
        fillLight.directionZ = -0.9;
        fillLight.color = Quantity_Color(0.9, 0.95, 1.0, Quantity_TOC_RGB);
        fillLight.intensity = 0.4;
        m_lights.push_back(fillLight);
        
        // Rim light
        LightSettings rimLight;
        rimLight.name = "Rim Light";
        rimLight.type = "directional";
        rimLight.directionX = 0.2;
        rimLight.directionY = -0.8;
        rimLight.directionZ = 0.6;
        rimLight.color = Quantity_Color(1.0, 1.0, 0.9, Quantity_TOC_RGB);
        rimLight.intensity = 0.6;
        m_lights.push_back(rimLight);
        
        m_environmentSettings.ambientColor = Quantity_Color(0.2, 0.2, 0.25, Quantity_TOC_RGB);
        m_environmentSettings.ambientIntensity = 0.3;
        
    } else if (presetName == "Outdoor") {
        m_lights.clear();
        
        // Sun light
        LightSettings sunLight;
        sunLight.name = "Sun";
        sunLight.type = "directional";
        sunLight.directionX = 0.3;
        sunLight.directionY = 0.7;
        sunLight.directionZ = -0.6;
        sunLight.color = Quantity_Color(1.0, 0.95, 0.8, Quantity_TOC_RGB);
        sunLight.intensity = 1.2;
        m_lights.push_back(sunLight);
        
        // Sky light
        LightSettings skyLight;
        skyLight.name = "Sky";
        skyLight.type = "directional";
        skyLight.directionX = -0.2;
        skyLight.directionY = 0.1;
        skyLight.directionZ = -0.9;
        skyLight.color = Quantity_Color(0.6, 0.8, 1.0, Quantity_TOC_RGB);
        skyLight.intensity = 0.8;
        m_lights.push_back(skyLight);
        
        m_environmentSettings.ambientColor = Quantity_Color(0.4, 0.5, 0.6, Quantity_TOC_RGB);
        m_environmentSettings.ambientIntensity = 0.5;
        
    } else if (presetName == "Dramatic") {
        m_lights.clear();
        
        // Main dramatic light
        LightSettings dramaticLight;
        dramaticLight.name = "Dramatic";
        dramaticLight.type = "directional";
        dramaticLight.directionX = 0.8;
        dramaticLight.directionY = 0.2;
        dramaticLight.directionZ = -0.6;
        dramaticLight.color = Quantity_Color(1.0, 0.9, 0.7, Quantity_TOC_RGB);
        dramaticLight.intensity = 1.5;
        m_lights.push_back(dramaticLight);
        
        // Subtle fill
        LightSettings subtleFill;
        subtleFill.name = "Subtle Fill";
        subtleFill.type = "directional";
        subtleFill.directionX = -0.3;
        subtleFill.directionY = -0.1;
        subtleFill.directionZ = -0.9;
        subtleFill.color = Quantity_Color(0.3, 0.4, 0.6, Quantity_TOC_RGB);
        subtleFill.intensity = 0.2;
        m_lights.push_back(subtleFill);
        
        m_environmentSettings.ambientColor = Quantity_Color(0.1, 0.1, 0.15, Quantity_TOC_RGB);
        m_environmentSettings.ambientIntensity = 0.1;
    }
    
    notifySettingsChanged();
}

void LightingConfig::applyStudioPreset()
{
    m_lights.clear();
    
    // Key light
    LightSettings keyLight;
    keyLight.name = "Key Light";
    keyLight.type = "directional";
    keyLight.directionX = 0.7;
    keyLight.directionY = 0.3;
    keyLight.directionZ = -0.6;
    keyLight.color = Quantity_Color(1.0, 0.95, 0.9, Quantity_TOC_RGB);
    keyLight.intensity = 1.0;
    m_lights.push_back(keyLight);
    
    // Fill light
    LightSettings fillLight;
    fillLight.name = "Fill Light";
    fillLight.type = "directional";
    fillLight.directionX = -0.4;
    fillLight.directionY = 0.2;
    fillLight.directionZ = -0.9;
    fillLight.color = Quantity_Color(0.9, 0.95, 1.0, Quantity_TOC_RGB);
    fillLight.intensity = 0.4;
    m_lights.push_back(fillLight);
    
    // Rim light
    LightSettings rimLight;
    rimLight.name = "Rim Light";
    rimLight.type = "directional";
    rimLight.directionX = 0.2;
    rimLight.directionY = -0.8;
    rimLight.directionZ = 0.6;
    rimLight.color = Quantity_Color(1.0, 1.0, 0.9, Quantity_TOC_RGB);
    rimLight.intensity = 0.6;
    m_lights.push_back(rimLight);
    
    m_environmentSettings.ambientColor = Quantity_Color(0.2, 0.2, 0.25, Quantity_TOC_RGB);
    m_environmentSettings.ambientIntensity = 0.3;
    
    notifySettingsChanged();
}

void LightingConfig::applyOutdoorPreset()
{
    m_lights.clear();
    
    // Sun light
    LightSettings sunLight;
    sunLight.name = "Sun";
    sunLight.type = "directional";
    sunLight.directionX = 0.3;
    sunLight.directionY = 0.7;
    sunLight.directionZ = -0.6;
    sunLight.color = Quantity_Color(1.0, 0.95, 0.8, Quantity_TOC_RGB);
    sunLight.intensity = 1.2;
    m_lights.push_back(sunLight);
    
    // Sky light
    LightSettings skyLight;
    skyLight.name = "Sky";
    skyLight.type = "directional";
    skyLight.directionX = -0.2;
    skyLight.directionY = 0.1;
    skyLight.directionZ = -0.9;
    skyLight.color = Quantity_Color(0.6, 0.8, 1.0, Quantity_TOC_RGB);
    skyLight.intensity = 0.8;
    m_lights.push_back(skyLight);
    
    m_environmentSettings.ambientColor = Quantity_Color(0.4, 0.5, 0.6, Quantity_TOC_RGB);
    m_environmentSettings.ambientIntensity = 0.5;
    
    notifySettingsChanged();
}

void LightingConfig::applyDramaticPreset()
{
    m_lights.clear();
    
    // Main dramatic light
    LightSettings dramaticLight;
    dramaticLight.name = "Dramatic";
    dramaticLight.type = "directional";
    dramaticLight.directionX = 0.8;
    dramaticLight.directionY = 0.2;
    dramaticLight.directionZ = -0.6;
    dramaticLight.color = Quantity_Color(1.0, 0.9, 0.7, Quantity_TOC_RGB);
    dramaticLight.intensity = 1.5;
    m_lights.push_back(dramaticLight);
    
    // Subtle fill
    LightSettings subtleFill;
    subtleFill.name = "Subtle Fill";
    subtleFill.type = "directional";
    subtleFill.directionX = -0.3;
    subtleFill.directionY = -0.1;
    subtleFill.directionZ = -0.9;
    subtleFill.color = Quantity_Color(0.3, 0.4, 0.6, Quantity_TOC_RGB);
    subtleFill.intensity = 0.2;
    m_lights.push_back(subtleFill);
    
    m_environmentSettings.ambientColor = Quantity_Color(0.1, 0.1, 0.15, Quantity_TOC_RGB);
    m_environmentSettings.ambientIntensity = 0.1;
    
    notifySettingsChanged();
}

void LightingConfig::applyWarmPreset()
{
    m_lights.clear();
    
    // Warm main light
    LightSettings warmLight;
    warmLight.name = "Warm Main";
    warmLight.type = "directional";
    warmLight.directionX = 0.5;
    warmLight.directionY = 0.3;
    warmLight.directionZ = -0.8;
    warmLight.color = Quantity_Color(1.0, 0.8, 0.6, Quantity_TOC_RGB);
    warmLight.intensity = 1.0;
    m_lights.push_back(warmLight);
    
    // Warm fill light
    LightSettings warmFill;
    warmFill.name = "Warm Fill";
    warmFill.type = "directional";
    warmFill.directionX = -0.3;
    warmFill.directionY = 0.2;
    warmFill.directionZ = -0.9;
    warmFill.color = Quantity_Color(1.0, 0.9, 0.7, Quantity_TOC_RGB);
    warmFill.intensity = 0.5;
    m_lights.push_back(warmFill);
    
    // Warm back light
    LightSettings warmBack;
    warmBack.name = "Warm Back";
    warmBack.type = "directional";
    warmBack.directionX = 0.2;
    warmBack.directionY = -0.6;
    warmBack.directionZ = 0.8;
    warmBack.color = Quantity_Color(1.0, 0.7, 0.5, Quantity_TOC_RGB);
    warmBack.intensity = 0.4;
    m_lights.push_back(warmBack);
    
    m_environmentSettings.ambientColor = Quantity_Color(0.3, 0.25, 0.2, Quantity_TOC_RGB);
    m_environmentSettings.ambientIntensity = 0.4;
    
    notifySettingsChanged();
}

void LightingConfig::applyCoolPreset()
{
    m_lights.clear();
    
    // Cool main light
    LightSettings coolLight;
    coolLight.name = "Cool Main";
    coolLight.type = "directional";
    coolLight.directionX = 0.6;
    coolLight.directionY = 0.4;
    coolLight.directionZ = -0.7;
    coolLight.color = Quantity_Color(0.8, 0.9, 1.0, Quantity_TOC_RGB);
    coolLight.intensity = 1.0;
    m_lights.push_back(coolLight);
    
    // Cool fill light
    LightSettings coolFill;
    coolFill.name = "Cool Fill";
    coolFill.type = "directional";
    coolFill.directionX = -0.4;
    coolFill.directionY = 0.3;
    coolFill.directionZ = -0.9;
    coolFill.color = Quantity_Color(0.7, 0.8, 1.0, Quantity_TOC_RGB);
    coolFill.intensity = 0.6;
    m_lights.push_back(coolFill);
    
    // Cool back light
    LightSettings coolBack;
    coolBack.name = "Cool Back";
    coolBack.type = "directional";
    coolBack.directionX = 0.3;
    coolBack.directionY = -0.7;
    coolBack.directionZ = 0.6;
    coolBack.color = Quantity_Color(0.6, 0.7, 1.0, Quantity_TOC_RGB);
    coolBack.intensity = 0.3;
    m_lights.push_back(coolBack);
    
    m_environmentSettings.ambientColor = Quantity_Color(0.2, 0.25, 0.3, Quantity_TOC_RGB);
    m_environmentSettings.ambientIntensity = 0.3;
    
    notifySettingsChanged();
}

void LightingConfig::applyMinimalPreset()
{
    m_lights.clear();
    
    // Single main light
    LightSettings mainLight;
    mainLight.name = "Main Light";
    mainLight.type = "directional";
    mainLight.directionX = 0.5;
    mainLight.directionY = 0.5;
    mainLight.directionZ = -1.0;
    mainLight.color = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
    mainLight.intensity = 0.8;
    m_lights.push_back(mainLight);
    
    // Subtle fill
    LightSettings subtleFill;
    subtleFill.name = "Subtle Fill";
    subtleFill.type = "directional";
    subtleFill.directionX = -0.2;
    subtleFill.directionY = 0.1;
    subtleFill.directionZ = -0.9;
    subtleFill.color = Quantity_Color(0.9, 0.9, 0.9, Quantity_TOC_RGB);
    subtleFill.intensity = 0.3;
    m_lights.push_back(subtleFill);
    
    m_environmentSettings.ambientColor = Quantity_Color(0.5, 0.5, 0.5, Quantity_TOC_RGB);
    m_environmentSettings.ambientIntensity = 0.6;
    
    notifySettingsChanged();
}

std::vector<std::string> LightingConfig::getAvailablePresets() const
{
    return {"Studio", "Outdoor", "Dramatic", "Warm", "Cool", "Minimal"};
}

void LightingConfig::resetToDefaults()
{
    initializeDefaultLights();
    notifySettingsChanged();
}

void LightingConfig::applySettingsToScene()
{
    // This method will be called when settings are applied
    // The actual scene update is handled by SceneManager
    // This method serves as a notification point for settings application
    LOG_INF_S("LightingConfig: Applying settings to scene");
    notifySettingsChanged();
}

void LightingConfig::notifySettingsChanged()
{
    for (auto& callback : m_callbacks) {
        callback();
    }
}

void LightingConfig::addSettingsChangedCallback(std::function<void()> callback)
{
    m_callbacks.push_back(callback);
}

std::string LightingConfig::colorToString(const Quantity_Color& color) const
{
    std::ostringstream oss;
    oss << color.Red() << "," << color.Green() << "," << color.Blue();
    return oss.str();
}

Quantity_Color LightingConfig::stringToColor(const std::string& str) const
{
    std::istringstream iss(str);
    std::string token;
    std::vector<double> values;
    
    while (std::getline(iss, token, ',')) {
        values.push_back(std::stod(token));
    }
    
    if (values.size() >= 3) {
        return Quantity_Color(values[0], values[1], values[2], Quantity_TOC_RGB);
    }
    
    return Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
}

std::string LightingConfig::boolToString(bool value) const
{
    return value ? "true" : "false";
}

bool LightingConfig::stringToBool(const std::string& str) const
{
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return (lowerStr == "true" || lowerStr == "1" || lowerStr == "yes");
} 