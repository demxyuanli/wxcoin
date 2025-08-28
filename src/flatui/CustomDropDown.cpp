#include "flatui/CustomDropDown.h"
#include "config/SvgIconManager.h"
#include "config/ThemeManager.h"
#include <wx/dcbuffer.h>
#include <wx/settings.h>

// Define the custom event
wxDEFINE_EVENT(wxEVT_CUSTOM_DROPDOWN_SELECTION, wxCommandEvent);

// Event table for CustomDropDown
wxBEGIN_EVENT_TABLE(CustomDropDown, wxControl)
EVT_PAINT(CustomDropDown::OnPaint)
EVT_SIZE(CustomDropDown::OnSize)
EVT_LEFT_DOWN(CustomDropDown::OnMouseDown)
EVT_MOTION(CustomDropDown::OnMouseMove)
EVT_LEAVE_WINDOW(CustomDropDown::OnMouseLeave)
EVT_KILL_FOCUS(CustomDropDown::OnKillFocus)
EVT_KEY_DOWN(CustomDropDown::OnKeyDown)
wxEND_EVENT_TABLE()

CustomDropDown::CustomDropDown(wxWindow* parent, wxWindowID id, const wxString& value,
	const wxPoint& pos, const wxSize& size, long style)
	: wxControl(parent, id, pos, size, style | wxBORDER_NONE),
	m_selection(wxNOT_FOUND),
	m_value(value),
	m_borderColour(CFG_COLOUR("DropdownBorderColour")),
	m_dropDownButtonColour(CFG_COLOUR("DropdownBackgroundColour")),
	m_dropDownButtonHoverColour(CFG_COLOUR("DropdownHoverColour")),
	m_popupBackgroundColour(CFG_COLOUR("DropdownBackgroundColour")),
	m_popupBorderColour(CFG_COLOUR("DropdownBorderColour")),
	m_selectionBackgroundColour(CFG_COLOUR("DropdownSelectionBgColour")),
	m_selectionForegroundColour(CFG_COLOUR("DropdownSelectionTextColour")),
	m_borderWidth(DEFAULT_BORDER_WIDTH),
	m_style(CustomDropDownStyle::Text_Dropdown),
	m_leftIconName(""),
	m_leftIconSize(16, 16),
	m_isDropDownShown(false),
	m_isButtonHovered(false),
	m_isButtonPressed(false),
	m_popup(nullptr),
	m_minDropDownWidth(0),
	m_maxDropDownHeight(200)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetDoubleBuffered(true);
	SetCanFocus(true);
}

CustomDropDown::~CustomDropDown()
{
	DestroyPopup();
}

void CustomDropDown::Clear()
{
	m_items.clear();
	m_selection = wxNOT_FOUND;
	m_value.Clear();
	Refresh();
}

void CustomDropDown::Append(const wxString& item)
{
	m_items.push_back(item);
	Refresh();
}

void CustomDropDown::Insert(const wxString& item, unsigned int pos)
{
	if (pos >= m_items.size()) {
		m_items.push_back(item);
	}
	else {
		m_items.insert(m_items.begin() + pos, item);
		if (m_selection >= (int)pos) {
			m_selection++;
		}
	}
	Refresh();
}

void CustomDropDown::Delete(unsigned int n)
{
	if (n < m_items.size()) {
		m_items.erase(m_items.begin() + n);
		if (m_selection == (int)n) {
			m_selection = wxNOT_FOUND;
			m_value.Clear();
		}
		else if (m_selection > (int)n) {
			m_selection--;
		}
		Refresh();
	}
}

unsigned int CustomDropDown::GetCount() const
{
	return m_items.size();
}

wxString CustomDropDown::GetString(unsigned int n) const
{
	if (n < m_items.size()) {
		return m_items[n];
	}
	return wxEmptyString;
}

void CustomDropDown::SetString(unsigned int n, const wxString& s)
{
	if (n < m_items.size()) {
		m_items[n] = s;
		if (m_selection == (int)n) {
			m_value = s;
		}
		Refresh();
	}
}

