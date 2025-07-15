#pragma once

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <Inventor/SbVec3f.h>
#include <string>
#include <map>

class wxTextCtrl;
class wxButton;
class wxCheckBox;
class wxNotebook;
class wxPanel;
class PickingAidManager; // Forward declaration

struct GeometryParameters {
    // Common parameters
    std::string geometryType;
    
    // Box parameters
    double width = 2.0;
    double height = 2.0;
    double depth = 2.0;
    
    // Sphere parameters
    double radius = 1.0;
    
    // Cylinder parameters
    double cylinderRadius = 1.0;
    double cylinderHeight = 2.0;
    
    // Cone parameters
    double bottomRadius = 1.0;
    double topRadius = 0.0;
    double coneHeight = 2.0;
    
    // Torus parameters
    double majorRadius = 2.0;
    double minorRadius = 0.5;
    
    // Truncated Cylinder parameters
    double truncatedBottomRadius = 1.0;
    double truncatedTopRadius = 0.5;
    double truncatedHeight = 2.0;
};

class PositionDialog : public wxDialog
{
public:
    PositionDialog(wxWindow* parent, const wxString& title, PickingAidManager* pickingAidManager, const std::string& geometryType = "");
    ~PositionDialog() {}

    void SetPosition(const SbVec3f& position);
    SbVec3f GetPosition() const;
    
    void SetGeometryType(const std::string& geometryType);
    GeometryParameters GetGeometryParameters() const;

private:
    // Tab control
    wxNotebook* m_notebook;
    wxPanel* m_positionPanel;
    wxPanel* m_parametersPanel;
    
    // Position tab controls
    wxTextCtrl* m_xTextCtrl;
    wxTextCtrl* m_yTextCtrl;
    wxTextCtrl* m_zTextCtrl;
    wxTextCtrl* m_referenceZTextCtrl;
    wxCheckBox* m_showGridCheckBox;
    wxButton* m_pickButton;
    
    // Parameters tab controls
    std::map<std::string, wxTextCtrl*> m_parameterControls;
    wxStaticText* m_geometryTypeLabel;
    
    // Common controls
    wxButton* m_okButton;
    wxButton* m_cancelButton;

    PickingAidManager* m_pickingAidManager;
    GeometryParameters m_geometryParams;

    void CreatePositionTab();
    void CreateParametersTab();
    void UpdateParametersTab();
    void LoadParametersFromControls();
    void SaveParametersToControls();
    
    void OnPickButton(wxCommandEvent& event);
    void OnOkButton(wxCommandEvent& event);
    void OnCancelButton(wxCommandEvent& event);
    void OnReferenceZChanged(wxCommandEvent& event);
    void OnShowGridChanged(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    DECLARE_EVENT_TABLE()
};