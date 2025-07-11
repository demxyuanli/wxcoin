#ifndef FLATUI_SPACER_CONTROL_H
#define FLATUI_SPACER_CONTROL_H

#include <wx/wx.h>
class FlatUISpacerControl : public wxControl
{
public:
    FlatUISpacerControl(wxWindow* parent, int width = 10, wxWindowID id = wxID_ANY);

    virtual ~FlatUISpacerControl();

    void SetSpacerWidth(int width);

    int GetSpacerWidth() const { return m_width; }

    void SetDrawSeparator(bool draw) { m_drawSeparator = draw; Refresh(); }

    bool GetDrawSeparator() const { return m_drawSeparator; }
    
    void SetAutoExpand(bool autoExpand) { m_autoExpand = autoExpand; }

    void SetShowDragFlag(bool dragFlag) { m_showDragFlag = dragFlag; }
    
    bool GetAutoExpand() const { return m_autoExpand; }
    
    int CalculateAutoWidth(int availableWidth) const;
    
    void SetCanDragWindow(bool canDrag);
    
    bool GetCanDragWindow() const { return m_canDragWindow; }

protected:
    void OnPaint(wxPaintEvent& evt);
    void OnLeftDown(wxMouseEvent& evt);
    void OnLeftUp(wxMouseEvent& evt);
    void OnMotion(wxMouseEvent& evt);
    
private:
    int m_width;            
    bool m_drawSeparator;   
    bool m_autoExpand;
    bool m_canDragWindow;   
    bool m_showDragFlag;
    bool m_dragging;       
    wxPoint m_dragStartPos; 
};

#endif // FLATUI_SPACER_CONTROL_H 