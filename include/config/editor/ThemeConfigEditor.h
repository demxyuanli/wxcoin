#ifndef THEME_CONFIG_EDITOR_H
#define THEME_CONFIG_EDITOR_H

#include "config/editor/ConfigCategoryEditor.h"

class ThemeConfigEditor : public ConfigCategoryEditor {
public:
    ThemeConfigEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId);
    virtual ~ThemeConfigEditor();
    
    virtual void loadConfig() override;
    
private:
    void createUI();
    void groupItemsBySection();
    
    std::map<std::string, std::vector<ConfigItem>> m_sectionGroups;
};

#endif // THEME_CONFIG_EDITOR_H

