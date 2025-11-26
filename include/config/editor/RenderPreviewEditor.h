#ifndef RENDER_PREVIEW_EDITOR_H
#define RENDER_PREVIEW_EDITOR_H

#include "config/editor/ConfigCategoryEditor.h"
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/colour.h>
#include <wx/filedlg.h>
#include <string>
#include <memory>
#include "renderpreview/RenderLightSettings.h"
#include "renderpreview/ConfigValidator.h"
#include "renderpreview/UndoManager.h"

// Forward declarations
class PreviewCanvas;
class GlobalSettingsPanel;
class ObjectSettingsPanel;

class RenderPreviewEditor : public ConfigCategoryEditor {
public:
    RenderPreviewEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId);
    virtual ~RenderPreviewEditor();
    
    virtual void loadConfig() override;
    virtual void saveConfig() override;
    virtual void resetConfig() override;
    
private:
    void createUI();
    
    // Configuration methods
    void saveConfiguration();
    void loadConfiguration();
    void resetToDefaults();
    void applyLoadedConfigurationToCanvas();
    
    // Global settings methods
    void applyGlobalSettingsToCanvas();
    void applyObjectSettingsToCanvas();
    
    // UI Components
    PreviewCanvas* m_renderCanvas;
    wxNotebook* m_notebook;
    
    // Panel instances
    GlobalSettingsPanel* m_globalSettingsPanel;
    ObjectSettingsPanel* m_objectSettingsPanel;
    
    // Data
    std::vector<RenderLightSettings> m_lights;
    int m_currentLightIndex;
    
    // Features
    std::unique_ptr<UndoManager> m_undoManager;
    bool m_validationEnabled;
};

#endif // RENDER_PREVIEW_EDITOR_H

