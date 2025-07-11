#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <wx/fileconf.h>
#include <string>
#include <memory>
#include <vector>

class ConfigManager {
private:
    bool initialized;
    std::string configFilePath;
    std::unique_ptr<wxFileConfig> fileConfig;

    ConfigManager();
    ~ConfigManager();
    std::string findConfigFile();

public:
    static ConfigManager& getInstance();

    bool initialize(const std::string& configFilePath);
    std::string getString(const std::string& section, const std::string& key, const std::string& defaultValue);
    int getInt(const std::string& section, const std::string& key, int defaultValue);
    double getDouble(const std::string& section, const std::string& key, double defaultValue);
    bool getBool(const std::string& section, const std::string& key, bool defaultValue);
    void setString(const std::string& section, const std::string& key, const std::string& value);
    void setInt(const std::string& section, const std::string& key, int value);
    void setDouble(const std::string& section, const std::string& key, double value);
    void setBool(const std::string& section, const std::string& key, bool value);
    bool save();
    bool reload();
    std::string getConfigFilePath() const;
    std::vector<std::string> getSections();
    std::vector<std::string> getKeys(const std::string& section);
};

#endif // CONFIG_MANAGER_H