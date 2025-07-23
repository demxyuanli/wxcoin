#ifndef FLATUI_TITLED_PANEL_H
#define FLATUI_TITLED_PANEL_H

#include "flatui/FlatUIThemeAware.h"
#include <wx/panel.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <vector>

class FlatUITitledPanel : public FlatUIThemeAware {
public:
    FlatUITitledPanel(wxWindow* parent, const wxString& title, long style = 0);
    virtual ~FlatUITitledPanel();

    void SetTitle(const wxString& title);
    void AddToolButton(wxWindow* button);
    void ClearToolButtons();

    virtual void OnThemeChanged() override;
    virtual void UpdateThemeValues() override;

protected:
    wxPanel* m_titleBar;           
    wxStaticText* m_titleLabel;   
    wxBoxSizer* m_toolBarSizer;   
    wxBoxSizer* m_mainSizer;       
};

#endif // FLATUI_TITLED_PANEL_H 