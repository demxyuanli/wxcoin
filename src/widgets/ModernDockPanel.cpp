#include "widgets/ModernDockPanel.h"
#include "widgets/ModernDockManager.h"
#include "DPIManager.h"
#include "config/ThemeManager.h"
#include "logger/Logger.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/menu.h>
#include <algorithm>

wxBEGIN_EVENT_TABLE(ModernDockPanel, wxPanel)
    EVT_PAINT(ModernDockPanel::OnPaint)
    EVT_SIZE(ModernDockPanel::OnSize)
    EVT_LEFT_DOWN(ModernDockPanel::OnLeftDown)
    EVT_LEFT_UP(ModernDockPanel::OnLeftUp)
    EVT_RIGHT_DOWN(ModernDockPanel::OnRightDown)
    EVT_MOTION(ModernDockPanel::OnMouseMove)
    EVT_LEAVE_WINDOW(ModernDockPanel::OnMouseLeave)
    EVT_ENTER_WINDOW(ModernDockPanel::OnMouseEnter)
    EVT_TIMER(wxID_ANY, ModernDockPanel::OnTimer)
    EVT_CONTEXT_MENU(ModernDockPanel::OnContextMenu)
wxEND_EVENT_TABLE()

ModernDockPanel::ModernDockPanel(ModernDockManager* manager, wxWindow* parent, const wxString& title)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
      m_manager(manager),
      m_title(title),
      m_dockArea(DockArea::Center),
      m_state(DockPanelState::Normal),
      m_selectedIndex(-1),
      m_hoveredTabIndex(-1),
      m_hoveredCloseIndex(-1),
      m_showTabs(true),
      m_tabCloseMode(TabCloseMode::ShowOnHover),
      m_dragging(false),
      m_draggedTabIndex(-1),
      m_animationTimer(this),
      m_animating(false),
      m_animationProgress(0.0),
      m_animationDuration(250),
      m_dockingEnabled(true),
      m_systemButtonsVisible(true)
{
    InitializePanel();
}

ModernDockPanel::~ModernDockPanel()
{
    // Cleanup is handled automatically by smart pointers and wxWidgets
}

void ModernDockPanel::InitializePanel()
{
    // Set background style for custom painting
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetDoubleBuffered(true);
    
    // Initialize DPI-aware sizing
    double dpiScale = DPIManager::getInstance().getDPIScale();
    m_tabHeight = static_cast<int>(DEFAULT_TAB_HEIGHT * dpiScale);
    m_tabMinWidth = static_cast<int>(DEFAULT_TAB_MIN_WIDTH * dpiScale);
    m_tabMaxWidth = static_cast<int>(DEFAULT_TAB_MAX_WIDTH * dpiScale);
    m_tabSpacing = DEFAULT_TAB_SPACING;
    m_closeButtonSize = static_cast<int>(DEFAULT_CLOSE_BUTTON_SIZE * dpiScale);
    m_contentMargin = static_cast<int>(DEFAULT_CONTENT_MARGIN * dpiScale);
    
    // Initialize style configuration
    m_tabStyle = TabStyle::DEFAULT;
    m_tabBorderStyle = TabBorderStyle::SOLID;
    m_tabCornerRadius = static_cast<int>(DEFAULT_TAB_CORNER_RADIUS * dpiScale);
    m_tabBorderTop = static_cast<int>(DEFAULT_TAB_BORDER_TOP * dpiScale);
    m_tabBorderBottom = static_cast<int>(DEFAULT_TAB_BORDER_BOTTOM * dpiScale);
    m_tabBorderLeft = static_cast<int>(DEFAULT_TAB_BORDER_LEFT * dpiScale);
    m_tabBorderRight = static_cast<int>(DEFAULT_TAB_BORDER_RIGHT * dpiScale);
    m_tabPadding = static_cast<int>(DEFAULT_TAB_PADDING * dpiScale);
    m_tabTopMargin = static_cast<int>(DEFAULT_TAB_TOP_MARGIN * dpiScale);
    
    // Initialize fonts
    m_tabFont = CFG_FONT();
    m_titleFont = CFG_FONT();
    
    // Initialize system buttons
    m_systemButtons = new DockSystemButtons(this);
    m_systemButtons->Show(false); // Initially hidden, will be shown in title bar
    
    // Initialize colors from theme manager
    UpdateThemeColors();
    
    // Set minimum size
    SetMinSize(wxSize(m_tabMinWidth, m_tabHeight + 100));
}

