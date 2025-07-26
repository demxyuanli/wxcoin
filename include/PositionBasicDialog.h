#pragma once

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <string>
#include <map>
#include <vector>
#include <Inventor/SbVec3f.h>
#include "GeometryDialogTypes.h"

// Forward declarations
class PickingAidManager;

// Callback function type for position picking completion
typedef std::function<void(const SbVec3f&)> PositionPickingCallback;

class PositionBasicDialog : public wxDialog {
public:
    PositionBasicDialog(wxWindow* parent, const wxString& title, PickingAidManager* pickingAidManager, const std::string& geometryType = "");
    ~PositionBasicDialog() {}

    void SetPosition(const SbVec3f& position);
    SbVec3f GetPosition() const;
    void SetGeometryType(const std::string& geometryType);
    BasicGeometryParameters GetBasicParameters() const;

private:
    wxTextCtrl* m_xTextCtrl;
    wxTextCtrl* m_yTextCtrl;
    wxTextCtrl* m_zTextCtrl;
    wxTextCtrl* m_referenceZTextCtrl;
    wxCheckBox* m_showGridCheckBox;
    wxButton* m_pickButton;
    std::map<std::string, wxTextCtrl*> m_parameterControls;
    wxStaticText* m_geometryTypeLabel;
    
    wxPanel* m_positionPanel;
    wxPanel* m_parametersPanel;
    wxBoxSizer* m_parametersSizer;
    PickingAidManager* m_pickingAidManager;
    PositionPickingCallback m_pickingCallback;

    BasicGeometryParameters m_basicParams;
    void CreatePositionTab();
    void CreateParametersTab();
    void UpdateParametersTab();
    void LoadParametersFromControls();
    void SaveParametersToControls();
    void OnPickButton(wxCommandEvent& event);
    void OnOkButton(wxCommandEvent& event);
    void OnCancelButton(wxCommandEvent& event);
    void OnShowGridChanged(wxCommandEvent& event);
    void OnReferenceZChanged(wxCommandEvent& event);
    
    DECLARE_EVENT_TABLE()

public:
    // Public method to set picking callback
    void SetPickingCallback(PositionPickingCallback callback) { m_pickingCallback = callback; }
    
    // Public method to handle picking completion
    void OnPickingComplete(const SbVec3f& position);
}; 