#include "widgets/FlatNotebook.h"
#include "config/ThemeManager.h"
#include "config/FontManager.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/settings.h>

wxBEGIN_EVENT_TABLE(FlatNotebook, wxNotebook)
    EVT_PAINT(FlatNotebook::OnPaint)
    EVT_SIZE(FlatNotebook::OnSize)
    EVT_ERASE_BACKGROUND(FlatNotebook::OnEraseBackground)
    EVT_MOTION(FlatNotebook::OnMouseMove)
    EVT_LEAVE_WINDOW(FlatNotebook::OnMouseLeave)
    EVT_ENTER_WINDOW(FlatNotebook::OnMouseEnter)
    EVT_LEFT_DOWN(FlatNotebook::OnLeftDown)
wxEND_EVENT_TABLE()

FlatNotebook::FlatNotebook(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
    : wxNotebook(parent, id, pos, size, style | wxNB_LEFT | wxBORDER_NONE, name)
    , m_tabStyle(TabStyle::DEFAULT)
    , m_tabHorizontalPadding(DEFAULT_TAB_HORIZONTAL_PADDING)
    , m_tabVerticalPadding(DEFAULT_TAB_VERTICAL_PADDING)
    , m_tabSpacing(DEFAULT_TAB_SPACING)
    , m_tabBorderWidth(DEFAULT_TAB_BORDER_WIDTH)
    , m_tabBorderTop(DEFAULT_TAB_BORDER_TOP)
    , m_tabCornerRadius(DEFAULT_TAB_CORNER_RADIUS)
    , m_useConfigFont(true)
    , m_hoveredTab(-1)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetDoubleBuffered(true);
    
    InitializeDefaultColors();
    ReloadFontFromConfig();
    UpdateBackgroundColor();
    
    ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
        OnThemeChanged();
    });
}

FlatNotebook::~FlatNotebook()
{
    ThemeManager::getInstance().removeThemeChangeListener(this);
}

void FlatNotebook::InitializeDefaultColors()
{
    // Use FlatBar-style colors
    m_tabBackgroundColor = CFG_COLOUR("SecondaryBackgroundColour");
    m_activeTabBackgroundColor = CFG_COLOUR("BarActiveTabBgColour");
    m_hoverTabBackgroundColor = CFG_COLOUR("HomespaceHoverBgColour");
    m_tabTextColor = CFG_COLOUR("BarInactiveTextColour");
    m_activeTabTextColor = CFG_COLOUR("BarActiveTextColour");
    m_hoverTabTextColor = CFG_COLOUR("BarActiveTextColour");
    m_tabBorderColor = CFG_COLOUR("BarTabBorderColour");
    m_tabBorderTopColor = CFG_COLOUR("BarTabBorderTopColour");
}

void FlatNotebook::OnThemeChanged()
{
    InitializeDefaultColors();
    ReloadFontFromConfig();
    UpdateBackgroundColor();
    Refresh();
}

void FlatNotebook::ReloadFontFromConfig()
{
    if (m_useConfigFont)
    {
        m_customFont = CFG_FONT();
    }
    Refresh();
}

void FlatNotebook::OnPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(this);
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    
    if (!gc)
    {
        event.Skip();
        return;
    }
    
    gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);
    
    DrawBackground(*gc);
    DrawTabs(*gc);
    
    delete gc;
    event.Skip();
}

void FlatNotebook::OnSize(wxSizeEvent& event)
{
    Refresh();
    event.Skip();
}

void FlatNotebook::OnEraseBackground(wxEraseEvent& event)
{
}

void FlatNotebook::OnMouseMove(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    int tabIndex = GetTabAtPosition(pos);
    UpdateHoverTab(tabIndex);
    event.Skip();
}

void FlatNotebook::OnMouseLeave(wxMouseEvent& event)
{
    UpdateHoverTab(-1);
    event.Skip();
}

void FlatNotebook::OnMouseEnter(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    int tabIndex = GetTabAtPosition(pos);
    UpdateHoverTab(tabIndex);
    event.Skip();
}

void FlatNotebook::OnLeftDown(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    int tabIndex = GetTabAtPosition(pos);
    
    if (tabIndex >= 0 && tabIndex < GetPageCount())
    {
        SetSelection(tabIndex);
        Refresh();
    }
    
    event.Skip();
}

void FlatNotebook::DrawBackground(wxGraphicsContext& gc)
{
    wxSize size = GetClientSize();
    wxColour bgColor = m_tabBackgroundColor;
    
    gc.SetBrush(wxBrush(bgColor));
    gc.SetPen(*wxTRANSPARENT_PEN);
    gc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());
}

