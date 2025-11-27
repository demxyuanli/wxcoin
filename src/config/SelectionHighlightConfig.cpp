#include "config/SelectionHighlightConfig.h"
#include "config/ConfigManager.h"
#include "config/ThemeManager.h"
#include "logger/Logger.h"
#include <sstream>

SelectionHighlightConfigManager* SelectionHighlightConfigManager::instance = nullptr;

SelectionHighlightConfig::SelectionHighlightConfig() {
    // Face highlight defaults
    // Hover: yellow-orange
    faceHighlight.hoverDiffuse = ColorRGB(1.0f, 0.8f, 0.2f);
    faceHighlight.hoverAmbient = ColorRGB(0.5f, 0.4f, 0.1f);
    faceHighlight.hoverSpecular = ColorRGB(1.0f, 0.9f, 0.3f);
    faceHighlight.hoverEmissive = ColorRGB(0.3f, 0.2f, 0.05f);
    faceHighlight.hoverTransparency = 0.3f;
    faceHighlight.hoverShininess = 0.7f;
    
    // Selection: cyan-blue
    faceHighlight.selectionDiffuse = ColorRGB(0.2f, 0.8f, 1.0f);
    faceHighlight.selectionAmbient = ColorRGB(0.1f, 0.4f, 0.5f);
    faceHighlight.selectionSpecular = ColorRGB(0.3f, 0.9f, 1.0f);
    faceHighlight.selectionEmissive = ColorRGB(0.1f, 0.3f, 0.4f);
    faceHighlight.selectionTransparency = 0.2f;
    faceHighlight.selectionShininess = 0.8f;
    
    // Edge highlight defaults
    // Hover: yellow-orange
    edgeHighlight.hoverDiffuse = ColorRGB(1.0f, 0.8f, 0.2f);
    edgeHighlight.hoverAmbient = ColorRGB(0.5f, 0.4f, 0.1f);
    edgeHighlight.hoverSpecular = ColorRGB(1.0f, 0.9f, 0.3f);
    edgeHighlight.hoverEmissive = ColorRGB(0.3f, 0.2f, 0.05f);
    edgeHighlight.lineWidth = 3.0f;
    
    // Selection: cyan-blue
    edgeHighlight.selectionDiffuse = ColorRGB(0.2f, 0.8f, 1.0f);
    edgeHighlight.selectionAmbient = ColorRGB(0.1f, 0.4f, 0.5f);
    edgeHighlight.selectionSpecular = ColorRGB(0.3f, 0.9f, 1.0f);
    edgeHighlight.selectionEmissive = ColorRGB(0.1f, 0.3f, 0.4f);
    edgeHighlight.selectionLineWidth = 4.0f;
    
    // Edge color: black
    edgeColor = ColorRGB(0.0f, 0.0f, 0.0f);
    
    // Vertex highlight defaults
    // Hover: yellow-orange
    vertexHighlight.hoverDiffuse = ColorRGB(1.0f, 0.8f, 0.2f);
    vertexHighlight.hoverAmbient = ColorRGB(0.5f, 0.4f, 0.1f);
    vertexHighlight.hoverSpecular = ColorRGB(1.0f, 0.9f, 0.3f);
    vertexHighlight.hoverEmissive = ColorRGB(0.3f, 0.2f, 0.05f);
    vertexHighlight.pointSize = 6.0f;
    
    // Selection: cyan-blue
    vertexHighlight.selectionDiffuse = ColorRGB(0.2f, 0.8f, 1.0f);
    vertexHighlight.selectionAmbient = ColorRGB(0.1f, 0.4f, 0.5f);
    vertexHighlight.selectionSpecular = ColorRGB(0.3f, 0.9f, 1.0f);
    vertexHighlight.selectionEmissive = ColorRGB(0.1f, 0.3f, 0.4f);
    vertexHighlight.selectionPointSize = 8.0f;
    
    // Vertex color: red
    vertexColor = ColorRGB(1.0f, 0.0f, 0.0f);
    
    // FaceQuery highlight defaults (same as face)
    faceQueryHighlight = faceHighlight;
}

SelectionHighlightConfigManager::SelectionHighlightConfigManager() : initialized(false) {
    loadDefaults();
}

SelectionHighlightConfigManager& SelectionHighlightConfigManager::getInstance() {
    if (!instance) {
        instance = new SelectionHighlightConfigManager();
    }
    return *instance;
}

