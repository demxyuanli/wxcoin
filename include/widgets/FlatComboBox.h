#ifndef FLAT_COMBO_BOX_H
#define FLAT_COMBO_BOX_H

#include <wx/wx.h>
#include <wx/control.h>
#include <wx/choice.h>
#include <wx/bitmap.h>
#include <wx/popupwin.h>
#include <vector>

// Forward declarations
class FlatComboBox;
class FlatComboBoxPopup;

// Custom events
wxDECLARE_EVENT(wxEVT_FLAT_COMBO_BOX_SELECTION_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_FLAT_COMBO_BOX_DROPDOWN_OPENED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_FLAT_COMBO_BOX_DROPDOWN_CLOSED, wxCommandEvent);

class FlatComboBox : public wxControl
{
public:
	// Combo box styles inspired by PyQt-Fluent-Widgets
	enum class ComboBoxStyle {
		DEFAULT_STYLE,  // Normal dropdown
		EDITABLE,       // Editable dropdown
		SEARCH,         // Searchable dropdown
		MULTI_SELECT    // Multi-select dropdown
	};

	// Combo box states
	enum class ComboBoxState {
		DEFAULT_STATE,
		HOVERED,
		FOCUSED,
		DROPDOWN_OPEN,
		DISABLED
	};

	// Item types for different kinds of combo box items
	enum class ItemType {
		NORMAL,         // Regular text item
		COLOR_PICKER,   // Color picker item
		CHECKBOX,       // Checkbox item
		RADIO_BUTTON,   // Radio button item
		SEPARATOR       // Separator line
	};

	struct ComboBoxItem {
		ItemType type;
		wxString text;
		wxBitmap icon;
		wxVariant data;
		bool enabled;
		bool checked;       // For checkbox/radio items
		wxColour color;     // For color picker items
		wxString group;     // For grouping radio buttons

		ComboBoxItem(ItemType itemType = ItemType::NORMAL,
			const wxString& itemText = wxEmptyString,
			const wxBitmap& itemIcon = wxNullBitmap,
			const wxVariant& itemData = wxVariant(),
			bool itemEnabled = true,
			bool itemChecked = false,
			const wxColour& itemColor = wxTransparentColour,
			const wxString& itemGroup = wxEmptyString)
			: type(itemType), text(itemText), icon(itemIcon), data(itemData),
			  enabled(itemEnabled), checked(itemChecked), color(itemColor), group(itemGroup) {
		}

		// Convenience constructors
		static ComboBoxItem Normal(const wxString& text, const wxBitmap& icon = wxNullBitmap,
			const wxVariant& data = wxVariant(), bool enabled = true) {
			return ComboBoxItem(ItemType::NORMAL, text, icon, data, enabled);
		}

		static ComboBoxItem ColorPicker(const wxString& text, const wxColour& color,
			const wxBitmap& icon = wxNullBitmap, bool enabled = true) {
			return ComboBoxItem(ItemType::COLOR_PICKER, text, icon, wxVariant(), enabled, false, color);
		}

		static ComboBoxItem Checkbox(const wxString& text, bool checked = false,
			const wxBitmap& icon = wxNullBitmap, bool enabled = true) {
			return ComboBoxItem(ItemType::CHECKBOX, text, icon, wxVariant(), enabled, checked);
		}

		static ComboBoxItem RadioButton(const wxString& text, const wxString& group,
			bool checked = false, const wxBitmap& icon = wxNullBitmap, bool enabled = true) {
			return ComboBoxItem(ItemType::RADIO_BUTTON, text, icon, wxVariant(), enabled, checked, wxTransparentColour, group);
		}

		static ComboBoxItem Separator() {
			return ComboBoxItem(ItemType::SEPARATOR, wxEmptyString);
		}
	};

	FlatComboBox(wxWindow* parent,
		wxWindowID id = wxID_ANY,
		const wxString& value = wxEmptyString,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		ComboBoxStyle style = ComboBoxStyle::DEFAULT_STYLE,
		long style_flags = 0);

	virtual ~FlatComboBox();

	// Item management
	void Clear();
	void Append(const wxString& item, const wxBitmap& icon = wxNullBitmap, const wxVariant& data = wxVariant());
	void Insert(const wxString& item, unsigned int pos, const wxBitmap& icon = wxNullBitmap, const wxVariant& data = wxVariant());
	void Delete(unsigned int n);
	unsigned int GetCount() const;
	const std::vector<ComboBoxItem>& GetItems() const;
	wxString GetString(unsigned int n) const;
	void SetString(unsigned int n, const wxString& s);

