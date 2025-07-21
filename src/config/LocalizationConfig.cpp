#include "config/LocalizationConfig.h"
#include "logger/Logger.h"
#include <wx/filename.h>
#include <wx/dir.h>
#include <algorithm>

LocalizationConfig::LocalizationConfig()
    : m_initialized(false)
    , m_currentLanguage("en_US")
    , m_configFilePath("")
{
}

LocalizationConfig::~LocalizationConfig() = default;

LocalizationConfig& LocalizationConfig::getInstance()
{
    static LocalizationConfig instance;
    return instance;
}

bool LocalizationConfig::initialize(const std::string& language, const std::string& configPath)
{
    if (m_initialized) {
        LOG_WRN_S("LocalizationConfig already initialized");
        return true;
    }

    m_configFilePath = configPath;
    m_currentLanguage = language;

    // Try to load the specified language
    if (!loadLanguageFile(language)) {
        LOG_WRN_S("Failed to load language: " + language + ", falling back to English");
        m_currentLanguage = "en_US";
        if (!loadLanguageFile("en_US")) {
            LOG_ERR_S("Failed to load English fallback language");
            return false;
        }
    }

    m_initialized = true;
    LOG_INF_S("LocalizationConfig initialized with language: " + m_currentLanguage);
    return true;
}

std::string LocalizationConfig::getText(const std::string& key, const std::string& defaultValue)
{
    if (!m_initialized) {
        LOG_WRN_S("LocalizationConfig not initialized, returning default value");
        return defaultValue;
    }

    // Check cache first
    auto cacheIt = m_textCache.find(key);
    if (cacheIt != m_textCache.end()) {
        return cacheIt->second;
    }

    // Try to get from config file
    if (m_fileConfig) {
        wxString value;
        if (m_fileConfig->Read(key, &value)) {
            std::string text = value.ToStdString();
            m_textCache[key] = text;
            return text;
        }
    }

    // Return default value if not found
    LOG_WRN_S("Text key not found: " + key + ", using default: " + defaultValue);
    return defaultValue;
}

std::string LocalizationConfig::getText(const std::string& section, const std::string& key, 
                                       const std::string& defaultValue)
{
    if (!m_initialized) {
        LOG_WRN_S("LocalizationConfig not initialized, returning default value");
        return defaultValue;
    }

    // Create cache key
    std::string cacheKey = section + "." + key;

    // Check cache first
    auto cacheIt = m_textCache.find(cacheKey);
    if (cacheIt != m_textCache.end()) {
        return cacheIt->second;
    }

    // Try to get from config file
    if (m_fileConfig) {
        wxString value;
        std::string fullKey = section + "/" + key;
        LOG_DBG_S("Trying to read: " + fullKey);
        
        // Try different reading methods
        bool readSuccess = false;
        
        // Method 1: Try setting group first (most reliable for INI files)
        m_fileConfig->SetPath("/" + section);
        if (m_fileConfig->Read(key, &value)) {
            readSuccess = true;
        }
        m_fileConfig->SetPath("/"); // Reset path
        
        // Method 2: Try with section/key format if Method 1 failed
        if (!readSuccess && m_fileConfig->Read(fullKey, &value)) {
            readSuccess = true;
        }
        
        // Method 3: Try direct key (without section) if still not found
        if (!readSuccess && m_fileConfig->Read(key, &value)) {
            readSuccess = true;
        }
        
        if (readSuccess) {
            std::string text = value.ToStdString();
            m_textCache[cacheKey] = text;
            LOG_DBG_S("Found text: " + text);
            return text;
        } else {
            LOG_WRN_S("Failed to read key: " + fullKey);
        }
    } else {
        LOG_WRN_S("m_fileConfig is null");
    }

    // Return default value if not found
    LOG_WRN_S("Text key not found: " + section + "/" + key + ", using default: " + defaultValue);
    return defaultValue;
}

void LocalizationConfig::setText(const std::string& key, const std::string& value)
{
    if (!m_initialized) {
        LOG_WRN_S("LocalizationConfig not initialized, cannot set text");
        return;
    }

    m_textCache[key] = value;
    if (m_fileConfig) {
        m_fileConfig->Write(key, wxString(value));
    }
}

void LocalizationConfig::setText(const std::string& section, const std::string& key, const std::string& value)
{
    if (!m_initialized) {
        LOG_WRN_S("LocalizationConfig not initialized, cannot set text");
        return;
    }

    std::string cacheKey = section + "." + key;
    m_textCache[cacheKey] = value;
    if (m_fileConfig) {
        m_fileConfig->Write(section + "/" + key, wxString(value));
    }
}

