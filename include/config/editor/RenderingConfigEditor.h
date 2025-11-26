#ifndef RENDERING_CONFIG_EDITOR_H
#define RENDERING_CONFIG_EDITOR_H

#include "config/editor/ConfigCategoryEditor.h"

class RenderingConfigEditor : public ConfigCategoryEditor {
public:
    RenderingConfigEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId);
    virtual ~RenderingConfigEditor();
    
    virtual void loadConfig() override;
    
private:
    void createUI();
    void groupItemsBySection();
    
    std::map<std::string, std::vector<ConfigItem>> m_sectionGroups;
};

#endif // RENDERING_CONFIG_EDITOR_H

