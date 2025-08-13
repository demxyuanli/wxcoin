#include "widgets/FlatTreeView.h"
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>
#include <wx/scrolwin.h>
#include <algorithm>
#include <functional>
#include "config/FontManager.h"

// Custom events definitions
wxDEFINE_EVENT(wxEVT_FLAT_TREE_ITEM_SELECTED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_TREE_ITEM_EXPANDED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_TREE_ITEM_COLLAPSED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_TREE_ITEM_CLICKED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_TREE_COLUMN_CLICKED, wxCommandEvent);

// Event table
BEGIN_EVENT_TABLE(FlatTreeView, wxScrolledWindow)
    EVT_PAINT(FlatTreeView::OnPaint)
    EVT_SIZE(FlatTreeView::OnSize)
    EVT_LEFT_DOWN(FlatTreeView::OnMouse)
    EVT_LEFT_UP(FlatTreeView::OnMouse)
    EVT_LEFT_DCLICK(FlatTreeView::OnMouse)
    EVT_MOTION(FlatTreeView::OnMouse)
    EVT_LEAVE_WINDOW(FlatTreeView::OnMouse)
    EVT_KEY_DOWN(FlatTreeView::OnKeyDown)
    EVT_SCROLLWIN(FlatTreeView::OnScroll)
END_EVENT_TABLE()

// FlatTreeItem implementation
FlatTreeItem::FlatTreeItem(const wxString& text, ItemType type)
    : m_text(text)
    , m_type(type)
    , m_visible(true)
    , m_selected(false)
    , m_expanded(false)
    , m_parent(nullptr)
{
}

FlatTreeItem::~FlatTreeItem()
{
}

void FlatTreeItem::SetText(const wxString& text)
{
    m_text = text;
}

void FlatTreeItem::SetType(ItemType type)
{
    m_type = type;
}

void FlatTreeItem::SetIcon(const wxBitmap& icon)
{
    m_icon = icon;
}

void FlatTreeItem::SetVisible(bool visible)
{
    m_visible = visible;
}

void FlatTreeItem::SetSelected(bool selected)
{
    m_selected = selected;
}

void FlatTreeItem::SetExpanded(bool expanded)
{
    m_expanded = expanded;
}

void FlatTreeItem::AddChild(std::shared_ptr<FlatTreeItem> child)
{
    if (child) {
        child->SetParent(this);
        m_children.push_back(child);
    }
}

void FlatTreeItem::RemoveChild(std::shared_ptr<FlatTreeItem> child)
{
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        (*it)->SetParent(nullptr);
        m_children.erase(it);
    }
}

void FlatTreeItem::ClearChildren()
{
    for (auto& child : m_children) {
        child->SetParent(nullptr);
    }
    m_children.clear();
}

void FlatTreeItem::SetParent(FlatTreeItem* parent)
{
    m_parent = parent;
}

void FlatTreeItem::SetColumnData(int column, const wxString& data)
{
    m_columnData[column] = data;
}

wxString FlatTreeItem::GetColumnData(int column) const
{
    auto it = m_columnData.find(column);
    return it != m_columnData.end() ? it->second : wxEmptyString;
}

void FlatTreeItem::SetColumnIcon(int column, const wxBitmap& icon)
{
    m_columnIcons[column] = icon;
}

wxBitmap FlatTreeItem::GetColumnIcon(int column) const
{
    auto it = m_columnIcons.find(column);
    return it != m_columnIcons.end() ? it->second : wxNullBitmap;
}

int FlatTreeItem::GetLevel() const
{
    int level = 0;
    FlatTreeItem* parent = m_parent;
    while (parent) {
        level++;
        parent = parent->GetParent();
    }
    return level;
}

// FlatTreeColumn implementation
FlatTreeColumn::FlatTreeColumn(const wxString& title, ColumnType type, int width)
    : m_title(title)
    , m_type(type)
    , m_width(width)
    , m_visible(true)
    , m_sortable(false)
{
}

void FlatTreeColumn::SetTitle(const wxString& title)
{
    m_title = title;
}

void FlatTreeColumn::SetType(ColumnType type)
{
    m_type = type;
}

void FlatTreeColumn::SetWidth(int width)
{
    m_width = width;
}

