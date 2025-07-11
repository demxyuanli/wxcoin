#ifndef CONSTANTS_CONFIG_H
#define CONSTANTS_CONFIG_H

#include "config/ConfigManager.h"
#include <wx/colour.h>
#include <wx/string.h>
#include <wx/wx.h>
#include <map>
#include <vector>
#include <string>

class ConstantsConfig {
public:
    static ConstantsConfig& getInstance();

    void initialize(ConfigManager& config);
    const wxString& getDefaultFontFaceName() const;
    int getDefaultFontSize() const;

    std::string getStringValue(const std::string& key) const;
    int getIntValue(const std::string& key) const;
    double getDoubleValue(const std::string& key) const;
    wxColour getColourValue(const std::string& key) const;
    wxFont getDefaultFont() const;

private:
    ConstantsConfig();
    ConstantsConfig(const ConstantsConfig&) = delete;
    ConstantsConfig& operator=(const ConstantsConfig&) = delete;

    wxString defaultFontFaceName;
    int defaultFontSize;

    std::map<std::string, std::string> configMap;
};

#endif // CONSTANTS_CONFIG_H 