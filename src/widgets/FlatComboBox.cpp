#include "widgets/FlatComboBox.h"
#include <wx/combobox.h>
#include "config/ThemeManager.h"
#include "config/SvgIconManager.h"

wxDEFINE_EVENT(wxEVT_FLAT_COMBO_BOX_SELECTION_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_COMBO_BOX_DROPDOWN_OPENED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_COMBO_BOX_DROPDOWN_CLOSED, wxCommandEvent);

BEGIN_EVENT_TABLE(FlatComboBox, wxControl)
EVT_PAINT(FlatComboBox::OnPaint)
EVT_SIZE(FlatComboBox::OnSize)
EVT_LEFT_DOWN(FlatComboBox::OnMouseDown)
EVT_LEFT_UP(FlatComboBox::OnMouseUp)
EVT_MOTION(FlatComboBox::OnMouseMove)
EVT_LEAVE_WINDOW(FlatComboBox::OnMouseLeave)
EVT_ENTER_WINDOW(FlatComboBox::OnMouseEnter)
EVT_SET_FOCUS(FlatComboBox::OnFocus)
EVT_KILL_FOCUS(FlatComboBox::OnKillFocus)
EVT_KEY_DOWN(FlatComboBox::OnKeyDown)
EVT_CHAR(FlatComboBox::OnChar)
END_EVENT_TABLE()

FlatComboBox::FlatComboBox(wxWindow* parent, wxWindowID id, const wxString& value,
	const wxPoint& pos, const wxSize& size, ComboBoxStyle style, long style_flags)
	: wxControl(parent, id, pos, size, style_flags | wxBORDER_NONE)
	, m_comboBoxStyle(style)
	, m_state(ComboBoxState::DEFAULT_STATE)
	, m_enabled(true)
	, m_editable(false)
	, m_borderWidth(DEFAULT_BORDER_WIDTH)
	, m_cornerRadius(DEFAULT_CORNER_RADIUS)
	, m_padding(DEFAULT_PADDING)
	, m_dropdownButtonWidth(DEFAULT_DROPDOWN_BUTTON_WIDTH)
	, m_maxVisibleItems(DEFAULT_MAX_VISIBLE_ITEMS)
	, m_dropdownWidth(DEFAULT_DROPDOWN_WIDTH)
	, m_isFocused(false)
	, m_isHovered(false)
	, m_isPressed(false)
	, m_dropdownShown(false)
	, m_dropdownButtonHovered(false)
	, m_popup(nullptr)
{
	// Initialize default colors based on style
	InitializeDefaultColors();

	// Set default size if not specified
	if (size == wxDefaultSize) {
		SetInitialSize(DoGetBestSize());
	}

	// Theme change listener
	ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
		InitializeDefaultColors();
		Refresh();
		});
}

FlatComboBox::~FlatComboBox()
{
	if (m_popup) {
		// Clear parent pointer in popup to prevent invalid access
		static_cast<FlatComboBoxPopup*>(m_popup)->ClearParent();
		m_popup->Destroy();
		m_popup = nullptr;
	}
	ThemeManager::getInstance().removeThemeChangeListener(this);
}

void FlatComboBox::InitializeDefaultColors()
{
	switch (m_comboBoxStyle) {
	case ComboBoxStyle::DEFAULT_STYLE:
	case ComboBoxStyle::EDITABLE:
		m_backgroundColor = CFG_COLOUR("SecondaryBackgroundColour");
		m_focusedColor = CFG_COLOUR("SecondaryBackgroundColour");
		m_textColor = CFG_COLOUR("PrimaryTextColour");
		m_borderColor = CFG_COLOUR("ButtonBorderColour");
		m_dropdownBackgroundColor = CFG_COLOUR("PanelBgColour");
		m_dropdownBorderColor = CFG_COLOUR("PanelBorderColour");
		break;
	case ComboBoxStyle::SEARCH:
		m_backgroundColor = CFG_COLOUR("SecondaryBackgroundColour");
		m_focusedColor = CFG_COLOUR("SecondaryBackgroundColour");
		m_textColor = CFG_COLOUR("PrimaryTextColour");
		m_borderColor = CFG_COLOUR("ButtonBorderColour");
		m_dropdownBackgroundColor = CFG_COLOUR("PanelBgColour");
		m_dropdownBorderColor = CFG_COLOUR("PanelBorderColour");
		break;
	}
}

