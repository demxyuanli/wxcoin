#include <iostream>
#include <wx/wx.h>
#include <wx/filename.h>
#include "config/LocalizationConfig.h"

int main()
{
    // Initialize wxWidgets
    wxInitializer initializer;
    if (!initializer.IsOk()) {
        std::cerr << "Failed to initialize wxWidgets" << std::endl;
        return 1;
    }

    std::cout << "=== Localization Debug Test ===" << std::endl;

    // Check if config files exist
    std::cout << "\nChecking config files..." << std::endl;
    if (wxFileExists("config/zh_CN.ini")) {
        std::cout << "✓ config/zh_CN.ini exists" << std::endl;
    } else {
        std::cout << "✗ config/zh_CN.ini not found" << std::endl;
    }
    
    if (wxFileExists("config/en_US.ini")) {
        std::cout << "✓ config/en_US.ini exists" << std::endl;
    } else {
        std::cout << "✗ config/en_US.ini not found" << std::endl;
    }

    // Initialize localization
    std::cout << "\nInitializing localization..." << std::endl;
    LocalizationConfig& loc = LocalizationConfig::getInstance();
    if (!loc.initialize("zh_CN", "config")) {
        std::cerr << "✗ Failed to initialize localization system" << std::endl;
        return 1;
    }
    std::cout << "✓ Localization initialized" << std::endl;
    std::cout << "Current language: " << loc.getCurrentLanguage() << std::endl;

    // Test text retrieval
    std::cout << "\nTesting text retrieval..." << std::endl;
    
    std::string title = loc.getText("RenderingSettingsDialog", "Title", "DEFAULT_TITLE");
    std::cout << "RenderingSettingsDialog Title: '" << title << "'" << std::endl;
    
    std::string apply = loc.getText("RenderingSettingsDialog", "Apply", "DEFAULT_APPLY");
    std::cout << "RenderingSettingsDialog Apply: '" << apply << "'" << std::endl;
    
    std::string material = loc.getText("RenderingSettingsDialog", "Material", "DEFAULT_MATERIAL");
    std::cout << "RenderingSettingsDialog Material: '" << material << "'" << std::endl;

    // Test with L macro
    std::cout << "\nTesting L macro..." << std::endl;
    std::string macroTitle = L("RenderingSettingsDialog/Title");
    std::cout << "L macro Title: '" << macroTitle << "'" << std::endl;

    // List available languages
    std::cout << "\nAvailable languages:" << std::endl;
    auto languages = loc.getAvailableLanguages();
    for (const auto& lang : languages) {
        std::cout << "  - " << lang << std::endl;
    }

    std::cout << "\n=== Test completed ===" << std::endl;
    return 0;
} 