void FlatTreeColumn::SetVisible(bool visible)
{
    m_visible = visible;
}

void FlatTreeColumn::SetSortable(bool sortable)
{
    m_sortable = sortable;
}

// FlatTreeView implementation
FlatTreeView::FlatTreeView(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
    : wxScrolledWindow(parent, id, pos, size, style | wxBORDER_NONE)
    , m_itemHeight(24)
    , m_indentWidth(20)
    , m_showLines(true)
    , m_showRootLines(true)
    , m_scrollY(0)
    , m_totalHeight(0)
    , m_needsLayout(true)
    , m_hoveredItem(nullptr)
    , m_lastClickedItem(nullptr)
    , m_treeHScrollBar(nullptr)
    , m_treeHScrollPos(0)
    , m_treeContentWidth(0)
    , m_useConfigFont(true)
    , m_isResizingColumn(false)
    , m_resizingColumnIndex(-1)
    , m_resizeStartX(0)
    , m_initialColumnWidth(0)
    , m_headerResizeMargin(4)
{
    // Set default colors
    m_backgroundColor = wxColour(255, 255, 255);
    m_textColor = wxColour(0, 0, 0);
    m_selectionColor = wxColour(0, 120, 215);
    m_lineColor = wxColour(200, 200, 200);

    // Add default tree column
    AddColumn("Tree", FlatTreeColumn::ColumnType::TREE, 200);

    // Set background color
    SetBackgroundColour(m_backgroundColor);
    // Enable buffered painting to satisfy wxAutoBufferedPaintDC requirements
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetDoubleBuffered(true);
    
    // Enable scrolling
    SetScrollRate(0, m_itemHeight);

    // Independent horizontal scrollbar for first column
    m_treeHScrollBar = new wxScrollBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_HORIZONTAL);
    m_treeHScrollBar->Bind(wxEVT_SCROLL_THUMBTRACK, [this](wxScrollEvent& e){ m_treeHScrollPos = e.GetPosition(); Refresh(); });
    m_treeHScrollBar->Bind(wxEVT_SCROLL_CHANGED, [this](wxScrollEvent& e){ m_treeHScrollPos = e.GetPosition(); Refresh(); });

    // Load font from config (consistent with other flat controls)
    ReloadFontFromConfig();
}

FlatTreeView::~FlatTreeView()
{
}

void FlatTreeView::SetRoot(std::shared_ptr<FlatTreeItem> root)
{
    m_root = root;
    m_needsLayout = true;
    Refresh();
}

void FlatTreeView::AddItem(std::shared_ptr<FlatTreeItem> parent, std::shared_ptr<FlatTreeItem> child)
{
    if (parent && child) {
        parent->AddChild(child);
        m_needsLayout = true;
        Refresh();
    }
}

void FlatTreeView::RemoveItem(std::shared_ptr<FlatTreeItem> item)
{
    if (item && item->GetParent()) {
        item->GetParent()->RemoveChild(item);
        m_needsLayout = true;
        Refresh();
    }
}

void FlatTreeView::Clear()
{
    m_root.reset();
    m_needsLayout = true;
    Refresh();
}

void FlatTreeView::AddColumn(const wxString& title, FlatTreeColumn::ColumnType type, int width)
{
    m_columns.push_back(std::make_unique<FlatTreeColumn>(title, type, width));
    Refresh();
}

void FlatTreeView::RemoveColumn(int index)
{
    if (index >= 0 && index < static_cast<int>(m_columns.size())) {
        m_columns.erase(m_columns.begin() + index);
        Refresh();
    }
}

void FlatTreeView::ClearColumns()
{
    m_columns.clear();
    Refresh();
}

void FlatTreeView::SetColumnWidth(int column, int width)
{
    if (column >= 0 && column < static_cast<int>(m_columns.size())) {
        m_columns[column]->SetWidth(width);
        Refresh();
    }
}

int FlatTreeView::GetColumnWidth(int column) const
{
    if (column >= 0 && column < static_cast<int>(m_columns.size())) {
        return m_columns[column]->GetWidth();
    }
    return 0;
}

void FlatTreeView::SelectItem(std::shared_ptr<FlatTreeItem> item, bool select)
{
    if (item) {
        item->SetSelected(select);
        Refresh();
    }
}