void CustomDropDown::SetSelection(int n)
{
	if (n == wxNOT_FOUND) {
		m_selection = wxNOT_FOUND;
		m_value.Clear();
	}
	else if (n >= 0 && n < (int)m_items.size()) {
		m_selection = n;
		m_value = m_items[n];
	}
	Refresh();
}

int CustomDropDown::GetSelection() const
{
	return m_selection;
}

wxString CustomDropDown::GetStringSelection() const
{
	if (m_selection >= 0 && m_selection < (int)m_items.size()) {
		return m_items[m_selection];
	}
	return wxEmptyString;
}

bool CustomDropDown::SetStringSelection(const wxString& s)
{
	for (size_t i = 0; i < m_items.size(); ++i) {
		if (m_items[i] == s) {
			SetSelection(i);
			return true;
		}
	}
	return false;
}

void CustomDropDown::SetValue(const wxString& value)
{
	m_value = value;

	// Try to find matching item
	for (size_t i = 0; i < m_items.size(); ++i) {
		if (m_items[i] == value) {
			m_selection = i;
			break;
		}
	}

	Refresh();
}

wxString CustomDropDown::GetValue() const
{
	return m_value;
}

bool CustomDropDown::SetBackgroundColour(const wxColour& colour)
{
	bool result = wxControl::SetBackgroundColour(colour);
	Refresh();
	return result;
}

bool CustomDropDown::SetForegroundColour(const wxColour& colour)
{
	bool result = wxControl::SetForegroundColour(colour);
	Refresh();
	return result;
}

void CustomDropDown::SetBorderColour(const wxColour& colour)
{
	m_borderColour = colour;
	Refresh();
}

void CustomDropDown::SetBorderWidth(int width)
{
	m_borderWidth = wxMax(0, width);
	Refresh();
}

void CustomDropDown::SetDropDownButtonColour(const wxColour& colour)
{
	m_dropDownButtonColour = colour;
	Refresh();
}

void CustomDropDown::SetDropDownButtonHoverColour(const wxColour& colour)
{
	m_dropDownButtonHoverColour = colour;
	Refresh();
}

void CustomDropDown::SetPopupBackgroundColour(const wxColour& colour)
{
	m_popupBackgroundColour = colour;
	if (m_popup) {
		m_popup->SetBackgroundColour(colour);
	}
}

void CustomDropDown::SetPopupBorderColour(const wxColour& colour)
{
	m_popupBorderColour = colour;
	if (m_popup) {
		m_popup->SetBorderColour(colour);
	}
}

void CustomDropDown::SetSelectionBackgroundColour(const wxColour& colour)
{
	m_selectionBackgroundColour = colour;
	if (m_popup) {
		m_popup->SetSelectionBackgroundColour(colour);
	}
}

void CustomDropDown::SetSelectionForegroundColour(const wxColour& colour)
{
	m_selectionForegroundColour = colour;
	if (m_popup) {
		m_popup->SetSelectionForegroundColour(colour);
	}
}

void CustomDropDown::ShowDropDown()
{
	if (m_isDropDownShown || m_items.empty()) {
		return;
	}

	CreatePopup();
	if (m_popup) {
		PositionPopup();
		m_popup->Show();
		m_popup->SetFocus();
		m_isDropDownShown = true;
		Refresh();
	}
}

void CustomDropDown::HideDropDown()
{
	if (!m_isDropDownShown) {
		return;
	}

	DestroyPopup();
	m_isDropDownShown = false;
	Refresh();
}

bool CustomDropDown::IsDropDownShown() const
{
	return m_isDropDownShown;
}

