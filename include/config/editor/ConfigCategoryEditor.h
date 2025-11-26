#ifndef CONFIG_CATEGORY_EDITOR_H
#define CONFIG_CATEGORY_EDITOR_H

#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <string>
#include <map>
#include <vector>
#include <functional>
#include "config/UnifiedConfigManager.h"

class ConfigItemEditor;
struct ConfigItem;

class ConfigCategoryEditor : public wxScrolledWindow {
public:
    ConfigCategoryEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId);
    virtual ~ConfigCategoryEditor();
    
    virtual void loadConfig() = 0;
    virtual void saveConfig();
    virtual void resetConfig();
    virtual bool hasChanges() const;
    
    void setChangeCallback(std::function<void()> callback);
    
    // Check if editor is already initialized (has UI created)
    bool isInitialized() const { return !m_editors.empty(); }
    
    // Refresh values from config without recreating UI
    void refreshValues();
    
protected:
    UnifiedConfigManager* m_configManager;
    std::string m_categoryId;
    std::map<std::string, ConfigItemEditor*> m_editors;
    std::map<std::string, std::string> m_originalValues;
    std::function<void()> m_changeCallback;
    
    void createItemEditor(const ConfigItem& item);
    void onItemChanged(const std::string& key, const std::string& value);
    void addSectionHeader(wxSizer* sizer, const std::string& sectionName);
    void addItemEditor(wxSizer* sizer, ConfigItemEditor* editor);
};

#endif // CONFIG_CATEGORY_EDITOR_H