void ModernDockPanel::UpdateThemeColors()
{
    // Get colors from theme manager
    m_backgroundColor = CFG_COLOUR("PanelBgColour");
    m_tabActiveColor = CFG_COLOUR("TabActiveColour");
    m_tabInactiveColor = CFG_COLOUR("TabInactiveColour");
    m_tabHoverColor = CFG_COLOUR("TabHoverColour");
    m_textColor = CFG_COLOUR("PanelTextColour");
    m_borderColor = CFG_COLOUR("PanelBorderColour");
    
    // Extended colors
    m_tabBorderTopColor = CFG_COLOUR("TabBorderTopColour");
    m_tabBorderBottomColor = CFG_COLOUR("TabBorderBottomColour");
    m_tabBorderLeftColor = CFG_COLOUR("TabBorderLeftColour");
    m_tabBorderRightColor = CFG_COLOUR("TabBorderRightColour");
    m_tabActiveTextColor = CFG_COLOUR("TabActiveTextColour");
    m_tabHoverTextColor = CFG_COLOUR("TabHoverTextColour");
    m_closeButtonNormalColor = CFG_COLOUR("CloseButtonNormalColour");
    m_titleBarBgColor = CFG_COLOUR("TitleBarBgColour");
    m_titleBarTextColor = CFG_COLOUR("TitleBarTextColour");
    m_titleBarBorderColor = CFG_COLOUR("TitleBarBorderColour");
    
    // Update fonts from theme
    m_tabFont = CFG_FONT();
    m_titleFont = CFG_FONT();
    
    // Refresh display when colors change
    Refresh();
}

void ModernDockPanel::OnThemeChanged()
{
    // Update colors when theme changes
    UpdateThemeColors();
}

void ModernDockPanel::AddContent(wxWindow* content, const wxString& title, const wxBitmap& icon, bool select)
{
    if (!content) return;
    
    // Create content item
    auto contentItem = std::make_unique<ContentItem>(content, title, icon);
    
    // Reparent content to this panel
    content->Reparent(this);
    content->Hide(); // Initially hidden, will be shown when selected
    
    // Add to collection
    m_contents.push_back(std::move(contentItem));
    
    // Select if requested or if this is the first content
    if (select || m_contents.size() == 1) {
        SelectContent(static_cast<int>(m_contents.size() - 1));
    }
    
    // Update layout
    UpdateLayout();
    Refresh();
}

void ModernDockPanel::RemoveContent(wxWindow* content)
{
    if (!content) return;
    
    // Find content index
    for (size_t i = 0; i < m_contents.size(); ++i) {
        if (m_contents[i]->content == content) {
            RemoveContent(static_cast<int>(i));
            return;
        }
    }
}

void ModernDockPanel::RemoveContent(int index)
{
    if (index < 0 || index >= static_cast<int>(m_contents.size())) return;
    
    // Hide and unparent the content
    m_contents[index]->content->Hide();
    m_contents[index]->content->Reparent(GetParent());
    
    // Remove from collection
    m_contents.erase(m_contents.begin() + index);
    
    // Adjust selected index
    if (m_selectedIndex == index) {
        // Select adjacent tab
        if (m_selectedIndex >= static_cast<int>(m_contents.size())) {
            m_selectedIndex = static_cast<int>(m_contents.size()) - 1;
        }
        if (m_selectedIndex >= 0) {
            SelectContent(m_selectedIndex);
        } else {
            m_selectedIndex = -1;
        }
    } else if (m_selectedIndex > index) {
        m_selectedIndex--;
    }
    
    // Update layout
    UpdateLayout();
    Refresh();
}

void ModernDockPanel::SelectContent(int index)
{
    if (index < 0 || index >= static_cast<int>(m_contents.size())) return;
    
    // Hide currently selected content
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_contents.size())) {
        m_contents[m_selectedIndex]->content->Hide();
    }
    
    // Update selection
    m_selectedIndex = index;
    
    // Show new selected content
    if (m_selectedIndex >= 0) {
        auto* content = m_contents[m_selectedIndex]->content;
        content->Show();
        content->SetSize(m_contentRect);
    }
    
    Refresh();
}

void ModernDockPanel::SelectContent(wxWindow* content)
{
    for (size_t i = 0; i < m_contents.size(); ++i) {
        if (m_contents[i]->content == content) {
            SelectContent(static_cast<int>(i));
            return;
        }
    }
}

int ModernDockPanel::GetContentCount() const
{
    return static_cast<int>(m_contents.size());
}

wxWindow* ModernDockPanel::GetContent(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_contents.size())) return nullptr;
    return m_contents[index]->content;
}

wxWindow* ModernDockPanel::GetSelectedContent() const
{
    if (m_selectedIndex < 0 || m_selectedIndex >= static_cast<int>(m_contents.size())) return nullptr;
    return m_contents[m_selectedIndex]->content;
}

int ModernDockPanel::GetSelectedIndex() const
{
    return m_selectedIndex;
}

wxString ModernDockPanel::GetContentTitle(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_contents.size())) return wxEmptyString;
    return m_contents[index]->title;
}

void ModernDockPanel::SetContentTitle(int index, const wxString& title)
{
    if (index < 0 || index >= static_cast<int>(m_contents.size())) return;
    m_contents[index]->title = title;
    Refresh();
}

wxBitmap ModernDockPanel::GetContentIcon(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_contents.size())) return wxNullBitmap;
    return m_contents[index]->icon;
}

void ModernDockPanel::SetContentIcon(int index, const wxBitmap& icon)
{
    if (index < 0 || index >= static_cast<int>(m_contents.size())) return;
    m_contents[index]->icon = icon;
    Refresh();
}

void ModernDockPanel::SetTitle(const wxString& title)
{
    m_title = title;
    Refresh();
}

void ModernDockPanel::SetState(DockPanelState state)
{
    if (m_state != state) {
        m_state = state;
        UpdateLayout();
        Refresh();
    }
}

void ModernDockPanel::SetFloating(bool floating)
{
    SetState(floating ? DockPanelState::Floating : DockPanelState::Normal);
}

