#include "widgets/FlatNotebook.h"
#include "config/ThemeManager.h"
#include "config/FontManager.h"
#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dc.h>
#include <wx/sizer.h>
#include <wx/graphics.h>
#include <wx/scrolbar.h>
#include <wx/settings.h>
#include <cmath>

wxBEGIN_EVENT_TABLE(FlatNotebook, wxPanel)
    EVT_PAINT(FlatNotebook::OnPaint)
    EVT_LEFT_DOWN(FlatNotebook::OnLeftDown)
    EVT_MOTION(FlatNotebook::OnMouseMove)
    EVT_LEAVE_WINDOW(FlatNotebook::OnMouseLeave)
    EVT_SIZE(FlatNotebook::OnSize)
    EVT_SCROLL(FlatNotebook::OnScroll)
wxEND_EVENT_TABLE()

FlatNotebook::FlatNotebook(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
    : wxPanel(parent, id, pos, size, style, name)
    , m_selectedPage(-1)
    , m_hoveredTab(-1)
    , m_tabStyle(TabStyle::DEFAULT)
    , m_tabPosition(TabPosition::Top)  // Default position
    , m_tabHeight(24)
    , m_tabPadding(DEFAULT_TAB_HORIZONTAL_PADDING)
    , m_tabHorizontalPadding(DEFAULT_TAB_HORIZONTAL_PADDING)
    , m_tabVerticalPadding(DEFAULT_TAB_VERTICAL_PADDING)
    , m_tabSpacing(DEFAULT_TAB_SPACING)
    , m_tabBorderLeft(DEFAULT_TAB_BORDER_LEFT)
    , m_tabBorderTop(DEFAULT_TAB_BORDER_TOP)
    , m_tabBorderBottom(DEFAULT_TAB_BORDER_BOTTOM)
    , m_useConfigFont(true)
    , m_scrollBar(nullptr)
    , m_scrollOffset(0)
{
    // Set background style for custom painting
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetDoubleBuffered(true);

    // Set tab position based on style
    if (style & wxNB_LEFT) {
        m_tabPosition = TabPosition::Left;
    } else if (style & wxNB_RIGHT) {
        m_tabPosition = TabPosition::Right;
    } else if (style & wxNB_BOTTOM) {
        m_tabPosition = TabPosition::Bottom;
    } else {
        m_tabPosition = TabPosition::Top;  // Default to top
    }

    // Initialize FlatBar-style tab properties
    m_tabHeight = 24;
    m_tabPadding = DEFAULT_TAB_HORIZONTAL_PADDING;

    // Initialize colors from theme
    InitializeDefaultColors();

    // Set minimum size
    SetMinSize(wxSize(200, m_tabHeight + 100));

    ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
        OnThemeChanged();
    });
}

FlatNotebook::~FlatNotebook()
{
    ThemeManager::getInstance().removeThemeChangeListener(this);
    if (m_scrollBar) {
        m_scrollBar->Destroy();
        m_scrollBar = nullptr;
    }
}

int FlatNotebook::AddPage(wxWindow* page, const wxString& text, bool select, int imageId)
{
    wxUnusedVar(imageId);

    if (!page) return -1;

    // Create page info
    auto pageInfo = std::make_unique<PageInfo>(page, text);

    // Reparent page to this notebook
    page->Reparent(this);
    page->Hide(); // Initially hidden

    // Add to pages collection
    m_pages.push_back(std::move(pageInfo));
    int pageIndex = static_cast<int>(m_pages.size() - 1);

    // Select if requested or if this is the first page
    if (select || m_selectedPage == -1) {
        SetSelection(pageIndex);
    }

    // Update layout
    UpdateTabLayout();

    return pageIndex;
}

bool FlatNotebook::DeletePage(size_t page)
{
    if (page >= m_pages.size()) return false;

    // Hide and unparent the page
    m_pages[page]->page->Hide();
    m_pages[page]->page->Reparent(GetParent());

    // Remove from collection
    m_pages.erase(m_pages.begin() + page);

    // Adjust selected page
    if (m_selectedPage == static_cast<int>(page)) {
        // Select adjacent page
        if (m_selectedPage >= static_cast<int>(m_pages.size())) {
            m_selectedPage = static_cast<int>(m_pages.size()) - 1;
        }
        if (m_selectedPage >= 0) {
            SelectPage(m_selectedPage);
        }
        else {
            m_selectedPage = -1;
        }
    }
    else if (m_selectedPage > static_cast<int>(page)) {
        m_selectedPage--;
    }

    // Update layout
    UpdateTabLayout();

    return true;
}

bool FlatNotebook::DeleteAllPages()
{
    // Hide and unparent all pages
    for (auto& pageInfo : m_pages) {
        pageInfo->page->Hide();
        pageInfo->page->Reparent(GetParent());
    }

    m_pages.clear();
    m_selectedPage = -1;
    m_hoveredTab = -1;

    return true;
}

int FlatNotebook::GetSelection() const
{
    return m_selectedPage;
}

