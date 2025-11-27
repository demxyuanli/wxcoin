#include "config/UnifiedConfigManager.h"
#include "config/ConfigManager.h"
#include "config/ThemeManager.h"
#include "config/RenderingConfig.h"
#include "config/LightingConfig.h"
#include "config/SelectionHighlightConfig.h"
#include "config/SelectionColorConfig.h"
#include "config/EdgeSettingsConfig.h"
#include "config/LoggerConfig.h"
#include "config/FontManager.h"
#include "logger/Logger.h"
#include <sstream>
#include <algorithm>

UnifiedConfigManager& UnifiedConfigManager::getInstance() {
    static UnifiedConfigManager instance;
    return instance;
}

UnifiedConfigManager::UnifiedConfigManager() 
    : m_configManager(nullptr) {
}

UnifiedConfigManager::~UnifiedConfigManager() {
}

void UnifiedConfigManager::initialize(ConfigManager& configManager) {
    m_configManager = &configManager;

    registerBuiltinCategories();
    scanAndRegisterAllConfigs(configManager);

}

void UnifiedConfigManager::registerBuiltinCategories() {

    addCategory("System", "System", "settings");
    addCategory("Appearance", "Appearance", "palette");
    addCategory("Rendering", "Rendering", "render");
    addCategory("Lighting", "Lighting", "light");
    addCategory("Edge Settings", "Edge Settings", "edge");
    addCategory("Render Preview", "Render Preview", "preview");
    addCategory("Navigation", "Navigation", "navi");
    addCategory("UI Components", "UI Components", "ui");


    addCategory("Layout", "Layout & Sizes", "size");
    addCategory("Dock Layout", "Dock Layout", "layout");
    addCategory("Typography", "Typography", "font");


    addCategory("Selection", "Selection", "select");
    addCategory("Compatibility", "Compatibility", "compat");

    addCategory("General", "General", "settings");
}

void UnifiedConfigManager::addCategory(const std::string& id, const std::string& displayName, const std::string& icon) {
    ConfigCategory cat;
    cat.id = id;
    cat.displayName = displayName;
    cat.icon = icon;
    m_categories[id] = cat;
}

void UnifiedConfigManager::scanAdditionalConfigFiles() {
    // List of additional config files to scan
    std::vector<std::string> additionalFiles = {
        "config/lighting_settings.ini",
        "config/rendering_presets.ini",
        "config/rendering_settings.ini"
    };

    for (const auto& configFile : additionalFiles) {
        try {
            wxFileConfig additionalConfig(wxEmptyString, wxEmptyString, configFile, wxEmptyString, wxCONFIG_USE_LOCAL_FILE);

            // Save current path to restore later
            wxString oldPath = additionalConfig.GetPath();
            additionalConfig.SetPath("/");

            // Get all sections from this file
            wxString sectionStr;
            long sectionIndex;
            bool hasSections = additionalConfig.GetFirstGroup(sectionStr, sectionIndex);

            while (hasSections) {
                std::string sectionName = sectionStr.ToStdString();
                std::string category = determineCategoryFromSection(sectionName);

                // Create category if it doesn't exist
                if (m_categories.find(category) == m_categories.end()) {
                    addCategory(category, category, "settings");
                }

                // Set path to section
                additionalConfig.SetPath("/" + sectionStr);

                // Get all keys in this section
                wxString keyStr;
                long keyIndex;
                bool hasKeys = additionalConfig.GetFirstEntry(keyStr, keyIndex);
                int keyCount = 0;

                while (hasKeys) {
                    std::string keyName = keyStr.ToStdString();
                    std::string fullKey = sectionName + "." + keyName;
                    std::string value = additionalConfig.Read(keyStr, "").ToStdString();

                    ConfigItem item;
                    item.key = fullKey;
                    item.displayName = keyName;
                    item.description = "Configuration item from " + configFile + " [" + sectionName + "]";
                    item.section = sectionName;
                    item.category = category;
                    item.currentValue = value;
                    item.defaultValue = value;

                    // Determine value type
                    item.type = determineValueType(value, keyName);

                    m_items[fullKey] = item;
                    m_categories[category].items.push_back(fullKey);

                    keyCount++;
                    hasKeys = additionalConfig.GetNextEntry(keyStr, keyIndex);
                }


                // Restore path and get next section
                additionalConfig.SetPath("/");
                hasSections = additionalConfig.GetNextGroup(sectionStr, sectionIndex);
            }


        } catch (const std::exception& e) {
            LOG_WRN("Failed to scan additional config file '" + configFile + "': " + e.what(), "UnifiedConfigManager");
        } catch (...) {
            LOG_WRN("Failed to scan additional config file '" + configFile + "' (unknown error)", "UnifiedConfigManager");
        }
    }
}