void SelectionHighlightConfigManager::loadDefaults() {
    // Face highlight defaults
    // Hover: yellow-orange
    config.faceHighlight.hoverDiffuse = ColorRGB(1.0f, 0.8f, 0.2f);
    config.faceHighlight.hoverAmbient = ColorRGB(0.5f, 0.4f, 0.1f);
    config.faceHighlight.hoverSpecular = ColorRGB(1.0f, 0.9f, 0.3f);
    config.faceHighlight.hoverEmissive = ColorRGB(0.3f, 0.2f, 0.05f);
    config.faceHighlight.hoverTransparency = 0.3f;
    config.faceHighlight.hoverShininess = 0.7f;
    
    // Selection: cyan-blue
    config.faceHighlight.selectionDiffuse = ColorRGB(0.2f, 0.8f, 1.0f);
    config.faceHighlight.selectionAmbient = ColorRGB(0.1f, 0.4f, 0.5f);
    config.faceHighlight.selectionSpecular = ColorRGB(0.3f, 0.9f, 1.0f);
    config.faceHighlight.selectionEmissive = ColorRGB(0.1f, 0.3f, 0.4f);
    config.faceHighlight.selectionTransparency = 0.2f;
    config.faceHighlight.selectionShininess = 0.8f;
    
    // Edge highlight defaults
    // Hover: yellow-orange
    config.edgeHighlight.hoverDiffuse = ColorRGB(1.0f, 0.8f, 0.2f);
    config.edgeHighlight.hoverAmbient = ColorRGB(0.5f, 0.4f, 0.1f);
    config.edgeHighlight.hoverSpecular = ColorRGB(1.0f, 0.9f, 0.3f);
    config.edgeHighlight.hoverEmissive = ColorRGB(0.3f, 0.2f, 0.05f);
    config.edgeHighlight.lineWidth = 3.0f;
    
    // Selection: cyan-blue
    config.edgeHighlight.selectionDiffuse = ColorRGB(0.2f, 0.8f, 1.0f);
    config.edgeHighlight.selectionAmbient = ColorRGB(0.1f, 0.4f, 0.5f);
    config.edgeHighlight.selectionSpecular = ColorRGB(0.3f, 0.9f, 1.0f);
    config.edgeHighlight.selectionEmissive = ColorRGB(0.1f, 0.3f, 0.4f);
    config.edgeHighlight.selectionLineWidth = 4.0f;
    
    // Edge color: black
    config.edgeColor = ColorRGB(0.0f, 0.0f, 0.0f);
    
    // Vertex highlight defaults
    // Hover: yellow-orange
    config.vertexHighlight.hoverDiffuse = ColorRGB(1.0f, 0.8f, 0.2f);
    config.vertexHighlight.hoverAmbient = ColorRGB(0.5f, 0.4f, 0.1f);
    config.vertexHighlight.hoverSpecular = ColorRGB(1.0f, 0.9f, 0.3f);
    config.vertexHighlight.hoverEmissive = ColorRGB(0.3f, 0.2f, 0.05f);
    config.vertexHighlight.pointSize = 6.0f;
    
    // Selection: cyan-blue
    config.vertexHighlight.selectionDiffuse = ColorRGB(0.2f, 0.8f, 1.0f);
    config.vertexHighlight.selectionAmbient = ColorRGB(0.1f, 0.4f, 0.5f);
    config.vertexHighlight.selectionSpecular = ColorRGB(0.3f, 0.9f, 1.0f);
    config.vertexHighlight.selectionEmissive = ColorRGB(0.1f, 0.3f, 0.4f);
    config.vertexHighlight.selectionPointSize = 8.0f;
    
    // Vertex color: red
    config.vertexColor = ColorRGB(1.0f, 0.0f, 0.0f);
    
    // FaceQuery highlight defaults (same as face)
    config.faceQueryHighlight = config.faceHighlight;
}