void FlatTreeView::ExpandItem(std::shared_ptr<FlatTreeItem> item, bool expand)
{
    if (item && item->HasChildren()) {
        item->SetExpanded(expand);
        m_needsLayout = true;
        Refresh();
        
        if (expand && m_itemExpandedCallback) {
            m_itemExpandedCallback(item);
        } else if (!expand && m_itemCollapsedCallback) {
            m_itemCollapsedCallback(item);
        }
    }
}

void FlatTreeView::ExpandAll()
{
    if (m_root) {
        ExpandAllRecursive(m_root);
        m_needsLayout = true;
        Refresh();
    }
}

void FlatTreeView::CollapseAll()
{
    if (m_root) {
        CollapseAllRecursive(m_root);
        m_needsLayout = true;
        Refresh();
    }
}

std::shared_ptr<FlatTreeItem> FlatTreeView::FindItem(const wxString& text, std::shared_ptr<FlatTreeItem> startFrom)
{
    if (!startFrom) {
        startFrom = m_root;
    }
    
    if (!startFrom) {
        return nullptr;
    }
    
    if (startFrom->GetText() == text) {
        return startFrom;
    }
    
    for (auto& child : startFrom->GetChildren()) {
        auto found = FindItem(text, child);
        if (found) {
            return found;
        }
    }
    
    return nullptr;
}

std::vector<std::shared_ptr<FlatTreeItem>> FlatTreeView::GetSelectedItems() const
{
    std::vector<std::shared_ptr<FlatTreeItem>> selected;
    if (m_root) {
        CollectSelectedItems(m_root, selected);
    }
    return selected;
}

void FlatTreeView::SetItemHeight(int height)
{
    m_itemHeight = height;
    SetScrollRate(0, m_itemHeight);
    m_needsLayout = true;
    Refresh();
}

void FlatTreeView::SetIndentWidth(int width)
{
    m_indentWidth = width;
    Refresh();
}

void FlatTreeView::SetShowLines(bool show)
{
    m_showLines = show;
    Refresh();
}

void FlatTreeView::SetShowRootLines(bool show)
{
    m_showRootLines = show;
    Refresh();
}

void FlatTreeView::SetBackgroundColor(const wxColour& color)
{
    m_backgroundColor = color;
    SetBackgroundColour(color);
    Refresh();
}

void FlatTreeView::SetTextColor(const wxColour& color)
{
    m_textColor = color;
    Refresh();
}

void FlatTreeView::SetSelectionColor(const wxColour& color)
{
    m_selectionColor = color;
    Refresh();
}

void FlatTreeView::SetLineColor(const wxColour& color)
{
    m_lineColor = color;
    Refresh();
}

void FlatTreeView::OnItemClicked(std::function<void(std::shared_ptr<FlatTreeItem>, int)> callback)
{
    m_itemClickedCallback = callback;
}

void FlatTreeView::OnItemExpanded(std::function<void(std::shared_ptr<FlatTreeItem>)> callback)
{
    m_itemExpandedCallback = callback;
}

void FlatTreeView::OnItemCollapsed(std::function<void(std::shared_ptr<FlatTreeItem>)> callback)
{
    m_itemCollapsedCallback = callback;
}

void FlatTreeView::OnPaint(wxPaintEvent& event)
{
    wxUnusedVar(event);
    wxAutoBufferedPaintDC dc(this);
    
    if (m_needsLayout) {
        CalculateLayout();
    }
    
    DrawBackground(dc);
    DrawColumnHeaders(dc);
    // Prepare DC for scrolled drawing of rows
    PrepareDC(dc);
    DrawItems(dc);
}

void FlatTreeView::OnSize(wxSizeEvent& event)
{
    m_needsLayout = true;
    Refresh();
    RepositionTreeHScrollBar();
    event.Skip();
}