wxSize CustomDropDown::DoGetBestSize() const
{
	wxClientDC dc(const_cast<CustomDropDown*>(this));
	dc.SetFont(GetFont());

	int width = 0;
	int height = 0;

	switch (m_style) {
	case CustomDropDownStyle::DropdownOnly:
		// Just enough space for the dropdown icon
		width = m_borderWidth * 2 + 20; // 20 pixels for icon area
		height = m_borderWidth * 2 + 20;
		break;

	case CustomDropDownStyle::Text_Dropdown:
	{
		wxSize textSize = dc.GetTextExtent(m_value.IsEmpty() ? "Wg" : m_value);
		width = textSize.GetWidth() + DEFAULT_TEXT_MARGIN * 2 + DEFAULT_BUTTON_WIDTH + m_borderWidth * 2; // No separator in flat design
		height = textSize.GetHeight() + DEFAULT_TEXT_MARGIN * 2 + m_borderWidth * 2;
	}
	break;

	case CustomDropDownStyle::Icon_Text_Dropdown:
	{
		wxSize textSize = dc.GetTextExtent(m_value.IsEmpty() ? "Wg" : m_value);
		width = m_leftIconSize.GetWidth() + DEFAULT_TEXT_MARGIN * 3 + textSize.GetWidth() + DEFAULT_BUTTON_WIDTH + m_borderWidth * 2; // No separators in flat design
		height = wxMax(textSize.GetHeight(), m_leftIconSize.GetHeight()) + DEFAULT_TEXT_MARGIN * 2 + m_borderWidth * 2;
	}
	break;
	}

	return wxSize(width, height);
}

void CustomDropDown::SetMinDropDownWidth(int width)
{
	m_minDropDownWidth = wxMax(0, width);
}

void CustomDropDown::SetMaxDropDownHeight(int height)
{
	m_maxDropDownHeight = wxMax(50, height);
}

void CustomDropDown::SetDropDownStyle(CustomDropDownStyle style)
{
	m_style = style;
	Refresh();
}

CustomDropDownStyle CustomDropDown::GetDropDownStyle() const
{
	return m_style;
}

void CustomDropDown::SetLeftIcon(const wxString& iconName)
{
	m_leftIconName = iconName;
	Refresh();
}

wxString CustomDropDown::GetLeftIcon() const
{
	return m_leftIconName;
}

void CustomDropDown::SetLeftIconSize(const wxSize& size)
{
	m_leftIconSize = size;
	Refresh();
}

wxSize CustomDropDown::GetLeftIconSize() const
{
	return m_leftIconSize;
}

void CustomDropDown::OnPaint(wxPaintEvent& event)
{
	wxAutoBufferedPaintDC dc(this);

	DrawBackground(dc);
	DrawBorder(dc);

	// Draw content based on style
	switch (m_style) {
	case CustomDropDownStyle::Icon_Text_Dropdown:
		DrawLeftIcon(dc);
		DrawText(dc);
		DrawDropDownButton(dc);
		break;

	case CustomDropDownStyle::Text_Dropdown:
		DrawText(dc);
		DrawDropDownButton(dc);
		break;

	case CustomDropDownStyle::DropdownOnly:
		DrawDropDownButton(dc);
		break;
	}
}

void CustomDropDown::OnSize(wxSizeEvent& event)
{
	Refresh();
	event.Skip();
}

void CustomDropDown::OnMouseDown(wxMouseEvent& event)
{
	wxPoint pos = event.GetPosition();
	wxRect buttonRect = GetDropDownButtonRect();

	if (buttonRect.Contains(pos)) {
		m_isButtonPressed = true;
		if (m_isDropDownShown) {
			HideDropDown();
		}
		else {
			ShowDropDown();
		}
		Refresh();
	}
	else {
		SetFocus();
	}
}

void CustomDropDown::OnMouseMove(wxMouseEvent& event)
{
	wxPoint pos = event.GetPosition();
	wxRect buttonRect = GetDropDownButtonRect();

	bool wasHovered = m_isButtonHovered;
	m_isButtonHovered = buttonRect.Contains(pos);

	if (wasHovered != m_isButtonHovered) {
		Refresh();
	}
}