void FlatComboBox::SetComboBoxStyle(ComboBoxStyle style)
{
	m_comboBoxStyle = style;
	InitializeDefaultColors();
	this->Refresh();
}

void FlatComboBox::SetBackgroundColor(const wxColour& color)
{
	m_backgroundColor = color;
	this->Refresh();
}

void FlatComboBox::SetTextColor(const wxColour& color)
{
	m_textColor = color;
	this->Refresh();
}

void FlatComboBox::SetBorderColor(const wxColour& color)
{
	m_borderColor = color;
	this->Refresh();
}

void FlatComboBox::SetBorderWidth(int width)
{
	m_borderWidth = width;
	this->Refresh();
}

void FlatComboBox::SetCornerRadius(int radius)
{
	m_cornerRadius = radius;
	this->Refresh();
}

void FlatComboBox::SetPadding(int horizontal, int vertical)
{
	m_padding = horizontal; // Use single padding value
	this->Refresh();
}

void FlatComboBox::GetPadding(int& horizontal, int& vertical) const
{
	horizontal = m_padding;
	vertical = m_padding;
}

void FlatComboBox::SetEnabled(bool enabled)
{
	m_enabled = enabled;
	wxControl::Enable(enabled);
	this->Refresh();
}

// Missing function implementations
wxSize FlatComboBox::DoGetBestSize() const
{
	wxSize textSize = GetTextExtent(m_value.IsEmpty() ? "Sample Text" : m_value);
	int width = textSize.GetWidth() + 2 * m_padding + 2 * m_borderWidth + m_dropdownButtonWidth;
	int height = textSize.GetHeight() + 2 * m_padding + 2 * m_borderWidth;

	return wxSize(width, height);
}

void FlatComboBox::Append(const wxString& item, const wxBitmap& icon, const wxVariant& data)
{
	ComboBoxItem newItem(ItemType::NORMAL, item, icon, data);
	m_items.push_back(newItem);
	this->Refresh();
}

void FlatComboBox::Append(const ComboBoxItem& item)
{
	m_items.push_back(item);
	this->Refresh();
}

void FlatComboBox::Insert(const ComboBoxItem& item, unsigned int pos)
{
	if (pos > m_items.size()) {
		pos = m_items.size();
	}
	m_items.insert(m_items.begin() + pos, item);
	this->Refresh();
}

void FlatComboBox::AppendColorPicker(const wxString& text, const wxColour& color, const wxBitmap& icon)
{
	Append(ComboBoxItem::ColorPicker(text, color, icon));
}

void FlatComboBox::AppendCheckbox(const wxString& text, bool checked, const wxBitmap& icon)
{
	Append(ComboBoxItem::Checkbox(text, checked, icon));
}

void FlatComboBox::AppendRadioButton(const wxString& text, const wxString& group, bool checked, const wxBitmap& icon)
{
	Append(ComboBoxItem::RadioButton(text, group, checked, icon));
}

void FlatComboBox::AppendSeparator()
{
	Append(ComboBoxItem::Separator());
}

bool FlatComboBox::IsItemChecked(unsigned int n) const
{
	if (n < m_items.size()) {
		return m_items[n].checked;
	}
	return false;
}

void FlatComboBox::SetItemChecked(unsigned int n, bool checked)
{
	if (n < m_items.size()) {
		// For radio buttons, ensure only one in the group is checked
		if (checked && m_items[n].type == ItemType::RADIO_BUTTON) {
			const wxString& group = m_items[n].group;
			for (size_t i = 0; i < m_items.size(); ++i) {
				if (m_items[i].type == ItemType::RADIO_BUTTON && m_items[i].group == group) {
					m_items[i].checked = (i == n);
				}
			}
		} else {
			m_items[n].checked = checked;
		}
		this->Refresh();
	}
}