void FlatTreeView::OnMouse(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    
    // Header resizing
    if (event.LeftDown()) {
        // Check if click is on header separator
        if (pos.y >= 0 && pos.y <= m_itemHeight) {
            int sepIndex = HitTestHeaderSeparator(pos);
            if (sepIndex >= 0) {
                m_isResizingColumn = true;
                m_resizingColumnIndex = sepIndex;
                m_resizeStartX = pos.x;
                m_initialColumnWidth = m_columns[sepIndex]->GetWidth();
                CaptureMouse();
                return;
            }
        }
    }

    if (event.LeftUp() && m_isResizingColumn) {
        m_isResizingColumn = false;
        if (HasCapture()) ReleaseMouse();
        return;
    }

    if (event.Dragging() && m_isResizingColumn && m_resizingColumnIndex >= 0) {
        int dx = pos.x - m_resizeStartX;
        int newWidth = std::max(40, m_initialColumnWidth + dx);
        m_columns[m_resizingColumnIndex]->SetWidth(newWidth);
        m_needsLayout = true;
        Refresh();
        return;
    }

    if (event.LeftDown() || event.LeftDClick()) {
        auto item = HitTest(pos);
        if (item) {
            int column = HitTestColumn(pos);
            
            if (column == 0) { // Tree column
                if (item->HasChildren()) {
                    ExpandItem(item, !item->IsExpanded());
                }
            }
            
            if (m_itemClickedCallback) {
                m_itemClickedCallback(item, column);
            }
            
            m_lastClickedItem = item;
        }
    }
    
    if (event.Moving()) {
        auto item = HitTest(pos);
        if (item != m_hoveredItem) {
            m_hoveredItem = item;
            Refresh();
        }
    }
    
    if (event.Leaving()) {
        m_hoveredItem = nullptr;
        Refresh();
    }
    
    event.Skip();
}

void FlatTreeView::OnKeyDown(wxKeyEvent& event)
{
    switch (event.GetKeyCode()) {
        case WXK_SPACE:
            if (m_lastClickedItem) {
                SelectItem(m_lastClickedItem, !m_lastClickedItem->IsSelected());
            }
            break;
        case WXK_RETURN:
            if (m_lastClickedItem && m_lastClickedItem->HasChildren()) {
                ExpandItem(m_lastClickedItem, !m_lastClickedItem->IsExpanded());
            }
            break;
        default:
            event.Skip();
            break;
    }
}

void FlatTreeView::OnScroll(wxScrollWinEvent& event)
{
    m_scrollY = GetScrollPos(wxVERTICAL) * m_itemHeight;
    // Use full repaint to avoid ghosting of header and top scrollbar
    Refresh();
    RepositionTreeHScrollBar();
    event.Skip();
}

void FlatTreeView::DrawBackground(wxDC& dc)
{
    dc.SetBackground(wxBrush(m_backgroundColor));
    dc.Clear();
}

void FlatTreeView::DrawColumnHeaders(wxDC& dc)
{
    if (m_columns.empty()) return;
    
    dc.SetPen(wxPen(m_lineColor));
    dc.SetBrush(wxBrush(wxColour(240, 240, 240)));
    
    int y = 0;
    int x = 0;
    
    for (size_t i = 0; i < m_columns.size(); ++i) {
        if (m_columns[i]->IsVisible()) {
            dc.DrawRectangle(x, y, m_columns[i]->GetWidth(), m_itemHeight);
            
            dc.SetTextForeground(m_textColor);
            dc.SetFont(GetFont());
            
            wxString title = m_columns[i]->GetTitle();
            wxSize textSize = dc.GetTextExtent(title);
            
            int textX = x + 5;
            int textY = y + (m_itemHeight - textSize.GetHeight()) / 2;
            
            dc.DrawText(title, textX, textY);
            
            x += m_columns[i]->GetWidth();
        }
    }
    
    // Draw bottom line
    dc.DrawLine(0, m_itemHeight, GetClientSize().GetWidth(), m_itemHeight);

    // Position top tree h-scrollbar immediately after header and keep it fixed (non-scrolled layer)
    RepositionTreeHScrollBar();
}

void FlatTreeView::DrawItems(wxDC& dc)
{
    if (!m_root) return;
    
    wxSize cs = GetClientSize();
    int barH = (m_treeHScrollBar && m_treeHScrollBar->IsShown()) ? m_treeHScrollBar->GetBestSize().GetHeight() : 0;
    int headerY = m_itemHeight + (barH > 0 ? barH : 0) + 1;
    dc.SetClippingRegion(0, headerY, cs.GetWidth(), cs.GetHeight() - headerY);
    // With PrepareDC, logical origin is shifted up by scroll offset.
    // Start drawing at headerY + m_scrollY so device position aligns just below header.
    int startY = headerY + m_scrollY;
    DrawItemRecursive(dc, m_root, startY, 0);

    dc.DestroyClippingRegion();
}