void UnifiedConfigManager::scanAndRegisterAllConfigs(ConfigManager& configManager) {
    // Scan main config file
    registerConfigManagerItems(configManager);

    // Scan additional config files
    scanAdditionalConfigFiles();

    // Register specialized config items
    registerThemeConfigItems();
    registerRenderingConfigItems();
    registerLightingConfigItems();
    registerSelectionConfigItems();
    registerEdgeConfigItems();
    registerLoggerConfigItems();
    registerFontConfigItems();
}

void UnifiedConfigManager::registerConfigManagerItems(ConfigManager& configManager) {
    auto sections = configManager.getSections();
    for (const auto& section : sections) {
        // Determine category for this section
        std::string category = determineCategoryFromSection(section);
        
        // Create category if it doesn't exist
        if (m_categories.find(category) == m_categories.end()) {
            addCategory(category, section, "settings");
        }
        
        auto keys = configManager.getKeys(section);
        for (const auto& key : keys) {
            ConfigItem item;
            item.key = section + "." + key;
            item.displayName = key;
            item.description = "Configuration item from " + section + " section";
            item.section = section;
            item.category = category;
            
            std::string value = configManager.getString(section, key, "");
            item.currentValue = value;
            item.defaultValue = value;
            
            // Determine value type
            item.type = determineValueType(value, key);

            // Prepare lowerKey for special handling
            std::string lowerKey = key;
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

            // Set min/max for numeric types and enhance descriptions
            if (item.type == ConfigValueType::Int) {
                item.minValue = 0;
                item.maxValue = 10000;

                // Special ranges for specific keys
                if (lowerKey.find("fontsize") != std::string::npos) {
                    item.minValue = 6;
                    item.maxValue = 72;
                    item.description = "Font size in points";
                } else if (lowerKey.find("size") != std::string::npos ||
                          lowerKey.find("width") != std::string::npos ||
                          lowerKey.find("height") != std::string::npos) {
                    item.minValue = 0;
                    item.maxValue = 10000;
                    item.description = "Size in pixels";
                } else if (lowerKey.find("margin") != std::string::npos ||
                          lowerKey.find("padding") != std::string::npos ||
                          lowerKey.find("spacing") != std::string::npos) {
                    item.minValue = 0;
                    item.maxValue = 100;
                    item.description = "Spacing in pixels";
                }
            } else if (item.type == ConfigValueType::Double) {
                item.minValue = 0.0;
                item.maxValue = 10000.0;

                // Special ranges for specific keys
                if (lowerKey.find("colorr") != std::string::npos ||
                    lowerKey.find("colorg") != std::string::npos ||
                    lowerKey.find("colorb") != std::string::npos) {
                    item.minValue = 0.0;
                    item.maxValue = 1.0;
                    item.description = "Color component value (0.0-1.0)";
                } else if (lowerKey.find("transparency") != std::string::npos ||
                          lowerKey.find("alpha") != std::string::npos) {
                    item.minValue = 0.0;
                    item.maxValue = 1.0;
                    item.description = "Transparency value (0.0=opaque, 1.0=transparent)";
                } else if (lowerKey.find("intensity") != std::string::npos) {
                    item.minValue = 0.0;
                    item.maxValue = 2.0;
                    item.description = "Intensity multiplier";
                }
            }

            // Special handling for color values
            if (item.type == ConfigValueType::Color) {
                // Extract first color value if it's multi-theme format
                size_t semicolonPos = value.find(';');
                if (semicolonPos != std::string::npos) {
                    item.currentValue = value.substr(0, semicolonPos);
                    item.defaultValue = value.substr(0, semicolonPos);
                }
                item.description = "Color value in RGB format (r,g,b)";
            }

            // Enhance descriptions for specific keys
            if (lowerKey.find("position") != std::string::npos) {
                item.description = "Window position (Center or x,y coordinates)";
            } else if (lowerKey.find("framesize") != std::string::npos) {
                item.description = "Window size in pixels (width,height)";
            } else if (lowerKey.find("title") != std::string::npos) {
                item.description = "Window title text";
            } else if (lowerKey.find("loglevel") != std::string::npos) {
                item.description = "Minimum log level to display";
            }
            
            m_items[item.key] = item;
            m_categories[category].items.push_back(item.key);
        }
    }
}

