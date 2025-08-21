#ifndef FLATBAR_NOTEBOOK_H
#define FLATBAR_NOTEBOOK_H

#include <wx/panel.h>
#include <wx/notebook.h>
#include <vector>
#include <memory>

class FlatBarNotebook : public wxPanel
{
public:
    FlatBarNotebook(wxWindow* parent, wxWindowID id = wxID_ANY, 
                    const wxPoint& pos = wxDefaultPosition, 
                    const wxSize& size = wxDefaultSize);

    // Page management
    int AddPage(wxWindow* page, const wxString& text, bool select = false, int imageId = wxNOT_FOUND);
    bool DeletePage(size_t page);
    bool DeleteAllPages();
    
    // Selection
    int GetSelection() const;
    bool SetSelection(size_t page);
    wxWindow* GetPage(size_t page) const;
    wxString GetPageText(size_t page) const;
    bool SetPageText(size_t page, const wxString& text);
    
    // Page count
    size_t GetPageCount() const;
    
    // Event handling
    void OnPageChanged(int page);

protected:
    void OnPaint(wxPaintEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnSize(wxSizeEvent& event);
    
    void PaintTabs(wxDC& dc);
    void UpdateTabLayout();
    int HitTestTab(const wxPoint& pos) const;
    void SelectPage(int page);
    
    // FlatBar-style tab rendering
    void RenderTab(wxDC& dc, const wxRect& rect, const wxString& text, bool isActive, bool isHovered);
    
    // Tab layout calculation
    int CalculateTabWidth(wxDC& dc, const wxString& text) const;
    
    // Theme management
    void UpdateThemeColors();

private:
    struct PageInfo
    {
        wxWindow* page;
        wxString text;
        wxRect tabRect;
        bool isActive;
        
        PageInfo(wxWindow* p, const wxString& t) : page(p), text(t), isActive(false) {}
    };
    
    std::vector<std::unique_ptr<PageInfo>> m_pages;
    int m_selectedPage;
    int m_hoveredTab;
    
    // FlatBar-style tab properties
    int m_tabHeight;
    int m_tabPadding;
    int m_tabSpacing;
    int m_tabBorderTop;
    int m_tabBorderLeft;
    int m_tabBorderRight;
    
    // Colors
    wxColour m_activeTabBgColour;
    wxColour m_activeTabTextColour;
    wxColour m_inactiveTabTextColour;
    wxColour m_tabBorderTopColour;
    wxColour m_tabBorderColour;
    
    wxDECLARE_EVENT_TABLE();
};

#endif // FLATBAR_NOTEBOOK_H
