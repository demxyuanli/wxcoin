#ifndef LAYOUT_CONFIG_EDITOR_H
#define LAYOUT_CONFIG_EDITOR_H

#include "config/editor/ConfigCategoryEditor.h"

class LayoutConfigEditor : public ConfigCategoryEditor {
public:
    LayoutConfigEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId);
    virtual ~LayoutConfigEditor();
    
    virtual void loadConfig() override;
    
private:
    void createUI();
    void groupItemsBySection();
    void organizeByComponent();
    
    std::map<std::string, std::vector<ConfigItem>> m_sectionGroups;
    std::map<std::string, std::vector<ConfigItem>> m_componentGroups;  // Group by UI component
};

#endif // LAYOUT_CONFIG_EDITOR_H