wxColour FlatComboBox::GetItemColor(unsigned int n) const
{
	if (n < m_items.size()) {
		return m_items[n].color;
	}
	return wxTransparentColour;
}

void FlatComboBox::SetItemColor(unsigned int n, const wxColour& color)
{
	if (n < m_items.size()) {
		m_items[n].color = color;
		this->Refresh();
	}
}

FlatComboBox::ItemType FlatComboBox::GetItemType(unsigned int n) const
{
	if (n < m_items.size()) {
		return m_items[n].type;
	}
	return ItemType::NORMAL;
}

unsigned int FlatComboBox::GetCount() const
{
	return m_items.size();
}

const std::vector<FlatComboBox::ComboBoxItem>& FlatComboBox::GetItems() const
{
	return m_items;
}

wxString FlatComboBox::GetString(unsigned int n) const
{
	if (n < m_items.size()) {
		return m_items[n].text;
	}
	return wxEmptyString;
}

void FlatComboBox::SetString(unsigned int n, const wxString& s)
{
	if (n < m_items.size()) {
		m_items[n].text = s;
		if (m_selection == static_cast<int>(n)) {
			m_value = s;
		}
		this->Refresh();
	}
}

void FlatComboBox::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxRect rect = this->GetClientRect();

	// Draw background
	DrawBackground(dc);

	// Draw border
	DrawBorder(dc);

	// Draw text
	DrawText(dc);

	// Draw dropdown button
	DrawDropdownButton(dc);
}

void FlatComboBox::OnSize(wxSizeEvent& event)
{
	this->Refresh();
	event.Skip();
}

void FlatComboBox::OnMouseDown(wxMouseEvent& event)
{
	if (!m_enabled) {
		event.Skip();
		return;
	}

	m_isPressed = true;
	this->SetFocus();
	CaptureMouse();

	// Click anywhere in the combobox should open the dropdown
	HandleDropdownClick();

	this->Refresh();
	event.Skip();
}

void FlatComboBox::OnMouseUp(wxMouseEvent& event)
{
	if (!m_enabled) {
		event.Skip();
		return;
	}

	if (HasCapture()) {
		ReleaseMouse();
	}

	m_isPressed = false;
	this->Refresh();
	event.Skip();
}

void FlatComboBox::OnMouseMove(wxMouseEvent& event)
{
	if (!m_enabled) {
		event.Skip();
		return;
	}

	bool wasHovered = m_isHovered;
	m_isHovered = true;

	// Check if dropdown button is hovered
	wxPoint pos = event.GetPosition();
	bool wasButtonHovered = m_dropdownButtonHovered;
	m_dropdownButtonHovered = GetDropdownButtonRect().Contains(pos);

	if (!wasHovered || wasButtonHovered != m_dropdownButtonHovered) {
		this->Refresh();
	}

	event.Skip();
}

void FlatComboBox::OnMouseLeave(wxMouseEvent& event)
{
	if (!m_enabled) {
		event.Skip();
		return;
	}

	m_isHovered = false;
	m_dropdownButtonHovered = false;
	this->Refresh();
	event.Skip();
}

void FlatComboBox::OnMouseEnter(wxMouseEvent& event)
{
	if (!m_enabled) {
		event.Skip();
		return;
	}

	m_isHovered = true;
	this->Refresh();
	event.Skip();
}

void FlatComboBox::OnFocus(wxFocusEvent& event)
{
	if (!m_enabled) {
		event.Skip();
		return;
	}

	m_isFocused = true;
	UpdateState(ComboBoxState::FOCUSED);
	this->Refresh();
	event.Skip();
}