void ModernDockPanel::SetTabCloseMode(TabCloseMode mode)
{
    m_tabCloseMode = mode;
    Refresh();
}

void ModernDockPanel::SetShowTabs(bool show)
{
    if (m_showTabs != show) {
        m_showTabs = show;
        UpdateLayout();
        Refresh();
    }
}

void ModernDockPanel::StartDrag(int tabIndex, const wxPoint& startPos)
{
    if (tabIndex < 0 || tabIndex >= static_cast<int>(m_contents.size())) return;
    
    // Don't start dragging if docking is disabled
    if (!m_dockingEnabled) return;
    
    // Don't start dragging immediately, just record the start position
    // Dragging will only start when mouse moves beyond threshold
    m_draggedTabIndex = tabIndex;
    m_dragStartPos = startPos;
    m_lastMousePos = startPos;
    
    // Don't set m_dragging = true yet, don't capture mouse yet
    // This will be done in OnMouseMove when threshold is exceeded
}

int ModernDockPanel::HitTestTab(const wxPoint& pos) const
{
    for (size_t i = 0; i < m_tabRects.size(); ++i) {
        if (m_tabRects[i].Contains(pos)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool ModernDockPanel::HitTestCloseButton(const wxPoint& pos, int& tabIndex) const
{
    for (size_t i = 0; i < m_closeButtonRects.size(); ++i) {
        if (m_closeButtonRects[i].Contains(pos)) {
            tabIndex = static_cast<int>(i);
            return true;
        }
    }
    tabIndex = -1;
    return false;
}

wxRect ModernDockPanel::GetTabRect(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_tabRects.size())) return wxRect();
    return m_tabRects[index];
}

wxRect ModernDockPanel::GetContentRect() const
{
    return m_contentRect;
}

void ModernDockPanel::UpdateLayout()
{
    wxSize size = GetClientSize();
    
    if (m_showTabs && !m_contents.empty()) {
        // Calculate tab bar area
        m_tabBarRect = wxRect(0, 0, size.x, m_tabHeight);
        m_contentRect = wxRect(m_contentMargin, m_tabHeight + m_contentMargin,
                              size.x - 2 * m_contentMargin,
                              size.y - m_tabHeight - 2 * m_contentMargin);
    } else {
        // No tabs, content area starts after title bar
        m_tabBarRect = wxRect();
        int titleBarHeight = 24; // Same as in RenderTitleBar
        m_contentRect = wxRect(m_contentMargin, titleBarHeight + m_contentMargin,
                              size.x - 2 * m_contentMargin,
                              size.y - titleBarHeight - 2 * m_contentMargin);
    }
    
    // Calculate tab layout
    if (m_showTabs) {
        CalculateTabLayout();
    }
    
    // Update selected content size
    if (m_selectedIndex >= 0 && m_selectedIndex < static_cast<int>(m_contents.size())) {
        m_contents[m_selectedIndex]->content->SetSize(m_contentRect);
    }
}

void ModernDockPanel::CalculateTabLayout()
{
    m_tabRects.clear();
    m_closeButtonRects.clear();
    
    if (!m_showTabs || m_contents.empty()) return;
    
    int tabCount = static_cast<int>(m_contents.size());
    
    // Calculate tab widths based on content
    std::vector<int> tabWidths;
    int totalTabWidth = 0;
    
    for (const auto& content : m_contents) {
        // Calculate text width
        wxClientDC dc(this);
        dc.SetFont(m_tabFont);
        wxSize textSize = dc.GetTextExtent(content->title);
        
        // Tab width = text width + padding + close button (if applicable)
        int tabWidth = textSize.GetWidth() + m_tabPadding * 2;
        if (m_tabCloseMode != TabCloseMode::ShowNever) {
            tabWidth += m_closeButtonSize + 4;
        }
        
        // Ensure minimum width
        tabWidth = std::max(tabWidth, m_tabMinWidth);
        
        tabWidths.push_back(tabWidth);
        totalTabWidth += tabWidth;
    }
    
    // Add spacing between tabs
    totalTabWidth += (tabCount - 1) * m_tabSpacing;
    
    // Create tab rectangles with 2px top margin from parent panel
    int x = 0;
    for (int i = 0; i < tabCount; ++i) {
        wxRect tabRect(x, 2, tabWidths[i], m_tabHeight); // Top margin: 2px from parent
        m_tabRects.push_back(tabRect);
        
        // Calculate close button rect
        wxRect closeRect = CalculateCloseButtonRect(tabRect);
        m_closeButtonRects.push_back(closeRect);
        
        x += tabWidths[i] + m_tabSpacing;
    }
}

wxRect ModernDockPanel::CalculateCloseButtonRect(const wxRect& tabRect) const
{
    int margin = 4;
    int size = m_closeButtonSize;
    int x = tabRect.GetRight() - margin - size;
    int y = tabRect.y + (tabRect.height - size) / 2;
    return wxRect(x, y, size, size);
}

wxRect ModernDockPanel::CalculateTitleBarCloseButtonRect(int tabIndex) const
{
    if (tabIndex < 0 || tabIndex >= static_cast<int>(m_contents.size())) return wxRect();
    
    // Calculate tab position in title bar (similar to RenderTitleBarTabs)
    int x = 0;
    for (int i = 0; i < tabIndex; ++i) {
        wxClientDC dc(const_cast<ModernDockPanel*>(this));
        dc.SetFont(m_tabFont);
        wxSize textSize = dc.GetTextExtent(m_contents[i]->title);
        int tabWidth = textSize.GetWidth() + m_tabPadding * 2;
        if (m_tabCloseMode != TabCloseMode::ShowNever) {
            tabWidth += m_closeButtonSize + 4;
        }
        tabWidth = std::max(tabWidth, m_tabMinWidth);
        x += tabWidth + m_tabSpacing;
    }
    
    // Calculate current tab width
    wxClientDC dc(const_cast<ModernDockPanel*>(this));
    dc.SetFont(m_tabFont);
    wxSize textSize = dc.GetTextExtent(m_contents[tabIndex]->title);
    int tabWidth = textSize.GetWidth() + m_tabPadding * 2;
    if (m_tabCloseMode != TabCloseMode::ShowNever) {
        tabWidth += m_closeButtonSize + 4;
    }
    tabWidth = std::max(tabWidth, m_tabMinWidth);
    
    // Calculate close button position
    int margin = 4;
    int size = m_closeButtonSize;
    int closeX = x + tabWidth - margin - size;
    int closeY = (24 - size) / 2; // Center in title bar height
    
    return wxRect(closeX, closeY, size, size);
}

void ModernDockPanel::OnPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(this);
    
    // Create graphics context for smooth rendering
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (!gc) {
        event.Skip();
        return;
    }
    
    // Clear background
    gc->SetBrush(wxBrush(m_backgroundColor));
    gc->DrawRectangle(0, 0, GetSize().x, GetSize().y);
    
    // Render title bar with appropriate method based on content count
    if (!m_title.IsEmpty()) {
        if (m_contents.size() > 1) {
            // Multiple contents - render tabs in title bar
            RenderTitleBarTabs(gc);
        } else {
            // Single content - render normal title bar
            RenderTitleBar(gc);
        }
    }
    
    // Render tab bar only if we have tabs and single content
    if (m_showTabs && m_contents.size() == 1 && !m_contents.empty()) {
        RenderTabBar(gc);
    }
    
    // Render border
    gc->SetPen(wxPen(m_borderColor, 1));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->DrawRectangle(0, 0, GetSize().x, GetSize().y);
    
    delete gc;
}