std::string UnifiedConfigManager::determineCategoryFromSection(const std::string& section) const {
    // Map section names to logical categories based on functionality

    // === Appearance and Theme ===
    if (section == "Theme" || section == "ThemeColors" || section == "SvgTheme") {
        return "Appearance";
    }

    // === System and Application ===
    else if (section == "Logger" || section == "SplashScreen") {
        return "System";
    }
    
    // === General (Application-wide settings) ===
    else if (section == "MainApplication") {
        return "General";
    }

    // === Typography ===
    else if (section == "Font") {
        return "Typography";
    }

    // === Dock Layout ===
    else if (section == "DockLayout") {
        return "Dock Layout";
    }
    
    // === Layout and Sizes ===
    else if (section.find("Size") != std::string::npos ||
             section == "BarSizes" || section == "ButtonBarSizes" || section == "PanelSizes" ||
             section == "DockingSizes" || section == "FrameSizes" || section == "GallerySizes" ||
             section == "HomeSpace" || section == "HomeMenu" || section == "ScrollBar" ||
             section == "Separators" || section == "Icons") {
        return "Layout";
    }

    // === UI Components ===
    else if (section == "DockArea" || section == "ActBar" || section == "ButtonBar") {
        return "UI Components";
    }

    // === Navigation ===
    else if (section == "DragOverlay" || section == "NavigationCube" || section == "RotationCenter") {
        return "Navigation";
    }

    // === Rendering ===
    else if (section == "Canvas" || section == "GeometrySelectionColors" ||
             section == "Material" || section == "Lighting" || section == "Texture" ||
             section.find("Rendering") != std::string::npos ||
             section.find("Preset") != std::string::npos ||
             section.find("AntiAliasing") != std::string::npos ||
             section.find("Performance") != std::string::npos ||
             section.find("Quality") != std::string::npos ||
             section.find("Feature") != std::string::npos ||
             section.find("User_Preference") != std::string::npos) {
        return "Rendering";
    }

    // === Lighting ===
    else if (section == "Environment" || section.find("Light") != std::string::npos) {
        return "Lighting";
    }

    // === Compatibility ===
    else if (section == "FlatUIConstants") {
        return "Compatibility";
    }

    // === Default ===
    else {
        return "General";
    }
}