bool FlatNotebook::SetSelection(size_t page)
{
    if (page >= m_pages.size()) return false;

    SelectPage(static_cast<int>(page));
    return true;
}

wxWindow* FlatNotebook::GetPage(size_t page) const
{
    if (page >= m_pages.size()) return nullptr;
    return m_pages[page]->page;
}

wxString FlatNotebook::GetPageText(size_t page) const
{
    if (page >= m_pages.size()) return wxEmptyString;
    return m_pages[page]->text;
}

bool FlatNotebook::SetPageText(size_t page, const wxString& text)
{
    if (page >= m_pages.size()) return false;

    m_pages[page]->text = text;
    UpdateTabLayout();

    return true;
}

size_t FlatNotebook::GetPageCount() const
{
    return m_pages.size();
}

void FlatNotebook::OnPageChanged(int page)
{
    // This can be overridden by derived classes
    wxUnusedVar(page);
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
    m_tabBorderLeftColor = CFG_COLOUR("BarTabBorderTopColour");
}

void FlatNotebook::OnThemeChanged()
{
    InitializeDefaultColors();
    ReloadFontFromConfig();
}

void FlatNotebook::ReloadFontFromConfig()
{
    if (m_useConfigFont)
    {
        m_customFont = CFG_FONT();
    }
}

