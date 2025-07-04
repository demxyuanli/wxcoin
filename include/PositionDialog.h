#pragma once

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <Inventor/SbVec3f.h>

class wxTextCtrl;
class wxButton;
class wxCheckBox;
class PickingAidManager; // Forward declaration

class PositionDialog : public wxDialog
{
public:
    PositionDialog(wxWindow* parent, const wxString& title, PickingAidManager* pickingAidManager);
    ~PositionDialog() {}

    void SetPosition(const SbVec3f& position);
    SbVec3f GetPosition() const;

private:
    wxTextCtrl* m_xTextCtrl;
    wxTextCtrl* m_yTextCtrl;
    wxTextCtrl* m_zTextCtrl;
    wxTextCtrl* m_referenceZTextCtrl;
    wxCheckBox* m_showGridCheckBox;
    wxButton* m_pickButton;
    wxButton* m_okButton;
    wxButton* m_cancelButton;

    PickingAidManager* m_pickingAidManager; // Member variable

    void OnPickButton(wxCommandEvent& event);
    void OnOkButton(wxCommandEvent& event);
    void OnCancelButton(wxCommandEvent& event);
    void OnReferenceZChanged(wxCommandEvent& event);
    void OnShowGridChanged(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    DECLARE_EVENT_TABLE()
};