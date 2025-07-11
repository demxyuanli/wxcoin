#ifndef FLATUIPINBUTTON_H
#define FLATUIPINBUTTON_H

#include <wx/wx.h>
#include <wx/control.h>

class FlatUIPinButton : public wxControl
{
public:
    FlatUIPinButton(wxWindow* parent, wxWindowID id = wxID_ANY,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize,
                    long style = 0);
    
    virtual ~FlatUIPinButton();
    
    // Size management
    virtual wxSize DoGetBestSize() const override;
    
    // Event notification
    void NotifyPinClicked();
    
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
    void DrawPinIcon(wxDC& dc);
    void DrawSvgIcon(wxDC& dc);
    void DrawFallbackIcon(wxDC& dc);
    
    wxDECLARE_EVENT_TABLE();
};

// Custom event for pin button click
wxDECLARE_EVENT(wxEVT_PIN_BUTTON_CLICKED, wxCommandEvent);

#endif // FLATUIPINBUTTON_H 