void FlatNotebook::UpdateBackgroundColor()
{
    SetBackgroundColour(m_tabBackgroundColor);
}

void FlatNotebook::DrawTabs(wxGraphicsContext& gc)
{
    int pageCount = GetPageCount();
    if (pageCount == 0) return;
    
    int currentSelection = GetSelection();
    
    for (int i = 0; i < pageCount; ++i)
    {
        wxRect tabRect = GetTabRect(i);
        bool isActive = (i == currentSelection);
        bool isHovered = (i == m_hoveredTab);
        
        DrawTab(gc, i, tabRect, isActive, isHovered);
    }
}

void FlatNotebook::DrawTab(wxGraphicsContext& gc, int tabIndex, const wxRect& tabRect, bool isActive, bool isHovered)
{
    // FlatBar style: only active tab has background and borders
    if (isActive)
    {
        // Draw background (excluding top border area)
        wxColour bgColor = GetCurrentTabBackgroundColor(tabIndex, isActive, isHovered);
        gc.SetBrush(wxBrush(bgColor));
        gc.SetPen(*wxTRANSPARENT_PEN);
        gc.DrawRectangle(tabRect.x, tabRect.y + m_tabBorderTop, tabRect.width, tabRect.height - m_tabBorderTop);
        
        // Draw borders (top, left, right)
        DrawTabBorder(gc, tabRect, isActive, isHovered);
    }
    else
    {
        // Inactive tab: no background, no borders (FlatBar style)
        // Only draw text
    }
    
    // Draw tab content (text)
    DrawTabContent(gc, tabIndex, tabRect, isActive, isHovered);
}

void FlatNotebook::DrawTabContent(wxGraphicsContext& gc, int tabIndex, const wxRect& tabRect, bool isActive, bool isHovered)
{
    if (tabIndex < 0 || tabIndex >= GetPageCount()) return;
    
    wxString pageText = GetPageText(tabIndex);
    if (pageText.IsEmpty()) return;
    
    wxColour textColor = GetCurrentTabTextColor(tabIndex, isActive, isHovered);
    
    wxFont font = m_useConfigFont ? m_customFont : GetFont();
    if (!font.IsOk())
    {
        font = GetFont();
    }
    
    gc.SetFont(font, textColor);
    
    double textWidth, textHeight, descent, externalLeading;
    gc.GetTextExtent(pageText, &textWidth, &textHeight, &descent, &externalLeading);
    
    double textX = tabRect.x + m_tabHorizontalPadding;
    double textY = tabRect.y + (tabRect.height - textHeight) / 2.0;
    
    gc.DrawText(pageText, textX, textY);
}

void FlatNotebook::DrawTabBorder(wxGraphicsContext& gc, const wxRect& tabRect, bool isActive, bool isHovered)
{
    // FlatBar style: only active tab has borders
    if (!isActive) return;
    
    // Draw top border
    if (m_tabBorderTop > 0 && m_tabBorderTopColor.IsOk())
    {
        gc.SetPen(wxPen(m_tabBorderTopColor, m_tabBorderTop));
        gc.StrokeLine(tabRect.x, tabRect.y + m_tabBorderTop / 2.0, 
                     tabRect.x + tabRect.width, tabRect.y + m_tabBorderTop / 2.0);
    }
    
    // Draw left border
    if (m_tabBorderWidth > 0 && m_tabBorderColor.IsOk())
    {
        gc.SetPen(wxPen(m_tabBorderColor, m_tabBorderWidth));
        gc.StrokeLine(tabRect.x, tabRect.y + m_tabBorderTop, 
                     tabRect.x, tabRect.y + tabRect.height);
    }
    
    // Draw right border
    if (m_tabBorderWidth > 0 && m_tabBorderColor.IsOk())
    {
        gc.SetPen(wxPen(m_tabBorderColor, m_tabBorderWidth));
        gc.StrokeLine(tabRect.x + tabRect.width, tabRect.y + m_tabBorderTop, 
                     tabRect.x + tabRect.width, tabRect.y + tabRect.height);
    }
}

