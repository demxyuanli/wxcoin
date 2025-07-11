#ifndef FLATUI_SYSTEM_BUTTONS_H
#define FLATUI_SYSTEM_BUTTONS_H

#include <wx/wx.h>

class FlatUISystemButtons : public wxControl
{
public:
    FlatUISystemButtons(wxWindow* parent, wxWindowID id = wxID_ANY);
    virtual ~FlatUISystemButtons();

    // Method to set the geometry from FlatUIBar
    void SetButtonRects(const wxRect& minimizeRect, const wxRect& maximizeRect, const wxRect& closeRect);
    void SetButtonWidth(int width) { m_buttonWidth = width; }
    void SetButtonSpacing(int spacing) { m_buttonSpacing = spacing; }

    int GetConfiguredButtonWidth() const { return m_buttonWidth; }
    int GetConfiguredButtonSpacing() const { return m_buttonSpacing; }


    int GetRequiredWidth() const;
    static int GetAllRequiredWidth(int buttonWidth, int buttonSpacing);

    void OnPaint(wxPaintEvent& evt);
    void OnMouseDown(wxMouseEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);
    void OnMouseLeave(wxMouseEvent& evt);

private:
    wxRect m_minimizeButtonRect;
    wxRect m_maximizeButtonRect;
    wxRect m_closeButtonRect;

    bool m_minimizeButtonHover;
    bool m_maximizeButtonHover;
    bool m_closeButtonHover;

    int m_buttonWidth;
    int m_buttonSpacing;

    wxFrame* GetTopLevelFrame() const;
    void HandleSystemButtonAction(const wxPoint& pos, wxMouseEvent& evt);
    void PaintButton(wxDC& dc, const wxRect& rect, const wxString& symbol, bool hover, bool isClose = false, bool isMaximized = false);
    void PaintSvgButton(wxDC& dc, const wxRect& rect, const wxString& iconName, bool hover, bool isClose = false);

};

#endif // FLATUI_SYSTEM_BUTTONS_H 