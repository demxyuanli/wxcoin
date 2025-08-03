#pragma once

#include <wx/dialog.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/listbox.h>
#include <wx/textctrl.h>
#include <wx/spinctrl.h>
#include <vector>
#include <string>
#include <memory>
#include "renderpreview/RenderLightSettings.h"
#include "renderpreview/ConfigValidator.h"
#include "renderpreview/UndoManager.h"

// Forward declarations
class PreviewCanvas;
class LightManagementPanel;
class MaterialPanel;
class GlobalSettingsPanel;
class ObjectSettingsPanel;

class RenderPreviewDialog : public wxDialog
{
public:
    RenderPreviewDialog(wxWindow* parent);

    // Global settings accessors
    void updateGlobalLighting();
    void updateGlobalAntiAliasing();
    void updateGlobalRenderingMode();
    
    // Object settings accessors
    void applyObjectSettingsToCanvas();
    
    // Global settings methods
    void applyGlobalSettingsToCanvas();
    
    // Configuration methods
    void saveConfiguration();
    void loadConfiguration();
    void resetToDefaults();
    
    // New feature methods
    void saveCurrentState(const std::string& description = "");
    void applySnapshot(const ConfigSnapshot& snapshot);
    ConfigSnapshot createSnapshot() const;
    bool validateCurrentSettings();
    void setAutoApply(bool enabled);
    void setValidationEnabled(bool enabled);
    
    // Light management
    void updateLightList();
    
    // Event handlers for global settings
    void onGlobalLightingChanged(wxCommandEvent& event);
    void onGlobalAntiAliasingChanged(wxCommandEvent& event);
    void onGlobalRenderingModeChanged(wxCommandEvent& event);
    
    // Event handlers for object settings
    void onObjectMaterialChanged(wxCommandEvent& event);
    void onObjectTextureChanged(wxCommandEvent& event);
    
    // Dialog event handlers
    void OnReset(wxCommandEvent& event);
    void OnApply(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnUndo(wxCommandEvent& event);
    void OnRedo(wxCommandEvent& event);

private:
    void createUI();
    
    // UI Components
    PreviewCanvas* m_renderCanvas;
    wxButton* m_applyButton;
    
    // Panel instances
    GlobalSettingsPanel* m_globalSettingsPanel;
    ObjectSettingsPanel* m_objectSettingsPanel;
    
    // Data
    std::vector<RenderLightSettings> m_lights;
    int m_currentLightIndex;
    
    // New features
    std::unique_ptr<UndoManager> m_undoManager;
    bool m_autoApply;
    bool m_validationEnabled;
    
    DECLARE_EVENT_TABLE()
};
