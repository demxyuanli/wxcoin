#include "flatui/FlatUIGallery.h"
#include "flatui/FlatUIPanel.h"
#include "flatui/FlatUIEventManager.h"
#include "logger/Logger.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include "config/ThemeManager.h"

FlatUIGallery::FlatUIGallery(FlatUIPanel* parent)
	: FlatUIThemeAware(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
	m_itemStyle(ItemStyle::DEFAULT),
	m_itemBorderStyle(ItemBorderStyle::SOLID),
	m_layoutStyle(LayoutStyle::HORIZONTAL),
	m_itemBgColour(GetThemeColour("GalleryItemBgColour")),
	m_itemHoverBgColour(GetThemeColour("GalleryItemHoverBgColour")),
	m_itemSelectedBgColour(GetThemeColour("GalleryItemSelectedBgColour")),
	m_itemBorderColour(GetThemeColour("GalleryItemBorderColour")),
	m_itemBorderWidth(0),
	m_itemCornerRadius(0),
	m_galleryBorderWidth(0),
	m_selectedItem(-1),
	m_hoveredItem(-1),
	m_hoverEffectsEnabled(true),
	m_selectionEnabled(true),
	m_hasDropdown(false),
	m_dropdownWidth(0)
{
	SetDoubleBuffered(true);
	SetBackgroundStyle(wxBG_STYLE_PAINT);

	m_galleryBgColour = GetThemeColour("ActBarBackgroundColour");
	m_galleryBorderColour = GetThemeColour("ActBarBackgroundColour");
	m_itemSpacing = GetThemeInt("GalleryItemSpacing");
	m_itemPadding = GetThemeInt("GalleryItemPadding");
	int targetH = GetThemeInt("GalleryTargetHeight");
	int horizMargin = GetThemeInt("GalleryHorizontalMargin");
	int galleryVerticalPadding = GetThemeInt("GalleryInternalVerticalPadding");
	SetMinSize(wxSize(targetH * 2, targetH));

	Bind(wxEVT_PAINT, &FlatUIGallery::OnPaint, this);
	Bind(wxEVT_LEFT_DOWN, &FlatUIGallery::OnMouseDown, this);
	Bind(wxEVT_MOTION, &FlatUIGallery::OnMouseMove, this);
	Bind(wxEVT_LEAVE_WINDOW, &FlatUIGallery::OnMouseLeave, this);
	Bind(wxEVT_SIZE, &FlatUIGallery::OnSize, this);

	// Register theme change listener
	auto& themeManager = ThemeManager::getInstance();
	themeManager.addThemeChangeListener(this, [this]() {
		RefreshTheme();
		});
}

FlatUIGallery::~FlatUIGallery()
{
	// Clean up any resources if needed
}

void FlatUIGallery::AddItem(const wxBitmap& bitmap, int id)
{
	Freeze();
	ItemInfo info;
	info.bitmap = bitmap;
	info.id = id;
	info.hovered = false;
	info.selected = false;
	m_items.push_back(info);

	// Best size is now determined by DoGetBestSize.
	// We just need to inform the layout system that our best size might have changed.
	InvalidateBestSize();

	wxWindow* parent = GetParent();
	if (parent) {
		// It's usually better to let the panel manage its own updates if needed.
		// panel->UpdatePanelSize(); might still be relevant if panel's size depends on gallery's exact new width.
		FlatUIPanel* panel = dynamic_cast<FlatUIPanel*>(parent);
		if (panel) {
			panel->UpdatePanelSize(); // This will re-query gallery's best size.
		}
		// Alternatively, or in addition, a simple Layout() on parent might suffice if parent is a sizer window.
		// parent->Layout();
	}

	Refresh(); // Refresh this gallery control
	Thaw();

	// Optional: Log the new best size for debugging
	wxSize bestSize = GetBestSize();
	LOG_INF("Added item to FlatUIGallery, new best size: " +
		std::to_string(bestSize.GetWidth()) + "x" +
		std::to_string(bestSize.GetHeight()), "FlatUIGallery");
}

wxSize FlatUIGallery::DoGetBestSize() const
{
	wxSize size;
	int horizMargin = CFG_INT("GalleryHorizontalMargin");
	int totalWidth = horizMargin;
	bool hasItems = false;
	int itemCount = 0;
	for (const auto& item : m_items) {
		if (item.bitmap.IsOk()) {
			hasItems = true;
			int itemWidth = item.bitmap.GetWidth() + 2 * m_itemPadding;
			if (itemCount > 0) {
				totalWidth += m_itemSpacing;
			}
			totalWidth += itemWidth;
			itemCount++;
		}
	}
	if (hasItems) totalWidth += 3 * horizMargin;
	else totalWidth = GetMinSize().GetWidth() > 0 ? GetMinSize().GetWidth() : 100;

	if (m_hasDropdown) totalWidth += m_dropdownWidth;

	int targetH = CFG_INT("GalleryTargetHeight");
	int totalHeight = targetH + 2 * CFG_INT("GalleryVerticalMargin");
	return wxSize(totalWidth, totalHeight);
}

void FlatUIGallery::RecalculateLayout()
{
	// Layout recalculation will be done in OnPaint for now
	// In future, this could pre-calculate item positions
	Refresh();
}

void FlatUIGallery::OnSize(wxSizeEvent& evt)
{
	RecalculateLayout();
	evt.Skip();
}

void FlatUIGallery::OnPaint(wxPaintEvent& evt)
{
	wxAutoBufferedPaintDC dc(this);
	wxSize size = GetSize();

	dc.SetBackground(m_galleryBgColour);
	dc.Clear();

	// Draw gallery border if configured
	if (m_galleryBorderWidth > 0) {
		dc.SetPen(wxPen(m_galleryBorderColour, m_galleryBorderWidth));
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());
	}

	if (m_items.empty()) {
		// Draw placeholder text
		dc.SetTextForeground(CFG_COLOUR("GalleryTextColour"));
		wxString text = "Gallery";
		wxSize textSize = dc.GetTextExtent(text);
		int x = (size.GetWidth() - textSize.GetWidth()) / 2;
		int y = (size.GetHeight() - textSize.GetHeight()) / 2;
		dc.DrawText(text, x, y);
		return;
	}

	int x = CFG_INT("GalleryHorizontalMargin");
	int y = CFG_INT("GalleryVerticalMargin");

	for (size_t i = 0; i < m_items.size(); ++i) {
		auto& item = m_items[i];
		if (item.bitmap.IsOk()) {
			int itemWidth = item.bitmap.GetWidth() + 2 * m_itemPadding;
			int itemHeight = item.bitmap.GetHeight() + 2 * m_itemPadding;

			// Calculate actual y position for this specific item (top-aligned)
			int item_y = CFG_INT("GalleryVerticalMargin");

			// Update item rect for hit testing
			item.rect = wxRect(x, item_y, itemWidth, itemHeight);

			// Draw item
			DrawItem(dc, item, i);

			x += itemWidth + m_itemSpacing;
		}
	}

	// LOG_DBG("FlatUIGallery OnPaint - Size: " +
	//     std::to_string(size.GetWidth()) + "x" +
	//     std::to_string(size.GetHeight()), "FlatUIGallery");
}

