#ifndef CONFIG_EDITOR_FACTORY_H
#define CONFIG_EDITOR_FACTORY_H

#include <wx/wx.h>
#include <string>
#include "config/editor/ConfigCategoryEditor.h"

class UnifiedConfigManager;

class ConfigEditorFactory {
public:
    static ConfigCategoryEditor* createEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId);
};

#endif // CONFIG_EDITOR_FACTORY_H



