#include "config/LocalizationConfig.h"
#include "logger/Logger.h"
#include <iostream>

int main() {
    std::cout << "=== Testing Localization System ===" << std::endl;
    
    // Initialize localization system
    LocalizationConfig& loc = LocalizationConfig::getInstance();
    
    if (!loc.initialize("zh_CN", "config/localization.ini")) {
        std::cout << "Failed to initialize localization system" << std::endl;
        return 1;
    }
    
    std::cout << "Localization system initialized with language: " << loc.getCurrentLanguage() << std::endl;
    
    // Test Chinese text retrieval
    std::cout << "\n=== Testing Chinese Text ===" << std::endl;
    std::cout << "Title: " << loc.getText("MeshQualityDialog/Title") << std::endl;
    std::cout << "Apply: " << loc.getText("MeshQualityDialog/Apply") << std::endl;
    std::cout << "Cancel: " << loc.getText("MeshQualityDialog/Cancel") << std::endl;
    std::cout << "Validate: " << loc.getText("MeshQualityDialog/Validate") << std::endl;
    std::cout << "Export Report: " << loc.getText("MeshQualityDialog/ExportReport") << std::endl;
    
    std::cout << "\nBasic Quality: " << loc.getText("MeshQualityDialog/BasicQuality") << std::endl;
    std::cout << "Mesh Deflection: " << loc.getText("MeshQualityDialog/MeshDeflection") << std::endl;
    std::cout << "Subdivision: " << loc.getText("MeshQualityDialog/Subdivision") << std::endl;
    std::cout << "Smoothing: " << loc.getText("MeshQualityDialog/Smoothing") << std::endl;
    
    // Test English text
    std::cout << "\n=== Testing English Text ===" << std::endl;
    if (loc.setLanguage("en_US")) {
        std::cout << "Title: " << loc.getText("MeshQualityDialog/Title") << std::endl;
        std::cout << "Apply: " << loc.getText("MeshQualityDialog/Apply") << std::endl;
        std::cout << "Cancel: " << loc.getText("MeshQualityDialog/Cancel") << std::endl;
        std::cout << "Validate: " << loc.getText("MeshQualityDialog/Validate") << std::endl;
        std::cout << "Export Report: " << loc.getText("MeshQualityDialog/ExportReport") << std::endl;
        
        std::cout << "\nBasic Quality: " << loc.getText("MeshQualityDialog/BasicQuality") << std::endl;
        std::cout << "Mesh Deflection: " << loc.getText("MeshQualityDialog/MeshDeflection") << std::endl;
        std::cout << "Subdivision: " << loc.getText("MeshQualityDialog/Subdivision") << std::endl;
        std::cout << "Smoothing: " << loc.getText("MeshQualityDialog/Smoothing") << std::endl;
    }
    
    // Test section-based text retrieval
    std::cout << "\n=== Testing Section-based Text ===" << std::endl;
    std::cout << "Settings Applied: " << loc.getText("Messages", "SettingsApplied") << std::endl;
    std::cout << "Validation Success: " << loc.getText("Messages", "ValidationSuccess") << std::endl;
    
    // Test available languages
    std::cout << "\n=== Available Languages ===" << std::endl;
    auto languages = loc.getAvailableLanguages();
    for (const auto& lang : languages) {
        std::cout << "- " << lang << std::endl;
    }
    
    std::cout << "\n=== Localization Test Complete ===" << std::endl;
    return 0;
} 