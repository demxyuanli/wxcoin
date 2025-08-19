#include "widgets/ModernDockPanel.h"
#include "widgets/ModernDockManager.h"
#include "DPIManager.h"
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
      m_animationDuration(250)
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
    
    // Initialize colors (these would normally come from theme manager)
    m_backgroundColor = wxColour(45, 45, 48);
    m_tabActiveColor = wxColour(0, 122, 204);
    m_tabInactiveColor = wxColour(63, 63, 70);
    m_tabHoverColor = wxColour(28, 151, 234);
    m_textColor = wxColour(241, 241, 241);
    m_borderColor = wxColour(63, 63, 70);
    
    // Set minimum size
    SetMinSize(wxSize(m_tabMinWidth, m_tabHeight + 100));
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
    
    m_dragging = true;
    m_draggedTabIndex = tabIndex;
    m_dragStartPos = startPos;
    m_lastMousePos = startPos;
    
    // Notify manager to start drag operation
    if (m_manager) {
        m_manager->StartDrag(this, startPos);
    }
    
    CaptureMouse();
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
        // No tabs, full content area
        m_tabBarRect = wxRect();
        m_contentRect = wxRect(m_contentMargin, m_contentMargin,
                              size.x - 2 * m_contentMargin,
                              size.y - 2 * m_contentMargin);
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
    
    int availableWidth = m_tabBarRect.width;
    int tabCount = static_cast<int>(m_contents.size());
    
    // Calculate tab width
    int tabWidth = std::max(m_tabMinWidth, 
                           std::min(m_tabMaxWidth, availableWidth / tabCount));
    
    // Create tab rectangles
    int x = 0;
    for (int i = 0; i < tabCount; ++i) {
        wxRect tabRect(x, 0, tabWidth, m_tabHeight);
        m_tabRects.push_back(tabRect);
        
        // Calculate close button rect
        wxRect closeRect = CalculateCloseButtonRect(tabRect);
        m_closeButtonRects.push_back(closeRect);
        
        x += tabWidth + m_tabSpacing;
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
    
    // Render tab bar
    if (m_showTabs && !m_contents.empty()) {
        RenderTabBar(gc);
    }
    
    // Render border
    gc->SetPen(wxPen(m_borderColor, 1));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);
    gc->DrawRectangle(0, 0, GetSize().x, GetSize().y);
    
    delete gc;
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
    
    // Choose tab color
    wxColour tabColor = m_tabInactiveColor;
    if (selected) {
        tabColor = m_tabActiveColor;
    } else if (hovered) {
        tabColor = m_tabHoverColor;
    }
    
    // Draw tab background
    gc->SetBrush(wxBrush(tabColor));
    gc->SetPen(wxPen(m_borderColor, 1));
    gc->DrawRectangle(rect.x, rect.y, rect.width, rect.height);
    
    // Draw tab text
    gc->SetFont(GetFont(), m_textColor);
    
    const wxString& title = m_contents[index]->title;
    double textWidth, textHeight;
    gc->GetTextExtent(title, &textWidth, &textHeight);
    
    int textX = rect.x + 8; // Left margin
    int textY = rect.y + (rect.height - textHeight) / 2;
    
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
    
    // Draw close button
    bool showClose = (m_tabCloseMode == TabCloseMode::ShowAlways) ||
                     (m_tabCloseMode == TabCloseMode::ShowOnHover && (hovered || selected));
    
    if (showClose && index < static_cast<int>(m_closeButtonRects.size())) {
        bool closeHovered = (index == m_hoveredCloseIndex);
        RenderCloseButton(gc, m_closeButtonRects[index], closeHovered);
    }
}

void ModernDockPanel::RenderCloseButton(wxGraphicsContext* gc, const wxRect& rect, bool hovered)
{
    if (!gc) return;
    
    // Draw close button background
    if (hovered) {
        gc->SetBrush(wxBrush(wxColour(232, 17, 35))); // Red background on hover
        gc->DrawRectangle(rect.x, rect.y, rect.width, rect.height);
    }
    
    // Draw X icon
    wxColour lineColor = hovered ? *wxWHITE : m_textColor;
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
        m_dragging = false;
        m_draggedTabIndex = -1;
        
        if (HasCapture()) {
            ReleaseMouse();
        }
        
        // Notify manager to complete drag
        if (m_manager) {
            m_manager->CompleteDrag(ClientToScreen(event.GetPosition()));
        }
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
        
        // Check if drag threshold exceeded
        if (!m_dragging || 
            (abs(screenPos.x - m_dragStartPos.x) > DRAG_THRESHOLD ||
             abs(screenPos.y - m_dragStartPos.y) > DRAG_THRESHOLD)) {
            
            // Notify manager
            if (m_manager) {
                m_manager->UpdateDrag(screenPos);
            }
        }
        
        m_lastMousePos = screenPos;
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
    if (tabIndex != m_selectedIndex) {
        SelectContent(tabIndex);
    }
    
    // Start potential drag operation
    StartDrag(tabIndex, ClientToScreen(pos));
}

void ModernDockPanel::HandleTabDoubleClick(int tabIndex)
{
    // Float the tab in a new window
    if (m_manager && tabIndex >= 0 && tabIndex < static_cast<int>(m_contents.size())) {
        // Implementation would create floating window
    }
}

void ModernDockPanel::HandleTabRightClick(int tabIndex, const wxPoint& pos)
{
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
    // Implementation for tab insertion animation
    StartAnimation(200);
}

void ModernDockPanel::AnimateTabRemoval(int index)
{
    // Implementation for tab removal animation
    StartAnimation(200);
}

void ModernDockPanel::AnimateResize(const wxSize& targetSize)
{
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