void ModernDockPanel::RenderTitleBar(wxGraphicsContext* gc)
{
    if (!gc || m_title.IsEmpty()) return;
    
    // Use FlatBar-style title bar rendering
    wxColour titleBarBgColour = CFG_COLOUR("BarBackgroundColour");
    wxColour titleBarTextColour = CFG_COLOUR("BarActiveTextColour");
    wxColour titleBarBorderColour = CFG_COLOUR("BarBorderColour");
    
    // Calculate title bar dimensions
    int titleBarHeight = 24; // Similar to FlatBar height
    int titleBarY = 0;
    
    // Draw title bar background
    gc->SetBrush(wxBrush(titleBarBgColour));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRectangle(0, titleBarY, GetSize().x, titleBarHeight);
    
    // Draw title bar separator line (like FlatBar)
    gc->SetPen(wxPen(titleBarBorderColour, 1));
    gc->StrokeLine(0, titleBarHeight, GetSize().x, titleBarHeight);
    
    // Draw title text
    gc->SetFont(m_titleFont, titleBarTextColour);
    
    double textWidth, textHeight;
    gc->GetTextExtent(m_title, &textWidth, &textHeight);
    
    int textX = 8; // Left margin
    int textY = titleBarY + (titleBarHeight - textHeight) / 2;
    
    gc->DrawText(m_title, textX, textY);
    
    // Position system buttons on the right side of title bar
    if (m_systemButtons && m_systemButtonsVisible && m_dockingEnabled) {
        wxSize buttonSize = m_systemButtons->GetBestSize();
        int buttonAreaWidth = buttonSize.GetWidth();
        int buttonX = GetSize().x - buttonAreaWidth - 2; // Right margin: 2px
        int buttonY = titleBarY + (titleBarHeight - buttonSize.GetHeight()) / 2;
        
        // Ensure system buttons panel has the correct size 
        m_systemButtons->SetSize(buttonSize);
        m_systemButtons->SetPosition(wxPoint(buttonX, buttonY));
        m_systemButtons->Show(true);
    } else if (m_systemButtons) {
        // Hide system buttons if not visible or docking disabled
        m_systemButtons->Show(false);
    }
    
    // Update tab bar position to account for title bar
    m_tabBarRect.y = titleBarHeight + 2; // Add 2px top margin for tabs
    m_tabBarRect.height = GetSize().y - titleBarHeight - 2;
    

}