void FlatNotebook::OnPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(this);

    // Clear background
    dc.SetBackground(wxBrush(m_tabBackgroundColor));
    dc.Clear();

    // Paint content area border
    if (m_selectedPage >= 0) {
        wxSize clientSize = GetClientSize();
        dc.SetPen(wxPen(CFG_COLOUR("BarBorderColour"), 1));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);

        switch (m_tabPosition) {
        case FlatNotebook::TabPosition::Left:
            // Left tabs: draw right border of tab area (left border of content area)
            dc.DrawLine(m_tabPadding * 2 + 100 + 4, 0, m_tabPadding * 2 + 100 + 4, clientSize.GetHeight());
            break;
        case FlatNotebook::TabPosition::Right: {
            // Right tabs: draw left border of tab area (right border of content area)
            // Calculate tab area width
            int tabAreaWidth = 0;
            if (!m_pages.empty()) {
                wxClientDC calcDC(this);
                wxFont font = m_useConfigFont ? m_customFont : GetFont();
                if (!font.IsOk()) {
                    font = GetFont();
                }
                calcDC.SetFont(font);
                int maxTabWidth = 0;
                for (const auto& pageInfo : m_pages) {
                    wxSize textSize = calcDC.GetTextExtent(pageInfo->text);
                    int tabWidth = textSize.GetHeight() + m_tabVerticalPadding * 2;
                    if (tabWidth > maxTabWidth) {
                        maxTabWidth = tabWidth;
                    }
                }
                tabAreaWidth = maxTabWidth + 8; // Add margin
            }
            dc.DrawLine(clientSize.GetWidth() - tabAreaWidth, 0,
                      clientSize.GetWidth() - tabAreaWidth, clientSize.GetHeight());
            break;
        }
        case FlatNotebook::TabPosition::Top:
            dc.DrawLine(0, m_tabHeight + 4, clientSize.GetWidth(), m_tabHeight + 4);
            break;
        case FlatNotebook::TabPosition::Bottom:
            dc.DrawLine(0, clientSize.GetHeight() - m_tabHeight - 4,
                      clientSize.GetWidth(), clientSize.GetHeight() - m_tabHeight - 4);
            break;
        }
    }

    // Paint tabs using FlatBar style (using DC instead of GraphicsContext)
    DrawTabs(dc);
    
    // Draw tab bar right border (skip active tab area)
    if (!m_pages.empty()) {
        wxSize clientSize = GetClientSize();
        dc.SetPen(wxPen(m_tabBorderColor, 1));
        
        if (m_tabPosition == TabPosition::Left || m_tabPosition == TabPosition::Right) {
            if (m_tabPosition == TabPosition::Left) {
                // Left tabs: draw right border of tab area, but skip active tab
                int maxTabRight = 0;
                for (const auto& pageInfo : m_pages) {
                    int tabRight = pageInfo->tabRect.GetRight();
                    if (tabRight > maxTabRight) {
                        maxTabRight = tabRight;
                    }
                }
                if (maxTabRight > 0 && m_selectedPage >= 0 && m_selectedPage < static_cast<int>(m_pages.size())) {
                    // Draw vertical line at the right edge, but skip active tab area
                    const wxRect& activeTabRect = m_pages[m_selectedPage]->tabRect;
                    // Draw line above active tab
                    if (activeTabRect.GetTop() > 0) {
                        dc.DrawLine(maxTabRight, 0, maxTabRight, activeTabRect.GetTop());
                    }
                    // Draw line below active tab
                    if (activeTabRect.GetBottom() < clientSize.GetHeight()) {
                        dc.DrawLine(maxTabRight, activeTabRect.GetBottom() + 1, maxTabRight, clientSize.GetHeight());
                    }
                } else if (maxTabRight > 0) {
                    // No active tab, draw full line
                    dc.DrawLine(maxTabRight, 0, maxTabRight, clientSize.GetHeight());
                }
            } else {
                // Right tabs: draw left border of tab area, but skip active tab
                int minTabLeft = clientSize.GetWidth();
                for (const auto& pageInfo : m_pages) {
                    int tabLeft = pageInfo->tabRect.GetLeft();
                    if (tabLeft < minTabLeft) {
                        minTabLeft = tabLeft;
                    }
                }
                if (minTabLeft < clientSize.GetWidth() && m_selectedPage >= 0 && m_selectedPage < static_cast<int>(m_pages.size())) {
                    // Draw vertical line at the left edge, but skip active tab area
                    const wxRect& activeTabRect = m_pages[m_selectedPage]->tabRect;
                    // Draw line above active tab
                    if (activeTabRect.GetTop() > 0) {
                        dc.DrawLine(minTabLeft, 0, minTabLeft, activeTabRect.GetTop());
                    }
                    // Draw line below active tab
                    if (activeTabRect.GetBottom() < clientSize.GetHeight()) {
                        dc.DrawLine(minTabLeft, activeTabRect.GetBottom() + 1, minTabLeft, clientSize.GetHeight());
                    }
                } else if (minTabLeft < clientSize.GetWidth()) {
                    // No active tab, draw full line
                    dc.DrawLine(minTabLeft, 0, minTabLeft, clientSize.GetHeight());
                }
            }
        } else {
            // Horizontal tabs: draw border at the edge of tab area, but skip active tab
            if (m_tabPosition == TabPosition::Top) {
                // Top tabs: draw bottom border of tab area, but skip active tab
                int tabY = 4;
                int tabBottom = tabY + m_tabHeight;
                
                // Draw bottom border line (draw after tabs to ensure visibility)
                if (m_selectedPage >= 0 && m_selectedPage < static_cast<int>(m_pages.size())) {
                    // Draw horizontal line at the bottom edge, but skip active tab area
                    const wxRect& activeTabRect = m_pages[m_selectedPage]->tabRect;
                    // Draw line to the left of active tab
                    if (activeTabRect.GetLeft() > 0) {
                        dc.DrawLine(0, tabBottom, activeTabRect.GetLeft(), tabBottom);
                    }
                    // Draw line to the right of active tab
                    if (activeTabRect.GetRight() < clientSize.GetWidth()) {
                        dc.DrawLine(activeTabRect.GetRight() + 1, tabBottom, clientSize.GetWidth(), tabBottom);
                    }
                } else {
                    // No active tab, draw full line
                    dc.DrawLine(0, tabBottom, clientSize.GetWidth(), tabBottom);
                }
                
                // Also draw right border at the right edge of tab area, but skip active tab
                int maxTabRight = 0;
                for (const auto& pageInfo : m_pages) {
                    int tabRight = pageInfo->tabRect.GetRight();
                    if (tabRight > maxTabRight) {
                        maxTabRight = tabRight;
                    }
                }
                if (maxTabRight > 0) {
                    if (m_selectedPage >= 0 && m_selectedPage < static_cast<int>(m_pages.size())) {
                        // Draw vertical line at the right edge, but skip active tab area
                        const wxRect& activeTabRect = m_pages[m_selectedPage]->tabRect;
                        // If active tab is at the rightmost position, don't draw line at its right edge
                        // Otherwise, draw the line normally
                        if (activeTabRect.GetRight() < maxTabRight) {
                            // Active tab is not rightmost, draw full line
                            dc.DrawLine(maxTabRight, tabY, maxTabRight, tabBottom);
                        }
                        // If active tab is rightmost, its right edge is not drawn (handled in RenderTab)
                    } else {
                        // No active tab, draw full line
                        dc.DrawLine(maxTabRight, tabY, maxTabRight, tabBottom);
                    }
                }
            } else {
                // Bottom tabs: draw top border of tab area, but skip active tab
                int tabY = clientSize.GetHeight() - m_tabHeight - 4;
                int tabTop = tabY;
                
                if (m_selectedPage >= 0 && m_selectedPage < static_cast<int>(m_pages.size())) {
                    // Draw horizontal line at the top edge, but skip active tab area
                    const wxRect& activeTabRect = m_pages[m_selectedPage]->tabRect;
                    // Draw line to the left of active tab
                    if (activeTabRect.GetLeft() > 0) {
                        dc.DrawLine(0, tabTop, activeTabRect.GetLeft(), tabTop);
                    }
                    // Draw line to the right of active tab
                    if (activeTabRect.GetRight() < clientSize.GetWidth()) {
                        dc.DrawLine(activeTabRect.GetRight() + 1, tabTop, clientSize.GetWidth(), tabTop);
                    }
                } else {
                    // No active tab, draw full line
                    dc.DrawLine(0, tabTop, clientSize.GetWidth(), tabTop);
                }
                
                // Also draw right border at the right edge of tab area, but skip active tab
                int maxTabRight = 0;
                for (const auto& pageInfo : m_pages) {
                    int tabRight = pageInfo->tabRect.GetRight();
                    if (tabRight > maxTabRight) {
                        maxTabRight = tabRight;
                    }
                }
                if (maxTabRight > 0) {
                    int tabBottom = tabY + m_tabHeight;
                    
                    if (m_selectedPage >= 0 && m_selectedPage < static_cast<int>(m_pages.size())) {
                        // Draw vertical line at the right edge, but skip active tab area
                        const wxRect& activeTabRect = m_pages[m_selectedPage]->tabRect;
                        // If active tab is at the rightmost position, don't draw line at its right edge
                        // Otherwise, draw the line normally
                        if (activeTabRect.GetRight() < maxTabRight) {
                            // Active tab is not rightmost, draw full line
                            dc.DrawLine(maxTabRight, tabTop, maxTabRight, tabBottom);
                        }
                        // If active tab is rightmost, its right edge is not drawn (handled in RenderTab)
                    } else {
                        // No active tab, draw full line
                        dc.DrawLine(maxTabRight, tabTop, maxTabRight, tabBottom);
                    }
                }
            }
        }
    }
}

