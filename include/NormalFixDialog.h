#pragma once

#include <wx/dialog.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/spinctrl.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/listbox.h>
#include <wx/slider.h>
#include <wx/grid.h>
#include <wx/scrolwin.h>
#include <memory>
#include <vector>
#include <string>

class OCCViewer;
class OCCGeometry;

// Forward declarations
class TopoDS_Shape;

/**
 * @brief Dialog for normal fixing parameters and geometry information
 */
class NormalFixDialog : public wxDialog {
public:
    NormalFixDialog(wxWindow* parent, OCCViewer* viewer, wxWindowID id = wxID_ANY, 
                   const wxString& title = "Normal Fix Settings", 
                   const wxPoint& pos = wxDefaultPosition, 
                   const wxSize& size = wxSize(600, 500));

    // Dialog result
    struct NormalFixSettings {
        bool autoCorrect = true;
        bool showNormals = false;
        double normalLength = 1.0;
        bool showCorrectNormals = true;
        bool showIncorrectNormals = true;
        double qualityThreshold = 0.8;
        bool applyToSelected = true;
        bool applyToAll = false;
    };

    NormalFixSettings getSettings() const;
    void setSettings(const NormalFixSettings& settings);

private:
    void createControls();
    void createInfoPage();
    void createSettingsPage();
    void createPreviewPage();
    
    void updateGeometryInfo();
    void updateNormalInfo();
    void updateSettings();
    void analyzeFaceNormals(const TopoDS_Shape& shape, const std::string& shapeName);
    void onGeometrySelectionChanged(wxCommandEvent& event);
    void onSettingsChanged(wxCommandEvent& event);
    void onSpinCtrlChanged(wxSpinDoubleEvent& event);
    void onSliderChanged(wxScrollEvent& event);
    void onPreviewNormals(wxCommandEvent& event);
    void onApply(wxCommandEvent& event);
    void onOK(wxCommandEvent& event);
    void onCancel(wxCommandEvent& event);
    void onReset(wxCommandEvent& event);

    OCCViewer* m_viewer;
    
    // Notebook pages
    wxNotebook* m_notebook;
    wxScrolledWindow* m_infoPage;
    wxScrolledWindow* m_settingsPage;
    wxScrolledWindow* m_previewPage;
    
    // Info page controls
    wxListBox* m_geometryList;
    wxStaticText* m_geometryName;
    wxStaticText* m_faceCount;
    wxStaticText* m_normalQuality;
    wxStaticText* m_normalStatus;
    wxGrid* m_normalDetails;
    
    // Settings page controls
    wxCheckBox* m_autoCorrectCheck;
    wxCheckBox* m_showNormalsCheck;
    wxSpinCtrlDouble* m_normalLengthSpin;
    wxCheckBox* m_showCorrectCheck;
    wxCheckBox* m_showIncorrectCheck;
    wxSlider* m_qualityThresholdSlider;
    wxStaticText* m_qualityThresholdLabel;
    wxCheckBox* m_applyToSelectedCheck;
    wxCheckBox* m_applyToAllCheck;
    
    // Preview page controls
    wxButton* m_previewButton;
    wxStaticText* m_previewStatus;
    
    // Buttons
    wxButton* m_applyButton;
    wxButton* m_okButton;
    wxButton* m_cancelButton;
    wxButton* m_resetButton;
    
    // Current settings
    NormalFixSettings m_settings;
    
    DECLARE_EVENT_TABLE()
};