void FlatComboBox::OnKillFocus(wxFocusEvent& event)
{
	if (!m_enabled) {
		event.Skip();
		return;
	}

	m_isFocused = false;
	m_isPressed = false;
	UpdateState(ComboBoxState::DEFAULT_STATE);
	this->Refresh();
	event.Skip();
}

void FlatComboBox::OnKeyDown(wxKeyEvent& event)
{
	if (!m_enabled) {
		event.Skip();
		return;
	}

	switch (event.GetKeyCode()) {
	case WXK_TAB:
		event.Skip();
		break;
	case WXK_RETURN:
	case WXK_SPACE:
		HandleDropdownClick();
		break;
	default:
		event.Skip();
		break;
	}
}

void FlatComboBox::OnChar(wxKeyEvent& event)
{
	if (!m_enabled || !m_editable) {
		event.Skip();
		return;
	}

	// Handle character input for editable combo box
	int keyCode = event.GetKeyCode();
	if (keyCode >= 32 && keyCode < 127) {
		wxString newValue = m_value + wxString((wxChar)keyCode);
		SetValue(newValue);
	}

	event.Skip();
}

// Helper functions
void FlatComboBox::DrawBackground(wxDC& dc)
{
	wxRect rect = this->GetClientRect();
	wxColour bgColor = GetCurrentBackgroundColor();

	dc.SetBrush(wxBrush(bgColor));
	dc.SetPen(wxPen(bgColor));
	DrawRoundedRectangle(dc, rect, m_cornerRadius);
}

void FlatComboBox::DrawBorder(wxDC& dc)
{
	wxRect rect = this->GetClientRect();
	wxColour borderColor = GetCurrentBorderColor();

	dc.SetBrush(wxBrush(*wxTRANSPARENT_BRUSH));
	dc.SetPen(wxPen(borderColor, m_borderWidth));
	DrawRoundedRectangle(dc, rect, m_cornerRadius);
}

void FlatComboBox::DrawText(wxDC& dc)
{
	wxRect textRect = GetTextRect();
	if (textRect.IsEmpty()) return;

	dc.SetTextForeground(GetCurrentTextColor());
	dc.SetFont(this->GetFont());

	wxString displayText = m_value;
	if (displayText.IsEmpty() && m_selection >= 0 && m_selection < (int)m_items.size()) {
		displayText = m_items[m_selection].text;
	}

	dc.DrawText(displayText, textRect.x, textRect.y);
}

void FlatComboBox::DrawDropdownButton(wxDC& dc)
{
	wxRect buttonRect = GetDropdownButtonRect();
	if (buttonRect.IsEmpty()) return;

	// Try to draw SVG dropdown icon
	wxBitmap dropdownIcon = SVG_ICON("chevron-down", wxSize(12, 12));

	// Draw SVG icon if available, otherwise fallback to drawing arrow manually
	if (dropdownIcon.IsOk()) {
		int iconX = buttonRect.x + (buttonRect.width - dropdownIcon.GetWidth()) / 2;
		int iconY = buttonRect.y + (buttonRect.height - dropdownIcon.GetHeight()) / 2;
		dc.DrawBitmap(dropdownIcon, iconX, iconY, true);
	} else {
		// Fallback to drawing arrow manually if SVG icon is not available
		dc.SetPen(wxPen(GetCurrentTextColor(), 1));

		int centerX = buttonRect.x + buttonRect.width / 2;
		int centerY = buttonRect.y + buttonRect.height / 2;
		int arrowSize = 4;

		wxPoint points[3];
		points[0] = wxPoint(centerX - arrowSize, centerY - arrowSize);
		points[1] = wxPoint(centerX + arrowSize, centerY - arrowSize);
		points[2] = wxPoint(centerX, centerY + arrowSize);

		dc.DrawPolygon(3, points);
	}
}

void FlatComboBox::DrawRoundedRectangle(wxDC& dc, const wxRect& rect, int radius)
{
	dc.DrawRoundedRectangle(rect, radius);
}