void FlatNotebook::DrawTabs(wxDC& dc)
{
    if (m_pages.empty()) return;

    wxSize clientSize = GetClientSize();

    // Set up drawing context
    wxFont tabFont = m_useConfigFont ? m_customFont : GetFont();
    if (!tabFont.IsOk()) {
        tabFont = GetFont();
    }
    dc.SetFont(tabFont);

    if (m_tabPosition == TabPosition::Top || m_tabPosition == TabPosition::Bottom) {
        // Horizontal layout for top/bottom tabs
        int currentX = 4; // Left margin
        int tabY = (m_tabPosition == TabPosition::Top) ? 4 : clientSize.GetHeight() - m_tabHeight - 4;

        // Paint each tab
        for (size_t i = 0; i < m_pages.size(); ++i) {
            bool isActive = (static_cast<int>(i) == m_selectedPage);
            bool isHovered = (static_cast<int>(i) == m_hoveredTab);

            // Calculate tab width
            wxSize textSize = dc.GetTextExtent(m_pages[i]->text);
            int tabWidth = textSize.GetWidth() + m_tabHorizontalPadding * 2; // padding

            // Create tab rectangle (horizontal layout)
            wxRect tabRect(currentX, tabY, tabWidth, m_tabHeight);
            m_pages[i]->tabRect = tabRect;

            // Render tab using FlatBar style
            RenderTab(dc, tabRect, m_pages[i]->text, isActive, isHovered);

            currentX += tabWidth + m_tabSpacing; // spacing
        }
    }
    else {
        // Vertical layout for left/right tabs
        // For rotated text: width = original text height, height = original text width
        int currentY = 4 - m_scrollOffset; // Top margin with scroll offset

        // Paint each tab
        for (size_t i = 0; i < m_pages.size(); ++i) {
            bool isActive = (static_cast<int>(i) == m_selectedPage);
            bool isHovered = (static_cast<int>(i) == m_hoveredTab);

            // Calculate tab dimensions for rotated text
            wxSize textSize = dc.GetTextExtent(m_pages[i]->text);
            // For vertical tabs: width = original height (rotated), height = original width (rotated)
            int tabWidth = textSize.GetHeight() + m_tabVerticalPadding * 2; // Use vertical padding for width
            int tabHeight = textSize.GetWidth() + m_tabHorizontalPadding * 2; // Use horizontal padding for height
            
            int tabX = (m_tabPosition == TabPosition::Left) ? 4 : clientSize.GetWidth() - tabWidth - 4;

            // Create tab rectangle (vertical layout) - store original position for hit testing
            wxRect tabRect(tabX, currentY, tabWidth, tabHeight);
            m_pages[i]->tabRect = tabRect;

            // Only render if tab is visible
            if (tabRect.GetBottom() >= 0 && tabRect.GetTop() <= clientSize.GetHeight()) {
                RenderTab(dc, tabRect, m_pages[i]->text, isActive, isHovered);
            }

            currentY += tabHeight + m_tabSpacing; // spacing
        }
    }
}