ConfigValueType UnifiedConfigManager::determineValueType(const std::string& value, const std::string& key) const {
    if (value.empty()) {
        return ConfigValueType::String;
    }

    // Check for boolean values
    std::string lowerValue = value;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);
    if (lowerValue == "true" || lowerValue == "false" || lowerValue == "1" || lowerValue == "0") {
        return ConfigValueType::Bool;
    }

    std::string lowerKey = key;
    std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

    // Special handling based on key patterns and usage patterns

    // 1. Color-related keys (RGB components)
    if (lowerKey.find("colorr") != std::string::npos ||
        lowerKey.find("colorg") != std::string::npos ||
        lowerKey.find("colorb") != std::string::npos ||
        lowerKey.find("colourr") != std::string::npos ||
        lowerKey.find("colourg") != std::string::npos ||
        lowerKey.find("colourb") != std::string::npos) {
        return ConfigValueType::Double;
    }

    // 2. Color keys (full RGB values)
    if (lowerKey.find("color") != std::string::npos ||
        lowerKey.find("colour") != std::string::npos ||
        key.find("Color") != std::string::npos ||
        key.find("Colour") != std::string::npos) {
        // Try to parse as color (r,g,b format)
        try {
            std::string firstPart = value.substr(0, value.find(';'));
            std::istringstream iss(firstPart);
            std::string token;
            int count = 0;
            while (std::getline(iss, token, ',')) {
                count++;
                (void)std::stod(token); // Try to parse as number
            }
            if (count == 3) {
                return ConfigValueType::Color;
            }
        } catch (...) {
            // Not a valid color, continue
        }
    }

    // 3. Size/dimension keys - check for size pairs first
    if (lowerKey.find("framesize") != std::string::npos ||
        (lowerKey.find("size") != std::string::npos && value.find(',') != std::string::npos)) {
        // Check if it's a size pair like "1200,700"
        size_t commaPos = value.find(',');
        if (commaPos != std::string::npos) {
            // Try to parse both parts as integers
            try {
                std::string part1 = value.substr(0, commaPos);
                std::string part2 = value.substr(commaPos + 1);
                (void)std::stoi(part1);
                (void)std::stoi(part2);
                return ConfigValueType::Size;
            } catch (...) {
                // Not a valid size pair, continue
            }
        }
    }

    if (lowerKey.find("size") != std::string::npos ||
        lowerKey.find("width") != std::string::npos ||
        lowerKey.find("height") != std::string::npos ||
        lowerKey.find("radius") != std::string::npos ||
        lowerKey.find("margin") != std::string::npos ||
        lowerKey.find("padding") != std::string::npos ||
        lowerKey.find("spacing") != std::string::npos ||
        lowerKey.find("thickness") != std::string::npos ||
        lowerKey.find("border") != std::string::npos) {
        // Try to parse as integer first
        try {
            size_t pos = 0;
            (void)std::stoi(value, &pos);
            if (pos == value.length()) {
                return ConfigValueType::Int;
            }
        } catch (...) {
        }
        // If not integer, try double
        try {
            size_t pos = 0;
            (void)std::stod(value, &pos);
            if (pos == value.length()) {
                return ConfigValueType::Double;
            }
        } catch (...) {
        }
    }

    // 4. Position/coordinate keys
    if (lowerKey.find("position") != std::string::npos ||
        lowerKey.find("pos") != std::string::npos ||
        lowerKey.find("coordinate") != std::string::npos ||
        lowerKey.find("coord") != std::string::npos) {
        // Usually stored as strings like "Center" or coordinates like "1200,700"
        return ConfigValueType::String;
    }

    // 5. Font-related keys
    if (lowerKey.find("fontfamily") != std::string::npos ||
        lowerKey.find("fontstyle") != std::string::npos ||
        lowerKey.find("fontweight") != std::string::npos) {
        // Check if value is a wxFont enum constant
        if (value.find("wxFONT") != std::string::npos || 
            value.find("wxFONT") == 0) {
            return ConfigValueType::Enum;
        }
        // If it's a numeric value, treat as Int
        try {
            size_t pos = 0;
            (void)std::stoi(value, &pos);
            if (pos == value.length()) {
                return ConfigValueType::Int;
            }
        } catch (...) {
            // Not a number, treat as Enum
            return ConfigValueType::Enum;
        }
    }
    if (lowerKey.find("fontsize") != std::string::npos) {
        // Font size is always an integer
        return ConfigValueType::Int;
    }
    if (lowerKey.find("fontfacename") != std::string::npos ||
        lowerKey.find("fontname") != std::string::npos) {
        // Font face name is always a string
        return ConfigValueType::String;
    }

    // 6. Title/text keys
    if (lowerKey.find("title") != std::string::npos ||
        lowerKey.find("text") != std::string::npos ||
        lowerKey.find("label") != std::string::npos ||
        lowerKey.find("message") != std::string::npos ||
        lowerKey.find("name") != std::string::npos) {
        return ConfigValueType::String;
    }

    // 7. Level/enum keys
    if (lowerKey.find("level") != std::string::npos ||
        lowerKey.find("mode") != std::string::npos ||
        lowerKey.find("style") != std::string::npos ||
        lowerKey.find("type") != std::string::npos) {
        // Check if it matches known enum values
        if (lowerValue == "debug" || lowerValue == "dbg" || lowerValue == "inf" || lowerValue == "info" ||
            lowerValue == "wrn" || lowerValue == "warning" || lowerValue == "err" || lowerValue == "error") {
            return ConfigValueType::Enum;
        }
        // Other common enum values
        if (lowerValue == "solid" || lowerValue == "wireframe" || lowerValue == "points" ||
            lowerValue == "normal" || lowerValue == "high" || lowerValue == "low" ||
            lowerValue == "default" || lowerValue == "dark" || lowerValue == "blue" ||
            lowerValue == "smooth" || lowerValue == "flat") {
            return ConfigValueType::Enum;
        }
    }

    // 8. Path/directory keys
    if (lowerKey.find("path") != std::string::npos ||
        lowerKey.find("dir") != std::string::npos ||
        lowerKey.find("directory") != std::string::npos ||
        lowerKey.find("file") != std::string::npos) {
        return ConfigValueType::String;
    }

    // 9. Transparency/alpha keys
    if (lowerKey.find("transparency") != std::string::npos ||
        lowerKey.find("alpha") != std::string::npos ||
        lowerKey.find("opacity") != std::string::npos) {
        return ConfigValueType::Double;
    }

    // 10. Intensity/strength keys
    if (lowerKey.find("intensity") != std::string::npos ||
        lowerKey.find("strength") != std::string::npos ||
        lowerKey.find("brightness") != std::string::npos ||
        lowerKey.find("contrast") != std::string::npos) {
        return ConfigValueType::Double;
    }

    // Try to parse as integer
    try {
        size_t pos = 0;
        (void)std::stoi(value, &pos);
        if (pos == value.length()) {
            return ConfigValueType::Int;
        }
    } catch (...) {
    }

    // Try to parse as double
    try {
        size_t pos = 0;
        (void)std::stod(value, &pos);
        if (pos == value.length()) {
            return ConfigValueType::Double;
        }
    } catch (...) {
    }

    // Default to string
    return ConfigValueType::String;
}

