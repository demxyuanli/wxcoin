#pragma once

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/statline.h>
#include "DockManager.h"

namespace ads {

// Forward declarations
class DockLayoutPreview;

/**
 * @brief Configuration structure for dock layout
 */
struct DockLayoutConfig {
    // Default sizes for each area (in pixels or percentage)
    int topAreaHeight = 150;
    int bottomAreaHeight = 200;
    int leftAreaWidth = 250;
    int rightAreaWidth = 250;
    int centerMinWidth = 400;
    int centerMinHeight = 300;
    
    // Use percentage instead of pixels
    bool usePercentage = false;
    
    // Percentage values (0-100)
    int topAreaPercent = 20;
    int bottomAreaPercent = 25;
    int leftAreaPercent = 20;
    int rightAreaPercent = 20;
    
    // Minimum sizes
    int minAreaSize = 100;
    int splitterWidth = 4;
    
    // Layout options
    bool showTopArea = true;
    bool showBottomArea = true;
    bool showLeftArea = true;
    bool showRightArea = true;
    
    // Animation
    bool enableAnimation = true;
    int animationDuration = 200;
    
    // Save/Load
    void SaveToConfig();
    void LoadFromConfig();
};

/**
 * @brief Dialog for configuring dock layout
 */
class DockLayoutConfigDialog : public wxDialog {
public:
    DockLayoutConfigDialog(wxWindow* parent, DockLayoutConfig& config, DockManager* dockManager = nullptr);
    
    // Get the modified configuration
    DockLayoutConfig GetConfig() const { return m_config; }
    
private:
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
    DockLayoutPreview* m_previewPanel;
    
    DockLayoutConfig m_config;
    DockManager* m_dockManager;
    
    // Methods
    void CreateControls();
    void CreateSizeControls(wxWindow* parent, wxSizer* sizer);
    void CreateVisibilityControls(wxWindow* parent, wxSizer* sizer);
    void CreateOptionControls(wxWindow* parent, wxSizer* sizer);
    void CreatePreviewPanel(wxWindow* parent, wxSizer* sizer);
    
    void OnUsePercentageChanged(wxCommandEvent& event);
    void OnValueChanged(wxSpinEvent& event);
    void OnCheckChanged(wxCommandEvent& event);
    void OnApply(wxCommandEvent& event);
    void OnReset(wxCommandEvent& event);
    
    void UpdatePreview();
    void UpdateControlStates();
    void UpdateControlValues();
    void ApplyToManager();
    
    wxDECLARE_EVENT_TABLE();
};

/**
 * @brief Preview panel for dock layout
 */
class DockLayoutPreview : public wxPanel {
public:
    DockLayoutPreview(wxWindow* parent);
    
    void SetConfig(const DockLayoutConfig& config);
    
protected:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    
private:
    DockLayoutConfig m_config;
    
    void DrawLayoutPreview(wxDC& dc);
    wxRect CalculateAreaRect(DockWidgetArea area, const wxRect& totalRect);
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace ads