void FlatUIGallery::DrawItem(wxDC& dc, const ItemInfo& item, int index)
{
	wxRect itemRect = item.rect;
	bool isHovered = (m_hoverEffectsEnabled && index == m_hoveredItem);
	bool isSelected = (m_selectionEnabled && index == m_selectedItem);

	// Draw item background
	if (m_itemStyle != ItemStyle::DEFAULT || isHovered || isSelected) {
		DrawItemBackground(dc, itemRect, isHovered, isSelected);
	}

	// Draw item border
	if (m_itemStyle == ItemStyle::BORDERED ||
		m_itemStyle == ItemStyle::ROUNDED ||
		(m_itemStyle == ItemStyle::DEFAULT && (isHovered || isSelected))) {
		DrawItemBorder(dc, itemRect, isHovered, isSelected);
	}

	// Draw shadow for SHADOWED style
	if (m_itemStyle == ItemStyle::SHADOWED) {
		wxColour shadowColour = m_galleryBgColour.ChangeLightness(85);
		dc.SetPen(wxPen(shadowColour, 1));
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.DrawLine(itemRect.GetLeft() + 2, itemRect.GetBottom() + 1,
			itemRect.GetRight() + 1, itemRect.GetBottom() + 1);
		dc.DrawLine(itemRect.GetRight() + 1, itemRect.GetTop() + 2,
			itemRect.GetRight() + 1, itemRect.GetBottom() + 1);
	}

	// Draw the bitmap
	if (item.bitmap.IsOk()) {
		int bitmapX = itemRect.GetLeft() + m_itemPadding;
		int bitmapY = itemRect.GetTop() + m_itemPadding;
		dc.DrawBitmap(item.bitmap, bitmapX, bitmapY, true);
	}
}

void FlatUIGallery::DrawItemBackground(wxDC& dc, const wxRect& rect, bool isHovered, bool isSelected)
{
	wxColour bgColour = m_itemBgColour;
	if (isSelected) {
		bgColour = m_itemSelectedBgColour;
	}
	else if (isHovered) {
		bgColour = m_itemHoverBgColour;
	}

	dc.SetBrush(wxBrush(bgColour));
	dc.SetPen(*wxTRANSPARENT_PEN);

	if (m_itemStyle == ItemStyle::ROUNDED || m_itemCornerRadius > 0) {
		dc.DrawRoundedRectangle(rect, m_itemCornerRadius);
	}
	else {
		dc.DrawRectangle(rect);
	}
}