void CustomDropDown::OnMouseLeave(wxMouseEvent& event)
{
	if (m_isButtonHovered) {
		m_isButtonHovered = false;
		Refresh();
	}
}

void CustomDropDown::OnKillFocus(wxFocusEvent& event)
{
	if (m_isDropDownShown) {
		// Delay hiding to allow popup to handle events
		CallAfter([this]() {
			if (m_isDropDownShown && (!m_popup || !m_popup->HasFocus())) {
				HideDropDown();
			}
			});
	}
	event.Skip();
}

void CustomDropDown::OnKeyDown(wxKeyEvent& event)
{
	int key = event.GetKeyCode();

	switch (key) {
	case WXK_DOWN:
	case WXK_SPACE:
		if (!m_isDropDownShown) {
			ShowDropDown();
		}
		break;

	case WXK_UP:
		if (m_isDropDownShown) {
			HideDropDown();
		}
		break;

	case WXK_ESCAPE:
		if (m_isDropDownShown) {
			HideDropDown();
		}
		break;

	default:
		event.Skip();
		break;
	}
}

void CustomDropDown::DrawBackground(wxDC& dc)
{
	wxRect rect = GetClientRect();
	dc.SetBrush(wxBrush(GetBackgroundColour()));
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(rect);
}

void CustomDropDown::DrawBorder(wxDC& dc)
{
	if (m_borderWidth <= 0) {
		return;
	}

	wxRect rect = GetClientRect();

	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SetPen(wxPen(m_borderColour, m_borderWidth));

	// For flat design, draw only a simple outline border
	dc.DrawRectangle(rect.x, rect.y, rect.width, rect.height);

	// For flat design, no internal separator lines - keep it minimal
}

void CustomDropDown::DrawText(wxDC& dc)
{
	wxRect textRect = GetTextRect();

	dc.SetFont(GetFont());
	dc.SetTextForeground(GetForegroundColour());
	dc.SetClippingRegion(textRect);

	if (!m_value.IsEmpty()) {
		wxSize textSize = dc.GetTextExtent(m_value);
		int textY = textRect.y + (textRect.height - textSize.GetHeight()) / 2;
		dc.DrawText(m_value, textRect.x, textY);
	}

	dc.DestroyClippingRegion();
}

void CustomDropDown::DrawDropDownButton(wxDC& dc)
{
	wxRect buttonRect = GetDropDownButtonRect();

	// Flat design: use simple color fill without borders or inner effects
	wxColour buttonColour = m_isButtonHovered ? m_dropDownButtonHoverColour : m_dropDownButtonColour;
	dc.SetBrush(wxBrush(buttonColour));
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(buttonRect);

	// Draw 12x12 down.svg icon
	DrawDropDownIcon(dc, buttonRect);
}

void CustomDropDown::DrawDropDownIcon(wxDC& dc, const wxRect& buttonRect)
{
	// Use 12x12 down.svg icon
	wxSize iconSize(12, 12);
	wxBitmap iconBitmap = SvgIconManager::GetInstance().GetIconBitmap("down", iconSize);

	if (iconBitmap.IsOk()) {
		// Center the icon in the button rect
		int iconX = buttonRect.x + (buttonRect.width - iconSize.GetWidth()) / 2;
		int iconY = buttonRect.y + (buttonRect.height - iconSize.GetHeight()) / 2;

		dc.DrawBitmap(iconBitmap, iconX, iconY, true);
	}
	else {
		// Fallback to simple arrow if SVG icon is not available
		dc.SetPen(wxPen(GetForegroundColour(), 1));
		int centerX = buttonRect.x + buttonRect.width / 2;
		int centerY = buttonRect.y + buttonRect.height / 2;
		int arrowSize = 3; // Larger fallback arrow for 12x12 equivalent

		wxPoint points[3];
		points[0] = wxPoint(centerX - arrowSize, centerY - 1);
		points[1] = wxPoint(centerX + arrowSize, centerY - 1);
		points[2] = wxPoint(centerX, centerY + 2);

		dc.SetBrush(wxBrush(GetForegroundColour()));
		dc.DrawPolygon(3, points);
	}
}