void UnifiedConfigManager::registerThemeConfigItems() {
    if (!m_configManager) return;

    // Theme.CurrentTheme is already registered by registerConfigManagerItems
    // But we need to enhance it with enum values
    std::string themeKey = "Theme.CurrentTheme";
    auto it = m_items.find(themeKey);
    if (it != m_items.end()) {
        it->second.type = ConfigValueType::Enum;
        it->second.enumValues = {"default", "dark", "blue"};
        it->second.description = "Select the active theme (default/dark/blue)";
    }
}

void UnifiedConfigManager::registerRenderingConfigItems() {
    try {
        RenderingConfig& rc = RenderingConfig::getInstance();

        // Display Mode
        ConfigItem displayMode;
        displayMode.key = "Rendering.DisplayMode";
        displayMode.displayName = "Display Mode";
        displayMode.description = "Rendering display mode (Solid/Wireframe/Points)";
        displayMode.category = "Rendering";
        displayMode.type = ConfigValueType::Enum;
        displayMode.enumValues = {"Solid", "Wireframe", "Points"};
        displayMode.currentValue = "Solid";
        displayMode.defaultValue = "Solid";
        m_items[displayMode.key] = displayMode;
        m_categories["Rendering"].items.push_back(displayMode.key);

        // Shading Mode
        ConfigItem shadingMode;
        shadingMode.key = "Rendering.ShadingMode";
        shadingMode.displayName = "Shading Mode";
        shadingMode.description = "Shading algorithm (Smooth/Flat)";
        shadingMode.category = "Rendering";
        shadingMode.type = ConfigValueType::Enum;
        shadingMode.enumValues = {"Smooth", "Flat"};
        shadingMode.currentValue = "Smooth";
        shadingMode.defaultValue = "Smooth";
        m_items[shadingMode.key] = shadingMode;
        m_categories["Rendering"].items.push_back(shadingMode.key);

        // Quality Mode
        ConfigItem quality;
        quality.key = "Rendering.Quality";
        quality.displayName = "Rendering Quality";
        quality.description = "Rendering quality preset (Normal/High/Low)";
        quality.category = "Rendering";
        quality.type = ConfigValueType::Enum;
        quality.enumValues = {"Normal", "High", "Low"};
        quality.currentValue = "Normal";
        quality.defaultValue = "Normal";
        m_items[quality.key] = quality;
        m_categories["Rendering"].items.push_back(quality.key);
    } catch (...) {
        LOG_WRN("Failed to register rendering config items", "UnifiedConfigManager");
    }
}

void UnifiedConfigManager::registerLightingConfigItems() {
    try {
        LightingConfig& lc = LightingConfig::getInstance();
        
        ConfigItem ambientIntensity;
        ambientIntensity.key = "Lighting.AmbientIntensity";
        ambientIntensity.displayName = "Ambient Intensity";
        ambientIntensity.description = "Ambient light intensity";
        ambientIntensity.category = "Lighting";
        ambientIntensity.type = ConfigValueType::Double;
        ambientIntensity.minValue = 0.0;
        ambientIntensity.maxValue = 2.0;
        ambientIntensity.currentValue = std::to_string(lc.getEnvironmentSettings().ambientIntensity);
        ambientIntensity.defaultValue = "0.6";
        m_items[ambientIntensity.key] = ambientIntensity;
        m_categories["Lighting"].items.push_back(ambientIntensity.key);
    } catch (...) {
        LOG_WRN("Failed to register lighting config items", "UnifiedConfigManager");
    }
}