void SelectionHighlightConfigManager::initialize(ConfigManager& configManager) {
    if (initialized) {
        LOG_WRN("SelectionHighlightConfigManager already initialized", "SelectionHighlightConfigManager");
        return;
    }


    try {
        // Load face highlight settings
        std::string faceHoverDiffuseStr = configManager.getString("SelectionHighlight", 
            "FaceHoverDiffuseColor", "1.0,0.8,0.2");
        parseColorString(faceHoverDiffuseStr, config.faceHighlight.hoverDiffuse.r, 
            config.faceHighlight.hoverDiffuse.g, config.faceHighlight.hoverDiffuse.b);
        
        std::string faceSelectionDiffuseStr = configManager.getString("SelectionHighlight", 
            "FaceSelectionDiffuseColor", "0.2,0.8,1.0");
        parseColorString(faceSelectionDiffuseStr, config.faceHighlight.selectionDiffuse.r, 
            config.faceHighlight.selectionDiffuse.g, config.faceHighlight.selectionDiffuse.b);
        
        config.faceHighlight.hoverTransparency = static_cast<float>(configManager.getDouble("SelectionHighlight", 
            "FaceHoverTransparency", 0.3));
        config.faceHighlight.selectionTransparency = static_cast<float>(configManager.getDouble("SelectionHighlight", 
            "FaceSelectionTransparency", 0.2));
        
        // Load edge highlight settings
        std::string edgeHoverDiffuseStr = configManager.getString("SelectionHighlight", 
            "EdgeHoverDiffuseColor", "1.0,0.8,0.2");
        parseColorString(edgeHoverDiffuseStr, config.edgeHighlight.hoverDiffuse.r, 
            config.edgeHighlight.hoverDiffuse.g, config.edgeHighlight.hoverDiffuse.b);
        
        std::string edgeSelectionDiffuseStr = configManager.getString("SelectionHighlight", 
            "EdgeSelectionDiffuseColor", "0.2,0.8,1.0");
        parseColorString(edgeSelectionDiffuseStr, config.edgeHighlight.selectionDiffuse.r, 
            config.edgeHighlight.selectionDiffuse.g, config.edgeHighlight.selectionDiffuse.b);
        
        config.edgeHighlight.lineWidth = static_cast<float>(configManager.getDouble("SelectionHighlight", 
            "EdgeHoverLineWidth", 3.0));
        config.edgeHighlight.selectionLineWidth = static_cast<float>(configManager.getDouble("SelectionHighlight", 
            "EdgeSelectionLineWidth", 4.0));
        
        std::string edgeColorStr = configManager.getString("SelectionHighlight", 
            "EdgeColor", "0.0,0.0,0.0");
        parseColorString(edgeColorStr, config.edgeColor.r, config.edgeColor.g, config.edgeColor.b);
        
        // Load vertex highlight settings
        std::string vertexHoverDiffuseStr = configManager.getString("SelectionHighlight", 
            "VertexHoverDiffuseColor", "1.0,0.8,0.2");
        parseColorString(vertexHoverDiffuseStr, config.vertexHighlight.hoverDiffuse.r, 
            config.vertexHighlight.hoverDiffuse.g, config.vertexHighlight.hoverDiffuse.b);
        
        std::string vertexSelectionDiffuseStr = configManager.getString("SelectionHighlight", 
            "VertexSelectionDiffuseColor", "0.2,0.8,1.0");
        parseColorString(vertexSelectionDiffuseStr, config.vertexHighlight.selectionDiffuse.r, 
            config.vertexHighlight.selectionDiffuse.g, config.vertexHighlight.selectionDiffuse.b);
        
        config.vertexHighlight.pointSize = static_cast<float>(configManager.getDouble("SelectionHighlight", 
            "VertexHoverPointSize", 6.0));
        config.vertexHighlight.selectionPointSize = static_cast<float>(configManager.getDouble("SelectionHighlight", 
            "VertexSelectionPointSize", 8.0));
        
        std::string vertexColorStr = configManager.getString("SelectionHighlight", 
            "VertexColor", "1.0,0.0,0.0");
        parseColorString(vertexColorStr, config.vertexColor.r, config.vertexColor.g, config.vertexColor.b);
        
        initialized = true;
    }
    catch (const std::exception& e) {
        LOG_ERR("Failed to initialize SelectionHighlightConfigManager: " + std::string(e.what()), 
            "SelectionHighlightConfigManager");
        initialized = false;
    }
}