void ModernDockPanel::RenderTitleBarTabs(wxGraphicsContext* gc)
{
    if (!gc || m_contents.empty()) return;

    // Draw title bar background
    wxColour titleBarBgColour = CFG_COLOUR("BarBackgroundColour");
    gc->SetBrush(wxBrush(titleBarBgColour));
    gc->SetPen(*wxTRANSPARENT_PEN);
    gc->DrawRectangle(0, 0, GetSize().x, 24); // Title bar height is 24

    // Draw title bar separator line (like FlatBar)
    wxColour titleBarBorderColour = CFG_COLOUR("BarBorderColour");
    gc->SetPen(wxPen(titleBarBorderColour, 1));
    gc->StrokeLine(0, 24, GetSize().x, 24);

    // Calculate starting X position for tabs
    int x = 0;
    for (size_t i = 0; i < m_contents.size(); ++i) {
        bool selected = (static_cast<int>(i) == m_selectedIndex);
        bool hovered = (static_cast<int>(i) == m_hoveredTabIndex);
        
        wxClientDC dc(this);
        dc.SetFont(m_tabFont);
        wxSize textSize = dc.GetTextExtent(m_contents[i]->title);
        int tabWidth = textSize.GetWidth() + m_tabPadding * 2;
        if (m_tabCloseMode != TabCloseMode::ShowNever) {
            tabWidth += m_closeButtonSize + 4;
        }
        tabWidth = std::max(tabWidth, m_tabMinWidth);

        // Draw tab background based on selection and hover state
        if (selected) {
            // Active tab - similar to FlatBar active tab style
            wxColour activeTabBgColour = CFG_COLOUR("BarActiveTabBgColour");
            wxColour activeTabTextColour = CFG_COLOUR("BarActiveTextColour");
            wxColour tabBorderTopColour = CFG_COLOUR("BarTabBorderTopColour");
            wxColour tabBorderColour = CFG_COLOUR("BarTabBorderColour");
            
            // Fill background of active tab (excluding the top border)
            gc->SetBrush(wxBrush(activeTabBgColour));
            gc->SetPen(*wxTRANSPARENT_PEN);
            
            int tabBorderTop = 2;
            gc->DrawRectangle(x, tabBorderTop, tabWidth, 24 - tabBorderTop);
            
            // Draw borders like FlatBar
            if (tabBorderTop > 0) {
                gc->SetPen(wxPen(tabBorderTopColour, tabBorderTop));
                gc->StrokeLine(x, tabBorderTop / 2, x + tabWidth, tabBorderTop / 2);
            }
            
            // Draw left and right borders
            gc->SetPen(wxPen(tabBorderColour, 1));
            gc->StrokeLine(x, tabBorderTop, x, 24);
            gc->StrokeLine(x + tabWidth, tabBorderTop, x + tabWidth, 24);
            
            // Set text color for active tab
            gc->SetFont(m_tabFont, activeTabTextColour);
        } else if (hovered) {
            // Hovered tab - no background, just text color change
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetFont(m_tabFont, CFG_COLOUR("BarInactiveTextColour"));
        } else {
            // Inactive tab - no background, no borders
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetFont(m_tabFont, CFG_COLOUR("BarInactiveTextColour"));
        }

        // Draw text
        double textWidth, textHeight;
        gc->GetTextExtent(m_contents[i]->title, &textWidth, &textHeight);
        int textX = x + (tabWidth - textWidth) / 2;
        int textY = (24 - textHeight) / 2;
        gc->DrawText(m_contents[i]->title, textX, textY);

        // Draw close button (always visible, no hover response needed)
        if (m_tabCloseMode != TabCloseMode::ShowNever) {
            wxRect closeRect = CalculateTitleBarCloseButtonRect(static_cast<int>(i));
            RenderCloseButton(gc, closeRect, false);
        }

        x += tabWidth + m_tabSpacing;
    }

    // Position system buttons on the right side of title bar
    if (m_systemButtons && m_systemButtonsVisible && m_dockingEnabled) {
        wxSize buttonSize = m_systemButtons->GetBestSize();
        int buttonAreaWidth = buttonSize.GetWidth();
        int buttonX = GetSize().x - buttonAreaWidth - 2; // Right margin: 2px
        int buttonY = (24 - buttonSize.GetHeight()) / 2;
        
        m_systemButtons->SetSize(buttonSize);
        m_systemButtons->SetPosition(wxPoint(buttonX, buttonY));
        m_systemButtons->Show(true);
    } else if (m_systemButtons) {
        // Hide system buttons if not visible or docking disabled
        m_systemButtons->Show(false);
    }
}

void ModernDockPanel::RenderTabBar(wxGraphicsContext* gc)
{
    if (!gc || m_contents.empty()) return;
    
    // Render each tab
    for (size_t i = 0; i < m_contents.size(); ++i) {
        bool selected = (static_cast<int>(i) == m_selectedIndex);
        bool hovered = (static_cast<int>(i) == m_hoveredTabIndex);
        
        RenderTab(gc, static_cast<int>(i), m_tabRects[i], selected, hovered);
    }
}

