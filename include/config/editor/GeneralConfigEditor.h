#ifndef GENERAL_CONFIG_EDITOR_H
#define GENERAL_CONFIG_EDITOR_H

#include "config/editor/ConfigCategoryEditor.h"

class GeneralConfigEditor : public ConfigCategoryEditor {
public:
    GeneralConfigEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId);
    virtual ~GeneralConfigEditor();
    
    virtual void loadConfig() override;
    
private:
    void createUI();
    void groupItemsBySection();
    
    std::map<std::string, std::vector<ConfigItem>> m_sectionGroups;
};

#endif // GENERAL_CONFIG_EDITOR_H

