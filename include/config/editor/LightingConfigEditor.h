#ifndef LIGHTING_CONFIG_EDITOR_H
#define LIGHTING_CONFIG_EDITOR_H

#include "config/editor/ConfigCategoryEditor.h"

class LightingConfigEditor : public ConfigCategoryEditor {
public:
    LightingConfigEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId);
    virtual ~LightingConfigEditor();
    
    virtual void loadConfig() override;
    
private:
    void createUI();
    void groupItemsBySection();
    void organizeLightSettings();
    
    std::map<std::string, std::vector<ConfigItem>> m_sectionGroups;
    std::vector<std::string> m_lightSections;  // Light0, Light1, etc.
};

#endif // LIGHTING_CONFIG_EDITOR_H