void FlatNotebook::RenderTab(wxDC& dc, const wxRect& rect, const wxString& text, bool isActive, bool isHovered)
{
    // Determine border drawing based on tab position
    bool isVertical = (m_tabPosition == TabPosition::Left || m_tabPosition == TabPosition::Right);

    if (isActive) {
        // Active tab - FlatBar style
        wxColour activeTabBgColour = m_activeTabBackgroundColor;
        wxColour activeTabTextColour = m_activeTabTextColor;
        wxColour tabBorderColour = m_tabBorderColor;

        dc.SetBrush(wxBrush(activeTabBgColour));
        dc.SetTextForeground(activeTabTextColour);

        // Draw borders based on position
        dc.SetPen(*wxTRANSPARENT_PEN);

        if (isVertical) {
            // Vertical tabs (left/right)
            if (m_tabPosition == TabPosition::Left) {
                // Left tabs: left border is thick, right border not drawn
                int borderOffset = m_tabBorderLeft;
                int borderX = rect.x + borderOffset;

                // Fill background
                dc.DrawRectangle(borderX, rect.y, rect.width - borderOffset, rect.height);

                // Draw borders
                if (m_tabBorderLeft > 0) {
                    dc.SetPen(wxPen(m_tabBorderLeftColor, m_tabBorderLeft));
                    dc.DrawLine(rect.GetLeft() + m_tabBorderLeft / 2, rect.GetTop(),
                              rect.GetLeft() + m_tabBorderLeft / 2, rect.GetBottom() + 1);
                }
                dc.SetPen(wxPen(tabBorderColour, m_tabBorderTop));
                dc.DrawLine(rect.GetLeft() + borderOffset, rect.GetTop(),
                          rect.GetRight(), rect.GetTop());
                dc.DrawLine(rect.GetLeft() + borderOffset, rect.GetBottom() + 1,
                          rect.GetRight(), rect.GetBottom() + 1);
                // Right border not drawn for left tabs
            } else {
                // Right tabs: right border is thick, left border not drawn
                int borderOffset = m_tabBorderLeft;
                int borderX = rect.x;

                // Fill background
                dc.DrawRectangle(borderX, rect.y, rect.width - borderOffset, rect.height);

                // Draw borders
                if (m_tabBorderLeft > 0) {
                    dc.SetPen(wxPen(m_tabBorderLeftColor, m_tabBorderLeft));
                    dc.DrawLine(rect.GetRight() - m_tabBorderLeft / 2, rect.GetTop(),
                              rect.GetRight() - m_tabBorderLeft / 2, rect.GetBottom() + 1);
                }
                dc.SetPen(wxPen(tabBorderColour, m_tabBorderTop));
                dc.DrawLine(rect.GetLeft(), rect.GetTop(),
                          rect.GetRight() - borderOffset, rect.GetTop());
                dc.DrawLine(rect.GetLeft(), rect.GetBottom() + 1,
                          rect.GetRight() - borderOffset, rect.GetBottom() + 1);
                // Left border not drawn for right tabs
            }
        } else {
            // Horizontal tabs (top/bottom)
            if (m_tabPosition == TabPosition::Top) {
                // Top tabs: top border is thick, bottom border not drawn
                int borderOffset = m_tabBorderTop;
                int borderY = rect.y + borderOffset;

                // Fill background (leave 1 pixel at bottom for border line)
                dc.DrawRectangle(rect.x, borderY, rect.width, rect.height - borderOffset - 1);

                // Draw borders
                if (m_tabBorderTop > 0) {
                    dc.SetPen(wxPen(m_tabBorderLeftColor, m_tabBorderTop));
                    dc.DrawLine(rect.GetLeft(), rect.GetTop() + m_tabBorderTop / 2,
                              rect.GetRight() + 1, rect.GetTop() + m_tabBorderTop / 2);
                }
                // Left and right borders are thin lines
                dc.SetPen(wxPen(tabBorderColour, m_tabBorderLeft));
                dc.DrawLine(rect.GetLeft(), rect.GetTop() + borderOffset,
                          rect.GetLeft(), rect.GetBottom());
                dc.DrawLine(rect.GetRight() + 1, rect.GetTop() + borderOffset,
                          rect.GetRight() + 1, rect.GetBottom());
                // Bottom border not drawn for top tabs
            } else {
                // Bottom tabs: bottom border is thick, top border not drawn
                int borderOffset = m_tabBorderBottom;
                int borderY = rect.y;

                // Fill background
                dc.DrawRectangle(rect.x, borderY, rect.width, rect.height - borderOffset);

                // Draw borders
                if (m_tabBorderBottom > 0) {
                    dc.SetPen(wxPen(m_tabBorderLeftColor, m_tabBorderBottom));
                    dc.DrawLine(rect.GetLeft(), rect.GetBottom() - m_tabBorderBottom / 2,
                              rect.GetRight() + 1, rect.GetBottom() - m_tabBorderBottom / 2);
                }
                // Left and right borders are thin lines
                dc.SetPen(wxPen(tabBorderColour, m_tabBorderLeft));
                dc.DrawLine(rect.GetLeft(), rect.GetTop(),
                          rect.GetLeft(), rect.GetBottom() - borderOffset);
                dc.DrawLine(rect.GetRight() + 1, rect.GetTop(),
                          rect.GetRight() + 1, rect.GetBottom() - borderOffset);
                // Top border not drawn for bottom tabs
            }
        }
    }
    else if (isHovered) {
        // Hovered tab - draw hover background
        wxColour hoverTabBgColour = m_hoverTabBackgroundColor;
        wxColour hoverTabTextColour = m_hoverTabTextColor;

        dc.SetBrush(wxBrush(hoverTabBgColour));
        dc.SetTextForeground(hoverTabTextColour);
        dc.SetPen(*wxTRANSPARENT_PEN);

        if (isVertical) {
            // Vertical tabs (left/right)
            if (m_tabPosition == TabPosition::Left) {
                int borderOffset = m_tabBorderLeft;
                int borderX = rect.x + borderOffset;
                dc.DrawRectangle(borderX, rect.y, rect.width - borderOffset, rect.height);
            } else {
                int borderOffset = m_tabBorderLeft;
                int borderX = rect.x;
                dc.DrawRectangle(borderX, rect.y, rect.width - borderOffset, rect.height);
            }
        } else {
            // Horizontal tabs (top/bottom)
            if (m_tabPosition == TabPosition::Top) {
                int borderOffset = m_tabBorderTop;
                int borderY = rect.y + borderOffset;
                dc.DrawRectangle(rect.x, borderY, rect.width, rect.height - borderOffset);
            } else {
                int borderOffset = m_tabBorderBottom;
                int borderY = rect.y;
                dc.DrawRectangle(rect.x, borderY, rect.width, rect.height - borderOffset);
            }
        }
    }
    else {
        // Inactive tab - no background, no borders (FlatBar style)
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetTextForeground(m_tabTextColor);
    }

    // Draw tab text (always draw after background)
    DrawTabText(dc, rect, text);
}