void FlatUIGallery::DrawItemBorder(wxDC& dc, const wxRect& rect, bool isHovered, bool isSelected)
{
	wxColour borderColour = m_itemBorderColour;
	if ((isHovered || isSelected) && m_hoverEffectsEnabled) {
		borderColour = borderColour.ChangeLightness(80);
	}

	switch (m_itemBorderStyle) {
	case ItemBorderStyle::SOLID:
		dc.SetPen(wxPen(borderColour, m_itemBorderWidth));
		break;
	case ItemBorderStyle::DASHED:
		dc.SetPen(wxPen(borderColour, m_itemBorderWidth, wxPENSTYLE_SHORT_DASH));
		break;
	case ItemBorderStyle::DOTTED:
		dc.SetPen(wxPen(borderColour, m_itemBorderWidth, wxPENSTYLE_DOT));
		break;
	case ItemBorderStyle::DOUBLE:
	{
		// Draw double border
		dc.SetPen(wxPen(borderColour, 1));
		dc.DrawRectangle(rect);
		wxRect innerRect = rect;
		innerRect.Deflate(2);
		dc.DrawRectangle(innerRect);
		return;
	}
	case ItemBorderStyle::ROUNDED:
		dc.SetPen(wxPen(borderColour, m_itemBorderWidth));
		break;
	}

	dc.SetBrush(*wxTRANSPARENT_BRUSH);

	if (m_itemStyle == ItemStyle::ROUNDED ||
		m_itemBorderStyle == ItemBorderStyle::ROUNDED ||
		m_itemCornerRadius > 0) {
		dc.DrawRoundedRectangle(rect, m_itemCornerRadius);
	}
	else {
		dc.DrawRectangle(rect);
	}
}

void FlatUIGallery::OnMouseDown(wxMouseEvent& evt)
{
	wxPoint pos = evt.GetPosition();
	LOG_INF("Mouse down event in FlatUIGallery at position: (" +
		std::to_string(pos.x) + ", " + std::to_string(pos.y) + ")", "FlatUIGallery");

	for (size_t i = 0; i < m_items.size(); ++i) {
		if (m_items[i].rect.Contains(pos)) {
			LOG_INF("Clicked on item with ID: " + std::to_string(m_items[i].id), "FlatUIGallery");

			// Handle selection
			if (m_selectionEnabled) {
				SetSelectedItem(i);
			}

			// Send event
			wxCommandEvent event(wxEVT_BUTTON, m_items[i].id);
			event.SetEventObject(this);
			ProcessWindowEvent(event);
			break;
		}
	}
	evt.Skip();
}

void FlatUIGallery::OnMouseMove(wxMouseEvent& evt)
{
	if (!m_hoverEffectsEnabled) {
		evt.Skip();
		return;
	}

	wxPoint pos = evt.GetPosition();
	int oldHoveredItem = m_hoveredItem;
	m_hoveredItem = -1;

	for (size_t i = 0; i < m_items.size(); ++i) {
		if (m_items[i].rect.Contains(pos)) {
			m_hoveredItem = i;
			break;
		}
	}

	if (oldHoveredItem != m_hoveredItem) {
		Refresh();
	}

	evt.Skip();
}

void FlatUIGallery::OnMouseLeave(wxMouseEvent& evt)
{
	if (m_hoveredItem != -1) {
		m_hoveredItem = -1;
		Refresh();
	}
	evt.Skip();
}

// Style configuration method implementations
void FlatUIGallery::SetItemStyle(ItemStyle style)
{
	if (m_itemStyle != style) {
		m_itemStyle = style;
		Refresh();
	}
}

void FlatUIGallery::SetItemBorderStyle(ItemBorderStyle style)
{
	if (m_itemBorderStyle != style) {
		m_itemBorderStyle = style;
		Refresh();
	}
}

void FlatUIGallery::SetLayoutStyle(LayoutStyle style)
{
	if (m_layoutStyle != style) {
		m_layoutStyle = style;
		RecalculateLayout();
	}
}

void FlatUIGallery::SetItemBackgroundColour(const wxColour& colour)
{
	m_itemBgColour = colour;
	Refresh();
}

void FlatUIGallery::SetItemHoverBackgroundColour(const wxColour& colour)
{
	m_itemHoverBgColour = colour;
	Refresh();
}

void FlatUIGallery::SetItemSelectedBackgroundColour(const wxColour& colour)
{
	m_itemSelectedBgColour = colour;
	Refresh();
}

