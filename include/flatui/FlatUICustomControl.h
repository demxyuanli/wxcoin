#ifndef FLATUI_CUSTOM_CONTROL_H
#define FLATUI_CUSTOM_CONTROL_H

#include <wx/wx.h>

class FlatUICustomControl : public wxControl
{
public:
    FlatUICustomControl(wxWindow* parent, wxWindowID id = wxID_ANY);
    virtual ~FlatUICustomControl();

    void OnPaint(wxPaintEvent& evt);
    void OnMouseDown(wxMouseEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);
    void OnMouseLeave(wxMouseEvent& evt);
    
    void SetBackgroundColor(const wxColour& color);
    
private:
    wxColour m_backgroundColor;
    bool m_hover;
};

#endif // FLATUI_CUSTOM_CONTROL_H 