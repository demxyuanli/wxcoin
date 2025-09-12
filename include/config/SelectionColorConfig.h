#pragma once

#include <string>

struct SelectionColorSettings {
    // Selected geometry diffuse color
    float diffuseR = 1.0f;
    float diffuseG = 1.0f;
    float diffuseB = 0.6f;
    
    // Selected geometry ambient color
    float ambientR = 0.4f;
    float ambientG = 0.4f;
    float ambientB = 0.2f;
    
    // Selected geometry specular color
    float specularR = 1.0f;
    float specularG = 1.0f;
    float specularB = 0.7f;
    
    // Selected geometry emissive color
    float emissiveR = 0.2f;
    float emissiveG = 0.2f;
    float emissiveB = 0.1f;
    
    // Selected geometry transparency
    float transparency = 0.0f;
    
    // Selected geometry shininess
    float shininess = 0.8f;
    
    // Selected outline color
    float outlineR = 1.0f;
    float outlineG = 1.0f;
    float outlineB = 0.6f;
    
    // Selected outline width
    float outlineWidth = 2.0f;
    
    // Selected highlight edge color
    float highlightEdgeR = 1.0f;
    float highlightEdgeG = 1.0f;
    float highlightEdgeB = 0.6f;
};

class SelectionColorConfig {
private:
    static SelectionColorConfig* instance;
    SelectionColorSettings settings;
    bool initialized;

    SelectionColorConfig();
    void parseColorString(const std::string& colorStr, float& r, float& g, float& b);
    std::string getCurrentThemeValue(const std::string& valueStr);

public:
    static SelectionColorConfig& getInstance();
    void initialize(class ConfigManager& configManager);
    bool isInitialized() const { return initialized; }
    
    const SelectionColorSettings& getSettings() const { return settings; }
    
    // Get selected geometry colors
    void getSelectedGeometryDiffuseColor(float& r, float& g, float& b) const;
    void getSelectedGeometryAmbientColor(float& r, float& g, float& b) const;
    void getSelectedGeometrySpecularColor(float& r, float& g, float& b) const;
    void getSelectedGeometryEmissiveColor(float& r, float& g, float& b) const;
    
    // Get selected geometry material properties
    float getSelectedGeometryTransparency() const;
    float getSelectedGeometryShininess() const;
    
    // Get selected outline color and width
    void getSelectedOutlineColor(float& r, float& g, float& b) const;
    float getSelectedOutlineWidth() const;
    
    // Get selected highlight edge color
    void getSelectedHighlightEdgeColor(float& r, float& g, float& b) const;
};