bool LocalizationConfig::setLanguage(const std::string& language)
{
    if (language == m_currentLanguage) {
        return true;
    }

    if (loadLanguageFile(language)) {
        m_currentLanguage = language;
        clearCache();
        LOG_INF_S("Language changed to: " + language);
        return true;
    }

    LOG_ERR_S("Failed to change language to: " + language);
    return false;
}

bool LocalizationConfig::save()
{
    if (!m_initialized || !m_fileConfig) {
        LOG_WRN_S("LocalizationConfig not initialized, cannot save");
        return false;
    }

    bool result = m_fileConfig->Flush();
    if (result) {
        LOG_INF_S("Localization settings saved successfully");
    } else {
        LOG_ERR_S("Failed to save localization settings");
    }
    return result;
}

bool LocalizationConfig::reload()
{
    if (!m_initialized) {
        LOG_WRN_S("LocalizationConfig not initialized, cannot reload");
        return false;
    }

    clearCache();
    return loadLanguageFile(m_currentLanguage);
}

std::vector<std::string> LocalizationConfig::getAvailableLanguages() const
{
    std::vector<std::string> languages;
    
    // Check config directory for language files
    wxString configDir = wxFileName(m_configFilePath).GetPath();
    if (wxDirExists(configDir)) {
        wxDir dir(configDir);
        wxString filename;
        bool cont = dir.GetFirst(&filename, "*.ini", wxDIR_FILES);
        while (cont) {
            // Extract language code from filename (e.g., "zh_CN.ini" -> "zh_CN")
            wxString langCode = wxFileName(filename).GetName();
            languages.push_back(langCode.ToStdString());
            cont = dir.GetNext(&filename);
        }
    }

    // Add default languages if none found
    if (languages.empty()) {
        languages = {"en_US", "zh_CN"};
    }

    return languages;
}

bool LocalizationConfig::loadLanguageFile(const std::string& language)
{
    std::string filePath = findLanguageFile(language);
    if (filePath.empty()) {
        LOG_ERR_S("Language file not found for: " + language);
        return false;
    }

    try {
        m_fileConfig = std::make_unique<wxFileConfig>(wxString(filePath));
        LOG_INF_S("Loaded language file: " + filePath);
        
        // Test reading a value to verify the file is working
        wxString testValue;
        LOG_INF_S("Testing file reading...");
        
        // List all groups in the file
        wxString group;
        long index = 0;
        bool hasGroups = m_fileConfig->GetFirstGroup(group, index);
        LOG_INF_S("Has groups: " + std::to_string(hasGroups));
        while (hasGroups) {
            LOG_INF_S("Found group: " + group.ToStdString());
            hasGroups = m_fileConfig->GetNextGroup(group, index);
        }
        
        // Try to read any key to see if the file is working
        wxString anyValue;
        if (m_fileConfig->Read("Title", &anyValue)) {
            LOG_INF_S("Direct key read successful: " + anyValue.ToStdString());
        } else {
            LOG_WRN_S("Direct key read failed");
        }
        
        if (m_fileConfig->Read("RenderingSettingsDialog/Title", &testValue)) {
            LOG_INF_S("Test read successful: " + testValue.ToStdString());
        } else {
            LOG_WRN_S("Test read failed for RenderingSettingsDialog/Title");
            // Try alternative reading method
            m_fileConfig->SetPath("/RenderingSettingsDialog");
            if (m_fileConfig->Read("Title", &testValue)) {
                LOG_INF_S("Alternative test read successful: " + testValue.ToStdString());
            } else {
                LOG_WRN_S("Alternative test read also failed");
            }
            m_fileConfig->SetPath("/");
        }
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERR_S("Failed to load language file: " + filePath + ", error: " + e.what());
        return false;
    }
}

std::string LocalizationConfig::findLanguageFile(const std::string& language)
{
    // Try different possible paths
    std::vector<std::string> possiblePaths = {
        m_configFilePath + "/" + language + ".ini",
        "config/" + language + ".ini",
        "config/localization/" + language + ".ini",
        "localization/" + language + ".ini",
        language + ".ini"
    };

    for (const auto& path : possiblePaths) {
        if (wxFileExists(wxString(path))) {
            LOG_INF_S("Found language file: " + path);
            return path;
        }
    }

    // If no specific language file found, try to create default
    if (language == "zh_CN") {
        return createDefaultChineseConfig();
    } else if (language == "en_US") {
        return createDefaultEnglishConfig();
    }

    LOG_ERR_S("No language file found for: " + language);
    return "";
}

std::string LocalizationConfig::createDefaultChineseConfig()
{
    return "config/zh_CN.ini";
}

std::string LocalizationConfig::createDefaultEnglishConfig()
{
    return "config/en_US.ini";
}

void LocalizationConfig::clearCache()
{
    m_textCache.clear();
} 