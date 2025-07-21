#ifndef LOCALIZATION_CONFIG_H
#define LOCALIZATION_CONFIG_H

#include <string>
#include <map>
#include <memory>
#include <wx/fileconf.h>
#include "ConfigManager.h"

/**
 * @brief Localization configuration manager
 * 
 * Handles loading and accessing localized text from configuration files.
 * Supports multiple languages with fallback to English.
 */
class LocalizationConfig {
public:
    /**
     * @brief Get singleton instance
     * @return LocalizationConfig instance
     */
    static LocalizationConfig& getInstance();

    /**
     * @brief Initialize localization system
     * @param language Language code (e.g., "zh_CN", "en_US")
     * @param configPath Path to localization config file
     * @return true if initialization successful
     */
    bool initialize(const std::string& language = "zh_CN", 
                   const std::string& configPath = "config/localization.ini");

    /**
     * @brief Get localized text
     * @param key Text key identifier
     * @param defaultValue Default text if key not found
     * @return Localized text string
     */
    std::string getText(const std::string& key, const std::string& defaultValue = "");

    /**
     * @brief Get localized text with section
     * @param section Configuration section
     * @param key Text key identifier
     * @param defaultValue Default text if key not found
     * @return Localized text string
     */
    std::string getText(const std::string& section, const std::string& key, 
                       const std::string& defaultValue = "");

    /**
     * @brief Set localized text
     * @param key Text key identifier
     * @param value Localized text value
     */
    void setText(const std::string& key, const std::string& value);

    /**
     * @brief Set localized text with section
     * @param section Configuration section
     * @param key Text key identifier
     * @param value Localized text value
     */
    void setText(const std::string& section, const std::string& key, const std::string& value);

    /**
     * @brief Get current language
     * @return Current language code
     */
    std::string getCurrentLanguage() const { return m_currentLanguage; }

    /**
     * @brief Set language
     * @param language Language code
     * @return true if language change successful
     */
    bool setLanguage(const std::string& language);

    /**
     * @brief Check if localization is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Save localization settings
     * @return true if save successful
     */
    bool save();

    /**
     * @brief Reload localization settings
     * @return true if reload successful
     */
    bool reload();

    /**
     * @brief Get all available languages
     * @return Vector of language codes
     */
    std::vector<std::string> getAvailableLanguages() const;

    /**
     * @brief Get configuration file path
     * @return Configuration file path
     */
    std::string getConfigFilePath() const { return m_configFilePath; }

private:
    LocalizationConfig();
    ~LocalizationConfig();
    LocalizationConfig(const LocalizationConfig&) = delete;
    LocalizationConfig& operator=(const LocalizationConfig&) = delete;

    bool m_initialized;
    std::string m_currentLanguage;
    std::string m_configFilePath;
    std::unique_ptr<wxFileConfig> m_fileConfig;
    std::map<std::string, std::string> m_textCache;

    bool loadLanguageFile(const std::string& language);
    std::string findLanguageFile(const std::string& language);
    std::string createDefaultChineseConfig();
    std::string createDefaultEnglishConfig();
    void clearCache();
};

// Convenience macro for getting localized text
#define L(key) LocalizationConfig::getInstance().getText(key)
#define LS(section, key) LocalizationConfig::getInstance().getText(section, key)

#endif // LOCALIZATION_CONFIG_H 