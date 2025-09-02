#pragma once

#include <wx/dialog.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/statbox.h>

class ImportSettingsDialog : public wxDialog
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
    int getImportMode() const { return m_importMode; }
    
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
    void applyPreset(double deflection, double angular, bool lod, bool parallel);
    
    // Controls
    wxPanel* m_presetPanel;
    wxSpinCtrlDouble* m_deflectionCtrl;
    wxSpinCtrlDouble* m_angularDeflectionCtrl;
    wxCheckBox* m_lodCheckBox;
    wxCheckBox* m_parallelCheckBox;
    wxCheckBox* m_adaptiveCheckBox;
    wxCheckBox* m_autoOptimizeCheckBox;
    wxChoice* m_importModeChoice;
    wxStaticText* m_previewText;
    
    // Settings
    double m_deflection;
    double m_angularDeflection;
    bool m_enableLOD;
    bool m_parallelProcessing;
    bool m_adaptiveMeshing;
    bool m_autoOptimize;
    int m_importMode;
    
    wxDECLARE_EVENT_TABLE();
};