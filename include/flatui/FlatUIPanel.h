#ifndef FLATUIPANEL_H
#define FLATUIPANEL_H

#include <wx/wx.h>
#include <wx/vector.h>
#include <wx/timer.h>
#include <string>


// Forward declarations
class FlatUIPage;
class FlatUIButtonBar;
class FlatUIGallery;
// wxBoxSizer is a wxWidgets class, typically available through <wx/wx.h> or <wx/sizer.h>
// If wxBoxSizer* m_sizer; causes issues, ensure <wx/sizer.h> is included or wx/wx.h is sufficient.

enum class PanelBorderStyle {
    NONE,     
    THIN,     
    MEDIUM,   
    THICK,     
    ROUNDED  
};

enum class PanelHeaderStyle {
    NONE,    
    TOP,     
    LEFT,      
    EMBEDDED,  
    BOTTOM_CENTERED // Header at the bottom, text centered
};

class FlatUIPanel : public wxControl
{
public:
    FlatUIPanel(FlatUIPage* parent, const wxString& label, int orientation = wxVERTICAL);
    virtual ~FlatUIPanel();

    void AddButtonBar(FlatUIButtonBar* buttonBar, int proportion = 0, int flag = wxEXPAND | wxALL, int border = 5);
    void AddGallery(FlatUIGallery* gallery, int proportion = 0, int flag = wxEXPAND | wxALL, int border = 5);
    wxString GetLabel() const { return m_label; }
    
    void SetPanelBackgroundColour(const wxColour& colour);
    wxColour GetPanelBackgroundColour() const { return m_bgColour; }
    
    void SetBorderStyle(PanelBorderStyle style);
    PanelBorderStyle GetBorderStyle() const { return m_borderStyle; }
    
    void SetBorderColour(const wxColour& colour);
    wxColour GetBorderColour() const { return m_borderColour; }
    
    // Panel border configuration
    void SetPanelBorderWidths(int top, int bottom, int left, int right);
    void GetPanelBorderWidths(int& top, int& bottom, int& left, int& right) const;
    
    void SetHeaderStyle(PanelHeaderStyle style);
    PanelHeaderStyle GetHeaderStyle() const { return m_headerStyle; }
    
    void SetHeaderColour(const wxColour& colour);
    wxColour GetHeaderColour() const { return m_headerColour; }
    
    void SetHeaderTextColour(const wxColour& colour);
    wxColour GetHeaderTextColour() const { return m_headerTextColour; }
    
    // Header border configuration
    void SetHeaderBorderWidths(int top, int bottom, int left, int right);
    void GetHeaderBorderWidths(int& top, int& bottom, int& left, int& right) const;
    void SetHeaderBorderColour(const wxColour& colour);
    wxColour GetHeaderBorderColour() const { return m_headerBorderColour; }
    
    void SetLabel(const wxString& label);
    
    // Force recalculation of panel size based on child controls
    void UpdatePanelSize();

    void OnPaint(wxPaintEvent& evt); // Keep if specific OnPaint is needed, otherwise wxControl default
    
    // Timer event handler
    void OnTimer(wxTimerEvent& event);

private:
    void DrawWithDC(wxDC& dc, const wxSize& size);
    
    // Resize child controls to fit within the panel dimensions
    void ResizeChildControls(int width, int height);
    
    // Recalculate the best size based on child controls
    void RecalculateBestSize();
    
    wxString m_label;
    wxVector<FlatUIButtonBar*> m_buttonBars;
    wxVector<FlatUIGallery*> m_galleries;
    wxBoxSizer* m_sizer; // This was confirmed as a member
    int m_orientation;
    
    wxColour m_bgColour;            
    PanelBorderStyle m_borderStyle;  
    wxColour m_borderColour;
    
    // Panel border properties
    int m_panelBorderTop;
    int m_panelBorderBottom;
    int m_panelBorderLeft;
    int m_panelBorderRight;
    
    PanelHeaderStyle m_headerStyle; 
    wxColour m_headerColour;         
    wxColour m_headerTextColour;
    
    // Header border properties
    int m_headerBorderTop;
    int m_headerBorderBottom;
    int m_headerBorderLeft;
    int m_headerBorderRight;
    wxColour m_headerBorderColour;
    
    // Class level timer for size updates
    wxTimer m_resizeTimer;
};

#endif // FLATUIPANEL_H 