wxRect FlatNotebook::GetTabRect(int tabIndex) const
{
    if (tabIndex < 0 || tabIndex >= GetPageCount())
    {
        return wxRect();
    }
    
    wxSize clientSize = GetClientSize();
    int tabWidth = clientSize.GetWidth();
    
    wxFont font = m_useConfigFont ? m_customFont : GetFont();
    if (!font.IsOk())
    {
        font = GetFont();
    }
    
    wxClientDC dc(const_cast<FlatNotebook*>(this));
    dc.SetFont(font);
    
    wxString pageText = GetPageText(tabIndex);
    wxSize textSize = dc.GetTextExtent(pageText);
    
    int tabHeight = textSize.GetHeight() + m_tabVerticalPadding * 2;
    if (tabHeight < 30)
    {
        tabHeight = 30;
    }
    
    int yPos = 0;
    for (int i = 0; i < tabIndex; ++i)
    {
        wxString prevText = GetPageText(i);
        wxSize prevTextSize = dc.GetTextExtent(prevText);
        int prevTabHeight = prevTextSize.GetHeight() + m_tabVerticalPadding * 2;
        if (prevTabHeight < 30)
        {
            prevTabHeight = 30;
        }
        yPos += prevTabHeight + m_tabSpacing;
    }
    
    return wxRect(0, yPos, tabWidth, tabHeight);
}

int FlatNotebook::GetTabAtPosition(const wxPoint& pos) const
{
    int pageCount = GetPageCount();
    for (int i = 0; i < pageCount; ++i)
    {
        wxRect tabRect = GetTabRect(i);
        if (tabRect.Contains(pos))
        {
            return i;
        }
    }
    return -1;
}

void FlatNotebook::UpdateHoverTab(int tabIndex)
{
    if (m_hoveredTab != tabIndex)
    {
        m_hoveredTab = tabIndex;
        Refresh();
    }
}

wxColour FlatNotebook::GetCurrentTabBackgroundColor(int tabIndex, bool isActive, bool isHovered) const
{
    // FlatBar style: only active tab has background
    if (isActive)
    {
        return m_activeTabBackgroundColor;
    }
    else if (isHovered)
    {
        // Hovered inactive tab can have subtle background
        return m_hoverTabBackgroundColor;
    }
    else
    {
        // Inactive tab: transparent background
        return wxColour(0, 0, 0, 0); // Transparent
    }
}

wxColour FlatNotebook::GetCurrentTabTextColor(int tabIndex, bool isActive, bool isHovered) const
{
    if (isActive)
    {
        return m_activeTabTextColor;
    }
    else if (isHovered)
    {
        return m_hoverTabTextColor;
    }
    else
    {
        return m_tabTextColor;
    }
}

wxColour FlatNotebook::GetCurrentTabBorderColor(int tabIndex, bool isActive, bool isHovered) const
{
    // FlatBar style: only active tab has borders
    if (isActive)
    {
        return m_tabBorderColor;
    }
    return wxColour(); // No border for inactive tabs
}

void FlatNotebook::SetTabStyle(TabStyle style)
{
    m_tabStyle = style;
    Refresh();
}

void FlatNotebook::SetTabBackgroundColor(const wxColour& color)
{
    m_tabBackgroundColor = color;
    Refresh();
}

void FlatNotebook::SetActiveTabBackgroundColor(const wxColour& color)
{
    m_activeTabBackgroundColor = color;
    Refresh();
}

void FlatNotebook::SetHoverTabBackgroundColor(const wxColour& color)
{
    m_hoverTabBackgroundColor = color;
    Refresh();
}

void FlatNotebook::SetTabTextColor(const wxColour& color)
{
    m_tabTextColor = color;
    Refresh();
}

void FlatNotebook::SetActiveTabTextColor(const wxColour& color)
{
    m_activeTabTextColor = color;
    Refresh();
}

void FlatNotebook::SetHoverTabTextColor(const wxColour& color)
{
    m_hoverTabTextColor = color;
    Refresh();
}

void FlatNotebook::SetTabBorderColor(const wxColour& color)
{
    m_tabBorderColor = color;
    Refresh();
}

void FlatNotebook::SetTabBorderTopColor(const wxColour& color)
{
    m_tabBorderTopColor = color;
    Refresh();
}

void FlatNotebook::SetTabPadding(int horizontal, int vertical)
{
    m_tabHorizontalPadding = horizontal;
    m_tabVerticalPadding = vertical;
    Refresh();
}

void FlatNotebook::GetTabPadding(int& horizontal, int& vertical) const
{
    horizontal = m_tabHorizontalPadding;
    vertical = m_tabVerticalPadding;
}

void FlatNotebook::SetTabSpacing(int spacing)
{
    m_tabSpacing = spacing;
    Refresh();
}

void FlatNotebook::SetCustomFont(const wxFont& font)
{
    m_customFont = font;
    m_useConfigFont = false;
    Refresh();
}

void FlatNotebook::UseConfigFont(bool useConfig)
{
    m_useConfigFont = useConfig;
    if (useConfig)
    {
        ReloadFontFromConfig();
    }
    Refresh();
}