void FlatTreeView::DrawItemRecursive(wxDC& dc, std::shared_ptr<FlatTreeItem> item, int& y, int level)
{
    if (!item || !item->IsVisible()) return;
    
    // Draw current item
    DrawItem(dc, item, y, level);
    y += m_itemHeight;
    
    // Draw children if expanded
    if (item->IsExpanded()) {
        for (auto& child : item->GetChildren()) {
            DrawItemRecursive(dc, child, y, level + 1);
        }
    }
}

void FlatTreeView::DrawItem(wxDC& dc, std::shared_ptr<FlatTreeItem> item, int y, int level)
{
    if (!item) return;
    
    // Check if item is visible in current view (bottom bound)
    if (y > GetClientSize().GetHeight()) {
        return;
    }
    
    // Draw selection background
    if (item->IsSelected()) {
        dc.SetBrush(wxBrush(m_selectionColor));
        dc.SetPen(wxPen(m_selectionColor));
        dc.DrawRectangle(0, y, GetClientSize().GetWidth(), m_itemHeight);
    } else if (item == m_hoveredItem) {
        dc.SetBrush(wxBrush(wxColour(240, 240, 240)));
        dc.SetPen(wxPen(wxColour(240, 240, 240)));
        dc.DrawRectangle(0, y, GetClientSize().GetWidth(), m_itemHeight);
    }
    
    // Draw tree lines
    if (m_showLines) {
        DrawTreeLines(dc, item, y, level);
    }
    
    // Draw item content for each column
    int x = 0;
    for (size_t i = 0; i < m_columns.size(); ++i) {
        if (m_columns[i]->IsVisible()) {
            int colWidth = m_columns[i]->GetWidth();
            // Clip drawing to the bounds of this column to prevent overlap
            dc.SetClippingRegion(x, y, colWidth, m_itemHeight);
            if (i == 0) { // Tree column
                DrawTreeColumnContent(dc, item, x, y, level);
            } else { // Other columns
                DrawColumnContent(dc, item, i, x, y);
            }
            dc.DestroyClippingRegion();
            x += colWidth;
        }
    }
}

void FlatTreeView::DrawTreeColumnContent(wxDC& dc, std::shared_ptr<FlatTreeItem> item, int x, int y, int level)
{
    // Calculate positions
    int indentX = x + level * m_indentWidth - m_treeHScrollPos;
    int iconX = indentX + 20; // Space for expand/collapse button
    int textX = iconX + 20; // Space for icon
    
    // Draw expand/collapse button
    if (item->HasChildren()) {
        dc.SetPen(wxPen(m_textColor));
        dc.SetBrush(wxBrush(m_backgroundColor));
        
        int buttonSize = 12;
        int buttonX = indentX + 4;
        int buttonY = y + (m_itemHeight - buttonSize) / 2;
        
        dc.DrawRectangle(buttonX, buttonY, buttonSize, buttonSize);
        
        // Draw plus/minus sign
        dc.DrawLine(buttonX + 3, buttonY + buttonSize / 2, buttonX + buttonSize - 3, buttonY + buttonSize / 2);
        if (!item->IsExpanded()) {
            dc.DrawLine(buttonX + buttonSize / 2, buttonY + 3, buttonX + buttonSize / 2, buttonY + buttonSize - 3);
        }
    }
    
    // Draw item icon
    if (item->GetIcon().IsOk()) {
        wxBitmap icon = item->GetIcon();
        int iconSize = 16;
        int iconY = y + (m_itemHeight - iconSize) / 2;
        dc.DrawBitmap(icon, iconX, iconY, true);
    }
    
    // Draw item text
    dc.SetTextForeground(item->IsSelected() ? wxColour(255, 255, 255) : m_textColor);
    dc.SetFont(GetFont());
    
    wxString text = item->GetText();
    wxSize textSize = dc.GetTextExtent(text);
    int textY = y + (m_itemHeight - textSize.GetHeight()) / 2;
    
    // Ensure text does not draw into next column
    int treeColRight = x + m_columns[0]->GetWidth();
    int maxTextWidth = treeColRight - textX - 2;
    if (maxTextWidth > 0) {
        wxString elided = text;
        wxCoord tw, th;
        dc.GetTextExtent(elided, &tw, &th);
        if (tw > maxTextWidth) {
            // Simple right-side elide
            while (!elided.IsEmpty() && tw > maxTextWidth) {
                elided.RemoveLast();
                dc.GetTextExtent(elided + "...", &tw, &th);
            }
            elided += "...";
        }
        dc.DrawText(elided, textX, textY);
    }
}

