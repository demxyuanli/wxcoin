#ifndef DOCK_LAYOUT_CONFIG_EDITOR_H
#define DOCK_LAYOUT_CONFIG_EDITOR_H

#include "config/editor/ConfigCategoryEditor.h"
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/button.h>
#include <wx/statbox.h>
#include <wx/gbsizer.h>
#include "docking/DockLayoutConfig.h"

namespace ads {
    class DockLayoutPreview;
}

class DockLayoutConfigEditor : public ConfigCategoryEditor {
public:
    DockLayoutConfigEditor(wxWindow* parent, UnifiedConfigManager* configManager, const std::string& categoryId);
    virtual ~DockLayoutConfigEditor();
    
    virtual void loadConfig() override;
    virtual void saveConfig() override;
    virtual void resetConfig() override;
    virtual bool hasChanges() const override;
    
private:
    void createUI();
    void createSizeControls(wxWindow* parent, wxSizer* sizer);
    void createVisibilityControls(wxWindow* parent, wxSizer* sizer);
    void createOptionControls(wxWindow* parent, wxSizer* sizer);
    void createPreviewPanel(wxSizer* sizer);
    void createPresetButtons(wxSizer* sizer);
    
    void updateConfigFromControls();
    void updateControlsFromConfig();
    void updateControlStates();
    void updatePreview();
    
    // Event handlers
    void onUsePercentageChanged(wxCommandEvent& event);
    void onValueChanged(wxSpinEvent& event);
    void onCheckChanged(wxCommandEvent& event);
    void onPresetButton(wxCommandEvent& event);
    
    // UI Controls
    wxCheckBox* m_usePercentageCheck;
    
    // Size controls - pixels
    wxSpinCtrl* m_topHeightSpin;
    wxSpinCtrl* m_bottomHeightSpin;
    wxSpinCtrl* m_leftWidthSpin;
    wxSpinCtrl* m_rightWidthSpin;
    wxSpinCtrl* m_centerMinWidthSpin;
    wxSpinCtrl* m_centerMinHeightSpin;
    
    // Size controls - percentage
    wxSpinCtrl* m_topPercentSpin;
    wxSpinCtrl* m_bottomPercentSpin;
    wxSpinCtrl* m_leftPercentSpin;
    wxSpinCtrl* m_rightPercentSpin;
    
    // Visibility controls
    wxCheckBox* m_showTopCheck;
    wxCheckBox* m_showBottomCheck;
    wxCheckBox* m_showLeftCheck;
    wxCheckBox* m_showRightCheck;
    
    // Other options
    wxSpinCtrl* m_minSizeSpin;
    wxSpinCtrl* m_splitterWidthSpin;
    wxCheckBox* m_enableAnimationCheck;
    wxSpinCtrl* m_animationDurationSpin;
    
    // Preview panel
    ads::DockLayoutPreview* m_previewPanel;
    
    // Notebook
    wxNotebook* m_notebook;
    
    // Current configuration
    ads::DockLayoutConfig m_config;
    ads::DockLayoutConfig m_originalConfig;  // Store original for change detection
    
    wxDECLARE_EVENT_TABLE();
};

#endif // DOCK_LAYOUT_CONFIG_EDITOR_H