void UnifiedConfigManager::registerSelectionConfigItems() {
    try {
        SelectionHighlightConfigManager& shc = SelectionHighlightConfigManager::getInstance();
        
        ConfigItem faceHoverColor;
        faceHoverColor.key = "Selection.FaceHoverColor";
        faceHoverColor.displayName = "Face Hover Color";
        faceHoverColor.description = "Color for face hover highlight";
        faceHoverColor.category = "Selection";
        faceHoverColor.type = ConfigValueType::Color;
        std::ostringstream oss;
        oss << shc.getConfig().faceHighlight.hoverDiffuse.r << ","
            << shc.getConfig().faceHighlight.hoverDiffuse.g << ","
            << shc.getConfig().faceHighlight.hoverDiffuse.b;
        faceHoverColor.currentValue = oss.str();
        faceHoverColor.defaultValue = "1.0,0.8,0.2";
        m_items[faceHoverColor.key] = faceHoverColor;
        m_categories["Selection"].items.push_back(faceHoverColor.key);
        
        ConfigItem faceSelectionColor;
        faceSelectionColor.key = "Selection.FaceSelectionColor";
        faceSelectionColor.displayName = "Face Selection Color";
        faceSelectionColor.description = "Color for face selection highlight";
        faceSelectionColor.category = "Selection";
        faceSelectionColor.type = ConfigValueType::Color;
        std::ostringstream oss2;
        oss2 << shc.getConfig().faceHighlight.selectionDiffuse.r << ","
             << shc.getConfig().faceHighlight.selectionDiffuse.g << ","
             << shc.getConfig().faceHighlight.selectionDiffuse.b;
        faceSelectionColor.currentValue = oss2.str();
        faceSelectionColor.defaultValue = "0.2,0.8,1.0";
        m_items[faceSelectionColor.key] = faceSelectionColor;
        m_categories["Selection"].items.push_back(faceSelectionColor.key);
    } catch (...) {
        LOG_WRN("Failed to register selection config items", "UnifiedConfigManager");
    }
}

void UnifiedConfigManager::registerEdgeConfigItems() {
    // Edge config items are now registered automatically from config file
    // This method can be used to add additional edge-related items if needed
}

void UnifiedConfigManager::registerLoggerConfigItems() {
    // Logger config items are now registered automatically from config file
    // Enhance LogLevel with enum values if it exists
    std::string logLevelKey = "Logger.LogLevel";
    auto it = m_items.find(logLevelKey);
    if (it != m_items.end()) {
        it->second.type = ConfigValueType::Enum;
        it->second.enumValues = {"DEBUG", "DBG", "INF", "INFO", "WRN", "WARNING", "ERR", "ERROR"};
        it->second.description = "Minimum log level to display (DEBUG/INFO/WARNING/ERROR)";
    }
}

