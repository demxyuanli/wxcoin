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
    void applyLoadedConfigurationToCanvas();
    
    // New feature methods
    void saveCurrentState(const std::string& description = "");
    void applySnapshot(const ConfigSnapshot& snapshot);
    ConfigSnapshot createSnapshot() const;
    bool validateCurrentSettings();
    void setValidationEnabled(bool enabled);
    
    // Getter for render canvas
    PreviewCanvas* getRenderCanvas() const { return m_renderCanvas; }
    void setAutoApply(bool enabled); // Legacy method for backward compatibility
    
    // Light management
    void updateLightList();
    
    // Event handlers for global settings
    void onGlobalLightingChanged(wxCommandEvent& event);
    void onGlobalAntiAliasingChanged(wxCommandEvent& event);
    void onGlobalRenderingModeChanged(wxCommandEvent& event);
    
    // Event handlers for object settings
    void onObjectMaterialChanged(wxCommandEvent& event);
    void onObjectTextureChanged(wxCommandEvent& event);
    
    // Global dialog event handlers
    void OnCloseButton(wxCommandEvent& event);
    void OnHelp(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

private:
    void createUI();
    
    // UI Components
    PreviewCanvas* m_renderCanvas;
    
    // Global dialog buttons
    wxButton* m_closeButton;
    wxButton* m_helpButton;
    
    // Panel instances
    GlobalSettingsPanel* m_globalSettingsPanel;
    ObjectSettingsPanel* m_objectSettingsPanel;
    
    // Data
    std::vector<RenderLightSettings> m_lights;
    int m_currentLightIndex;
    
    // New features
    std::unique_ptr<UndoManager> m_undoManager;
    bool m_validationEnabled;
    
};
