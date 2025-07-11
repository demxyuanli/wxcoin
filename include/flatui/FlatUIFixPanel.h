#ifndef FLATUIFIXPANEL_H
#define FLATUIFIXPANEL_H

#include <wx/wx.h>
#include <wx/vector.h>

// Forward declarations
class FlatUIPage;
class FlatUIUnpinButton;

class FlatUIFixPanel : public wxPanel
{
public:
    FlatUIFixPanel(wxWindow* parent, wxWindowID id = wxID_ANY);
    virtual ~FlatUIFixPanel();

    // Page management
    void AddPage(FlatUIPage* page);
    void SetActivePage(size_t pageIndex);
    void SetActivePage(FlatUIPage* page);
    FlatUIPage* GetActivePage() const;
    size_t GetActivePageIndex() const { return m_activePageIndex; }
    size_t GetPageCount() const { return m_pages.size(); }
    FlatUIPage* GetPage(size_t index) const;
    
    // Layout management
    void UpdateLayout();
    void RecalculateSize();
    
    // Unpin button management
    FlatUIUnpinButton* GetUnpinButton() const { return m_unpinButton; }
    void ShowUnpinButton(bool show = true);
    void HideUnpinButton() { ShowUnpinButton(false); }
    
    // Content management
    void ClearContent(); // Clear all pages and reset state
    void ResetState(); // Reset internal state without removing pages
    
    // Scroll functionality
    void EnableScrolling(bool enable = true);
    bool IsScrollingEnabled() const { return m_scrollingEnabled; }
    void ScrollLeft();
    void ScrollRight();
    void UpdateScrollButtons();
    
    // Override to provide best size
    virtual wxSize DoGetBestSize() const override;

protected:
    void OnSize(wxSizeEvent& event);
    void OnPaint(wxPaintEvent& event);
    void OnScrollLeft(wxCommandEvent& event);
    void OnScrollRight(wxCommandEvent& event);

private:
    // Page management
    wxVector<FlatUIPage*> m_pages;
    size_t m_activePageIndex;
    
    // Controls
    FlatUIUnpinButton* m_unpinButton;
    
    // Scroll functionality
    bool m_scrollingEnabled;
    wxPanel* m_scrollContainer;
    wxButton* m_leftScrollButton;
    wxButton* m_rightScrollButton;
    int m_scrollOffset;
    int m_scrollStep;
    wxBoxSizer* m_mainSizer;
    wxBoxSizer* m_scrollSizer;
    
    // Layout
    void PositionUnpinButton();
    void PositionActivePage();
    void HideAllPages();
    void CreateScrollControls();
    void UpdateScrollPosition();
    bool NeedsScrolling() const;
    
    wxDECLARE_EVENT_TABLE();
};

#endif // FLATUIFIXPANEL_H 