	// Advanced item management
	void Append(const ComboBoxItem& item);
	void Insert(const ComboBoxItem& item, unsigned int pos);

	// Convenience methods for different item types
	void AppendColorPicker(const wxString& text, const wxColour& color, const wxBitmap& icon = wxNullBitmap);
	void AppendCheckbox(const wxString& text, bool checked = false, const wxBitmap& icon = wxNullBitmap);
	void AppendRadioButton(const wxString& text, const wxString& group, bool checked = false, const wxBitmap& icon = wxNullBitmap);
	void AppendSeparator();

	// Item state management
	bool IsItemChecked(unsigned int n) const;
	void SetItemChecked(unsigned int n, bool checked);
	wxColour GetItemColor(unsigned int n) const;
	void SetItemColor(unsigned int n, const wxColour& color);
	ItemType GetItemType(unsigned int n) const;

	// Selection
	void SetSelection(int n);
	int GetSelection() const;
	wxString GetStringSelection() const;
	bool SetStringSelection(const wxString& s);

	// Value
	void SetValue(const wxString& value);
	wxString GetValue() const;

	// Combo box style
	void SetComboBoxStyle(ComboBoxStyle style);
	ComboBoxStyle GetComboBoxStyle() const { return m_comboBoxStyle; }

	// Colors
	void SetBackgroundColor(const wxColour& color);
	wxColour GetBackgroundColor() const { return m_backgroundColor; }

	void SetFocusedColor(const wxColour& color);
	wxColour GetFocusedColor() const { return m_focusedColor; }

	void SetBorderColor(const wxColour& color);
	wxColour GetBorderColor() const { return m_borderColor; }

	void SetTextColor(const wxColour& color);
	wxColour GetTextColor() const { return m_textColor; }

	void SetDropdownBackgroundColor(const wxColour& color);
	wxColour GetDropdownBackgroundColor() const { return m_dropdownBackgroundColor; }

	void SetDropdownBorderColor(const wxColour& color);
	wxColour GetDropdownBorderColor() const { return m_dropdownBorderColor; }

	void SetHoverColor(const wxColour& color);
	wxColour GetHoverColor() const { return m_hoverColor; }

	void SetDisabledBackgroundColor(const wxColour& color);
	wxColour GetDisabledBackgroundColor() const { return m_disabledBackgroundColor; }

	void SetDisabledTextColor(const wxColour& color);
	wxColour GetDisabledTextColor() const { return m_disabledTextColor; }

	void SetDisabledBorderColor(const wxColour& color);
	wxColour GetDisabledBorderColor() const { return m_disabledBorderColor; }

	void SetDropdownHoverColor(const wxColour& color);
	wxColour GetDropdownHoverColor() const { return m_dropdownHoverColor; }

	// Dimensions
	void SetBorderWidth(int width);
	int GetBorderWidth() const { return m_borderWidth; }

	void SetCornerRadius(int radius);
	int GetCornerRadius() const { return m_cornerRadius; }

	void SetPadding(int horizontal, int vertical);
	void GetPadding(int& horizontal, int& vertical) const;

	void SetMaxVisibleItems(int maxItems);
	int GetMaxVisibleItems() const { return m_maxVisibleItems; }

	void SetDropdownWidth(int width);
	int GetDropdownWidth() const { return m_dropdownWidth; }

	// Icons
	void SetItemIcon(unsigned int index, const wxBitmap& icon);
	wxBitmap GetItemIcon(unsigned int index) const;

	void SetDropdownIcon(const wxBitmap& icon);
	wxBitmap GetDropdownIcon() const { return m_dropdownIcon; }

	// State
	void SetEnabled(bool enabled);
	bool IsEnabled() const { return m_enabled; }

	void SetEditable(bool editable);
	bool IsEditable() const { return m_editable; }

	// Sizing
	virtual wxSize DoGetBestSize() const override;