void CustomDropDown::DrawLeftIcon(wxDC& dc)
{
	if (m_leftIconName.IsEmpty()) {
		return;
	}

	wxRect iconRect = GetLeftIconRect();
	if (iconRect.width <= 0 || iconRect.height <= 0) {
		return;
	}

	wxBitmap iconBitmap = SvgIconManager::GetInstance().GetIconBitmap(m_leftIconName, m_leftIconSize);

	if (iconBitmap.IsOk()) {
		// Center the icon in the rect
		int iconX = iconRect.x + (iconRect.width - m_leftIconSize.GetWidth()) / 2;
		int iconY = iconRect.y + (iconRect.height - m_leftIconSize.GetHeight()) / 2;

		dc.DrawBitmap(iconBitmap, iconX, iconY, true);
	}
}

wxRect CustomDropDown::GetDropDownButtonRect() const
{
	wxRect rect = GetClientRect();

	if (m_style == CustomDropDownStyle::DropdownOnly) {
		// For dropdown-only style, use the entire area
		return wxRect(m_borderWidth, m_borderWidth,
			rect.width - m_borderWidth * 2,
			rect.height - m_borderWidth * 2);
	}
	else {
		// For other styles, button is on the right side
		return wxRect(rect.width - DEFAULT_BUTTON_WIDTH - m_borderWidth,
			m_borderWidth,
			DEFAULT_BUTTON_WIDTH,
			rect.height - m_borderWidth * 2);
	}
}

wxRect CustomDropDown::GetTextRect() const
{
	wxRect rect = GetClientRect();

	if (m_style == CustomDropDownStyle::DropdownOnly) {
		// No text area for dropdown-only style
		return wxRect(0, 0, 0, 0);
	}

	int leftMargin = m_borderWidth + DEFAULT_TEXT_MARGIN;
	int rightMargin = DEFAULT_BUTTON_WIDTH + m_borderWidth + DEFAULT_TEXT_MARGIN; // No separator in flat design

	if (m_style == CustomDropDownStyle::Icon_Text_Dropdown && !m_leftIconName.IsEmpty()) {
		// Make space for left icon
		leftMargin += m_leftIconSize.GetWidth() + DEFAULT_TEXT_MARGIN;
	}

	return wxRect(leftMargin,
		m_borderWidth,
		rect.width - leftMargin - rightMargin,
		rect.height - m_borderWidth * 2);
}

wxRect CustomDropDown::GetLeftIconRect() const
{
	if (m_style != CustomDropDownStyle::Icon_Text_Dropdown || m_leftIconName.IsEmpty()) {
		return wxRect(0, 0, 0, 0);
	}

	wxRect rect = GetClientRect();
	int iconWidth = m_leftIconSize.GetWidth() + DEFAULT_TEXT_MARGIN;

	return wxRect(m_borderWidth + DEFAULT_TEXT_MARGIN,
		m_borderWidth,
		iconWidth,
		rect.height - m_borderWidth * 2);
}

void CustomDropDown::CreatePopup()
{
	if (m_popup) {
		return;
	}

	m_popup = new CustomDropDownPopup(this);
	m_popup->SetItems(m_items);
	m_popup->SetSelection(m_selection);
	m_popup->SetBackgroundColour(m_popupBackgroundColour);
	m_popup->SetBorderColour(m_popupBorderColour);
	m_popup->SetSelectionBackgroundColour(m_selectionBackgroundColour);
	m_popup->SetSelectionForegroundColour(m_selectionForegroundColour);
}

void CustomDropDown::DestroyPopup()
{
	if (m_popup) {
		m_popup->Destroy();
		m_popup = nullptr;
	}
}

