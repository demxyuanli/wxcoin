#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include "config/ConfigManager.h"
#include <wx/colour.h>
#include <wx/font.h>
#include <string>
#include <map>
#include <vector>

// Theme configuration macros - unified across all files
#define CFG_COLOUR(key) ThemeManager::getInstance().getColour(key)
#define CFG_INT(key) ThemeManager::getInstance().getInt(key)
#define CFG_STRING(key) ThemeManager::getInstance().getString(key)
#define CFG_FONT() ThemeManager::getInstance().getDefaultFont()
#define CFG_FONTNAME() ThemeManager::getInstance().getDefaultFont().GetFaceName()
#define CFG_DEFAULTFONT() ThemeManager::getInstance().getDefaultFont()

struct ThemeProfile {
    std::string name;
    std::string displayName;
    std::map<std::string, wxColour> colours;
    std::map<std::string, int> integers;
    std::map<std::string, std::string> strings;
    wxFont defaultFont;
};

class ThemeManager {
public:
    static ThemeManager& getInstance();
    
    void initialize(ConfigManager& config);
    
    // Theme management
    bool loadTheme(const std::string& themeName);
    std::vector<std::string> getAvailableThemes() const;
    std::string getCurrentTheme() const;
    bool setCurrentTheme(const std::string& themeName);
    
    // Configuration access
    wxColour getColour(const std::string& key) const;
    int getInt(const std::string& key) const;
    std::string getString(const std::string& key) const;
    wxFont getDefaultFont() const;
    
    // Theme creation and management
    bool createTheme(const std::string& themeName, const ThemeProfile& profile);
    bool saveCurrentTheme();
    bool reloadThemes();
    
    // Notification system for theme changes
    void addThemeChangeListener(void* listener, std::function<void()> callback);
    void removeThemeChangeListener(void* listener);
    
private:
    ThemeManager();
    ~ThemeManager();
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;
    
    void loadBuiltinThemes();
    void notifyThemeChange();
    wxColour parseColour(const std::string& value) const;
    ThemeProfile loadThemeFromConfig(const std::string& themeName);
    std::vector<std::string> splitString(const std::string& str, char delimiter);
    void loadSizeConfigurations(ThemeProfile& theme);
    wxFont loadFont();
    
    ConfigManager* m_configManager;
    std::string m_currentTheme;
    std::map<std::string, ThemeProfile> m_themes;
    std::map<void*, std::function<void()>> m_listeners;
    bool m_initialized;
};

#endif // THEME_MANAGER_H 