void UnifiedConfigManager::registerFontConfigItems() {
    // Font config items are now registered automatically from config file
    // Enhance font size items with min/max values
    std::vector<std::string> fontSizeKeys = {
        "Font.DefaultFontSize", "Font.TitleFontSize", "Font.LabelFontSize",
        "Font.ButtonFontSize", "Font.TextCtrlFontSize", "Font.ChoiceFontSize",
        "Font.StatusFontSize", "Font.SmallFontSize", "Font.LargeFontSize",
        "Font.DockingTabFontSize", "Font.DockingTitleFontSize", "Font.DockingSystemButtonFontSize"
    };
    
    for (const auto& key : fontSizeKeys) {
        auto it = m_items.find(key);
        if (it != m_items.end() && it->second.type == ConfigValueType::Int) {
            it->second.minValue = 6;
            it->second.maxValue = 72;
            it->second.description = "Font size in points";
        }
    }

    // Enhance font family items with enum values
    std::vector<std::string> fontFamilyKeys = {
        "Font.DefaultFontFamily", "Font.TitleFontFamily", "Font.LabelFontFamily",
        "Font.ButtonFontFamily", "Font.TextCtrlFontFamily", "Font.ChoiceFontFamily",
        "Font.StatusFontFamily", "Font.SmallFontFamily", "Font.LargeFontFamily",
        "Font.DockingTabFontFamily", "Font.DockingTitleFontFamily", "Font.DockingSystemButtonFontFamily"
    };
    
    std::vector<std::string> fontFamilyValues = {
        "wxFONTFAMILY_DEFAULT", "wxFONTFAMILY_DECORATIVE", "wxFONTFAMILY_ROMAN",
        "wxFONTFAMILY_SCRIPT", "wxFONTFAMILY_SWISS", "wxFONTFAMILY_MODERN",
        "wxFONTFAMILY_TELETYPE", "wxFONTFAMILY_UNKNOWN"
    };

    for (const auto& key : fontFamilyKeys) {
        auto it = m_items.find(key);
        if (it != m_items.end() && it->second.type == ConfigValueType::Enum) {
            it->second.enumValues = fontFamilyValues;
            it->second.description = "Font family type";
        }
    }

    // Enhance font style items with enum values
    std::vector<std::string> fontStyleKeys = {
        "Font.DefaultFontStyle", "Font.TitleFontStyle", "Font.LabelFontStyle",
        "Font.ButtonFontStyle", "Font.TextCtrlFontStyle", "Font.ChoiceFontStyle",
        "Font.StatusFontStyle", "Font.SmallFontStyle", "Font.LargeFontStyle",
        "Font.DockingTabFontStyle", "Font.DockingTitleFontStyle", "Font.DockingSystemButtonFontStyle"
    };
    
    std::vector<std::string> fontStyleValues = {
        "wxFONTSTYLE_NORMAL", "wxFONTSTYLE_ITALIC", "wxFONTSTYLE_SLANT", "wxFONTSTYLE_MAX"
    };

    for (const auto& key : fontStyleKeys) {
        auto it = m_items.find(key);
        if (it != m_items.end() && it->second.type == ConfigValueType::Enum) {
            it->second.enumValues = fontStyleValues;
            it->second.description = "Font style";
        }
    }

    // Enhance font weight items with enum values
    std::vector<std::string> fontWeightKeys = {
        "Font.DefaultFontWeight", "Font.TitleFontWeight", "Font.LabelFontWeight",
        "Font.ButtonFontWeight", "Font.TextCtrlFontWeight", "Font.ChoiceFontWeight",
        "Font.StatusFontWeight", "Font.SmallFontWeight", "Font.LargeFontWeight",
        "Font.DockingTabFontWeight", "Font.DockingTitleFontWeight", "Font.DockingSystemButtonFontWeight"
    };
    
    std::vector<std::string> fontWeightValues = {
        "wxFONTWEIGHT_NORMAL", "wxFONTWEIGHT_LIGHT", "wxFONTWEIGHT_BOLD",
        "wxFONTWEIGHT_THIN", "wxFONTWEIGHT_EXTRALIGHT", "wxFONTWEIGHT_MEDIUM",
        "wxFONTWEIGHT_SEMIBOLD", "wxFONTWEIGHT_EXTRABOLD", "wxFONTWEIGHT_HEAVY",
        "wxFONTWEIGHT_EXTRAHEAVY", "wxFONTWEIGHT_INVALID", "wxFONTWEIGHT_MAX"
    };

    for (const auto& key : fontWeightKeys) {
        auto it = m_items.find(key);
        if (it != m_items.end() && it->second.type == ConfigValueType::Enum) {
            it->second.enumValues = fontWeightValues;
            it->second.description = "Font weight";
        }
    }
}

void UnifiedConfigManager::registerConfigItem(const ConfigItem& item) {
    m_items[item.key] = item;
    if (m_categories.find(item.category) != m_categories.end()) {
        m_categories[item.category].items.push_back(item.key);
    }
}

void UnifiedConfigManager::registerCategory(const ConfigCategory& category) {
    m_categories[category.id] = category;
}