void CustomDropDown::PositionPopup()
{
	if (!m_popup) {
		return;
	}

	wxPoint pos = ClientToScreen(wxPoint(0, GetSize().GetHeight()));
	wxSize popupSize = m_popup->GetBestSize();

	// Adjust width
	int width = wxMax(popupSize.GetWidth(), GetSize().GetWidth());
	if (m_minDropDownWidth > 0) {
		width = wxMax(width, m_minDropDownWidth);
	}

	// Adjust height
	int height = wxMin(popupSize.GetHeight(), m_maxDropDownHeight);

	// Check if popup fits below the control
	wxDisplay display(wxDisplay::GetFromWindow(this));
	wxRect screenRect = display.GetClientArea();

	if (pos.y + height > screenRect.GetBottom()) {
		// Show above the control
		pos.y = ClientToScreen(wxPoint(0, 0)).y - height;
	}

	m_popup->SetPosition(pos);
	m_popup->SetSize(width, height);
}

void CustomDropDown::OnPopupSelection(int selection)
{
	if (selection >= 0 && selection < (int)m_items.size()) {
		SetSelection(selection);

		// Send selection event
		wxCommandEvent event(wxEVT_CUSTOM_DROPDOWN_SELECTION, GetId());
		event.SetEventObject(this);
		event.SetInt(selection);
		event.SetString(m_items[selection]);
		ProcessEvent(event);
	}

	HideDropDown();
}

void CustomDropDown::OnPopupDismiss()
{
	HideDropDown();
}

// CustomDropDownPopup implementation
wxBEGIN_EVENT_TABLE(CustomDropDownPopup, wxPopupWindow)
EVT_PAINT(CustomDropDownPopup::OnPaint)
EVT_LEFT_DOWN(CustomDropDownPopup::OnMouseDown)
EVT_MOTION(CustomDropDownPopup::OnMouseMove)
EVT_KEY_DOWN(CustomDropDownPopup::OnKeyDown)
EVT_KILL_FOCUS(CustomDropDownPopup::OnKillFocus)
wxEND_EVENT_TABLE()

CustomDropDownPopup::CustomDropDownPopup(CustomDropDown* parent)
	: wxPopupWindow(parent),
	m_parent(parent),
	m_selection(wxNOT_FOUND),
	m_hoverItem(wxNOT_FOUND),
	m_backgroundColour(CFG_COLOUR("ThemeWhiteColour")),
	m_borderColour(CFG_COLOUR("DropdownBorderColour")),
	m_selectionBackgroundColour(CFG_COLOUR("DropdownSelectionBgColour")),
	m_selectionForegroundColour(CFG_COLOUR("DropdownSelectionTextColour")),
	m_itemHeight(20)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetCanFocus(true);
}

CustomDropDownPopup::~CustomDropDownPopup()
{
}

void CustomDropDownPopup::SetItems(const std::vector<wxString>& items)
{
	m_items = items;
	Refresh();
}

void CustomDropDownPopup::SetSelection(int selection)
{
	m_selection = selection;
	Refresh();
}

int CustomDropDownPopup::GetSelection() const
{
	return m_selection;
}

bool CustomDropDownPopup::SetBackgroundColour(const wxColour& colour)
{
	m_backgroundColour = colour;
	Refresh();
	return true;
}

void CustomDropDownPopup::SetBorderColour(const wxColour& colour)
{
	m_borderColour = colour;
	Refresh();
}

void CustomDropDownPopup::SetSelectionBackgroundColour(const wxColour& colour)
{
	m_selectionBackgroundColour = colour;
	Refresh();
}

void CustomDropDownPopup::SetSelectionForegroundColour(const wxColour& colour)
{
	m_selectionForegroundColour = colour;
	Refresh();
}

wxSize CustomDropDownPopup::GetBestSize() const
{
	if (m_items.empty()) {
		return wxSize(80, 50);
	}

	wxClientDC dc(const_cast<CustomDropDownPopup*>(this));
	dc.SetFont(GetFont());

	int maxWidth = 0;
	for (const wxString& item : m_items) {
		wxSize textSize = dc.GetTextExtent(item);
		maxWidth = wxMax(maxWidth, textSize.GetWidth());
	}

	int width = maxWidth + 20; // Add padding
	int height = m_items.size() * m_itemHeight + 2; // Add border

	return wxSize(width, height);
}

