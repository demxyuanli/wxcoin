#include "widgets/FlatTreeView.h"
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcbuffer.h>
#include <wx/scrolwin.h>
#include <wx/cursor.h>
#include <algorithm>
#include <functional>
#include "config/FontManager.h"
#include "config/SvgIconManager.h"

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
EVT_ERASE_BACKGROUND(FlatTreeView::OnEraseBackground)
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
	: wxScrolledWindow(parent, id, pos, size, style | wxBORDER_NONE | wxVSCROLL | wxHSCROLL)
	, m_itemHeight(22)
	, m_indentWidth(16)
	, m_showLines(true)
	, m_showRootLines(true)
	, m_scrollY(0)
	, m_scrollX(0)
	, m_totalHeight(0)
	, m_needsLayout(true)
	, m_hoveredItem(nullptr)
	, m_lastClickedItem(nullptr)
	, m_treeHScrollBar(nullptr)
	, m_treeHScrollPos(0)
	, m_treeContentWidth(0)
	, m_useConfigFont(true)
	, m_svgIconSize(12, 12)
	, m_showHeaderText(false)
	, m_isResizingColumn(false)
	, m_resizingColumnIndex(-1)
	, m_resizeStartX(0)
	, m_initialColumnWidth(0)
	, m_headerResizeMargin(4)
	, m_alwaysShowScrollbars(true)
{
	// Set default colors
	m_backgroundColor = wxColour(255, 255, 255);
	m_textColor = wxColour(0, 0, 0);
	m_selectionColor = wxColour(0, 120, 215);
	m_lineColor = wxColour(200, 200, 200);

	// Add default tree column (default width 100)
	AddColumn("Tree", FlatTreeColumn::ColumnType::TREE, 140);

	// Set background color
	SetBackgroundColour(m_backgroundColor);
	// Enable buffered painting to satisfy wxAutoBufferedPaintDC requirements
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetDoubleBuffered(true);

	// Enable scrolling with pixel-based scrolling
	SetScrollRate(1, 1);

	// Independent horizontal scrollbar for first column
	m_treeHScrollBar = new wxScrollBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_HORIZONTAL);
	m_treeHScrollBar->Bind(wxEVT_SCROLL_THUMBTRACK, [this](wxScrollEvent& e) { m_treeHScrollPos = e.GetPosition(); Refresh(); });
	m_treeHScrollBar->Bind(wxEVT_SCROLL_CHANGED, [this](wxScrollEvent& e) { m_treeHScrollPos = e.GetPosition(); Refresh(); });

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
		}
		else if (!expand && m_itemCollapsedCallback) {
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
	SetScrollRate(1, 1); // Use pixel-based scrolling
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

void FlatTreeView::OnEraseBackground(wxEraseEvent& event)
{
	// Prevent default background erase to avoid flicker; we'll clear in DrawBackground
	wxUnusedVar(event);
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

	// Update cursor over header separators
	if (pos.y >= 0 && pos.y <= m_itemHeight) {
		if (m_isResizingColumn) {
			SetCursor(wxCursor(wxCURSOR_SIZEWE));
		}
		else {
			int sepIndexHover = HitTestHeaderSeparator(pos);
			if (sepIndexHover >= 0) {
				SetCursor(wxCursor(wxCURSOR_SIZEWE));
			}
			else {
				SetCursor(wxCursor(wxCURSOR_ARROW));
			}
		}
	}
	else if (!m_isResizingColumn) {
		// Restore arrow cursor when leaving header region and not resizing
		SetCursor(wxCursor(wxCURSOR_ARROW));
	}

	// Header resizing
	if (event.LeftDown()) {
		// Check if click is on header separator
		if (pos.y >= 0 && pos.y <= m_itemHeight) {
			int sepIndex = HitTestHeaderSeparator(pos);
			if (sepIndex >= 0) {
				m_isResizingColumn = true;
				m_resizingColumnIndex = sepIndex;
				m_resizeStartX = pos.x + m_scrollX; // Apply horizontal scroll offset
				m_initialColumnWidth = m_columns[sepIndex]->GetWidth();
				CaptureMouse();
				SetCursor(wxCursor(wxCURSOR_SIZEWE));
				return;
			}
		}
	}

	if (event.LeftUp() && m_isResizingColumn) {
		m_isResizingColumn = false;
		if (HasCapture()) ReleaseMouse();
		SetCursor(wxCursor(wxCURSOR_ARROW));
		return;
	}

	if (event.Dragging() && m_isResizingColumn && m_resizingColumnIndex >= 0) {
		int dx = (pos.x + m_scrollX) - m_resizeStartX; // Apply horizontal scroll offset
		int newWidth = std::max(40, m_initialColumnWidth + dx);
		m_columns[m_resizingColumnIndex]->SetWidth(newWidth);
		m_needsLayout = true;
		Refresh();
		SetCursor(wxCursor(wxCURSOR_SIZEWE));
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
		if (!m_isResizingColumn) {
			SetCursor(wxCursor(wxCURSOR_ARROW));
		}
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
	int orient = event.GetOrientation();
	if (orient == wxVERTICAL) {
		// Get scroll position from the scrollbar
		int scrollPos = GetScrollPos(wxVERTICAL);
		m_scrollY = scrollPos;

		// Repaint only content area to reduce flicker
		RefreshContentArea();
		RepositionTreeHScrollBar();
	}
	else if (orient == wxHORIZONTAL) {
		// Get horizontal scroll position from the scrollbar
		int scrollPos = GetScrollPos(wxHORIZONTAL);
		m_scrollX = scrollPos;

		// Repaint entire control for horizontal scrolling
		Refresh();
	}
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
	int x = -m_scrollX; // Apply horizontal scroll offset

	for (size_t i = 0; i < m_columns.size(); ++i) {
		if (m_columns[i]->IsVisible()) {
			dc.DrawRectangle(x, y, m_columns[i]->GetWidth(), m_itemHeight);

			// Draw column icon only (no text)
			wxString svgIconName = GetColumnSvgIconName(i);

			if (!svgIconName.IsEmpty()) {
				// Try to draw SVG icon in header
				try {
					auto& iconManager = SvgIconManager::GetInstance();
					wxSize iconSize = m_columnSvgIconSizes[i];
					if (iconSize.GetWidth() <= 0 || iconSize.GetHeight() <= 0) {
						iconSize = wxSize(12, 12); // Default size
					}
					wxBitmap svgIcon = iconManager.GetIconBitmap(svgIconName, iconSize);
					if (svgIcon.IsOk()) {
						// Center the icon in the column header
						int iconX = x + (m_columns[i]->GetWidth() - iconSize.GetWidth()) / 2;
						int iconY = y + (m_itemHeight - iconSize.GetHeight()) / 2;
						dc.DrawBitmap(svgIcon, iconX, iconY, true);
					}
				}
				catch (...) {
					// If SVG fails, don't draw anything in header
				}
			}

			x += m_columns[i]->GetWidth();
		}
	}

	// Draw bottom line
	dc.DrawLine(-m_scrollX, m_itemHeight, GetClientSize().GetWidth() - m_scrollX, m_itemHeight);

	// Position top tree h-scrollbar immediately after header and keep it fixed (non-scrolled layer)
	RepositionTreeHScrollBar();
}

void FlatTreeView::DrawItems(wxDC& dc)
{
	if (!m_root) return;

	wxSize cs = GetClientSize();
	int barH = (m_treeHScrollBar && m_treeHScrollBar->IsShown()) ? m_treeHScrollBar->GetBestSize().GetHeight() : 0;
	int headerY = m_itemHeight + (barH > 0 ? barH : 0) + 1;

	// Set clipping region for content area only
	dc.SetClippingRegion(0, headerY, cs.GetWidth(), cs.GetHeight() - headerY);

	// Calculate drawing start position
	// m_scrollY is now in pixels, so we can use it directly
	int startY = headerY - m_scrollY;

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

	// Check if item is visible in current view
	// y is now in content coordinates (relative to header)
	wxSize cs = GetClientSize();
	int barH = (m_treeHScrollBar && m_treeHScrollBar->IsShown()) ? m_treeHScrollBar->GetBestSize().GetHeight() : 0;
	int headerY = m_itemHeight + (barH > 0 ? barH : 0) + 1;

	// Convert to screen coordinates for visibility check
	int screenY = y + headerY;
	if (screenY + m_itemHeight < headerY || screenY > cs.GetHeight()) {
		return; // Item is not visible
	}

	// Draw selection background
	if (item->IsSelected()) {
		dc.SetBrush(wxBrush(m_selectionColor));
		dc.SetPen(wxPen(m_selectionColor));
		dc.DrawRectangle(-m_scrollX, y, GetClientSize().GetWidth(), m_itemHeight);
	}
	else if (item == m_hoveredItem) {
		dc.SetBrush(wxBrush(wxColour(240, 240, 240)));
		dc.SetPen(wxPen(wxColour(240, 240, 240)));
		dc.DrawRectangle(-m_scrollX, y, GetClientSize().GetWidth(), m_itemHeight);
	}

	// Draw tree lines
	if (m_showLines) {
		DrawTreeLines(dc, item, y, level);
	}

	// Draw item content for each column
	int x = -m_scrollX; // Apply horizontal scroll offset
	for (size_t i = 0; i < m_columns.size(); ++i) {
		if (m_columns[i]->IsVisible()) {
			int colWidth = m_columns[i]->GetWidth();
			// Clip drawing to the bounds of this column to prevent overlap
			dc.SetClippingRegion(x, y, colWidth, m_itemHeight);
			if (i == 0) { // Tree column
				DrawTreeColumnContent(dc, item, x, y, level);
			}
			else { // Other columns
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
	int textX = iconX + 14; // Space for icon (reduced from 20 to 2 pixels)

	// Draw expand/collapse button
	if (item->HasChildren()) {
		dc.SetPen(wxPen(m_textColor));
		dc.SetBrush(wxBrush(m_backgroundColor));

		int buttonSize = 12; // Reduced from 12 to match 12x12 icon size
		int buttonX = indentX + 4;
		int buttonY = y + (m_itemHeight - buttonSize) / 2;

		dc.DrawRectangle(buttonX, buttonY, buttonSize, buttonSize);

		// Draw plus/minus sign
		dc.DrawLine(buttonX + 2, buttonY + buttonSize / 2, buttonX + buttonSize - 2, buttonY + buttonSize / 2);
		if (!item->IsExpanded()) {
			dc.DrawLine(buttonX + buttonSize / 2, buttonY + 2, buttonX + buttonSize / 2, buttonY + buttonSize - 2);
		}
	}

	// Draw item icon (SVG or bitmap)
	wxString svgIconName = GetItemSvgIconName(item);
	if (!svgIconName.IsEmpty()) {
		// Try to draw SVG icon
		try {
			auto& iconManager = SvgIconManager::GetInstance();
			wxBitmap svgIcon = iconManager.GetIconBitmap(svgIconName, m_svgIconSize);
			if (svgIcon.IsOk()) {
				int iconY = y + (m_itemHeight - m_svgIconSize.GetHeight()) / 2;
				dc.DrawBitmap(svgIcon, iconX, iconY, true);
			}
		}
		catch (...) {
			// Fallback to bitmap icon if SVG fails
			if (item->GetIcon().IsOk()) {
				wxBitmap icon = item->GetIcon();
				int iconSize = 12; // Use 12x12 size for consistency
				int iconY = y + (m_itemHeight - iconSize) / 2;
				dc.DrawBitmap(icon, iconX, iconY, true);
			}
		}
	}
	else if (item->GetIcon().IsOk()) {
		// Draw bitmap icon
		wxBitmap icon = item->GetIcon();
		int iconSize = 12; // Use 12x12 size for consistency
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
		// Try SVG icon first, then fallback to bitmap
		wxString svgIconName = GetItemColumnSvgIconName(item, columnIndex);
		if (!svgIconName.IsEmpty()) {
			try {
				auto& iconManager = SvgIconManager::GetInstance();
				wxSize iconSize = m_columnSvgIconSizes[columnIndex];
				if (iconSize.GetWidth() <= 0 || iconSize.GetHeight() <= 0) {
					iconSize = wxSize(12, 12); // Default size
				}
				wxBitmap svgIcon = iconManager.GetIconBitmap(svgIconName, iconSize);
				if (svgIcon.IsOk()) {
					int iconY = y + (m_itemHeight - iconSize.GetHeight()) / 2;
					dc.DrawBitmap(svgIcon, x + (column->GetWidth() - iconSize.GetWidth()) / 2, iconY, true);
				}
			}
			catch (...) {
				// Fallback to bitmap icon if SVG fails
				if (icon.IsOk()) {
					int iconSize = 12; // Use 12x12 size for consistency
					int iconY = y + (m_itemHeight - iconSize) / 2;
					dc.DrawBitmap(icon, x + (column->GetWidth() - iconSize) / 2, iconY, true);
				}
			}
		}
		else if (icon.IsOk()) {
			// Draw bitmap icon
			int iconSize = 12; // Use 12x12 size for consistency
			int iconY = y + (m_itemHeight - iconSize) / 2;
			dc.DrawBitmap(icon, x + (column->GetWidth() - iconSize) / 2, iconY, true);
		}
		break;
	}
	case FlatTreeColumn::ColumnType::BUTTON: {
		// Draw button-like appearance
		dc.SetPen(wxPen(wxColour(180, 180, 180)));
		dc.SetBrush(wxBrush(wxColour(240, 240, 240)));
		int buttonWidth = 24; // Reduced from 60 to match 12x12 icon size
		int buttonHeight = 16; // Reduced from 20 to match 12x12 icon size
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
		int checkboxSize = 12; // Use 12x12 size for consistency
		int checkboxX = x + (column->GetWidth() - checkboxSize) / 2;
		int checkboxY = y + (m_itemHeight - checkboxSize) / 2;
		dc.DrawRectangle(checkboxX, checkboxY, checkboxSize, checkboxSize);
		// Draw check mark if data is "true" or "1"
		if (data == "true" || data == "1") {
			dc.SetPen(wxPen(m_textColor, 2));
			dc.DrawLine(checkboxX + 3, checkboxY + 6, checkboxX + 5, checkboxY + 8);
			dc.DrawLine(checkboxX + 5, checkboxY + 8, checkboxX + 9, checkboxY + 4);
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
	int headerY = m_itemHeight + (barH > 0 ? barH : 0) + 1;

	// Convert screen coordinates to content coordinates
	int y = point.y - headerY + m_scrollY;

	if (y < 0) return nullptr;

	// Calculate item index based on pixel position
	int itemIndex = y / m_itemHeight;
	return GetItemByIndex(m_root, itemIndex);
}

int FlatTreeView::HitTestColumn(const wxPoint& point)
{
	int x = point.x + m_scrollX; // Apply horizontal scroll offset
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
	int x = point.x + m_scrollX; // Apply horizontal scroll offset
	int currentX = 0;
	for (size_t i = 0; i < m_columns.size(); ++i) {
		if (!m_columns[i]->IsVisible()) continue;
		currentX += m_columns[i]->GetWidth();
		// Separator area between column i and i+1 (we resize column i)
		if (std::abs(x - currentX) <= m_headerResizeMargin) {
			return static_cast<int>(i);
		}
	}
	return -1;
}

void FlatTreeView::CalculateLayout()
{
	int clientW, clientH;
	GetClientSize(&clientW, &clientH);
	int barH = (m_treeHScrollBar && m_treeHScrollBar->IsShown()) ? m_treeHScrollBar->GetBestSize().GetHeight() : 0;
	int headerY = m_itemHeight + (barH > 0 ? barH : 0) + 1;

	if (!m_root) {
		m_totalHeight = 0;
		// Set both scrollbars to show but with zero range
		int rowsVisible = std::max(0, clientH - headerY);
		
		if (m_alwaysShowScrollbars) {
			SetScrollbar(wxVERTICAL, 0, rowsVisible, 0, true);
			SetScrollbar(wxHORIZONTAL, 0, clientW, 0, true);
		} else {
			SetScrollbar(wxVERTICAL, 0, 0, 0, true);
			SetScrollbar(wxHORIZONTAL, 0, 0, 0, true);
		}
		return;
	}

	m_totalHeight = CalculateItemHeightRecursive(m_root);

	// 1. Calculate vertical scrollbar for rows area (below header and top horizontal scrollbar)
	int rowsVisible = std::max(0, clientH - headerY);

	if (m_totalHeight > rowsVisible) {
		// Vertical scrollbar for rows only
		int range = m_totalHeight - rowsVisible;
		int thumb = rowsVisible;
		int pos = std::min(m_scrollY, range);
		SetScrollbar(wxVERTICAL, pos, thumb, m_totalHeight, true);
	}
	else if (m_alwaysShowScrollbars) {
		// Always show scrollbar, but with zero range when not needed
		SetScrollbar(wxVERTICAL, 0, rowsVisible, m_totalHeight, true);
		m_scrollY = 0;
		SetScrollPos(wxVERTICAL, 0);
	}
	else {
		SetScrollbar(wxVERTICAL, 0, 0, 0, true);
		m_scrollY = 0;
		SetScrollPos(wxVERTICAL, 0);
	}

	// 2. Calculate horizontal scrollbar for entire control (at bottom)
	int totalContentWidth = 0;
	for (const auto& column : m_columns) {
		if (column->IsVisible()) {
			totalContentWidth += column->GetWidth();
		}
	}

	if (totalContentWidth > clientW) {
		// Horizontal scrollbar for entire control
		int range = totalContentWidth - clientW;
		int thumb = clientW;
		int pos = std::min(m_scrollX, range);
		SetScrollbar(wxHORIZONTAL, pos, thumb, totalContentWidth, true);
	}
	else if (m_alwaysShowScrollbars) {
		// Always show scrollbar, but with zero range when not needed
		SetScrollbar(wxHORIZONTAL, 0, clientW, totalContentWidth, true);
		m_scrollX = 0;
		SetScrollPos(wxHORIZONTAL, 0);
	}
	else {
		SetScrollbar(wxHORIZONTAL, 0, 0, 0, true);
		m_scrollX = 0;
		SetScrollPos(wxHORIZONTAL, 0);
	}

	// 3. Update the top first-column horizontal scrollbar
	{
		wxClientDC tdc(this);
		tdc.SetFont(GetFont());
		int maxDepth = 0;
		int maxText = 0;
		std::function<void(std::shared_ptr<FlatTreeItem>, int)> dfs = [&](std::shared_ptr<FlatTreeItem> it, int depth) {
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
			}
			else {
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
		}
		catch (...) {
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

	// Update vertical scrollbar
	if (m_totalHeight > visibleHeight) {
		// Use wxDataViewTreeCtrl style: pixel-based scrollbar
		int range = m_totalHeight - visibleHeight;
		int thumb = visibleHeight;
		int pos = std::min(m_scrollY, range);
		SetScrollbar(wxVERTICAL, pos, thumb, m_totalHeight, true);
	}
	else if (m_alwaysShowScrollbars) {
		// Always show scrollbar, but with zero range when not needed
		SetScrollbar(wxVERTICAL, 0, visibleHeight, m_totalHeight, true);
		m_scrollY = 0;
		SetScrollPos(wxVERTICAL, 0);
	}
	else {
		SetScrollbar(wxVERTICAL, 0, 0, 0, true);
		m_scrollY = 0;
		SetScrollPos(wxVERTICAL, 0);
	}

	// Update horizontal scrollbar
	int totalContentWidth = 0;
	for (const auto& column : m_columns) {
		if (column->IsVisible()) {
			totalContentWidth += column->GetWidth();
		}
	}

	if (totalContentWidth > clientSize.GetWidth()) {
		int range = totalContentWidth - clientSize.GetWidth();
		int thumb = clientSize.GetWidth();
		int pos = std::min(m_scrollX, range);
		SetScrollbar(wxHORIZONTAL, pos, thumb, totalContentWidth, true);
	}
	else if (m_alwaysShowScrollbars) {
		// Always show scrollbar, but with zero range when not needed
		SetScrollbar(wxHORIZONTAL, 0, clientSize.GetWidth(), totalContentWidth, true);
		m_scrollX = 0;
		SetScrollPos(wxHORIZONTAL, 0);
	}
	else {
		SetScrollbar(wxHORIZONTAL, 0, 0, 0, true);
		m_scrollX = 0;
		SetScrollPos(wxHORIZONTAL, 0);
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
		InvalidateItem(item);
	}
}

void FlatTreeView::EnsureVisible(std::shared_ptr<FlatTreeItem> item)
{
	if (!item) return;

	int itemY = CalculateItemY(item);
	if (itemY != -1) {
		// Convert item Y position to scroll position
		// itemY is relative to the content area (below header)
		int scrollPos = itemY;

		// Ensure the item is visible in the viewport
		wxSize clientSize = GetClientSize();
		int barH = (m_treeHScrollBar && m_treeHScrollBar->IsShown()) ? m_treeHScrollBar->GetBestSize().GetHeight() : 0;
		int headerY = m_itemHeight + (barH > 0 ? barH : 0) + 1;
		int visibleHeight = clientSize.GetHeight() - headerY;

		// If item is below visible area, scroll to show it
		if (itemY + m_itemHeight > m_scrollY + visibleHeight) {
			scrollPos = itemY - visibleHeight + m_itemHeight;
		}
		// If item is above visible area, scroll to show it at top
		else if (itemY < m_scrollY) {
			scrollPos = itemY;
		}

		// Set scroll position and update internal state
		SetScrollPos(wxVERTICAL, scrollPos);
		m_scrollY = scrollPos;
		Refresh();
	}
}

void FlatTreeView::InvalidateItem(std::shared_ptr<FlatTreeItem> item)
{
	if (!item) return;
	int y = CalculateItemY(item);
	if (y == -1) { RefreshContentArea(); return; }
	int barH = (m_treeHScrollBar && m_treeHScrollBar->IsShown()) ? m_treeHScrollBar->GetBestSize().GetHeight() : 0;
	int headerY = m_itemHeight + (barH > 0 ? barH : 0) + 1;
	wxRect rect(0, y, GetClientSize().GetWidth(), m_itemHeight);
	rect.y = std::max(rect.y, headerY);
	RefreshRect(rect, false);
}

void FlatTreeView::RefreshContentArea()
{
	int barH = (m_treeHScrollBar && m_treeHScrollBar->IsShown()) ? m_treeHScrollBar->GetBestSize().GetHeight() : 0;
	int headerY = m_itemHeight + (barH > 0 ? barH : 0) + 1;
	wxRect rect(0, headerY, GetClientSize().GetWidth(), GetClientSize().GetHeight() - headerY);
	RefreshRect(rect, false);
}

// SVG icon support methods
void FlatTreeView::SetSvgIcon(const wxString& iconName, const wxSize& size)
{
	m_svgIconName = iconName;
	m_svgIconSize = size;
	Refresh();
}

void FlatTreeView::SetColumnSvgIcon(int column, const wxString& iconName, const wxSize& size)
{
	if (column >= 0 && column < static_cast<int>(m_columns.size())) {
		m_columnSvgIconNames[column] = iconName;
		m_columnSvgIconSizes[column] = size;
		Refresh();
	}
}

void FlatTreeView::SetItemSvgIcon(std::shared_ptr<FlatTreeItem> item, const wxString& iconName, const wxSize& size)
{
	if (item) {
		m_itemSvgIconNames[item] = iconName;
		m_itemSvgIconSizes[item] = size;
		Refresh();
	}
}

void FlatTreeView::SetItemColumnSvgIcon(std::shared_ptr<FlatTreeItem> item, int column, const wxString& iconName, const wxSize& size)
{
	if (item && column >= 0 && column < static_cast<int>(m_columns.size())) {
		auto key = std::make_pair(item, column);
		m_itemColumnSvgIconNames[key] = iconName;
		m_itemColumnSvgIconSizes[key] = size;
		Refresh();
	}
}

wxString FlatTreeView::GetColumnSvgIconName(int column) const
{
	auto it = m_columnSvgIconNames.find(column);
	return it != m_columnSvgIconNames.end() ? it->second : wxEmptyString;
}

wxString FlatTreeView::GetItemSvgIconName(std::shared_ptr<FlatTreeItem> item) const
{
	auto it = m_itemSvgIconNames.find(item);
	return it != m_itemSvgIconNames.end() ? it->second : wxEmptyString;
}

wxString FlatTreeView::GetItemColumnSvgIconName(std::shared_ptr<FlatTreeItem> item, int column) const
{
	auto key = std::make_pair(item, column);
	auto it = m_itemColumnSvgIconNames.find(key);
	return it != m_itemColumnSvgIconNames.end() ? it->second : wxEmptyString;
}