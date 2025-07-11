#ifndef FLATUIPAGE_H
#define FLATUIPAGE_H

#include "flatui/FlatUIPinControl.h"
#include <wx/wx.h>
#include <wx/vector.h>
#include <string>

// Forward declarations
class FlatUIBar;
class FlatUIPanel;

class FlatUIPage : public wxControl
{
public:
    // Constructor takes a wxWindow* parent 
    FlatUIPage(wxWindow* parent, const wxString& label);
    virtual ~FlatUIPage();

    void InitializeLayout();

    void AddPanel(FlatUIPanel* panel);

    wxString GetLabel() const { return m_label; }
    
    wxVector<FlatUIPanel*>& GetPanels() { return m_panels; }
    const wxVector<FlatUIPanel*>& GetPanels() const { return m_panels; }

    void SetActive(bool active) { m_isActive = active; }
    bool IsActive() const { return m_isActive; }
    void RecalculatePageHeight(); // Declare the method

    void OnPaint(wxPaintEvent& evt);
    void OnSize(wxSizeEvent& evt);

    void UpdateLayout();

private:

    wxString m_label;
    wxVector<FlatUIPanel*> m_panels;
    wxBoxSizer* m_sizer;
    bool m_isActive; 

};

#endif // FLATUIPAGE_H 