void FlatUIGallery::SetItemBorderColour(const wxColour& colour)
{
	m_itemBorderColour = colour;
	Refresh();
}

void FlatUIGallery::SetItemBorderWidth(int width)
{
	m_itemBorderWidth = width;
	Refresh();
}

void FlatUIGallery::SetItemCornerRadius(int radius)
{
	m_itemCornerRadius = radius;
	Refresh();
}

void FlatUIGallery::SetItemPadding(int padding)
{
	if (m_itemPadding != padding) {
		m_itemPadding = padding;
		InvalidateBestSize();
		RecalculateLayout();
	}
}

void FlatUIGallery::SetGalleryBackgroundColour(const wxColour& colour)
{
	m_galleryBgColour = colour;
	Refresh();
}

void FlatUIGallery::SetGalleryBorderColour(const wxColour& colour)
{
	m_galleryBorderColour = colour;
	Refresh();
}

void FlatUIGallery::SetGalleryBorderWidth(int width)
{
	m_galleryBorderWidth = width;
	Refresh();
}

void FlatUIGallery::SetSelectedItem(int index)
{
	if (index < 0 || index >= static_cast<int>(m_items.size())) {
		index = -1;
	}

	if (m_selectedItem != index) {
		// Clear old selection
		if (m_selectedItem >= 0 && m_selectedItem < static_cast<int>(m_items.size())) {
			m_items[m_selectedItem].selected = false;
		}

		// Set new selection
		m_selectedItem = index;
		if (m_selectedItem >= 0) {
			m_items[m_selectedItem].selected = true;
		}

		Refresh();
	}
}

void FlatUIGallery::SetHoverEffectsEnabled(bool enabled)
{
	if (m_hoverEffectsEnabled != enabled) {
		m_hoverEffectsEnabled = enabled;
		m_hoveredItem = -1;
		Refresh();
	}
}

void FlatUIGallery::SetSelectionEnabled(bool enabled)
{
	if (m_selectionEnabled != enabled) {
		m_selectionEnabled = enabled;
		if (!enabled) {
			m_selectedItem = -1;
			for (auto& item : m_items) {
				item.selected = false;
			}
		}
		Refresh();
	}
}

void FlatUIGallery::OnThemeChanged()
{
	// Use batch update to avoid multiple refreshes
	BatchUpdateTheme();
}

void FlatUIGallery::UpdateThemeValues()
{
	// Update all theme-based colors and settings
	m_itemBgColour = GetThemeColour("GalleryItemBgColour");
	m_itemHoverBgColour = GetThemeColour("GalleryItemHoverBgColour");
	m_itemSelectedBgColour = GetThemeColour("GalleryItemSelectedBgColour");
	m_itemBorderColour = GetThemeColour("GalleryItemBorderColour");
	m_galleryBgColour = GetThemeColour("ActBarBackgroundColour");
	m_galleryBorderColour = GetThemeColour("ActBarBackgroundColour");

	m_itemSpacing = GetThemeInt("GalleryItemSpacing");
	m_itemPadding = GetThemeInt("GalleryItemPadding");
	int targetH = GetThemeInt("GalleryTargetHeight");
	int horizMargin = GetThemeInt("GalleryHorizontalMargin");
	int galleryVerticalPadding = GetThemeInt("GalleryInternalVerticalPadding");

	// Update control properties
	SetFont(GetThemeFont());
	SetBackgroundColour(m_galleryBgColour);
	SetMinSize(wxSize(targetH * 2, targetH));

	// Note: Don't call Refresh() here - it will be handled by the parent frame
}

void FlatUIGallery::RefreshTheme() {
	// Update all theme-based colors and settings
	m_itemBgColour = GetThemeColour("GalleryItemBgColour");
	m_itemHoverBgColour = GetThemeColour("GalleryItemHoverBgColour");
	m_itemSelectedBgColour = GetThemeColour("GalleryItemSelectedBgColour");
	m_itemBorderColour = GetThemeColour("GalleryItemBorderColour");
	m_galleryBgColour = GetThemeColour("ActBarBackgroundColour");
	m_galleryBorderColour = GetThemeColour("ActBarBackgroundColour");

	// Update theme-based integer values
	m_itemSpacing = GetThemeInt("GalleryItemSpacing");
	m_itemPadding = GetThemeInt("GalleryItemPadding");
	m_itemBorderWidth = GetThemeInt("GalleryItemBorderWidth");
	m_itemCornerRadius = GetThemeInt("GalleryItemCornerRadius");
	m_galleryBorderWidth = GetThemeInt("GalleryBorderWidth");

	// Update control properties
	SetFont(GetThemeFont());
	SetBackgroundColour(m_galleryBgColour);

	// Note: Refresh is handled by parent frame for performance
}