void FlatComboBox::UpdateState(ComboBoxState newState)
{
	m_state = newState;
}

void FlatComboBox::HandleDropdownClick()
{
	if (m_dropdownShown) {
		HideDropdown();
	}
	else {
		ShowDropdown();
	}
}

wxRect FlatComboBox::GetTextRect() const
{
	wxRect clientRect = this->GetClientRect();
	return wxRect(
		clientRect.x + m_padding + m_borderWidth,
		clientRect.y + m_padding + m_borderWidth,
		clientRect.width - 2 * (m_padding + m_borderWidth) - m_dropdownButtonWidth,
		clientRect.height - 2 * (m_padding + m_borderWidth)
	);
}

wxRect FlatComboBox::GetDropdownButtonRect() const
{
	wxRect clientRect = this->GetClientRect();
	return wxRect(
		clientRect.x + clientRect.width - m_dropdownButtonWidth - m_borderWidth,
		clientRect.y + m_borderWidth,
		m_dropdownButtonWidth,
		clientRect.height - 2 * m_borderWidth
	);
}

wxColour FlatComboBox::GetCurrentBackgroundColor() const
{
	if (!m_enabled) return wxColour(240, 240, 240);
	if (m_isFocused) return m_focusedColor;
	if (m_isHovered) return wxColour(250, 250, 250);
	return m_backgroundColor;
}

wxColour FlatComboBox::GetCurrentBorderColor() const
{
	if (!m_enabled) return wxColour(200, 200, 200);
	if (m_isFocused) return wxColour(0, 120, 215);
	return m_borderColor;
}

wxColour FlatComboBox::GetCurrentTextColor() const
{
	if (!m_enabled) return wxColour(128, 128, 128);
	return m_textColor;
}

void FlatComboBox::ShowDropdown()
{
	if (!m_dropdownShown && !m_items.empty()) {
		m_dropdownShown = true;
		UpdateState(ComboBoxState::DROPDOWN_OPEN);
		this->Refresh();

		// Create popup if not exists
		if (!m_popup) {
			m_popup = new FlatComboBoxPopup(this);
			m_popup->SetBackgroundColor(m_dropdownBackgroundColor);
			m_popup->SetBorderColor(m_dropdownBorderColor);
			m_popup->SetTextColor(m_textColor);
			m_popup->SetHoverColor(m_dropdownHoverColor);
		}

		// Set popup items and selection
		m_popup->SetItems(m_items);
		m_popup->SetSelection(m_selection);

		// Position and show popup
		wxPoint pos = this->GetScreenPosition();
		wxSize size = this->GetSize();
		wxSize popupSize = m_popup->GetBestSize();

		pos.y += size.y;
		popupSize.x = std::max(popupSize.x, size.x); // Match combobox width

		m_popup->SetSize(popupSize);
		m_popup->Move(pos);
		m_popup->Show();

		// Send dropdown opened event
		wxCommandEvent event(wxEVT_FLAT_COMBO_BOX_DROPDOWN_OPENED, this->GetId());
		this->ProcessWindowEvent(event);
	}
}

void FlatComboBox::HideDropdown()
{
	if (m_dropdownShown) {
		m_dropdownShown = false;
		UpdateState(ComboBoxState::DEFAULT_STATE);
		this->Refresh();

		// Hide popup
		if (m_popup) {
			m_popup->Hide();
		}

		// Send dropdown closed event
		wxCommandEvent event(wxEVT_FLAT_COMBO_BOX_DROPDOWN_CLOSED, this->GetId());
		this->ProcessWindowEvent(event);
	}
}

int FlatComboBox::GetSelection() const
{
	return m_selection;
}

void FlatComboBox::SetSelection(int n)
{
	if (n >= -1 && n < static_cast<int>(m_items.size())) {
		m_selection = n;
		if (n >= 0) {
			m_value = m_items[n].text;
		}
		else {
			m_value = wxEmptyString;
		}
		this->Refresh();

		// Send selection changed event
		wxCommandEvent event(wxEVT_FLAT_COMBO_BOX_SELECTION_CHANGED, this->GetId());
		event.SetString(m_value);
		this->ProcessWindowEvent(event);
	}
}

