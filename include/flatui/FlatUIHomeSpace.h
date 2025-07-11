#ifndef FLATUI_HOME_SPACE_H
#define FLATUI_HOME_SPACE_H

#include <wx/wx.h>

class FlatUIHomeMenu; // Forward declaration

class FlatUIHomeSpace : public wxControl
{
public:
    FlatUIHomeSpace(wxWindow* parent, wxWindowID id = wxID_ANY);
    virtual ~FlatUIHomeSpace();

    void SetIcon(const wxBitmap& icon = wxNullBitmap);
    void SetButtonWidth(int width) { m_buttonWidth = width; Refresh(); InvalidateBestSize(); }
    int GetButtonWidth() const { return m_buttonWidth; }

    void OnPaint(wxPaintEvent& evt);
    void OnMouseDown(wxMouseEvent& evt);
    void OnMouseMove(wxMouseEvent& evt);
    void OnMouseLeave(wxMouseEvent& evt);

    void CalculateButtonRect(const wxSize& controlSize);
    void OnHomeMenuClosed(FlatUIHomeMenu* closedMenu); // Method to be called by FlatUIHomeMenu

    void SetHomeMenu(FlatUIHomeMenu* menu);
    FlatUIHomeMenu* GetActiveHomeMenu() const { return m_activeHomeMenu; }

    void SetShow(bool isShow) { m_show = isShow; }

private:
    wxBitmap m_icon;
    wxRect m_buttonRect;
    bool m_hover;
    bool m_show = false;
    int m_buttonWidth;
    FlatUIHomeMenu* m_activeHomeMenu; // Pointer to the active menu

};

#endif // FLATUI_HOME_SPACE_H 