void ModernDockPanel::RenderTab(wxGraphicsContext* gc, int index, const wxRect& rect, bool selected, bool hovered)
{
    if (!gc || index < 0 || index >= static_cast<int>(m_contents.size())) return;
    
    const wxString& title = m_contents[index]->title;
    double textWidth, textHeight;
    gc->GetTextExtent(title, &textWidth, &textHeight);
    
    // Use FlatBar-style tab rendering
    if (selected) {
        // Active tab - similar to FlatBar active tab style
        wxColour activeTabBgColour = CFG_COLOUR("BarActiveTabBgColour");
        wxColour activeTabTextColour = CFG_COLOUR("BarActiveTextColour");
        wxColour tabBorderTopColour = CFG_COLOUR("BarTabBorderTopColour");
        wxColour tabBorderColour = CFG_COLOUR("BarTabBorderColour");
        
        // Fill background of active tab (excluding the top border)
        gc->SetBrush(wxBrush(activeTabBgColour));
        gc->SetPen(*wxTRANSPARENT_PEN);
        
        int tabBorderTop = 2;
        gc->DrawRectangle(rect.x, rect.y + tabBorderTop, rect.width, rect.height - tabBorderTop);
        
        // Draw borders like FlatBar
        if (tabBorderTop > 0) {
            gc->SetPen(wxPen(tabBorderTopColour, tabBorderTop));
            gc->StrokeLine(rect.GetLeft(), rect.GetTop() + tabBorderTop / 2,
                          rect.GetRight() + 1, rect.GetTop() + tabBorderTop / 2);
        }
        
                        // Draw left and right borders
                gc->SetPen(wxPen(tabBorderColour, 1));
                gc->StrokeLine(rect.GetLeft(), rect.GetTop() + tabBorderTop,
                              rect.GetLeft(), rect.GetBottom());
                gc->StrokeLine(rect.GetRight() + 1, rect.GetTop() + tabBorderTop,
                              rect.GetRight() + 1, rect.GetBottom()-4); // Right border: 4px inset
        
        // Set text color for active tab
        gc->SetFont(m_tabFont, activeTabTextColour);
    } else if (hovered) {
        // Hovered tab - no background, just text color change
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetFont(m_tabFont, CFG_COLOUR("BarInactiveTextColour"));
    } else {
        // Inactive tab - no background, no borders
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetFont(m_tabFont, CFG_COLOUR("BarInactiveTextColour"));
    }
    
                    // Draw tab text with FlatBar-style positioning
                int textX = rect.x + 8; // BarTabPadding equivalent
                int textY = rect.y + (rect.height - textHeight) / 2; // Normal vertical centering
    
    // Clip text if necessary
    int availableWidth = rect.width - 16; // Account for margins
    if (m_tabCloseMode != TabCloseMode::ShowNever) {
        availableWidth -= m_closeButtonSize + 4; // Account for close button
    }
    
    wxString displayTitle = title;
    if (textWidth > availableWidth) {
        // Truncate text with ellipsis
        while (displayTitle.Length() > 3 && textWidth > availableWidth) {
            displayTitle = displayTitle.Left(displayTitle.Length() - 1);
            gc->GetTextExtent(displayTitle + "...", &textWidth, &textHeight);
        }
        displayTitle += "...";
    }
    
    gc->DrawText(displayTitle, textX, textY);
    
    // Draw icon if available
    const wxBitmap& icon = m_contents[index]->icon;
    if (icon.IsOk()) {
        int iconSize = 16;
        int iconX = textX;
        int iconY = rect.y + (rect.height - iconSize) / 2;
        gc->DrawBitmap(icon, iconX, iconY, iconSize, iconSize);
        
        // Adjust text position
        textX += iconSize + 4;
        gc->DrawText(displayTitle, textX, textY);
    }
    
    // Draw close button (always visible, no hover response needed)
    if (m_tabCloseMode != TabCloseMode::ShowNever && index < static_cast<int>(m_closeButtonRects.size())) {
        RenderCloseButton(gc, m_closeButtonRects[index], false);
    }
}

void ModernDockPanel::RenderCloseButton(wxGraphicsContext* gc, const wxRect& rect, bool hovered)
{
    if (!gc) return;
    
    // Draw close button background using theme colors
    if (hovered) {
        wxColour hoverColor = CFG_COLOUR("CloseButtonHoverColour");
        gc->SetBrush(wxBrush(hoverColor));
        gc->DrawRectangle(rect.x, rect.y, rect.width, rect.height);
    }
    
    // Draw X icon using theme colors
    wxColour lineColor;
    if (hovered) {
        lineColor = CFG_COLOUR("CloseButtonTextColour");
    } else {
        lineColor = m_textColor; // Use panel text color for normal state
    }
    gc->SetPen(wxPen(lineColor, 1));
    
    int margin = 4;
    int x1 = rect.x + margin;
    int y1 = rect.y + margin;
    int x2 = rect.GetRight() - margin;
    int y2 = rect.GetBottom() - margin;
    
    // Draw X
    gc->StrokeLine(x1, y1, x2, y2);
    gc->StrokeLine(x2, y1, x1, y2);
}

void ModernDockPanel::OnSize(wxSizeEvent& event)
{
    UpdateLayout();
    
    // Update system buttons position when size changes
    if (m_systemButtons && !m_title.IsEmpty()) {
        int titleBarHeight = 24;
        int buttonAreaWidth = m_systemButtons->GetBestSize().GetWidth();
        int buttonX = GetSize().x - buttonAreaWidth - 4; // Right margin
        int buttonY = (titleBarHeight - m_systemButtons->GetBestSize().GetHeight()) / 2;
        
        m_systemButtons->SetPosition(wxPoint(buttonX, buttonY));
    }
    
    event.Skip();
}

