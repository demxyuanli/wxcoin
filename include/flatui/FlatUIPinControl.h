#ifndef FLATUIPINCONTROL_H
#define FLATUIPINCONTROL_H

#include <wx/wx.h>
#include <wx/control.h>

class FlatUIPinControl : public wxControl
{
public:
    FlatUIPinControl(wxWindow* parent, wxWindowID id = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = 0);
    
    virtual ~FlatUIPinControl();
    
    // Pin state management
    void SetPinned(bool pinned);
    bool IsPinned() const { return m_isPinned; }
    void TogglePinState();
    
    // Size management
    virtual wxSize DoGetBestSize() const override;
    
    // Event notification
    void NotifyPinStateChanged();
    
private:
    // Pin state
    bool m_isPinned;
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
    void DrawSvgIcon(wxDC& dc, const wxString& iconName);
    void DrawFallbackIcon(wxDC& dc);
    
    wxDECLARE_EVENT_TABLE();
};

// Custom event for pin state change
wxDECLARE_EVENT(wxEVT_PIN_STATE_CHANGED, wxCommandEvent);

#endif // FLATUIPINCONTROL_H