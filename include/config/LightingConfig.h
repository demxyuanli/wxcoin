#pragma once

#include <OpenCASCADE/Quantity_Color.hxx>
#include <string>
#include <vector>
#include <functional>
#include <memory>

struct LightSettings {
    bool enabled;
    std::string name;
    std::string type;  // "directional", "point", "spot", "ambient"
    
    // Position and direction
    double positionX, positionY, positionZ;
    double directionX, directionY, directionZ;
    
    // Color and intensity
    Quantity_Color color;
    double intensity;
    
    // Spot light specific
    double spotAngle;
    double spotExponent;
    
    // Attenuation
    double constantAttenuation;
    double linearAttenuation;
    double quadraticAttenuation;
    
    // Environment settings
    Quantity_Color ambientColor;
    double ambientIntensity;
    
    LightSettings()
        : enabled(true)
        , name("Main Light")
        , type("directional")
        , positionX(0.0), positionY(0.0), positionZ(0.0)
        , directionX(0.5), directionY(0.5), directionZ(-1.0)
        , color(1.0, 1.0, 1.0, Quantity_TOC_RGB)
        , intensity(1.0)
        , spotAngle(30.0)
        , spotExponent(1.0)
        , constantAttenuation(1.0)
        , linearAttenuation(0.0)
        , quadraticAttenuation(0.0)
        , ambientColor(0.4, 0.4, 0.4, Quantity_TOC_RGB)
        , ambientIntensity(1.0)
    {}
};

class LightingConfig {
public:
    static LightingConfig& getInstance();
    
    // Load and save settings
    bool loadFromFile(const std::string& filename = "");
    bool saveToFile(const std::string& filename = "") const;
    std::string getConfigFilePath() const;
    
    // Light management
    void addLight(const LightSettings& light);
    void removeLight(int index);
    void updateLight(int index, const LightSettings& light);
    const LightSettings& getLight(int index) const;
    std::vector<LightSettings>& getAllLights() { return m_lights; }
    const std::vector<LightSettings>& getAllLights() const { return m_lights; }
    
    // Environment settings
    const LightSettings& getEnvironmentSettings() const { return m_environmentSettings; }
    void setEnvironmentSettings(const LightSettings& settings);
    
    // Individual light setters
    void setLightEnabled(int index, bool enabled);
    void setLightName(int index, const std::string& name);
    void setLightType(int index, const std::string& type);
    void setLightPosition(int index, double x, double y, double z);
    void setLightDirection(int index, double x, double y, double z);
    void setLightColor(int index, const Quantity_Color& color);
    void setLightIntensity(int index, double intensity);
    void setLightSpotAngle(int index, double angle);
    void setLightSpotExponent(int index, double exponent);
    void setLightAttenuation(int index, double constant, double linear, double quadratic);
    
    // Environment setters
    void setEnvironmentAmbientColor(const Quantity_Color& color);
    void setEnvironmentAmbientIntensity(double intensity);
    
    // Preset lighting setups
    void applyPreset(const std::string& presetName);
    void applyStudioPreset();
    void applyOutdoorPreset();
    void applyDramaticPreset();
    void applyWarmPreset();
    void applyCoolPreset();
    void applyMinimalPreset();
    std::vector<std::string> getAvailablePresets() const;
    
    // Reset to defaults
    void resetToDefaults();
    
    // Apply settings to scene
    void applySettingsToScene();
    
    // Notification system
    void notifySettingsChanged();
    void addSettingsChangedCallback(std::function<void()> callback);
    
private:
    LightingConfig();
    ~LightingConfig() = default;
    LightingConfig(const LightingConfig&) = delete;
    LightingConfig& operator=(const LightingConfig&) = delete;
    
    // Helper methods
    std::string colorToString(const Quantity_Color& color) const;
    Quantity_Color stringToColor(const std::string& str) const;
    std::string boolToString(bool value) const;
    bool stringToBool(const std::string& str) const;
    
    // Initialize default lighting setup
    void initializeDefaultLights();
    
    std::vector<LightSettings> m_lights;
    LightSettings m_environmentSettings;
    std::vector<std::function<void()>> m_callbacks;
}; 