void SelectionHighlightConfigManager::save(ConfigManager& configManager) {
    if (!initialized) {
        LOG_WRN("SelectionHighlightConfigManager not initialized, cannot save", "SelectionHighlightConfigManager");
        return;
    }

    try {
        // Save face highlight settings
        std::string faceHoverDiffuse = std::to_string(config.faceHighlight.hoverDiffuse.r) + "," +
            std::to_string(config.faceHighlight.hoverDiffuse.g) + "," +
            std::to_string(config.faceHighlight.hoverDiffuse.b);
        configManager.setString("SelectionHighlight", "FaceHoverDiffuseColor", faceHoverDiffuse);
        
        std::string faceSelectionDiffuse = std::to_string(config.faceHighlight.selectionDiffuse.r) + "," +
            std::to_string(config.faceHighlight.selectionDiffuse.g) + "," +
            std::to_string(config.faceHighlight.selectionDiffuse.b);
        configManager.setString("SelectionHighlight", "FaceSelectionDiffuseColor", faceSelectionDiffuse);
        
        configManager.setDouble("SelectionHighlight", "FaceHoverTransparency", config.faceHighlight.hoverTransparency);
        configManager.setDouble("SelectionHighlight", "FaceSelectionTransparency", config.faceHighlight.selectionTransparency);
        
        // Save edge highlight settings
        std::string edgeHoverDiffuse = std::to_string(config.edgeHighlight.hoverDiffuse.r) + "," +
            std::to_string(config.edgeHighlight.hoverDiffuse.g) + "," +
            std::to_string(config.edgeHighlight.hoverDiffuse.b);
        configManager.setString("SelectionHighlight", "EdgeHoverDiffuseColor", edgeHoverDiffuse);
        
        std::string edgeSelectionDiffuse = std::to_string(config.edgeHighlight.selectionDiffuse.r) + "," +
            std::to_string(config.edgeHighlight.selectionDiffuse.g) + "," +
            std::to_string(config.edgeHighlight.selectionDiffuse.b);
        configManager.setString("SelectionHighlight", "EdgeSelectionDiffuseColor", edgeSelectionDiffuse);
        
        configManager.setDouble("SelectionHighlight", "EdgeHoverLineWidth", config.edgeHighlight.lineWidth);
        configManager.setDouble("SelectionHighlight", "EdgeSelectionLineWidth", config.edgeHighlight.selectionLineWidth);
        
        std::string edgeColor = std::to_string(config.edgeColor.r) + "," +
            std::to_string(config.edgeColor.g) + "," +
            std::to_string(config.edgeColor.b);
        configManager.setString("SelectionHighlight", "EdgeColor", edgeColor);
        
        // Save vertex highlight settings
        std::string vertexHoverDiffuse = std::to_string(config.vertexHighlight.hoverDiffuse.r) + "," +
            std::to_string(config.vertexHighlight.hoverDiffuse.g) + "," +
            std::to_string(config.vertexHighlight.hoverDiffuse.b);
        configManager.setString("SelectionHighlight", "VertexHoverDiffuseColor", vertexHoverDiffuse);
        
        std::string vertexSelectionDiffuse = std::to_string(config.vertexHighlight.selectionDiffuse.r) + "," +
            std::to_string(config.vertexHighlight.selectionDiffuse.g) + "," +
            std::to_string(config.vertexHighlight.selectionDiffuse.b);
        configManager.setString("SelectionHighlight", "VertexSelectionDiffuseColor", vertexSelectionDiffuse);
        
        configManager.setDouble("SelectionHighlight", "VertexHoverPointSize", config.vertexHighlight.pointSize);
        configManager.setDouble("SelectionHighlight", "VertexSelectionPointSize", config.vertexHighlight.selectionPointSize);
        
        std::string vertexColor = std::to_string(config.vertexColor.r) + "," +
            std::to_string(config.vertexColor.g) + "," +
            std::to_string(config.vertexColor.b);
        configManager.setString("SelectionHighlight", "VertexColor", vertexColor);
        
    }
    catch (const std::exception& e) {
        LOG_ERR("Failed to save SelectionHighlightConfigManager: " + std::string(e.what()), 
            "SelectionHighlightConfigManager");
    }
}

void SelectionHighlightConfigManager::parseColorString(const std::string& colorStr, float& r, float& g, float& b) {
    std::istringstream iss(colorStr);
    std::string token;
    
    if (std::getline(iss, token, ',')) r = std::stof(token);
    if (std::getline(iss, token, ',')) g = std::stof(token);
    if (std::getline(iss, token, ',')) b = std::stof(token);
}

std::string SelectionHighlightConfigManager::getCurrentThemeValue(const std::string& valueStr) {
    std::istringstream iss(valueStr);
    std::string token;
    std::vector<std::string> values;
    
    while (std::getline(iss, token, ';')) {
        values.push_back(token);
    }
    
    // Select corresponding value based on current theme
    std::string currentTheme = ThemeManager::getInstance().getCurrentTheme();
    
    if (currentTheme == "dark" && values.size() >= 2) {
        return values[1]; // dark theme
    }
    else if (currentTheme == "blue" && values.size() >= 3) {
        return values[2]; // blue theme
    }
    else {
        return values[0]; // default theme
    }
}

