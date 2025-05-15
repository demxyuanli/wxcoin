#pragma once

#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <Inventor/SbVec3f.h>

class PositionDialog : public wxDialog
{
public:
    PositionDialog(wxWindow* parent, const wxString& title);
    ~PositionDialog() {}

    void SetPosition(const SbVec3f& position);
    SbVec3f GetPosition() const;

private:
    wxTextCtrl* m_xTextCtrl;
    wxTextCtrl* m_yTextCtrl;
    wxTextCtrl* m_zTextCtrl;
    wxButton* m_pickButton;
    wxButton* m_okButton;
    wxButton* m_cancelButton;

    void OnPickButton(wxCommandEvent& event);
    void OnOkButton(wxCommandEvent& event);
    void OnCancelButton(wxCommandEvent& event);

    DECLARE_EVENT_TABLE()
};

// Define a global variable indicating if we're in coordinate picking mode
extern bool g_isPickingPosition; 