void FlatTreeView::DrawColumnContent(wxDC& dc, std::shared_ptr<FlatTreeItem> item, int columnIndex, int x, int y)
{
    if (columnIndex >= static_cast<int>(m_columns.size())) return;
    
    auto& column = m_columns[columnIndex];
    wxString data = item->GetColumnData(columnIndex);
    wxBitmap icon = item->GetColumnIcon(columnIndex);
    
    switch (column->GetType()) {
        case FlatTreeColumn::ColumnType::TEXT: {
            if (!data.IsEmpty()) {
                dc.SetTextForeground(item->IsSelected() ? wxColour(255, 255, 255) : m_textColor);
                dc.SetFont(GetFont());
                wxSize textSize = dc.GetTextExtent(data);
                int textY = y + (m_itemHeight - textSize.GetHeight()) / 2;
                dc.DrawText(data, x + 5, textY);
            }
            break;
        }
        case FlatTreeColumn::ColumnType::ICON: {
            if (icon.IsOk()) {
                int iconSize = 16;
                int iconY = y + (m_itemHeight - iconSize) / 2;
                dc.DrawBitmap(icon, x + (column->GetWidth() - iconSize) / 2, iconY, true);
            }
            break;
        }
        case FlatTreeColumn::ColumnType::BUTTON: {
            // Draw button-like appearance
            dc.SetPen(wxPen(wxColour(180, 180, 180)));
            dc.SetBrush(wxBrush(wxColour(240, 240, 240)));
            int buttonWidth = 60;
            int buttonHeight = 20;
            int buttonX = x + (column->GetWidth() - buttonWidth) / 2;
            int buttonY = y + (m_itemHeight - buttonHeight) / 2;
            dc.DrawRectangle(buttonX, buttonY, buttonWidth, buttonHeight);
            if (!data.IsEmpty()) {
                dc.SetTextForeground(m_textColor);
                dc.SetFont(GetFont());
                wxSize textSize = dc.GetTextExtent(data);
                int textX = buttonX + (buttonWidth - textSize.GetWidth()) / 2;
                int textY = buttonY + (buttonHeight - textSize.GetHeight()) / 2;
                dc.DrawText(data, textX, textY);
            }
            break;
        }
        case FlatTreeColumn::ColumnType::CHECKBOX: {
            // Draw checkbox
            dc.SetPen(wxPen(m_textColor));
            dc.SetBrush(wxBrush(m_backgroundColor));
            int checkboxSize = 16;
            int checkboxX = x + (column->GetWidth() - checkboxSize) / 2;
            int checkboxY = y + (m_itemHeight - checkboxSize) / 2;
            dc.DrawRectangle(checkboxX, checkboxY, checkboxSize, checkboxSize);
            // Draw check mark if data is "true" or "1"
            if (data == "true" || data == "1") {
                dc.SetPen(wxPen(m_textColor, 2));
                dc.DrawLine(checkboxX + 3, checkboxY + 8, checkboxX + 6, checkboxY + 11);
                dc.DrawLine(checkboxX + 6, checkboxY + 11, checkboxX + 12, checkboxY + 5);
            }
            break;
        }
        default: {
            break;
        }
    }
}

void FlatTreeView::DrawTreeLines(wxDC& dc, std::shared_ptr<FlatTreeItem> item, int y, int level)
{
    if (level == 0) return;
    
    dc.SetPen(wxPen(m_lineColor, 1, wxPENSTYLE_DOT));
    
    int lineY = y + m_itemHeight / 2;
    
    // Draw vertical line from parent
    if (item->GetParent()) {
        int startX = item->GetParent()->GetLevel() * m_indentWidth + 10;
        int endX = level * m_indentWidth + 10;
        
        dc.DrawLine(startX, lineY, endX, lineY);
    }
}

