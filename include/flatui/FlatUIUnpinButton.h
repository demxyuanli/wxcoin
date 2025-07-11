#ifndef FLATUIUNPINBUTTON_H
#define FLATUIUNPINBUTTON_H

#include <wx/wx.h>
#include <wx/control.h>

// Custom event for unpin button click
wxDECLARE_EVENT(wxEVT_UNPIN_BUTTON_CLICKED, wxCommandEvent);

class FlatUIUnpinButton : public wxControl
{
public:
    FlatUIUnpinButton(wxWindow* parent, wxWindowID id = wxID_ANY,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize,
                      long style = 0);
    
    virtual ~FlatUIUnpinButton();
    
    // Size management
    virtual wxSize DoGetBestSize() const override;
    
    // Event notification
    void NotifyUnpinClicked();
    
private:
    bool m_iconHover;
    
    // Icon properties
    static const int ICON_SIZE = 16;
    static const int CONTROL_WIDTH = 16;
    static const int CONTROL_HEIGHT = 16;
    
    // Event handlers
    void OnPaint(wxPaintEvent& evt);
    void OnSize(wxSizeEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);
    void OnMouseLeave(wxMouseEvent& evt);
    void OnLeftDown(wxMouseEvent& evt);
    
    // Drawing methods
    void DrawUnpinIcon(wxDC& dc);
    void DrawSvgIcon(wxDC& dc);
    void DrawFallbackIcon(wxDC& dc);
    
    wxDECLARE_EVENT_TABLE();
};

#endif // FLATUIUNPINBUTTON_H 