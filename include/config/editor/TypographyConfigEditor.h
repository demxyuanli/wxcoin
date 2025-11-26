#ifndef TYPOGRAPHY_CONFIG_EDITOR_H
#define TYPOGRAPHY_CONFIG_EDITOR_H

#include "config/editor/ConfigCategoryEditor.h"

class TypographyConfigEditor : public ConfigCategoryEditor {
public:
    TypographyConfigEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId);
    virtual ~TypographyConfigEditor();
    
    virtual void loadConfig() override;
    
private:
    void createUI();
    void groupItemsByFontType();
    void organizeFontProperties();
    
    std::map<std::string, std::vector<ConfigItem>> m_fontTypeGroups;  // Group by font type (Default, Title, Label, etc.)
    std::map<std::string, std::vector<ConfigItem>> m_propertyGroups;  // Group by property (Size, Family, Style, Weight, FaceName)
};

#endif // TYPOGRAPHY_CONFIG_EDITOR_H