std::shared_ptr<FlatTreeItem> FlatTreeView::HitTest(const wxPoint& point)
{
    if (!m_root) return nullptr;
    
    int barH = (m_treeHScrollBar && m_treeHScrollBar->IsShown()) ? m_treeHScrollBar->GetBestSize().GetHeight() : 0;
    int y = point.y - (m_itemHeight + (barH > 0 ? barH : 0) + 1) + m_scrollY; // rows coordinates
    if (y < 0) return nullptr;
    
    int itemIndex = y / m_itemHeight;
    return GetItemByIndex(m_root, itemIndex);
}

int FlatTreeView::HitTestColumn(const wxPoint& point)
{
    int x = point.x;
    int currentX = 0;
    
    for (size_t i = 0; i < m_columns.size(); ++i) {
        if (m_columns[i]->IsVisible()) {
            if (x >= currentX && x < currentX + m_columns[i]->GetWidth()) {
                return static_cast<int>(i);
            }
            currentX += m_columns[i]->GetWidth();
        }
    }
    
    return -1;
}

int FlatTreeView::HitTestHeaderSeparator(const wxPoint& point) const
{
    if (point.y < 0 || point.y > m_itemHeight) return -1;
    int currentX = 0;
    for (size_t i = 0; i < m_columns.size(); ++i) {
        if (!m_columns[i]->IsVisible()) continue;
        currentX += m_columns[i]->GetWidth();
        // Separator area between column i and i+1 (we resize column i)
        if (std::abs(point.x - currentX) <= m_headerResizeMargin) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void FlatTreeView::CalculateLayout()
{
    if (!m_root) {
        m_totalHeight = 0;
        return;
    }
    
    m_totalHeight = CalculateItemHeightRecursive(m_root);
    UpdateScrollbars();
    // Update the first-column horizontal scrollbar
    // compute content width: simple approximation based on max depth and text widths
    {
        wxClientDC tdc(this);
        tdc.SetFont(GetFont());
        int maxDepth = 0;
        int maxText = 0;
        std::function<void(std::shared_ptr<FlatTreeItem>, int)> dfs = [&](std::shared_ptr<FlatTreeItem> it, int depth){
            if (!it) return;
            maxDepth = std::max(maxDepth, depth);
            wxSize s = tdc.GetTextExtent(it->GetText());
            if (s.GetWidth() > maxText) maxText = s.GetWidth();
            if (it->IsExpanded()) {
                for (auto& ch : it->GetChildren()) dfs(ch, depth + 1);
            }
        };
        dfs(m_root, 0);
        int icons = 40;
        m_treeContentWidth = maxDepth * m_indentWidth + icons + maxText + 16;
        int treeColWidth = m_columns.empty() ? 0 : m_columns[0]->GetWidth();
        int range = std::max(0, m_treeContentWidth - treeColWidth);
        if (m_treeHScrollBar) {
            if (range > 0) {
                m_treeHScrollBar->SetScrollbar(m_treeHScrollPos, treeColWidth, m_treeContentWidth, treeColWidth);
                m_treeHScrollBar->Show();
                RepositionTreeHScrollBar();
            } else {
                m_treeHScrollBar->Hide();
                m_treeHScrollPos = 0;
            }
        }
    }
    m_needsLayout = false;
}

void FlatTreeView::RepositionTreeHScrollBar()
{
    if (!m_treeHScrollBar) return;
    int treeColWidth = m_columns.empty() ? 0 : m_columns[0]->GetWidth();
    int barHeight = m_treeHScrollBar->GetBestSize().GetHeight();
    // Pin directly below header at top-left, width equals first column width
    m_treeHScrollBar->SetSize(0, m_itemHeight, treeColWidth, barHeight);
    m_treeHScrollBar->Raise();
}

// Font configuration methods
void FlatTreeView::SetCustomFont(const wxFont& font)
{
    m_customFont = font;
    m_useConfigFont = false;
    SetFont(font);
    m_needsLayout = true;
    Refresh();
}

void FlatTreeView::UseConfigFont(bool useConfig)
{
    m_useConfigFont = useConfig;
    if (useConfig) {
        ReloadFontFromConfig();
    }
}

void FlatTreeView::ReloadFontFromConfig()
{
    if (m_useConfigFont) {
        try {
            FontManager& fm = FontManager::getInstance();
            wxFont f = fm.getLabelFont();
            if (f.IsOk()) {
                SetFont(f);
                m_customFont = f;
                m_needsLayout = true;
                Refresh();
            }
        } catch (...) {
            SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
        }
    }
}

int FlatTreeView::CalculateItemHeightRecursive(std::shared_ptr<FlatTreeItem> item)
{
    if (!item || !item->IsVisible()) return 0;
    
    int height = m_itemHeight;
    
    if (item->IsExpanded()) {
        for (auto& child : item->GetChildren()) {
            height += CalculateItemHeightRecursive(child);
        }
    }
    
    return height;
}

int FlatTreeView::CalculateItemY(std::shared_ptr<FlatTreeItem> item)
{
    if (!item) return 0;
    
    int y = m_itemHeight + 1; // Start below headers
    
    return CalculateItemYRecursive(m_root, item, y);
}

int FlatTreeView::CalculateItemYRecursive(std::shared_ptr<FlatTreeItem> current, std::shared_ptr<FlatTreeItem> target, int& y)
{
    if (!current || !current->IsVisible()) return -1;
    
    if (current == target) {
        return y;
    }
    
    y += m_itemHeight;
    
    if (current->IsExpanded()) {
        for (auto& child : current->GetChildren()) {
            int result = CalculateItemYRecursive(child, target, y);
            if (result != -1) {
                return result;
            }
        }
    }
    
    return -1;
}

void FlatTreeView::UpdateScrollbars()
{
    wxSize clientSize = GetClientSize();
    int barH = (m_treeHScrollBar && m_treeHScrollBar->IsShown()) ? m_treeHScrollBar->GetBestSize().GetHeight() : 0;
    int visibleHeight = clientSize.GetHeight() - (m_itemHeight + (barH > 0 ? barH : 0) + 1);
    
    if (m_totalHeight > visibleHeight) {
        SetScrollbars(0, m_itemHeight, 0, (m_totalHeight + m_itemHeight - 1) / m_itemHeight);
    } else {
        SetScrollbars(0, 0, 0, 0);
    }
}

std::shared_ptr<FlatTreeItem> FlatTreeView::GetItemByIndex(std::shared_ptr<FlatTreeItem> item, int& index)
{
    if (!item || !item->IsVisible()) return nullptr;
    
    if (index == 0) {
        return item;
    }
    
    index--;
    
    if (item->IsExpanded()) {
        for (auto& child : item->GetChildren()) {
            auto result = GetItemByIndex(child, index);
            if (result) {
                return result;
            }
        }
    }
    
    return nullptr;
}

void FlatTreeView::ExpandAllRecursive(std::shared_ptr<FlatTreeItem> item)
{
    if (!item) return;
    
    if (item->HasChildren()) {
        item->SetExpanded(true);
        for (auto& child : item->GetChildren()) {
            ExpandAllRecursive(child);
        }
    }
}

void FlatTreeView::CollapseAllRecursive(std::shared_ptr<FlatTreeItem> item)
{
    if (!item) return;
    
    if (item->HasChildren()) {
        item->SetExpanded(false);
        for (auto& child : item->GetChildren()) {
            CollapseAllRecursive(child);
        }
    }
}

void FlatTreeView::CollectSelectedItems(std::shared_ptr<FlatTreeItem> item, std::vector<std::shared_ptr<FlatTreeItem>>& selected) const
{
    if (!item) return;
    
    if (item->IsSelected()) {
        selected.push_back(item);
    }
    
    for (auto& child : item->GetChildren()) {
        CollectSelectedItems(child, selected);
    }
}

void FlatTreeView::RefreshItem(std::shared_ptr<FlatTreeItem> item)
{
    if (item) {
        Refresh();
    }
}

void FlatTreeView::EnsureVisible(std::shared_ptr<FlatTreeItem> item)
{
    if (!item) return;
    
    int itemY = CalculateItemY(item);
    if (itemY != -1) {
        int scrollPos = itemY / m_itemHeight;
        SetScrollPos(wxVERTICAL, scrollPos);
        m_scrollY = scrollPos * m_itemHeight;
        Refresh();
    }
}
