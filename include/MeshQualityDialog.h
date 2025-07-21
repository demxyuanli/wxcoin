#pragma once

#include <wx/dialog.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/choice.h>

class OCCViewer;
class wxSpinDoubleEvent;
class wxSpinEvent;

class MeshQualityDialog : public wxDialog
{
public:
    MeshQualityDialog(wxWindow* parent, OCCViewer* occViewer);
    virtual ~MeshQualityDialog();

private:
    void createControls();
    void layoutControls();
    void bindEvents();
    void updateControls();
    
    // Page creation methods
    void createBasicQualityPage();
    void createSubdivisionPage();
    void createSmoothingPage();
    void createAdvancedPage();
    
    // Event handlers
    void onDeflectionSlider(wxCommandEvent& event);
    void onDeflectionSpinCtrl(wxSpinDoubleEvent& event);
    void onLODEnable(wxCommandEvent& event);
    void onLODRoughDeflectionSlider(wxCommandEvent& event);
    void onLODRoughDeflectionSpinCtrl(wxSpinDoubleEvent& event);
    void onLODFineDeflectionSlider(wxCommandEvent& event);
    void onLODFineDeflectionSpinCtrl(wxSpinDoubleEvent& event);
    void onLODTransitionTimeSlider(wxCommandEvent& event);
    void onLODTransitionTimeSpinCtrl(wxSpinEvent& event);
    
    // Subdivision event handlers
    void onSubdivisionEnable(wxCommandEvent& event);
    void onSubdivisionLevelSlider(wxCommandEvent& event);
    void onSubdivisionLevelSpinCtrl(wxSpinEvent& event);
    void onSubdivisionMethodChoice(wxCommandEvent& event);
    void onSubdivisionCreaseAngleSlider(wxCommandEvent& event);
    void onSubdivisionCreaseAngleSpinCtrl(wxSpinDoubleEvent& event);
    
    // Smoothing event handlers
    void onSmoothingEnable(wxCommandEvent& event);
    void onSmoothingMethodChoice(wxCommandEvent& event);
    void onSmoothingIterationsSlider(wxCommandEvent& event);
    void onSmoothingIterationsSpinCtrl(wxSpinEvent& event);
    void onSmoothingStrengthSlider(wxCommandEvent& event);
    void onSmoothingStrengthSpinCtrl(wxSpinDoubleEvent& event);
    void onSmoothingCreaseAngleSlider(wxCommandEvent& event);
    void onSmoothingCreaseAngleSpinCtrl(wxSpinDoubleEvent& event);
    
    // Advanced event handlers
    void onTessellationMethodChoice(wxCommandEvent& event);
    void onTessellationQualitySlider(wxCommandEvent& event);
    void onTessellationQualitySpinCtrl(wxSpinEvent& event);
    void onFeaturePreservationSlider(wxCommandEvent& event);
    void onFeaturePreservationSpinCtrl(wxSpinDoubleEvent& event);
    void onParallelProcessingCheckBox(wxCommandEvent& event);
    void onAdaptiveMeshingCheckBox(wxCommandEvent& event);
    
    // Dialog event handlers
    void onApply(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);
    void onReset(wxCommandEvent& event);
    void onValidate(wxCommandEvent& event);
    void onExportReport(wxCommandEvent& event);

    OCCViewer* m_occViewer;
    wxNotebook* m_notebook;
    
    // Basic quality controls
    wxSlider* m_deflectionSlider;
    wxSpinCtrlDouble* m_deflectionSpinCtrl;
    wxCheckBox* m_lodEnableCheckBox;
    wxSlider* m_lodRoughDeflectionSlider;
    wxSpinCtrlDouble* m_lodRoughDeflectionSpinCtrl;
    wxSlider* m_lodFineDeflectionSlider;
    wxSpinCtrlDouble* m_lodFineDeflectionSpinCtrl;
    wxSlider* m_lodTransitionTimeSlider;
    wxSpinCtrl* m_lodTransitionTimeSpinCtrl;
    
    // Subdivision controls
    wxCheckBox* m_subdivisionEnableCheckBox;
    wxSlider* m_subdivisionLevelSlider;
    wxSpinCtrl* m_subdivisionLevelSpinCtrl;
    wxChoice* m_subdivisionMethodChoice;
    wxSlider* m_subdivisionCreaseAngleSlider;
    wxSpinCtrlDouble* m_subdivisionCreaseAngleSpinCtrl;
    
    // Smoothing controls
    wxCheckBox* m_smoothingEnableCheckBox;
    wxChoice* m_smoothingMethodChoice;
    wxSlider* m_smoothingIterationsSlider;
    wxSpinCtrl* m_smoothingIterationsSpinCtrl;
    wxSlider* m_smoothingStrengthSlider;
    wxSpinCtrlDouble* m_smoothingStrengthSpinCtrl;
    wxSlider* m_smoothingCreaseAngleSlider;
    wxSpinCtrlDouble* m_smoothingCreaseAngleSpinCtrl;
    
    // Advanced controls
    wxChoice* m_tessellationMethodChoice;
    wxSlider* m_tessellationQualitySlider;
    wxSpinCtrl* m_tessellationQualitySpinCtrl;
    wxSlider* m_featurePreservationSlider;
    wxSpinCtrlDouble* m_featurePreservationSpinCtrl;
    wxCheckBox* m_parallelProcessingCheckBox;
    wxCheckBox* m_adaptiveMeshingCheckBox;
    
    // Current values
    double m_currentDeflection;
    bool m_currentLODEnabled;
    double m_currentLODRoughDeflection;
    double m_currentLODFineDeflection;
    int m_currentLODTransitionTime;
    
    // Subdivision values
    bool m_currentSubdivisionEnabled;
    int m_currentSubdivisionLevel;
    int m_currentSubdivisionMethod;
    double m_currentSubdivisionCreaseAngle;
    
    // Smoothing values
    bool m_currentSmoothingEnabled;
    int m_currentSmoothingMethod;
    int m_currentSmoothingIterations;
    double m_currentSmoothingStrength;
    double m_currentSmoothingCreaseAngle;
    
    // Advanced values
    int m_currentTessellationMethod;
    int m_currentTessellationQuality;
    double m_currentFeaturePreservation;
    bool m_currentParallelProcessing;
    bool m_currentAdaptiveMeshing;
}; 