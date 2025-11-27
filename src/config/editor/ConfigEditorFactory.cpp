#include "config/editor/ConfigEditorFactory.h"
#include "config/editor/GeneralConfigEditor.h"
#include "config/editor/ThemeConfigEditor.h"
#include "config/editor/RenderingConfigEditor.h"
#include "config/editor/LightingConfigEditor.h"
#include "config/editor/TypographyConfigEditor.h"
#include "config/editor/LayoutConfigEditor.h"
#include "config/editor/DockLayoutConfigEditor.h"
#include "config/editor/LightingSettingsEditor.h"
#include "config/editor/EdgeSettingsEditor.h"
#include "config/editor/RenderPreviewEditor.h"
#include "config/UnifiedConfigManager.h"
#include "logger/Logger.h"

ConfigCategoryEditor* ConfigEditorFactory::createEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId) {
    // Map logical categories to specialized editors
    if (categoryId == "Appearance") {
        return new ThemeConfigEditor(parent, configManager, categoryId);
    } else if (categoryId == "Rendering") {
        return new RenderingConfigEditor(parent, configManager, categoryId);
    } else if (categoryId == "Lighting") {
        // Use LightingSettingsEditor for full GUI instead of LightingConfigEditor
        return new LightingSettingsEditor(parent, configManager, categoryId);
    } else if (categoryId == "Typography") {
        return new TypographyConfigEditor(parent, configManager, categoryId);
    } else if (categoryId == "Layout") {
        return new LayoutConfigEditor(parent, configManager, categoryId);
    } else if (categoryId == "Dock Layout") {
        return new DockLayoutConfigEditor(parent, configManager, categoryId);
    } else if (categoryId == "Edge Settings") {
        return new EdgeSettingsEditor(parent, configManager, categoryId);
    } else if (categoryId == "Render Preview") {
        return new RenderPreviewEditor(parent, configManager, categoryId);
    } else {
        // Use GeneralConfigEditor for all other categories:
        // System, UI Components, Navigation, Selection, Compatibility, General
        return new GeneralConfigEditor(parent, configManager, categoryId);
    }
}