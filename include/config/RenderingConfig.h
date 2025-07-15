#pragma once

#include <OpenCASCADE/Quantity_Color.hxx>
#include <string>

class RenderingConfig
{
public:
    struct MaterialSettings {
        Quantity_Color ambientColor;
        Quantity_Color diffuseColor;
        Quantity_Color specularColor;
        double shininess;
        double transparency;
        
        MaterialSettings() 
            : ambientColor(0.2, 0.2, 0.2, Quantity_TOC_RGB)
            , diffuseColor(0.68, 0.85, 1.0, Quantity_TOC_RGB)  // Light green as before
            , specularColor(0.9, 0.9, 0.9, Quantity_TOC_RGB)
            , shininess(30.0)
            , transparency(0.0)
        {}
    };
    
    struct LightingSettings {
        Quantity_Color ambientColor;
        Quantity_Color diffuseColor;
        Quantity_Color specularColor;
        double intensity;
        double ambientIntensity;
        
        LightingSettings()
            : ambientColor(0.3, 0.3, 0.3, Quantity_TOC_RGB)
            , diffuseColor(1.0, 1.0, 1.0, Quantity_TOC_RGB)
            , specularColor(1.0, 1.0, 1.0, Quantity_TOC_RGB)
            , intensity(0.8)
            , ambientIntensity(0.3)
        {}
    };
    
    struct TextureSettings {
        Quantity_Color color;
        double intensity;
        bool enabled;
        
        TextureSettings()
            : color(1.0, 1.0, 1.0, Quantity_TOC_RGB)
            , intensity(0.5)
            , enabled(false)
        {}
    };

public:
    static RenderingConfig& getInstance();
    
    // Load/Save configuration
    bool loadFromFile(const std::string& filename = "");
    bool saveToFile(const std::string& filename = "") const;
    
    // Getters
    const MaterialSettings& getMaterialSettings() const { return m_materialSettings; }
    const LightingSettings& getLightingSettings() const { return m_lightingSettings; }
    const TextureSettings& getTextureSettings() const { return m_textureSettings; }
    
    // Setters
    void setMaterialSettings(const MaterialSettings& settings);
    void setLightingSettings(const LightingSettings& settings);
    void setTextureSettings(const TextureSettings& settings);
    
    // Individual property setters
    void setMaterialAmbientColor(const Quantity_Color& color);
    void setMaterialDiffuseColor(const Quantity_Color& color);
    void setMaterialSpecularColor(const Quantity_Color& color);
    void setMaterialShininess(double shininess);
    void setMaterialTransparency(double transparency);
    
    void setLightAmbientColor(const Quantity_Color& color);
    void setLightDiffuseColor(const Quantity_Color& color);
    void setLightSpecularColor(const Quantity_Color& color);
    void setLightIntensity(double intensity);
    void setLightAmbientIntensity(double intensity);
    
    void setTextureColor(const Quantity_Color& color);
    void setTextureIntensity(double intensity);
    void setTextureEnabled(bool enabled);
    
    // Reset to defaults
    void resetToDefaults();

private:
    RenderingConfig();
    ~RenderingConfig() = default;
    RenderingConfig(const RenderingConfig&) = delete;
    RenderingConfig& operator=(const RenderingConfig&) = delete;
    
    std::string getConfigFilePath() const;
    Quantity_Color parseColor(const std::string& value, const Quantity_Color& defaultValue) const;
    std::string colorToString(const Quantity_Color& color) const;
    
    MaterialSettings m_materialSettings;
    LightingSettings m_lightingSettings;
    TextureSettings m_textureSettings;
    
    bool m_autoSave;
}; 