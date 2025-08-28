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

	struct ComboBoxItem {
		wxString text;
		wxBitmap icon;
		wxVariant data;
		bool enabled;
		bool selected;

		ComboBoxItem(const wxString& itemText = wxEmptyString,
			const wxBitmap& itemIcon = wxNullBitmap,
			const wxVariant& itemData = wxVariant())
			: text(itemText), icon(itemIcon), data(itemData), enabled(true), selected(false) {
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
	wxString GetString(unsigned int n) const;
	void SetString(unsigned int n, const wxString& s);

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
	wxColour m_borderColor;
	wxColour m_textColor;
	wxColour m_dropdownBackgroundColor;
	wxColour m_dropdownBorderColor;

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
class FlatComboBoxPopup : public wxPopupWindow
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

	wxSize GetBestSize() const;

protected:
	void OnPaint(wxPaintEvent& event);
	void OnMouseDown(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnKillFocus(wxFocusEvent& event);

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
