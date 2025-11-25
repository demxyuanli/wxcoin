#pragma once

#include <string>

// Color structure for RGB values (0.0-1.0)
struct ColorRGB {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    
    ColorRGB() = default;
    ColorRGB(float red, float green, float blue) : r(red), g(green), b(blue) {}
};

// Highlight settings for a specific selection type (face, edge, vertex)
struct SelectionHighlightSettings {
    // Hover (preselection) colors
    ColorRGB hoverDiffuse;
    ColorRGB hoverAmbient;
    ColorRGB hoverSpecular;
    ColorRGB hoverEmissive;
    float hoverTransparency = 0.3f;
    float hoverShininess = 0.7f;
    
    // Selection colors
    ColorRGB selectionDiffuse;
    ColorRGB selectionAmbient;
    ColorRGB selectionSpecular;
    ColorRGB selectionEmissive;
    float selectionTransparency = 0.2f;
    float selectionShininess = 0.8f;
    
    // Additional properties
    float lineWidth = 3.0f;  // For edges
    float selectionLineWidth = 4.0f;  // For selected edges
    float pointSize = 6.0f;  // For vertices
    float selectionPointSize = 8.0f;  // For selected vertices
};

// Complete highlight configuration
struct SelectionHighlightConfig {
    // Face selection highlight
    SelectionHighlightSettings faceHighlight;
    
    // Edge selection highlight
    SelectionHighlightSettings edgeHighlight;
    ColorRGB edgeColor;  // Normal edge display color
    
    // Vertex selection highlight
    SelectionHighlightSettings vertexHighlight;
    ColorRGB vertexColor;  // Normal vertex display color
    
    // FaceQuery highlight (if needed)
    SelectionHighlightSettings faceQueryHighlight;
    
    SelectionHighlightConfig();
};

class SelectionHighlightConfigManager {
private:
    static SelectionHighlightConfigManager* instance;
    SelectionHighlightConfig config;
    bool initialized;

    SelectionHighlightConfigManager();
    void parseColorString(const std::string& colorStr, float& r, float& g, float& b);
    std::string getCurrentThemeValue(const std::string& valueStr);
    void loadDefaults();

public:
    static SelectionHighlightConfigManager& getInstance();
    void initialize(class ConfigManager& configManager);
    bool isInitialized() const { return initialized; }
    
    const SelectionHighlightConfig& getConfig() const { return config; }
    SelectionHighlightConfig& getConfig() { return config; }
    
    // Save configuration
    void save(class ConfigManager& configManager);
    
    // Get face highlight colors
    const SelectionHighlightSettings& getFaceHighlight() const { return config.faceHighlight; }
    
    // Get edge highlight colors
    const SelectionHighlightSettings& getEdgeHighlight() const { return config.edgeHighlight; }
    const ColorRGB& getEdgeColor() const { return config.edgeColor; }
    
    // Get vertex highlight colors
    const SelectionHighlightSettings& getVertexHighlight() const { return config.vertexHighlight; }
    const ColorRGB& getVertexColor() const { return config.vertexColor; }
    
    // Get face query highlight colors
    const SelectionHighlightSettings& getFaceQueryHighlight() const { return config.faceQueryHighlight; }
};


