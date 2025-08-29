#pragma once

#include "viewer/ImageOutlinePass.h"
#include <wx/dialog.h>
#include <wx/slider.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>

// Forward declarations
class OCCViewer;

/**
 * @brief Dialog for controlling real-time outline rendering parameters
 * 
 * Provides UI controls for adjusting outline intensity, thickness,
 * depth/normal weights, and debug visualization modes.
 */
class OutlineSettingsDialog : public wxDialog {
public:
    /**
     * @brief Constructor
     * @param parent Parent window
     * @param occViewer OCC viewer to control
     */
    OutlineSettingsDialog(wxWindow* parent, OCCViewer* occViewer);
    
    /**
     * @brief Destructor
     */
    ~OutlineSettingsDialog();

private:
    void createControls();
    void updateControls();
    void onSliderChange(wxCommandEvent& event);
    void onCheckboxChange(wxCommandEvent& event);
    void onResetDefaults(wxCommandEvent& event);
    void onClose(wxCloseEvent& event);
    void applyParams();

    OCCViewer* m_occViewer;
    ImageOutlineParams m_params;

    // Controls
    wxCheckBox* m_enableOutline;
    wxCheckBox* m_enableHover;
    
    wxSlider* m_intensitySlider;
    wxSlider* m_thicknessSlider;
    wxSlider* m_depthWeightSlider;
    wxSlider* m_normalWeightSlider;
    wxSlider* m_depthThresholdSlider;
    wxSlider* m_normalThresholdSlider;
    
    wxStaticText* m_intensityLabel;
    wxStaticText* m_thicknessLabel;
    wxStaticText* m_depthWeightLabel;
    wxStaticText* m_normalWeightLabel;
    wxStaticText* m_depthThresholdLabel;
    wxStaticText* m_normalThresholdLabel;

    DECLARE_EVENT_TABLE()
};