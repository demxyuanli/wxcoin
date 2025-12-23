#ifndef FLAT_NOTEBOOK_H
#define FLAT_NOTEBOOK_H

#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/bitmap.h>
#include <wx/graphics.h>
#include <wx/scrolbar.h>
#include <vector>
#include <memory>

class FlatNotebook : public wxPanel
{
public:
    // Tab style options
    enum class TabStyle {
        DEFAULT,        // Default flat style
        MODERN,         // Modern flat style with subtle borders
        MINIMAL         // Minimal style with no borders
    };

    // Tab position options
    enum class TabPosition {
        Top,
        Bottom,
        Left,
        Right
    };

    FlatNotebook(wxWindow* parent,
                 wxWindowID id = wxID_ANY,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = wxTAB_TRAVERSAL | wxNO_BORDER,
                 const wxString& name = wxPanelNameStr);

    virtual ~FlatNotebook();

    // Tab position management
    void SetTabPosition(TabPosition position);
    TabPosition GetTabPosition() const { return m_tabPosition; }

    // Layout helpers
    wxRect GetContentRect(const wxSize& clientSize) const;

    // Drawing helpers
    void DrawTabs(wxDC& dc);
    void RenderTab(wxDC& dc, const wxRect& rect, const wxString& text, bool isActive, bool isHovered);
    void DrawTabText(wxDC& dc, const wxRect& rect, const wxString& text);

    // Layout update helpers
    void UpdateTabLayout();

    // Page management (wxNotebook-like interface)
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

    // Tab style management
    void SetTabStyle(TabStyle style);
    TabStyle GetTabStyle() const { return m_tabStyle; }

    // Colors
    void SetTabBackgroundColor(const wxColour& color);
    wxColour GetTabBackgroundColor() const { return m_tabBackgroundColor; }

    void SetActiveTabBackgroundColor(const wxColour& color);
    wxColour GetActiveTabBackgroundColor() const { return m_activeTabBackgroundColor; }

    void SetHoverTabBackgroundColor(const wxColour& color);
    wxColour GetHoverTabBackgroundColor() const { return m_hoverTabBackgroundColor; }

    void SetTabTextColor(const wxColour& color);
    wxColour GetTabTextColor() const { return m_tabTextColor; }

    void SetActiveTabTextColor(const wxColour& color);
    wxColour GetActiveTabTextColor() const { return m_activeTabTextColor; }

    void SetHoverTabTextColor(const wxColour& color);
    wxColour GetHoverTabTextColor() const { return m_hoverTabTextColor; }

    void SetTabBorderColor(const wxColour& color);
    wxColour GetTabBorderColor() const { return m_tabBorderColor; }

    // Tab spacing and padding
    void SetTabPadding(int horizontal, int vertical);
    void GetTabPadding(int& horizontal, int& vertical) const;

    void SetTabSpacing(int spacing);
    int GetTabSpacing() const { return m_tabSpacing; }

    // Font configuration
    void SetCustomFont(const wxFont& font);
    wxFont GetCustomFont() const { return m_customFont; }
    void UseConfigFont(bool useConfig = true);
    bool IsUsingConfigFont() const { return m_useConfigFont; }
    void ReloadFontFromConfig();

protected:
    // Event handlers
    void OnPaint(wxPaintEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnThemeChanged();

    // Tab layout calculation
    int CalculateTabWidth(wxDC& dc, const wxString& text) const;

    // Tab state tracking
    int HitTestTab(const wxPoint& pos) const;
    void SelectPage(int page);

    // Theme management
    void InitializeDefaultColors();

    // Tab state tracking

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

    // Style and appearance
    TabStyle m_tabStyle;
    TabPosition m_tabPosition;

    // Colors
    wxColour m_tabBackgroundColor;
    wxColour m_activeTabBackgroundColor;
    wxColour m_hoverTabBackgroundColor;
    wxColour m_tabTextColor;
    wxColour m_activeTabTextColor;
    wxColour m_hoverTabTextColor;
    wxColour m_tabBorderColor;
    wxColour m_tabBorderLeftColor;

    // Dimensions
    int m_tabHeight;
    int m_tabPadding;
    int m_tabHorizontalPadding;
    int m_tabVerticalPadding;
    int m_tabSpacing;
    int m_tabBorderLeft;
    int m_tabBorderTop;
    int m_tabBorderBottom;

    // Font configuration
    wxFont m_customFont;
    bool m_useConfigFont;

    // Scrolling for vertical tabs
    wxScrollBar* m_scrollBar;
    int m_scrollOffset;

    // Scrollbar management
    void UpdateScrollbar();
    void OnScroll(wxScrollEvent& event);

public:
    // Constants
    static const int DEFAULT_TAB_HORIZONTAL_PADDING = 12;
    static const int DEFAULT_TAB_VERTICAL_PADDING = 8;
    static const int DEFAULT_TAB_SPACING = 2;
    static const int DEFAULT_TAB_BORDER_LEFT = 2;
    static const int DEFAULT_TAB_BORDER_TOP = 1;
    static const int DEFAULT_TAB_BORDER_BOTTOM = 1;

    wxDECLARE_EVENT_TABLE();
    wxDECLARE_NO_COPY_CLASS(FlatNotebook);
};

#endif // FLAT_NOTEBOOK_H