wxString FlatComboBox::GetValue() const
{
	return m_value;
}

void FlatComboBox::SetValue(const wxString& value)
{
	if (m_value != value) {
		m_value = value;
		// Try auto-select matching item
		for (size_t i = 0; i < m_items.size(); ++i) {
			if (m_items[i].text == value) {
				m_selection = static_cast<int>(i);
				break;
			}
		}
		this->Refresh();
	}
}

void FlatComboBox::OnSelectionChanged(wxCommandEvent& event)
{
	// Send custom event
	wxCommandEvent customEvent(wxEVT_FLAT_COMBO_BOX_SELECTION_CHANGED, this->GetId());
	customEvent.SetString(GetValue());
	this->ProcessWindowEvent(customEvent);

	// Call base class handler
	event.Skip();
}

void FlatComboBox::OnDropdownOpened(wxCommandEvent& event)
{
	// Send custom event
	wxCommandEvent customEvent(wxEVT_FLAT_COMBO_BOX_DROPDOWN_OPENED, this->GetId());
	this->ProcessWindowEvent(customEvent);

	event.Skip();
}

void FlatComboBox::OnDropdownClosed(wxCommandEvent& event)
{
	// Send custom event
	wxCommandEvent customEvent(wxEVT_FLAT_COMBO_BOX_DROPDOWN_CLOSED, this->GetId());
	this->ProcessWindowEvent(customEvent);

	event.Skip();
}

// ============================================================================
// FlatComboBoxPopup implementation
// ============================================================================

BEGIN_EVENT_TABLE(FlatComboBoxPopup, wxPopupTransientWindow)
EVT_PAINT(FlatComboBoxPopup::OnPaint)
EVT_LEFT_DOWN(FlatComboBoxPopup::OnMouseDown)
EVT_MOTION(FlatComboBoxPopup::OnMouseMove)
EVT_KEY_DOWN(FlatComboBoxPopup::OnKeyDown)
END_EVENT_TABLE()

FlatComboBoxPopup::FlatComboBoxPopup(FlatComboBox* parent)
	: wxPopupTransientWindow(parent, wxBORDER_NONE)
	, m_parent(parent)
	, m_selection(-1)
	, m_hoverItem(-1)
	, m_itemHeight(24)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetDoubleBuffered(true);
}

FlatComboBoxPopup::~FlatComboBoxPopup()
{
}

void FlatComboBoxPopup::SetItems(const std::vector<FlatComboBox::ComboBoxItem>& items)
{
	m_items = items;
	Refresh();
}

void FlatComboBoxPopup::SetSelection(int selection)
{
	m_selection = selection;
	Refresh();
}

int FlatComboBoxPopup::GetSelection() const
{
	return m_selection;
}

void FlatComboBoxPopup::SetBackgroundColor(const wxColour& color)
{
	m_backgroundColor = color;
	Refresh();
}

void FlatComboBoxPopup::SetBorderColor(const wxColour& color)
{
	m_borderColor = color;
	Refresh();
}

void FlatComboBoxPopup::SetTextColor(const wxColour& color)
{
	m_textColor = color;
	Refresh();
}

void FlatComboBoxPopup::SetHoverColor(const wxColour& color)
{
	m_hoverColor = color;
	Refresh();
}

wxSize FlatComboBoxPopup::GetBestSize() const
{
	if (m_items.empty()) {
		return wxSize(200, 50);
	}

	int maxWidth = 200; // Minimum width
	int totalHeight = 2; // Border

	// Calculate item heights and max width
	for (const auto& item : m_items) {
		wxSize textSize = GetTextExtent(item.text);
		maxWidth = std::max(maxWidth, textSize.GetWidth() + 16); // padding
		totalHeight += m_itemHeight;
	}

	return wxSize(maxWidth, totalHeight);
}