void FlatNotebook::DrawTabText(wxDC& dc, const wxRect& rect, const wxString& text)
{
    if (m_tabPosition == TabPosition::Left || m_tabPosition == TabPosition::Right) {
        // Vertical tabs - rotate text
        // Try to create GraphicsContext from the DC first
        wxGraphicsContext* gc = nullptr;
        
        // Try different DC types
        if (wxAutoBufferedPaintDC* autoPaintDC = dynamic_cast<wxAutoBufferedPaintDC*>(&dc)) {
            gc = wxGraphicsContext::Create(*autoPaintDC);
        }
        else if (wxClientDC* clientDC = dynamic_cast<wxClientDC*>(&dc)) {
            gc = wxGraphicsContext::Create(*clientDC);
        }
        else if (wxMemoryDC* memDC = dynamic_cast<wxMemoryDC*>(&dc)) {
            gc = wxGraphicsContext::Create(*memDC);
        }
        else if (wxWindowDC* winDC = dynamic_cast<wxWindowDC*>(&dc)) {
            gc = wxGraphicsContext::Create(*winDC);
        }
        else if (wxPaintDC* paintDC = dynamic_cast<wxPaintDC*>(&dc)) {
            gc = wxGraphicsContext::Create(*paintDC);
        }
        
        // Fallback to creating from window if DC creation failed
        if (!gc) {
            gc = wxGraphicsContext::Create(this);
        }
        
        if (gc) {
            wxFont tabFont = m_useConfigFont ? m_customFont : GetFont();
            if (!tabFont.IsOk()) {
                tabFont = GetFont();
            }

            wxColour textColor = dc.GetTextForeground();
            gc->SetFont(tabFont, textColor);

            double textWidth, textHeight, descent, externalLeading;
            gc->GetTextExtent(text, &textWidth, &textHeight, &descent, &externalLeading);

            gc->PushState();
            double centerX = rect.x + rect.width / 2.0;
            double centerY = rect.y + rect.height / 2.0;

            // Rotate based on position: left = counterclockwise (-90°), right = clockwise (+90°)
            double rotation = (m_tabPosition == TabPosition::Left) ? -M_PI / 2.0 : M_PI / 2.0;
            gc->Translate(centerX, centerY);
            gc->Rotate(rotation);
            gc->DrawText(text, -textWidth / 2.0, -textHeight / 2.0);
            gc->PopState();

            delete gc;
        }
    } else {
        // Horizontal tabs (top/bottom) - normal text
        wxSize textSize = dc.GetTextExtent(text);
        int textX = rect.x + (rect.width - textSize.GetWidth()) / 2;
        int textY = rect.y + (rect.height - textSize.GetHeight()) / 2;
        dc.DrawText(text, textX, textY);
    }
}


void FlatNotebook::UpdateTabLayout()
{
    if (m_pages.empty()) return;

    // Update tab rectangles
    wxClientDC dc(this);
    wxFont font = m_useConfigFont ? m_customFont : GetFont();
    if (!font.IsOk()) {
        font = GetFont();
    }
    dc.SetFont(font);

    wxSize clientSize = GetClientSize();

    if (m_tabPosition == TabPosition::Top || m_tabPosition == TabPosition::Bottom) {
        // Horizontal layout for top/bottom tabs
        int currentX = 4; // Left margin
        int tabY = (m_tabPosition == TabPosition::Top) ? 4 : clientSize.GetHeight() - m_tabHeight - 4;

        for (auto& pageInfo : m_pages) {
            wxSize textSize = dc.GetTextExtent(pageInfo->text);
            int tabWidth = textSize.GetWidth() + m_tabHorizontalPadding * 2; // padding

            pageInfo->tabRect = wxRect(currentX, tabY, tabWidth, m_tabHeight);
            currentX += tabWidth + m_tabSpacing; // spacing
        }
    }
    else {
        // Vertical layout for left/right tabs
        // For rotated text: width = original text height, height = original text width
        int currentY = 4; // Top margin

        for (auto& pageInfo : m_pages) {
            // Calculate tab dimensions for rotated text
            wxSize textSize = dc.GetTextExtent(pageInfo->text);
            // For vertical tabs: width = original height (rotated), height = original width (rotated)
            int tabWidth = textSize.GetHeight() + m_tabVerticalPadding * 2; // Use vertical padding for width
            int tabHeight = textSize.GetWidth() + m_tabHorizontalPadding * 2; // Use horizontal padding for height
            
            int tabX = (m_tabPosition == TabPosition::Left) ? 4 : clientSize.GetWidth() - tabWidth - 4;

            pageInfo->tabRect = wxRect(tabX, currentY, tabWidth, tabHeight);
            currentY += tabHeight + m_tabSpacing; // spacing
        }
    }

    // Update content area for selected page
    if (m_selectedPage >= 0 && m_selectedPage < static_cast<int>(m_pages.size())) {
        wxRect contentRect = GetContentRect(clientSize);

        // Show selected page and hide others
        for (size_t i = 0; i < m_pages.size(); ++i) {
            if (static_cast<int>(i) == m_selectedPage) {
                m_pages[i]->page->Show();
                m_pages[i]->page->SetSize(contentRect);
                m_pages[i]->isActive = true;
            }
            else {
                m_pages[i]->page->Hide();
                m_pages[i]->isActive = false;
            }
        }
    }

    // Update scrollbar
    UpdateScrollbar();
}


