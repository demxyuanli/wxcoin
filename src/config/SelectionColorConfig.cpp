#include "config/SelectionColorConfig.h"
#include "config/ConfigManager.h"
#include "config/ThemeManager.h"
#include "logger/Logger.h"
#include <sstream>

SelectionColorConfig* SelectionColorConfig::instance = nullptr;

SelectionColorConfig::SelectionColorConfig() : initialized(false) {
}

SelectionColorConfig& SelectionColorConfig::getInstance() {
    if (!instance) {
        instance = new SelectionColorConfig();
    }
    return *instance;
}

void SelectionColorConfig::initialize(ConfigManager& configManager) {
    if (initialized) {
        LOG_WRN("SelectionColorConfig already initialized", "SelectionColorConfig");
        return;
    }


    try {
        // Get current theme
        std::string currentTheme = ThemeManager::getInstance().getCurrentTheme();

        // Read selected geometry diffuse color
        std::string diffuseColorStr = configManager.getString("GeometrySelectionColors", 
            "SelectedGeometryDiffuseColor", "1.0,1.0,0.6;1.0,1.0,0.6;1.0,1.0,0.6");
        parseColorString(getCurrentThemeValue(diffuseColorStr), settings.diffuseR, settings.diffuseG, settings.diffuseB);

        // Read selected geometry ambient color
        std::string ambientColorStr = configManager.getString("GeometrySelectionColors", 
            "SelectedGeometryAmbientColor", "0.4,0.4,0.2;0.4,0.4,0.2;0.4,0.4,0.2");
        parseColorString(getCurrentThemeValue(ambientColorStr), settings.ambientR, settings.ambientG, settings.ambientB);

        // Read selected geometry specular color
        std::string specularColorStr = configManager.getString("GeometrySelectionColors", 
            "SelectedGeometrySpecularColor", "1.0,1.0,0.7;1.0,1.0,0.7;1.0,1.0,0.7");
        parseColorString(getCurrentThemeValue(specularColorStr), settings.specularR, settings.specularG, settings.specularB);

        // Read selected geometry emissive color
        std::string emissiveColorStr = configManager.getString("GeometrySelectionColors", 
            "SelectedGeometryEmissiveColor", "0.2,0.2,0.1;0.2,0.2,0.1;0.2,0.2,0.1");
        parseColorString(getCurrentThemeValue(emissiveColorStr), settings.emissiveR, settings.emissiveG, settings.emissiveB);

        // Read selected geometry transparency
        std::string transparencyStr = configManager.getString("GeometrySelectionColors", 
            "SelectedGeometryTransparency", "0.0;0.0;0.0");
        settings.transparency = std::stof(getCurrentThemeValue(transparencyStr));

        // Read selected geometry shininess
        std::string shininessStr = configManager.getString("GeometrySelectionColors", 
            "SelectedGeometryShininess", "0.8;0.8;0.8");
        settings.shininess = std::stof(getCurrentThemeValue(shininessStr));

        // Read selected outline color
        std::string outlineColorStr = configManager.getString("GeometrySelectionColors", 
            "SelectedOutlineColor", "1.0,1.0,0.6;1.0,1.0,0.6;1.0,1.0,0.6");
        parseColorString(getCurrentThemeValue(outlineColorStr), settings.outlineR, settings.outlineG, settings.outlineB);

        // Read selected outline width
        std::string outlineWidthStr = configManager.getString("GeometrySelectionColors", 
            "SelectedOutlineWidth", "2.0;2.0;2.0");
        settings.outlineWidth = std::stof(getCurrentThemeValue(outlineWidthStr));

        // Read selected highlight edge color
        std::string highlightEdgeColorStr = configManager.getString("GeometrySelectionColors", 
            "SelectedHighlightEdgeColor", "1.0,1.0,0.6;1.0,1.0,0.6;1.0,1.0,0.6");
        parseColorString(getCurrentThemeValue(highlightEdgeColorStr), settings.highlightEdgeR, settings.highlightEdgeG, settings.highlightEdgeB);

        initialized = true;
        LOG_INF("Selected geometry diffuse color: " + std::to_string(settings.diffuseR) + "," + 
                std::to_string(settings.diffuseG) + "," + std::to_string(settings.diffuseB), "SelectionColorConfig");
    }
    catch (const std::exception& e) {
        LOG_ERR("Failed to initialize SelectionColorConfig: " + std::string(e.what()), "SelectionColorConfig");
        initialized = false;
    }
}

void SelectionColorConfig::parseColorString(const std::string& colorStr, float& r, float& g, float& b) {
    std::istringstream iss(colorStr);
    std::string token;
    
    if (std::getline(iss, token, ',')) r = std::stof(token);
    if (std::getline(iss, token, ',')) g = std::stof(token);
    if (std::getline(iss, token, ',')) b = std::stof(token);
}

std::string SelectionColorConfig::getCurrentThemeValue(const std::string& valueStr) {
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

void SelectionColorConfig::getSelectedGeometryDiffuseColor(float& r, float& g, float& b) const {
    r = settings.diffuseR;
    g = settings.diffuseG;
    b = settings.diffuseB;
}

void SelectionColorConfig::getSelectedGeometryAmbientColor(float& r, float& g, float& b) const {
    r = settings.ambientR;
    g = settings.ambientG;
    b = settings.ambientB;
}

void SelectionColorConfig::getSelectedGeometrySpecularColor(float& r, float& g, float& b) const {
    r = settings.specularR;
    g = settings.specularG;
    b = settings.specularB;
}

void SelectionColorConfig::getSelectedGeometryEmissiveColor(float& r, float& g, float& b) const {
    r = settings.emissiveR;
    g = settings.emissiveG;
    b = settings.emissiveB;
}

float SelectionColorConfig::getSelectedGeometryTransparency() const {
    return settings.transparency;
}

float SelectionColorConfig::getSelectedGeometryShininess() const {
    return settings.shininess;
}

void SelectionColorConfig::getSelectedOutlineColor(float& r, float& g, float& b) const {
    r = settings.outlineR;
    g = settings.outlineG;
    b = settings.outlineB;
}

float SelectionColorConfig::getSelectedOutlineWidth() const {
    return settings.outlineWidth;
}

void SelectionColorConfig::getSelectedHighlightEdgeColor(float& r, float& g, float& b) const {
    r = settings.highlightEdgeR;
    g = settings.highlightEdgeG;
    b = settings.highlightEdgeB;
}
