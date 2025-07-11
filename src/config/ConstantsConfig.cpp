#include "config/ConstantsConfig.h"
#include "config/ConfigManager.h"
#include <sstream>

#include <wx/wx.h>
#include <wx/settings.h>

ConstantsConfig& ConstantsConfig::getInstance() {
    static ConstantsConfig instance;
    return instance;
}

ConstantsConfig::ConstantsConfig()
    : defaultFontFaceName(""),
      defaultFontSize(8)
{
}

void ConstantsConfig::initialize(ConfigManager& config) {
    auto readColour = [&](const std::string& key) {
        std::string val = config.getString("FlatUIConstants", key, "");
        if (!val.empty()) {
            std::istringstream ss(val);
            int r, g, b;
            char comma;
            if (ss >> r >> comma >> g >> comma >> b) {
                return wxColour(r, g, b);
            }
        }
        return wxColour(255, 0, 0); 
    };


    std::string face = config.getString("FlatUIConstants", "DefaultFontFaceName", "");
    defaultFontFaceName = wxString(face);

    defaultFontSize = config.getInt("FlatUIConstants", "DefaultFontSize", 8);

    {
        auto keys = config.getKeys("FlatUIConstants");
        for (const auto& key : keys) {
            configMap[key] = config.getString("FlatUIConstants", key, "");
        }
    }
}


const wxString& ConstantsConfig::getDefaultFontFaceName() const {
    return defaultFontFaceName;
}

int ConstantsConfig::getDefaultFontSize() const {
    return defaultFontSize;
}

std::string ConstantsConfig::getStringValue(const std::string& key) const {
    auto it = configMap.find(key);
    if (it != configMap.end() && !it->second.empty()) return it->second;
    return "";
}

int ConstantsConfig::getIntValue(const std::string& key) const {
    auto it = configMap.find(key);
    if (it != configMap.end()) {
        try { return std::stoi(it->second); } catch (...) {}
    }
    return -1;
}

double ConstantsConfig::getDoubleValue(const std::string& key) const {
    auto it = configMap.find(key);
    if (it != configMap.end()) {
        try { return std::stod(it->second); } catch (...) {}
    }
    return -1.0;
}

wxColour ConstantsConfig::getColourValue(const std::string& key) const {
    std::string val = getStringValue(key);
    if (!val.empty()) {
        std::istringstream ss(val);
        int r, g, b;
        char comma;
        if (ss >> r >> comma >> g >> comma >> b) {
            return wxColour(r, g, b);
        }
    }
    return wxColour(255, 0, 0);
}

wxFont ConstantsConfig::getDefaultFont() const {
    // Get font family from config
    wxFontFamily family = wxFONTFAMILY_DEFAULT;
    std::string familyStr = getStringValue("DefaultFontFamily");
    if (familyStr == "wxFONTFAMILY_TELETYPE") {
        family = wxFONTFAMILY_TELETYPE;
    }
    else if (familyStr == "wxFONTFAMILY_SWISS") {
        family = wxFONTFAMILY_SWISS;
    }
    else if (familyStr == "wxFONTFAMILY_ROMAN") {
        family = wxFONTFAMILY_ROMAN;
    }
    else if (familyStr == "wxFONTFAMILY_MODERN") {
        family = wxFONTFAMILY_MODERN;
    }

    // Get font style from config
    wxFontStyle style = wxFONTSTYLE_NORMAL;
    std::string styleStr = getStringValue("DefaultFontStyle");
    if (styleStr == "wxFONTSTYLE_ITALIC") {
        style = wxFONTSTYLE_ITALIC;
    }
    else if (styleStr == "wxFONTSTYLE_SLANT") {
        style = wxFONTSTYLE_SLANT;
    }

    // Get font weight from config
    wxFontWeight weight = wxFONTWEIGHT_NORMAL;
    std::string weightStr = getStringValue("DefaultFontWeight");
    if (weightStr == "wxFONTWEIGHT_BOLD") {
        weight = wxFONTWEIGHT_BOLD;
    }
    else if (weightStr == "wxFONTWEIGHT_LIGHT") {
        weight = wxFONTWEIGHT_LIGHT;
    }

    // Create font with all configured parameters
    wxFont font(defaultFontSize, family, style, weight, false, defaultFontFaceName);

    // Fallback to system font if the configured font is not valid
    if (!font.IsOk()) {
        font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
        font.SetPointSize(defaultFontSize); // At least use the configured size
        if (!defaultFontFaceName.IsEmpty()) {
            font.SetFaceName(defaultFontFaceName); // Try to set the face name
        }
    }

    return font;
}