wxRect FlatNotebook::GetContentRect(const wxSize& clientSize) const
{
    switch (m_tabPosition) {
    case FlatNotebook::TabPosition::Top:
        return wxRect(0, 24 + 4, clientSize.GetWidth(), clientSize.GetHeight() - 24 - 4);
    case FlatNotebook::TabPosition::Bottom: {
        int contentHeight = clientSize.GetHeight() - 24 - 4;
        return wxRect(0, 0, clientSize.GetWidth(), contentHeight);
    }
    case FlatNotebook::TabPosition::Left: {
        // Calculate maximum tab width
        int maxTabWidth = 0;
        if (!m_pages.empty()) {
            wxClientDC dc(const_cast<FlatNotebook*>(this));
            wxFont font = m_useConfigFont ? m_customFont : GetFont();
            if (!font.IsOk()) {
                font = GetFont();
            }
            dc.SetFont(font);
            for (const auto& pageInfo : m_pages) {
                wxSize textSize = dc.GetTextExtent(pageInfo->text);
                int tabWidth = textSize.GetHeight() + m_tabVerticalPadding * 2;
                if (tabWidth > maxTabWidth) {
                    maxTabWidth = tabWidth;
                }
            }
        }
        int tabAreaWidth = maxTabWidth + 8; // Add margin
        return wxRect(tabAreaWidth, 0, clientSize.GetWidth() - tabAreaWidth, clientSize.GetHeight());
    }
    case FlatNotebook::TabPosition::Right: {
        // Calculate maximum tab width
        int maxTabWidth = 0;
        if (!m_pages.empty()) {
            wxClientDC dc(const_cast<FlatNotebook*>(this));
            wxFont font = m_useConfigFont ? m_customFont : GetFont();
            if (!font.IsOk()) {
                font = GetFont();
            }
            dc.SetFont(font);
            for (const auto& pageInfo : m_pages) {
                wxSize textSize = dc.GetTextExtent(pageInfo->text);
                int tabWidth = textSize.GetHeight() + m_tabVerticalPadding * 2;
                if (tabWidth > maxTabWidth) {
                    maxTabWidth = tabWidth;
                }
            }
        }
        int tabAreaWidth = maxTabWidth + 8; // Add margin
        return wxRect(0, 0, clientSize.GetWidth() - tabAreaWidth, clientSize.GetHeight());
    }
    default:
        return wxRect(0, 0, clientSize.GetWidth(), clientSize.GetHeight());
    }
}

