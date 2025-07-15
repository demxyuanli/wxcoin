#pragma once

#include <wx/dialog.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <vector>
#include <memory>

class OCCViewer;
class OCCGeometry;

class TransparencyDialog : public wxDialog
{
public:
    TransparencyDialog(wxWindow* parent, OCCViewer* occViewer, 
                      const std::vector<std::shared_ptr<OCCGeometry>>& selectedGeometries);
    virtual ~TransparencyDialog();

private:
    void createControls();
    void layoutControls();
    void bindEvents();
    void updateControls();
    void applyTransparency();
    
    void onTransparencySlider(wxCommandEvent& event);
    void onTransparencySpinCtrl(wxSpinDoubleEvent& event);
    void onApply(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);

    OCCViewer* m_occViewer;
    std::vector<std::shared_ptr<OCCGeometry>> m_selectedGeometries;
    
    wxSlider* m_transparencySlider;
    wxSpinCtrlDouble* m_transparencySpinCtrl;
    wxStaticText* m_infoText;
    
    double m_currentTransparency;
    double m_originalTransparency;
    
    enum
    {
        ID_TRANSPARENCY_SLIDER = 1000,
        ID_TRANSPARENCY_SPIN
    };
}; 