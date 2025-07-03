#pragma once

#include <wx/dialog.h>
#include <wx/spinctrl.h>
#include <wx/colour.h>
#include <wx/colordlg.h>

class NavigationCubeConfigDialog : public wxDialog {
public:
    NavigationCubeConfigDialog(wxWindow* parent, int x, int y, int size, int viewportSize, const wxColour& color, int maxX, int maxY);
    int GetX() const { return m_xCtrl->GetValue(); }
    int GetY() const { return m_yCtrl->GetValue(); }
    int GetSize() const { return m_sizeCtrl->GetValue(); }
    int GetViewportSize() const { return m_viewportSizeCtrl->GetValue(); }
    wxColour GetColor() const { return m_color; }

private:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnChooseColor(wxCommandEvent& event);

    wxSpinCtrl* m_xCtrl;
    wxSpinCtrl* m_yCtrl;
    wxSpinCtrl* m_sizeCtrl;
    wxSpinCtrl* m_viewportSizeCtrl;
    wxButton* m_colorButton;
    wxColour m_color;

    DECLARE_EVENT_TABLE()
}; 