int FlatNotebook::HitTestTab(const wxPoint& pos) const
{
    for (size_t i = 0; i < m_pages.size(); ++i) {
        // Adjust rect for scroll offset (only for right tabs)
        wxRect testRect = m_pages[i]->tabRect;
        if (m_tabPosition == TabPosition::Right) {
            testRect.y += m_scrollOffset;
        }
        if (testRect.Contains(pos)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void FlatNotebook::SelectPage(int page)
{
    if (page < 0 || page >= static_cast<int>(m_pages.size())) return;

    // Hide currently selected page
    if (m_selectedPage >= 0 && m_selectedPage < static_cast<int>(m_pages.size())) {
        m_pages[m_selectedPage]->page->Hide();
        m_pages[m_selectedPage]->isActive = false;
    }

    // Update selection
    m_selectedPage = page;

    // Show new selected page
    if (m_selectedPage >= 0) {
        wxSize clientSize = GetClientSize();
        wxRect contentRect = GetContentRect(clientSize);

        m_pages[m_selectedPage]->page->Show();
        m_pages[m_selectedPage]->page->SetSize(contentRect);
        m_pages[m_selectedPage]->isActive = true;

        // Notify page change
        OnPageChanged(m_selectedPage);
    }

    Refresh();
}

void FlatNotebook::OnLeftDown(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    int tabIndex = HitTestTab(pos);

    if (tabIndex >= 0 && tabIndex != m_selectedPage) {
        SelectPage(tabIndex);
    }

    event.Skip();
}

void FlatNotebook::OnMouseMove(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    int newHoveredTab = HitTestTab(pos);

    if (newHoveredTab != m_hoveredTab) {
        m_hoveredTab = newHoveredTab;
        Refresh();
    }

    event.Skip();
}

void FlatNotebook::OnMouseLeave(wxMouseEvent& event)
{
    if (m_hoveredTab != -1) {
        m_hoveredTab = -1;
        Refresh();
    }

    event.Skip();
}

void FlatNotebook::OnSize(wxSizeEvent& event)
{
    UpdateTabLayout();
    UpdateScrollbar();
    event.Skip();
}

void FlatNotebook::OnScroll(wxScrollEvent& event)
{
    if (m_scrollBar) {
        m_scrollOffset = event.GetPosition();
        Refresh();
    }
    event.Skip();
}

void FlatNotebook::UpdateScrollbar()
{
    // Only show scrollbar for right-side vertical tabs
    if (m_tabPosition != TabPosition::Right || m_pages.empty()) {
        if (m_scrollBar) {
            m_scrollBar->Hide();
        }
        m_scrollOffset = 0;
        return;
    }

    wxSize clientSize = GetClientSize();
    
    // Calculate total height needed for all tabs
    wxClientDC dc(this);
    wxFont font = m_useConfigFont ? m_customFont : GetFont();
    if (!font.IsOk()) {
        font = GetFont();
    }
    dc.SetFont(font);

    int totalHeight = 4; // Top margin
    for (const auto& pageInfo : m_pages) {
        wxSize textSize = dc.GetTextExtent(pageInfo->text);
        int tabHeight = textSize.GetWidth() + m_tabHorizontalPadding * 2;
        totalHeight += tabHeight + m_tabSpacing;
    }
    totalHeight += 4; // Bottom margin

    // Check if scrolling is needed
    if (totalHeight <= clientSize.GetHeight()) {
        if (m_scrollBar) {
            m_scrollBar->Hide();
        }
        m_scrollOffset = 0;
        return;
    }

    // Create scrollbar if needed
    if (!m_scrollBar) {
        m_scrollBar = new wxScrollBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);
    }

    // Position scrollbar on the left side of tabs
    int scrollBarWidth = wxSystemSettings::GetMetric(wxSYS_VSCROLL_X);
    int tabX = clientSize.GetWidth() - (m_tabVerticalPadding * 2 + 100) - 4;
    m_scrollBar->SetPosition(wxPoint(tabX - scrollBarWidth - 2, 0));
    m_scrollBar->SetSize(wxSize(scrollBarWidth, clientSize.GetHeight()));

    // Set scrollbar range
    int scrollRange = totalHeight - clientSize.GetHeight();
    m_scrollBar->SetScrollbar(m_scrollOffset, clientSize.GetHeight(), scrollRange, clientSize.GetHeight() / 10, true);
    m_scrollBar->Show();
}

void FlatNotebook::SetTabPosition(TabPosition position)
{
    if (m_tabPosition != position) {
        m_tabPosition = position;
        UpdateTabLayout();
        Refresh();
    }
}

void FlatNotebook::SetTabStyle(TabStyle style)
{
    m_tabStyle = style;
}

void FlatNotebook::SetTabBackgroundColor(const wxColour& color)
{
    m_tabBackgroundColor = color;
}

void FlatNotebook::SetActiveTabBackgroundColor(const wxColour& color)
{
    m_activeTabBackgroundColor = color;
}

void FlatNotebook::SetHoverTabBackgroundColor(const wxColour& color)
{
    m_hoverTabBackgroundColor = color;
}

void FlatNotebook::SetTabTextColor(const wxColour& color)
{
    m_tabTextColor = color;
}

void FlatNotebook::SetActiveTabTextColor(const wxColour& color)
{
    m_activeTabTextColor = color;
}

void FlatNotebook::SetHoverTabTextColor(const wxColour& color)
{
    m_hoverTabTextColor = color;
}

void FlatNotebook::SetTabBorderColor(const wxColour& color)
{
    m_tabBorderColor = color;
}

void FlatNotebook::SetTabPadding(int horizontal, int vertical)
{
    m_tabHorizontalPadding = horizontal;
    m_tabVerticalPadding = vertical;
}

void FlatNotebook::GetTabPadding(int& horizontal, int& vertical) const
{
    horizontal = m_tabHorizontalPadding;
    vertical = m_tabVerticalPadding;
}

void FlatNotebook::SetTabSpacing(int spacing)
{
    m_tabSpacing = spacing;
}

void FlatNotebook::SetCustomFont(const wxFont& font)
{
    m_customFont = font;
    m_useConfigFont = false;
}

void FlatNotebook::UseConfigFont(bool useConfig)
{
    m_useConfigFont = useConfig;
    if (useConfig)
    {
        ReloadFontFromConfig();
    }
}
