#pragma once

#include <wx/dialog.h>
#include <wx/slider.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/button.h>

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
    
    void onDeflectionSlider(wxCommandEvent& event);
    void onDeflectionSpinCtrl(wxSpinDoubleEvent& event);
    void onLODEnable(wxCommandEvent& event);
    void onLODRoughDeflectionSlider(wxCommandEvent& event);
    void onLODRoughDeflectionSpinCtrl(wxSpinDoubleEvent& event);
    void onLODFineDeflectionSlider(wxCommandEvent& event);
    void onLODFineDeflectionSpinCtrl(wxSpinDoubleEvent& event);
    void onLODTransitionTimeSlider(wxCommandEvent& event);
    void onLODTransitionTimeSpinCtrl(wxSpinEvent& event);
    void onApply(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);

    OCCViewer* m_occViewer;
    
    wxSlider* m_deflectionSlider;
    wxSpinCtrlDouble* m_deflectionSpinCtrl;
    wxCheckBox* m_lodEnableCheckBox;
    wxSlider* m_lodRoughDeflectionSlider;
    wxSpinCtrlDouble* m_lodRoughDeflectionSpinCtrl;
    wxSlider* m_lodFineDeflectionSlider;
    wxSpinCtrlDouble* m_lodFineDeflectionSpinCtrl;
    wxSlider* m_lodTransitionTimeSlider;
    wxSpinCtrl* m_lodTransitionTimeSpinCtrl;
    
    double m_currentDeflection;
    bool m_currentLODEnabled;
    double m_currentLODRoughDeflection;
    double m_currentLODFineDeflection;
    int m_currentLODTransitionTime;
}; 