std::vector<ConfigCategory> UnifiedConfigManager::getCategories() const {
    std::vector<ConfigCategory> result;
    for (const auto& pair : m_categories) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<ConfigItem> UnifiedConfigManager::getItemsForCategory(const std::string& categoryId) const {
    std::vector<ConfigItem> result;
    auto it = m_categories.find(categoryId);
    if (it != m_categories.end()) {
        for (const auto& key : it->second.items) {
            auto itemIt = m_items.find(key);
            if (itemIt != m_items.end()) {
                result.push_back(itemIt->second);
            }
        }
    }
    return result;
}

ConfigItem* UnifiedConfigManager::getItem(const std::string& key) {
    auto it = m_items.find(key);
    if (it != m_items.end()) {
        return &it->second;
    }
    return nullptr;
}

bool UnifiedConfigManager::setValue(const std::string& key, const std::string& value) {
    auto it = m_items.find(key);
    if (it == m_items.end()) {
        return false;
    }
    
    std::string errorMsg;
    if (!validateValue(key, value, errorMsg)) {
        LOG_ERR("Invalid value for " + key + ": " + errorMsg, "UnifiedConfigManager");
        return false;
    }
    
    auto conflicts = checkConflicts(key, value);
    if (!conflicts.empty()) {
        LOG_WRN("Conflicts detected for " + key, "UnifiedConfigManager");
    }
    
    it->second.currentValue = value;
    
    if (m_configManager) {
        size_t dotPos = key.find('.');
        if (dotPos != std::string::npos) {
            std::string section = key.substr(0, dotPos);
            std::string itemKey = key.substr(dotPos + 1);
            
            if (it->second.type == ConfigValueType::Bool) {
                m_configManager->setBool(section, itemKey, value == "true");
            } else if (it->second.type == ConfigValueType::Int) {
                m_configManager->setInt(section, itemKey, std::stoi(value));
            } else if (it->second.type == ConfigValueType::Double) {
                m_configManager->setDouble(section, itemKey, std::stod(value));
            } else {
                m_configManager->setString(section, itemKey, value);
            }
        }
    }
    
    auto listenerIt = m_listeners.find(key);
    if (listenerIt != m_listeners.end()) {
        for (auto& listener : listenerIt->second) {
            listener(value);
        }
    }
    
    return true;
}

std::string UnifiedConfigManager::getValue(const std::string& key) const {
    auto it = m_items.find(key);
    if (it != m_items.end()) {
        return it->second.currentValue;
    }
    return "";
}

bool UnifiedConfigManager::validateValue(const std::string& key, const std::string& value, std::string& errorMsg) const {
    auto it = m_items.find(key);
    if (it == m_items.end()) {
        errorMsg = "Unknown config key";
        return false;
    }
    
    const ConfigItem& item = it->second;
    
    if (item.type == ConfigValueType::Bool) {
        if (value != "true" && value != "false") {
            errorMsg = "Must be true or false";
            return false;
        }
    } else if (item.type == ConfigValueType::Int) {
        try {
            int val = std::stoi(value);
            if (val < item.minValue || val > item.maxValue) {
                errorMsg = "Value out of range [" + std::to_string((int)item.minValue) + 
                          ", " + std::to_string((int)item.maxValue) + "]";
                return false;
            }
        } catch (...) {
            errorMsg = "Invalid integer value";
            return false;
        }
    } else if (item.type == ConfigValueType::Double) {
        try {
            double val = std::stod(value);
            if (val < item.minValue || val > item.maxValue) {
                errorMsg = "Value out of range [" + std::to_string(item.minValue) + 
                          ", " + std::to_string(item.maxValue) + "]";
                return false;
            }
        } catch (...) {
            errorMsg = "Invalid double value";
            return false;
        }
    } else if (item.type == ConfigValueType::Enum) {
        if (std::find(item.enumValues.begin(), item.enumValues.end(), value) == item.enumValues.end()) {
            errorMsg = "Invalid enum value";
            return false;
        }
    } else if (item.type == ConfigValueType::Size) {
        // Validate size pair format like "width,height"
        size_t commaPos = value.find(',');
        if (commaPos == std::string::npos) {
            errorMsg = "Size must be in format 'width,height'";
            return false;
        }

        std::string widthStr = value.substr(0, commaPos);
        std::string heightStr = value.substr(commaPos + 1);

        try {
            int width = std::stoi(widthStr);
            int height = std::stoi(heightStr);
            if (width < 0 || height < 0) {
                errorMsg = "Size values must be non-negative";
                return false;
            }
        } catch (...) {
            errorMsg = "Invalid size format";
            return false;
        }
    }
    
    if (item.validator) {
        if (!item.validator(value, errorMsg)) {
            return false;
        }
    }
    
    return true;
}

std::vector<std::string> UnifiedConfigManager::checkConflicts(const std::string& key, const std::string& value) const {
    std::vector<std::string> conflicts;
    
    auto it = m_items.find(key);
    if (it == m_items.end()) {
        return conflicts;
    }
    
    const ConfigItem& item = it->second;
    
    for (const auto& conflictKey : item.conflicts) {
        auto conflictIt = m_items.find(conflictKey);
        if (conflictIt != m_items.end()) {
            if (conflictIt->second.currentValue == value) {
                conflicts.push_back(conflictKey);
            }
        }
    }
    
    return conflicts;
}

void UnifiedConfigManager::save() {
    if (m_configManager) {
        m_configManager->save();
    }
}

void UnifiedConfigManager::reload() {
    if (m_configManager) {
        m_configManager->reload();
        scanAndRegisterAllConfigs(*m_configManager);
    }
}

void UnifiedConfigManager::addChangeListener(const std::string& key, std::function<void(const std::string&)> listener) {
    m_listeners[key].push_back(listener);
}

void UnifiedConfigManager::removeChangeListener(const std::string& key, std::function<void(const std::string&)> listener) {
    // Note: Since function objects cannot be directly compared,
    // we remove all listeners for the given key when a listener is provided.
    // For more precise control, consider using a listener ID system.
    auto it = m_listeners.find(key);
    if (it != m_listeners.end()) {
        // Remove all listeners for this key
        // In a production system, you might want to use listener IDs for precise removal
        m_listeners.erase(it);
    }
}

void UnifiedConfigManager::printDiagnostics() const {
}

