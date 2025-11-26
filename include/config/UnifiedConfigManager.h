#ifndef UNIFIED_CONFIG_MANAGER_H
#define UNIFIED_CONFIG_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>

class ConfigManager;

enum class ConfigValueType {
    String,
    Int,
    Double,
    Bool,
    Color,
    Enum,
    Size
};

struct ConfigItem {
    std::string key;
    std::string displayName;
    std::string description;
    std::string section;
    std::string category;
    ConfigValueType type;
    std::string currentValue;
    std::string defaultValue;
    double minValue;
    double maxValue;
    std::vector<std::string> enumValues;
    std::vector<std::string> conflicts;
    std::function<bool(const std::string&, std::string&)> validator;
    
    ConfigItem() 
        : type(ConfigValueType::String)
        , minValue(0.0)
        , maxValue(100.0)
        , validator(nullptr)
    {}
};

struct ConfigCategory {
    std::string id;
    std::string displayName;
    std::string icon;
    std::vector<std::string> items;  // Keys of items in this category
};

class UnifiedConfigManager {
public:
    static UnifiedConfigManager& getInstance();
    
    void initialize(ConfigManager& configManager);
    
    // Category management
    void addCategory(const std::string& id, const std::string& displayName, const std::string& icon = "");
    void registerCategory(const ConfigCategory& category);
    std::vector<ConfigCategory> getCategories() const;
    
    // Item management
    void registerConfigItem(const ConfigItem& item);
    std::vector<ConfigItem> getItemsForCategory(const std::string& categoryId) const;
    ConfigItem* getItem(const std::string& key);
    
    // Value access
    std::string getValue(const std::string& key) const;
    bool setValue(const std::string& key, const std::string& value);
    
    // Validation and conflict checking
    bool validateValue(const std::string& key, const std::string& value, std::string& errorMsg) const;
    std::vector<std::string> checkConflicts(const std::string& key, const std::string& value) const;
    
    // Change listeners
    void addChangeListener(const std::string& key, std::function<void(const std::string&)> listener);
    void removeChangeListener(const std::string& key, std::function<void(const std::string&)> listener);
    
    // Save and reload
    void save();
    void reload();
    
    // Diagnostics
    void printDiagnostics() const;
    
private:
    UnifiedConfigManager();
    ~UnifiedConfigManager();
    
    // Disable copy and assignment
    UnifiedConfigManager(const UnifiedConfigManager&) = delete;
    UnifiedConfigManager& operator=(const UnifiedConfigManager&) = delete;
    
    void registerBuiltinCategories();
    void scanAndRegisterAllConfigs(ConfigManager& configManager);
    void scanAdditionalConfigFiles();
    void registerConfigManagerItems(ConfigManager& configManager);
    void registerThemeConfigItems();
    void registerRenderingConfigItems();
    void registerLightingConfigItems();
    void registerSelectionConfigItems();
    void registerEdgeConfigItems();
    void registerLoggerConfigItems();
    void registerFontConfigItems();
    std::string determineCategoryFromSection(const std::string& section) const;
    ConfigValueType determineValueType(const std::string& value, const std::string& key) const;
    
    ConfigManager* m_configManager;
    std::map<std::string, ConfigCategory> m_categories;
    std::map<std::string, ConfigItem> m_items;
    std::map<std::string, std::vector<std::function<void(const std::string&)>>> m_listeners;
};

#endif // UNIFIED_CONFIG_MANAGER_H