void FlatComboBoxPopup::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	wxRect rect = GetClientRect();

	// Draw background
	dc.SetBrush(wxBrush(m_backgroundColor));
	dc.SetPen(wxPen(m_borderColor));
	dc.DrawRectangle(rect);

	// Draw items
	DrawItems(dc);
}

void FlatComboBoxPopup::OnMouseDown(wxMouseEvent& event)
{
	wxPoint pos = event.GetPosition();
	int item = HitTest(pos);

	if (item >= 0 && item < static_cast<int>(m_items.size())) {
		const auto& clickedItem = m_items[item];

		// Skip separator items
		if (clickedItem.type == FlatComboBox::ItemType::SEPARATOR) {
			return;
		}

		// Handle different item types
		switch (clickedItem.type) {
		case FlatComboBox::ItemType::CHECKBOX:
			// Toggle checkbox state
			if (m_parent) {
				m_parent->SetItemChecked(item, !clickedItem.checked);
				// Update popup items to reflect the change
				m_items = m_parent->GetItems();
			}
			break;

		case FlatComboBox::ItemType::RADIO_BUTTON:
			// Select radio button (this will automatically uncheck others in the same group)
			if (m_parent) {
				m_parent->SetItemChecked(item, true);
				// Update popup items to reflect the change
				m_items = m_parent->GetItems();
			}
			m_selection = item;
			break;

		case FlatComboBox::ItemType::COLOR_PICKER:
		case FlatComboBox::ItemType::NORMAL:
		default:
			// Regular selection - hide popup immediately
			m_selection = item;
			if (m_parent) {
				m_parent->SetSelection(item);
			}
			Refresh();
			Hide();
			return;
		}

		// For checkboxes and radio buttons, don't hide popup immediately
		// Allow multiple selections or just show the state change
		Refresh();
	}
}

void FlatComboBoxPopup::OnMouseMove(wxMouseEvent& event)
{
	wxPoint pos = event.GetPosition();
	int item = HitTest(pos);
	SetHoverItem(item);
}

void FlatComboBoxPopup::OnKeyDown(wxKeyEvent& event)
{
	switch (event.GetKeyCode()) {
	case WXK_ESCAPE:
		Hide();
		break;
	case WXK_RETURN:
	case WXK_SPACE:
		if (m_selection >= 0 && m_selection < static_cast<int>(m_items.size()) && m_parent) {
			wxCommandEvent evt(wxEVT_FLAT_COMBO_BOX_SELECTION_CHANGED, m_parent->GetId());
			evt.SetString(m_items[m_selection].text);
			m_parent->ProcessWindowEvent(evt);
			Hide();
		}
		break;
	case WXK_UP:
		if (m_selection > 0) {
			m_selection--;
			SetHoverItem(m_selection);
		}
		break;
	case WXK_DOWN:
		if (m_selection < static_cast<int>(m_items.size()) - 1) {
			m_selection++;
			SetHoverItem(m_selection);
		}
		break;
	default:
		event.Skip();
		break;
	}
}