void ModernDockPanel::OnLeftDown(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    
    // Check for close button click
    int closeTabIndex;
    if (HitTestCloseButton(pos, closeTabIndex)) {
        HandleCloseButtonClick(closeTabIndex);
        return;
    }
    
    // Check for tab click
    int tabIndex = HitTestTab(pos);
    if (tabIndex >= 0) {
        HandleTabClick(tabIndex, pos);
        return;
    }
    
    event.Skip();
}

void ModernDockPanel::OnLeftUp(wxMouseEvent& event)
{
    if (m_dragging) {
        // Complete drag operation
        m_dragging = false;
        m_draggedTabIndex = -1;
        
        if (HasCapture()) {
            ReleaseMouse();
        }
        
        // Notify manager to complete drag
        if (m_manager) {
            m_manager->CompleteDrag(ClientToScreen(event.GetPosition()));
        }
    } else if (m_draggedTabIndex >= 0) {
        // Drag was not started (mouse didn't move beyond threshold)
        // Just clean up the potential drag state
        m_draggedTabIndex = -1;
    }
    
    event.Skip();
}

void ModernDockPanel::OnRightDown(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    int tabIndex = HitTestTab(pos);
    
    if (tabIndex >= 0) {
        HandleTabRightClick(tabIndex, pos);
    }
    
    event.Skip();
}

void ModernDockPanel::OnMouseMove(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    
    if (m_dragging) {
        // Update drag operation
        wxPoint screenPos = ClientToScreen(pos);
        
        // Notify manager
        if (m_manager) {
            m_manager->UpdateDrag(screenPos);
        }
        
        m_lastMousePos = screenPos;
    } else if (m_draggedTabIndex >= 0) {
        // Check if we should start dragging (mouse moved beyond threshold)
        wxPoint screenPos = ClientToScreen(pos);
        
        if (abs(screenPos.x - m_dragStartPos.x) > DRAG_THRESHOLD ||
            abs(screenPos.y - m_dragStartPos.y) > DRAG_THRESHOLD) {
            
            // Start dragging now
            m_dragging = true;
            
            // Notify manager to start drag operation
            if (m_manager) {
                m_manager->StartDrag(this, screenPos);
            }
            
            // Capture mouse for drag operation
            CaptureMouse();
        }
    } else {
        // Update hover state
        int newHoveredTab = HitTestTab(pos);
        int newHoveredClose = -1;
        
        HitTestCloseButton(pos, newHoveredClose);
        
        if (newHoveredTab != m_hoveredTabIndex || newHoveredClose != m_hoveredCloseIndex) {
            m_hoveredTabIndex = newHoveredTab;
            m_hoveredCloseIndex = newHoveredClose;
            Refresh();
        }
    }
    
    event.Skip();
}

void ModernDockPanel::OnMouseLeave(wxMouseEvent& event)
{
    if (m_hoveredTabIndex != -1 || m_hoveredCloseIndex != -1) {
        m_hoveredTabIndex = -1;
        m_hoveredCloseIndex = -1;
        Refresh();
    }
    
    event.Skip();
}

void ModernDockPanel::OnMouseEnter(wxMouseEvent& event)
{
    event.Skip();
}

void ModernDockPanel::OnTimer(wxTimerEvent& event)
{
    if (event.GetTimer().GetId() == m_animationTimer.GetId()) {
        UpdateAnimation();
    }
}

void ModernDockPanel::OnContextMenu(wxContextMenuEvent& event)
{
    wxPoint pos = event.GetPosition();
    if (pos == wxDefaultPosition) {
        pos = wxGetMousePosition();
    }
    pos = ScreenToClient(pos);
    
    int tabIndex = HitTestTab(pos);
    if (tabIndex >= 0) {
        // Show context menu for tab
        wxMenu menu;
        menu.Append(wxID_CLOSE, "&Close");
        menu.AppendSeparator();
        menu.Append(wxID_ANY, "Float &Window");
        menu.Append(wxID_ANY, "&Dock as Tabbed Document");
        
        PopupMenu(&menu);
    }
}

void ModernDockPanel::HandleTabClick(int tabIndex, const wxPoint& pos)
{
    wxUnusedVar(pos);
    if (tabIndex != m_selectedIndex) {
        SelectContent(tabIndex);
    }
    
    // Start potential drag operation
    StartDrag(tabIndex, ClientToScreen(pos));
}

void ModernDockPanel::HandleTabDoubleClick(int tabIndex)
{
    wxUnusedVar(tabIndex);
    // Float the tab in a new window
    if (m_manager && tabIndex >= 0 && tabIndex < static_cast<int>(m_contents.size())) {
        // Implementation would create floating window
    }
}

void ModernDockPanel::HandleTabRightClick(int tabIndex, const wxPoint& pos)
{
    wxUnusedVar(pos);
    if (tabIndex != m_selectedIndex) {
        SelectContent(tabIndex);
    }
    
    // Context menu is handled by OnContextMenu
}

void ModernDockPanel::HandleCloseButtonClick(int tabIndex)
{
    RemoveContent(tabIndex);
}

void ModernDockPanel::AnimateTabInsertion(int index)
{
    wxUnusedVar(index);
    // Implementation for tab insertion animation
    StartAnimation(200);
}

