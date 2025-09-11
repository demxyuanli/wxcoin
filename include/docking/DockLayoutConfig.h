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
    int leftAreaWidth = 200;     // Minimum width for left area (15/85 layout)
    int rightAreaWidth = 250;
    int centerMinWidth = 400;
    int centerMinHeight = 300;
    
    // Use percentage instead of pixels
    bool usePercentage = true;    // Default to true for 15/85 layout
    
    // Percentage values (0-100)
    int topAreaPercent = 0;     // No top area for clean 15/85 layout
    int bottomAreaPercent = 20; // 20% for bottom dock area (15/85 layout)
    int leftAreaPercent = 15;   // 15% for left dock area (15/85 layout)
    int rightAreaPercent = 0;   // No right area for clean 15/85 layout
    
    // Minimum sizes
    int minAreaSize = 100;
    int splitterWidth = 4;
    
    // Layout options
    bool showTopArea = false;     // Default to false for 20/80 layout
    bool showBottomArea = true;   // Default to true for 20/80 layout
    bool showLeftArea = true;     // Default to true for 20/80 layout
    bool showRightArea = false;   // Default to false for 20/80 layout
    
    // Animation
    bool enableAnimation = true;
    int animationDuration = 200;
    
    // Save/Load
    void SaveToConfig();
    void LoadFromConfig();
    
    // Comparison operators for caching
    bool operator==(const DockLayoutConfig& other) const {
        return topAreaHeight == other.topAreaHeight &&
               bottomAreaHeight == other.bottomAreaHeight &&
               leftAreaWidth == other.leftAreaWidth &&
               rightAreaWidth == other.rightAreaWidth &&
               centerMinWidth == other.centerMinWidth &&
               centerMinHeight == other.centerMinHeight &&
               usePercentage == other.usePercentage &&
               topAreaPercent == other.topAreaPercent &&
               bottomAreaPercent == other.bottomAreaPercent &&
               leftAreaPercent == other.leftAreaPercent &&
               rightAreaPercent == other.rightAreaPercent &&
               minAreaSize == other.minAreaSize &&
               splitterWidth == other.splitterWidth &&
               showTopArea == other.showTopArea &&
               showBottomArea == other.showBottomArea &&
               showLeftArea == other.showLeftArea &&
               showRightArea == other.showRightArea &&
               enableAnimation == other.enableAnimation &&
               animationDuration == other.animationDuration;
    }
    
    bool operator!=(const DockLayoutConfig& other) const {
        return !(*this == other);
    }
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