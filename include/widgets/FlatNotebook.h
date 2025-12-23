#ifndef FLAT_NOTEBOOK_H
#define FLAT_NOTEBOOK_H

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/bitmap.h>
#include <vector>

class FlatNotebook : public wxNotebook
{
public:
    // Tab style options
    enum class TabStyle {
        DEFAULT,        // Default flat style
        MODERN,         // Modern flat style with subtle borders
        MINIMAL         // Minimal style with no borders
    };

    FlatNotebook(wxWindow* parent,
                 wxWindowID id = wxID_ANY,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 long style = 0,
                 const wxString& name = wxNotebookNameStr);

    virtual ~FlatNotebook();

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

    void SetTabBorderTopColor(const wxColour& color);
    wxColour GetTabBorderTopColor() const { return m_tabBorderTopColor; }

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
    void OnSize(wxSizeEvent& event);
    void OnEraseBackground(wxEraseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnMouseEnter(wxMouseEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnThemeChanged();

    // Drawing methods
    void DrawBackground(wxGraphicsContext& gc);
    void DrawTabs(wxGraphicsContext& gc);
    void DrawTab(wxGraphicsContext& gc, int tabIndex, const wxRect& tabRect, bool isActive, bool isHovered);
    void DrawTabContent(wxGraphicsContext& gc, int tabIndex, const wxRect& tabRect, bool isActive, bool isHovered);
    void DrawTabBorder(wxGraphicsContext& gc, const wxRect& tabRect, bool isActive, bool isHovered);

    // Helper methods
    wxRect GetTabRect(int tabIndex) const;
    int GetTabAtPosition(const wxPoint& pos) const;
    void UpdateHoverTab(int tabIndex);
    wxColour GetCurrentTabBackgroundColor(int tabIndex, bool isActive, bool isHovered) const;
    wxColour GetCurrentTabTextColor(int tabIndex, bool isActive, bool isHovered) const;
    wxColour GetCurrentTabBorderColor(int tabIndex, bool isActive, bool isHovered) const;
    void InitializeDefaultColors();
    void UpdateBackgroundColor();

    // Tab state tracking
    struct TabState {
        bool isHovered;
        bool isPressed;
    };
    std::vector<TabState> m_tabStates;
    int m_hoveredTab;

private:
    // Style and appearance
    TabStyle m_tabStyle;

    // Colors
    wxColour m_tabBackgroundColor;
    wxColour m_activeTabBackgroundColor;
    wxColour m_hoverTabBackgroundColor;
    wxColour m_tabTextColor;
    wxColour m_activeTabTextColor;
    wxColour m_hoverTabTextColor;
    wxColour m_tabBorderColor;
    wxColour m_tabBorderTopColor;

    // Dimensions
    int m_tabHorizontalPadding;
    int m_tabVerticalPadding;
    int m_tabSpacing;
    int m_tabBorderWidth;
    int m_tabBorderTop;
    int m_tabCornerRadius;

    // Font configuration
    wxFont m_customFont;
    bool m_useConfigFont;

    // Constants
    static const int DEFAULT_TAB_HORIZONTAL_PADDING = 12;
    static const int DEFAULT_TAB_VERTICAL_PADDING = 8;
    static const int DEFAULT_TAB_SPACING = 2;
    static const int DEFAULT_TAB_BORDER_WIDTH = 1;
    static const int DEFAULT_TAB_BORDER_TOP = 2;
    static const int DEFAULT_TAB_CORNER_RADIUS = 0; // FlatBar style uses no corner radius

    wxDECLARE_EVENT_TABLE();
    wxDECLARE_NO_COPY_CLASS(FlatNotebook);
};

#endif // FLAT_NOTEBOOK_H

