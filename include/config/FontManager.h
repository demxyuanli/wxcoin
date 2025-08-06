#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <wx/font.h>
#include <wx/string.h>
#include <memory>
#include "config/ConfigManager.h"

// Forward declarations
class wxWindow;

class FontManager {
private:
    static FontManager* instance;
    ConfigManager* configManager;
    
    FontManager();
    ~FontManager();
    
    // Helper methods to convert string to wxFontFamily
    wxFontFamily stringToFontFamily(const wxString& familyStr);
    wxFontStyle stringToFontStyle(const wxString& styleStr);
    wxFontWeight stringToFontWeight(const wxString& weightStr);
    
    // Helper method to create font from config
    wxFont createFontFromConfig(const wxString& prefix);

public:
    static FontManager& getInstance();
    
    // Initialize font manager with config file
    bool initialize(const std::string& configFilePath = "");
    
    // Get fonts for different UI elements
    wxFont getDefaultFont();
    wxFont getTitleFont();
    wxFont getLabelFont();
    wxFont getButtonFont();
    wxFont getTextCtrlFont();
    wxFont getChoiceFont();
    wxFont getStatusFont();
    wxFont getSmallFont();
    wxFont getLargeFont();
    
    // Get font with custom size
    wxFont getFont(const wxString& fontType, int customSize = -1);
    
    // Apply font to wxWindow and its children
    void applyFontToWindow(wxWindow* window, const wxString& fontType);
    void applyFontToWindowAndChildren(wxWindow* window, const wxString& fontType);
    
    // Reload font configurations
    bool reloadConfig();
    
    // Get font information
    wxString getFontInfo(const wxString& fontType);
};

#endif // FONT_MANAGER_H 