void CustomDropDownPopup::OnPaint(wxPaintEvent& event)
{
	wxAutoBufferedPaintDC dc(this);

	// Draw background
	wxRect rect = GetClientRect();
	dc.SetBrush(wxBrush(m_backgroundColour));
	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.DrawRectangle(rect);

	// Draw border
	dc.SetBrush(*wxTRANSPARENT_BRUSH);
	dc.SetPen(wxPen(m_borderColour, 1));
	dc.DrawRectangle(rect);

	// Draw items
	DrawItems(dc);
}

void CustomDropDownPopup::OnMouseDown(wxMouseEvent& event)
{
	int item = HitTest(event.GetPosition());
	if (item != wxNOT_FOUND) {
		m_parent->OnPopupSelection(item);
	}
	else {
		m_parent->OnPopupDismiss();
	}
}

void CustomDropDownPopup::OnMouseMove(wxMouseEvent& event)
{
	int item = HitTest(event.GetPosition());
	SetHoverItem(item);
}

void CustomDropDownPopup::OnKeyDown(wxKeyEvent& event)
{
	int key = event.GetKeyCode();

	switch (key) {
	case WXK_UP:
		if (m_hoverItem > 0) {
			SetHoverItem(m_hoverItem - 1);
		}
		break;

	case WXK_DOWN:
		if (m_hoverItem < (int)m_items.size() - 1) {
			SetHoverItem(m_hoverItem + 1);
		}
		break;

	case WXK_RETURN:
	case WXK_SPACE:
		if (m_hoverItem != wxNOT_FOUND) {
			m_parent->OnPopupSelection(m_hoverItem);
		}
		break;

	case WXK_ESCAPE:
		m_parent->OnPopupDismiss();
		break;

	default:
		event.Skip();
		break;
	}
}

void CustomDropDownPopup::OnKillFocus(wxFocusEvent& event)
{
	CallAfter([this]() {
		if (!HasFocus() && (!m_parent || !m_parent->HasFocus())) {
			m_parent->OnPopupDismiss();
		}
		});
	event.Skip();
}

void CustomDropDownPopup::DrawItems(wxDC& dc)
{
	dc.SetFont(GetFont());

	for (size_t i = 0; i < m_items.size(); ++i) {
		wxRect itemRect(1, i * m_itemHeight + 1, GetSize().GetWidth() - 2, m_itemHeight);

		bool isSelected = (i == (size_t)m_selection);
		bool isHovered = (i == (size_t)m_hoverItem);

		if (isSelected || isHovered) {
			dc.SetBrush(wxBrush(m_selectionBackgroundColour));
			dc.SetPen(*wxTRANSPARENT_PEN);
			dc.DrawRectangle(itemRect);
			dc.SetTextForeground(m_selectionForegroundColour);
		}
		else {
			dc.SetTextForeground(GetForegroundColour());
		}

		wxSize textSize = dc.GetTextExtent(m_items[i]);
		int textY = itemRect.y + (itemRect.height - textSize.GetHeight()) / 2;
		dc.DrawText(m_items[i], itemRect.x + 5, textY);
	}
}

int CustomDropDownPopup::HitTest(const wxPoint& pos) const
{
	if (pos.x < 0 || pos.x >= GetSize().GetWidth() ||
		pos.y < 0 || pos.y >= GetSize().GetHeight()) {
		return wxNOT_FOUND;
	}

	int item = (pos.y - 1) / m_itemHeight;
	if (item >= 0 && item < (int)m_items.size()) {
		return item;
	}

	return wxNOT_FOUND;
}

void CustomDropDownPopup::SetHoverItem(int item)
{
	if (m_hoverItem != item) {
		m_hoverItem = item;
		Refresh();
	}
}