void ModernDockPanel::AnimateTabRemoval(int index)
{
    wxUnusedVar(index);
    // Implementation for tab removal animation
    StartAnimation(200);
}

void ModernDockPanel::AnimateResize(const wxSize& targetSize)
{
    wxUnusedVar(targetSize);
    m_animationStartSize = GetSize();
    m_animationTargetSize = targetSize;
    StartAnimation(250);
}

void ModernDockPanel::StartAnimation(int durationMs)
{
    m_animationDuration = durationMs;
    m_animationProgress = 0.0;
    m_animating = true;
    m_animationTimer.Start(1000 / ANIMATION_FPS);
}

void ModernDockPanel::UpdateAnimation()
{
    if (!m_animating) return;
    
    m_animationProgress += 1.0 / (m_animationDuration * ANIMATION_FPS / 1000.0);
    
    if (m_animationProgress >= 1.0) {
        m_animationProgress = 1.0;
        StopAnimation();
    }
    
    // Apply animation (specific implementation depends on animation type)
    Refresh();
}

void ModernDockPanel::StopAnimation()
{
    m_animating = false;
    m_animationTimer.Stop();
}

// Style configuration methods
void ModernDockPanel::SetTabStyle(TabStyle style)
{
    m_tabStyle = style;
    Refresh();
}

void ModernDockPanel::SetTabBorderStyle(TabBorderStyle style)
{
    m_tabBorderStyle = style;
    Refresh();
}

void ModernDockPanel::SetTabCornerRadius(int radius)
{
    m_tabCornerRadius = radius;
    Refresh();
}

void ModernDockPanel::SetTabBorderWidths(int top, int bottom, int left, int right)
{
    m_tabBorderTop = top;
    m_tabBorderBottom = bottom;
    m_tabBorderLeft = left;
    m_tabBorderRight = right;
    Refresh();
}

void ModernDockPanel::GetTabBorderWidths(int& top, int& bottom, int& left, int& right) const
{
    top = m_tabBorderTop;
    bottom = m_tabBorderBottom;
    left = m_tabBorderLeft;
    right = m_tabBorderRight;
}

void ModernDockPanel::SetTabBorderColours(const wxColour& top, const wxColour& bottom, const wxColour& left, const wxColour& right)
{
    m_tabBorderTopColor = top;
    m_tabBorderBottomColor = bottom;
    m_tabBorderLeftColor = left;
    m_tabBorderRightColor = right;
    Refresh();
}

void ModernDockPanel::GetTabBorderColours(wxColour& top, wxColour& bottom, wxColour& left, wxColour& right) const
{
    top = m_tabBorderTopColor;
    bottom = m_tabBorderBottomColor;
    left = m_tabBorderLeftColor;
    right = m_tabBorderRightColor;
}

void ModernDockPanel::SetTabPadding(int padding)
{
    m_tabPadding = padding;
    UpdateLayout();
    Refresh();
}

void ModernDockPanel::SetTabSpacing(int spacing)
{
    m_tabSpacing = spacing;
    UpdateLayout();
    Refresh();
}

void ModernDockPanel::SetTabTopMargin(int margin)
{
    m_tabTopMargin = margin;
    UpdateLayout();
    Refresh();
}

void ModernDockPanel::SetTabFont(const wxFont& font)
{
    m_tabFont = font;
    Refresh();
}

void ModernDockPanel::SetTitleFont(const wxFont& font)
{
    m_titleFont = font;
    Refresh();
}

// System buttons management methods
void ModernDockPanel::AddSystemButton(DockSystemButtonType type, const wxString& tooltip)
{
    if (m_systemButtons) {
        m_systemButtons->AddButton(type, tooltip);
    }
}

void ModernDockPanel::RemoveSystemButton(DockSystemButtonType type)
{
    if (m_systemButtons) {
        m_systemButtons->RemoveButton(type);
    }
}

void ModernDockPanel::SetSystemButtonEnabled(DockSystemButtonType type, bool enabled)
{
    if (m_systemButtons) {
        m_systemButtons->SetButtonEnabled(type, enabled);
    }
}

void ModernDockPanel::SetSystemButtonVisible(DockSystemButtonType type, bool visible)
{
    if (m_systemButtons) {
        m_systemButtons->SetButtonVisible(type, visible);
    }
}

void ModernDockPanel::SetSystemButtonIcon(DockSystemButtonType type, const wxBitmap& icon)
{
    if (m_systemButtons) {
        m_systemButtons->SetButtonIcon(type, icon);
    }
}

void ModernDockPanel::SetSystemButtonTooltip(DockSystemButtonType type, const wxString& tooltip)
{
    if (m_systemButtons) {
        m_systemButtons->SetButtonTooltip(type, tooltip);
    }
}

// Docking control methods
void ModernDockPanel::SetDockingEnabled(bool enabled)
{
    m_dockingEnabled = enabled;
    
    // If docking is disabled, also disable system buttons visibility
    if (!enabled) {
        SetSystemButtonsVisible(false);
    }
}

void ModernDockPanel::SetSystemButtonsVisible(bool visible)
{
    m_systemButtonsVisible = visible;
    
    // Update system buttons visibility
    if (m_systemButtons) {
        m_systemButtons->Show(visible && m_dockingEnabled);
    }
    
    // Refresh to update display
    Refresh();
}