	// Dropdown control
	void ShowDropdown();
	void HideDropdown();
	bool IsDropdownShown() const { return m_dropdownShown; }

protected:
	// Event handlers
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnMouseDown(wxMouseEvent& event);
	void OnMouseUp(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	void OnMouseLeave(wxMouseEvent& event);
	void OnMouseEnter(wxMouseEvent& event);
	void OnFocus(wxFocusEvent& event);
	void OnKillFocus(wxFocusEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnChar(wxKeyEvent& event);
	void OnSelectionChanged(wxCommandEvent& event);
	void OnDropdownOpened(wxCommandEvent& event);
	void OnDropdownClosed(wxCommandEvent& event);

	// Drawing methods
	void DrawBackground(wxDC& dc);
	void DrawBorder(wxDC& dc);
	void DrawText(wxDC& dc);
	void DrawDropdownButton(wxDC& dc);
	void DrawRoundedRectangle(wxDC& dc, const wxRect& rect, int radius);

	// Helper methods
	void UpdateState(ComboBoxState newState);
	void UpdateLayout();
	void HandleDropdownClick();
	void OnThemeChange();
	wxRect GetTextRect() const;
	wxRect GetDropdownButtonRect() const;
	wxColour GetCurrentBackgroundColor() const;
	wxColour GetCurrentBorderColor() const;
	wxColour GetCurrentTextColor() const;
	void CreateDefaultIcons();
	void InitializeDefaultColors();

private:
	// Items
	std::vector<ComboBoxItem> m_items;
	int m_selection;
	wxString m_value;

	// Style and appearance
	ComboBoxStyle m_comboBoxStyle;
	ComboBoxState m_state;
	bool m_enabled;
	bool m_editable;

	// Colors
	wxColour m_backgroundColor;
	wxColour m_focusedColor;
	wxColour m_hoverColor;
	wxColour m_borderColor;
	wxColour m_textColor;
	wxColour m_disabledBackgroundColor;
	wxColour m_disabledTextColor;
	wxColour m_disabledBorderColor;
	wxColour m_dropdownBackgroundColor;
	wxColour m_dropdownBorderColor;
	wxColour m_dropdownHoverColor;

	// Dimensions
	int m_borderWidth;
	int m_cornerRadius;
	int m_padding;
	int m_dropdownButtonWidth;
	int m_maxVisibleItems;
	int m_dropdownWidth;

	// Icons
	wxBitmap m_dropdownIcon;
	wxBitmap m_dropdownIconHover;

	// State tracking
	bool m_isFocused;
	bool m_isHovered;
	bool m_isPressed;
	bool m_dropdownShown;
	bool m_dropdownButtonHovered;

	// Layout
	wxRect m_textRect;
	wxRect m_dropdownButtonRect;

	// Popup
	FlatComboBoxPopup* m_popup;

	// Constants
	static const int DEFAULT_CORNER_RADIUS = 6;
	static const int DEFAULT_BORDER_WIDTH = 1;
	static const int DEFAULT_PADDING = 8;
	static const int DEFAULT_DROPDOWN_BUTTON_WIDTH = 20;
	static const int DEFAULT_MAX_VISIBLE_ITEMS = 8;
	static const int DEFAULT_DROPDOWN_WIDTH = 200;

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(FlatComboBox);
};

// Popup window for dropdown
class FlatComboBoxPopup : public wxPopupTransientWindow
{
public:
	FlatComboBoxPopup(FlatComboBox* parent);
	virtual ~FlatComboBoxPopup();

	void SetItems(const std::vector<FlatComboBox::ComboBoxItem>& items);
	void SetSelection(int selection);
	int GetSelection() const;

	void SetBackgroundColor(const wxColour& color);
	void SetBorderColor(const wxColour& color);
	void SetTextColor(const wxColour& color);
	void SetHoverColor(const wxColour& color);

	void ClearParent() { m_parent = nullptr; }

	wxSize GetBestSize() const;

protected:
	void OnPaint(wxPaintEvent& event);
	void OnMouseDown(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);

	// Override to control dismissal behavior
	bool ProcessLeftDown(wxMouseEvent& event) override;
	void OnDismiss() override;

	void DrawItems(wxDC& dc);
	int HitTest(const wxPoint& pos) const;
	void SetHoverItem(int item);

private:
	FlatComboBox* m_parent;
	std::vector<FlatComboBox::ComboBoxItem> m_items;
	int m_selection;
	int m_hoverItem;

	wxColour m_backgroundColor;
	wxColour m_borderColor;
	wxColour m_textColor;
	wxColour m_hoverColor;

	int m_itemHeight;

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(FlatComboBoxPopup);
};

#endif // FLAT_COMBO_BOX_H
