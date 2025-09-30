#pragma once

#include "widgets/FramelessModalPopup.h"
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/statbox.h>

class ImportSettingsDialog : public FramelessModalPopup
{
public:
    ImportSettingsDialog(wxWindow* parent);
    virtual ~ImportSettingsDialog();
    
    // Get settings
    double getMeshDeflection() const { return m_deflection; }
    double getAngularDeflection() const { return m_angularDeflection; }
    bool isLODEnabled() const { return m_enableLOD; }
    bool isParallelProcessing() const { return m_parallelProcessing; }
    bool isAdaptiveMeshing() const { return m_adaptiveMeshing; }
    bool isAutoOptimize() const { return m_autoOptimize; }
    bool isNormalProcessing() const { return m_normalProcessing; }
    int getImportMode() const { return m_importMode; }
    
    // New tessellation settings
    bool isFineTessellationEnabled() const { return m_enableFineTessellation; }
    double getTessellationDeflection() const { return m_tessellationDeflection; }
    double getTessellationAngle() const { return m_tessellationAngle; }
    int getTessellationMinPoints() const { return m_tessellationMinPoints; }
    int getTessellationMaxPoints() const { return m_tessellationMaxPoints; }
    bool isAdaptiveTessellationEnabled() const { return m_enableAdaptiveTessellation; }
    
private:
    void createControls();
    void layoutControls();
    void bindEvents();
    
    // Event handlers
    void onPresetPerformance(wxCommandEvent& event);
    void onPresetBalanced(wxCommandEvent& event);
    void onPresetQuality(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void onDeflectionChange(wxSpinDoubleEvent& event);
    
    // Apply preset values
    void applyPreset(double deflection, double angular, bool lod, bool parallel, bool normalProcessing);
    
    // Controls
    wxPanel* m_presetPanel;
    wxSpinCtrlDouble* m_deflectionCtrl;
    wxSpinCtrlDouble* m_angularDeflectionCtrl;
    wxCheckBox* m_lodCheckBox;
    wxCheckBox* m_parallelCheckBox;
    wxCheckBox* m_adaptiveCheckBox;
    wxCheckBox* m_autoOptimizeCheckBox;
    wxCheckBox* m_normalProcessingCheckBox;
    wxChoice* m_importModeChoice;
    wxStaticText* m_previewText;
    
    // New tessellation controls
    wxCheckBox* m_fineTessellationCheckBox;
    wxSpinCtrlDouble* m_tessellationDeflectionCtrl;
    wxSpinCtrlDouble* m_tessellationAngleCtrl;
    wxSpinCtrl* m_tessellationMinPointsCtrl;
    wxSpinCtrl* m_tessellationMaxPointsCtrl;
    wxCheckBox* m_adaptiveTessellationCheckBox;
    
    // Settings
    double m_deflection;
    double m_angularDeflection;
    bool m_enableLOD;
    bool m_parallelProcessing;
    bool m_adaptiveMeshing;
    bool m_autoOptimize;
    bool m_normalProcessing;
    int m_importMode;
    
    // New tessellation settings
    bool m_enableFineTessellation;
    double m_tessellationDeflection;
    double m_tessellationAngle;
    int m_tessellationMinPoints;
    int m_tessellationMaxPoints;
    bool m_enableAdaptiveTessellation;
    
    wxDECLARE_EVENT_TABLE();
};