void FlatComboBoxPopup::DrawItems(wxDC& dc)
{
	dc.SetFont(m_parent ? m_parent->GetFont() : wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
	dc.SetTextForeground(m_textColor);

	int y = 1; // Start after border

	for (size_t i = 0; i < m_items.size(); ++i) {
		const auto& item = m_items[i];
		wxRect itemRect(1, y, GetClientSize().GetWidth() - 2, m_itemHeight);

		// Skip drawing for separator items
		if (item.type == FlatComboBox::ItemType::SEPARATOR) {
			// Draw separator line
			dc.SetPen(wxPen(wxColour(200, 200, 200), 1));
			dc.DrawLine(itemRect.x + 4, itemRect.y + itemRect.height / 2,
					  itemRect.x + itemRect.width - 4, itemRect.y + itemRect.height / 2);
			y += m_itemHeight;
			continue;
		}

		// Draw selection/hover background
		if (static_cast<int>(i) == m_selection || static_cast<int>(i) == m_hoverItem) {
			dc.SetBrush(wxBrush(m_hoverColor));
			dc.SetPen(wxPen(m_hoverColor));
			dc.DrawRectangle(itemRect);
		}

		// Draw item content based on type
		wxRect contentRect = itemRect;
		contentRect.Deflate(8, 0); // Horizontal padding

		int textX = contentRect.x;
		int textY = contentRect.y + (itemRect.height - dc.GetTextExtent(item.text).y) / 2;

		// Draw icon if available
		if (item.icon.IsOk()) {
			int iconY = contentRect.y + (itemRect.height - item.icon.GetHeight()) / 2;
			dc.DrawBitmap(item.icon, textX, iconY, true);
			textX += item.icon.GetWidth() + 4;
		}

		// Draw control based on type
		switch (item.type) {
		case FlatComboBox::ItemType::CHECKBOX: {
			// Draw checkbox
			wxRect checkboxRect(textX, contentRect.y + (itemRect.height - 12) / 2, 12, 12);
			dc.SetBrush(wxBrush(item.checked ? wxColour(0, 120, 215) : wxColour(255, 255, 255)));
			dc.SetPen(wxPen(wxColour(128, 128, 128)));
			dc.DrawRectangle(checkboxRect);

			if (item.checked) {
				// Draw check mark
				dc.SetPen(wxPen(wxColour(255, 255, 255), 2));
				wxPoint points[3] = {
					wxPoint(checkboxRect.x + 2, checkboxRect.y + 6),
					wxPoint(checkboxRect.x + 5, checkboxRect.y + 9),
					wxPoint(checkboxRect.x + 10, checkboxRect.y + 4)
				};
				dc.DrawLines(3, points);
			}

			textX += 16;
			break;
		}
		case FlatComboBox::ItemType::RADIO_BUTTON: {
			// Draw radio button
			wxRect radioRect(textX, contentRect.y + (itemRect.height - 12) / 2, 12, 12);
			dc.SetBrush(wxBrush(wxColour(255, 255, 255)));
			dc.SetPen(wxPen(wxColour(128, 128, 128)));
			dc.DrawEllipse(radioRect);

			if (item.checked) {
				dc.SetBrush(wxBrush(wxColour(0, 120, 215)));
				dc.SetPen(wxPen(wxColour(0, 120, 215)));
				dc.DrawEllipse(radioRect.x + 3, radioRect.y + 3, 6, 6);
			}

			textX += 16;
			break;
		}
		case FlatComboBox::ItemType::COLOR_PICKER: {
			// Draw color swatch
			wxRect colorRect(textX, contentRect.y + (itemRect.height - 12) / 2, 24, 12);
			dc.SetBrush(wxBrush(item.color.IsOk() ? item.color : wxColour(128, 128, 128)));
			dc.SetPen(wxPen(wxColour(128, 128, 128)));
			dc.DrawRectangle(colorRect);

			textX += 28;
			break;
		}
		default:
			break;
		}

		// Draw text
		dc.SetTextForeground(item.enabled ? m_textColor : wxColour(128, 128, 128));
		dc.DrawText(item.text, textX, textY);

		y += m_itemHeight;
	}
}

int FlatComboBoxPopup::HitTest(const wxPoint& pos) const
{
	int y = 1; // Start after border

	for (size_t i = 0; i < m_items.size(); ++i) {
		wxRect itemRect(1, y, GetClientSize().GetWidth() - 2, m_itemHeight);
		if (itemRect.Contains(pos)) {
			return static_cast<int>(i);
		}
		y += m_itemHeight;
	}

	return -1;
}

void FlatComboBoxPopup::SetHoverItem(int item)
{
	if (m_hoverItem != item) {
		m_hoverItem = item;
		Refresh();
	}
}