#ifndef FLATUIBUTTONBAR_H
#define FLATUIBUTTONBAR_H

#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/vector.h>
#include <wx/menu.h>
#include <wx/dcbuffer.h>
#include "logger/Logger.h"
#include "flatui/FlatUIThemeAware.h"

class FlatUIPanel;

enum class ButtonDisplayStyle {
	ICON_ONLY,
	TEXT_ONLY,
	ICON_TEXT_BESIDE,
	ICON_TEXT_BELOW
};

enum class ButtonType {
	NORMAL,         // Standard button
	TOGGLE,         // Toggle button (on/off state)
	CHECKBOX,       // Checkbox
	RADIO,          // Radio button (part of radio group)
	CHOICE,         // Choice/dropdown control
	SEPARATOR       // Visual separator
};

class FlatUIButtonBar : public FlatUIThemeAware
{
public:
	enum class ButtonStyle {
		DEFAULT,
		RAISED,
		OUTLINED,
		GHOST,
		PILL
	};

	enum class ButtonBorderStyle {
		SOLID,
		DASHED,
		DOTTED,
		DOUBLE,
		ROUNDED
	};

	struct ButtonInfo {
		int id;
		wxString label;
		wxBitmap icon;
		wxString iconName; // SVG icon name for theme-aware updates
		wxSize iconSize;   // Icon size for theme-aware updates
		wxRect rect;
		wxMenu* menu = nullptr;
		bool isDropDown = false;
		bool hovered = false;
		bool pressed = false;
		wxSize textSize; // Cached text extent
		wxString tooltip; // Add tooltip field

		// Extended properties for new button types
		ButtonType type = ButtonType::NORMAL;
		bool checked = false;               // For toggle, checkbox, radio
		bool enabled = true;                // Control enabled/disabled state
		int radioGroup = -1;                // Radio button group ID (-1 means no group)
		wxArrayString choiceItems;          // Items for choice control
		int selectedChoice = -1;            // Selected item index for choice control
		wxString value;                     // Current value (for complex controls)

		// Visual properties
		wxColour customBgColor = wxNullColour;      // Custom background color
		wxColour customTextColor = wxNullColour;    // Custom text color
		wxColour customBorderColor = wxNullColour;  // Custom border color
		bool visible = true;                        // Control visibility

		ButtonInfo(int buttonId = wxID_ANY, ButtonType buttonType = ButtonType::NORMAL)
			: id(buttonId), type(buttonType) {
		}
	};

	FlatUIButtonBar(FlatUIPanel* parent);
	virtual ~FlatUIButtonBar();

	// Override theme change method
	virtual void OnThemeChanged() override;

	// Override batch update method
	virtual void UpdateThemeValues() override;

	// === BUTTON MANAGEMENT METHODS ===
	// Original button method
	void AddButton(int id, const wxString& label, const wxBitmap& bitmap = wxNullBitmap, wxMenu* menu = nullptr, const wxString& tooltip = wxEmptyString);

	// Enhanced button method with type support
	void AddButton(int id, ButtonType type, const wxString& label, const wxBitmap& bitmap = wxNullBitmap, const wxString& tooltip = wxEmptyString);

	// Specialized methods for different button types
	void AddToggleButton(int id, const wxString& label, bool initialState = false, const wxBitmap& bitmap = wxNullBitmap, const wxString& tooltip = wxEmptyString);
	void AddCheckBox(int id, const wxString& label, bool initialState = false, const wxString& tooltip = wxEmptyString);
	void AddRadioButton(int id, const wxString& label, int radioGroup, bool initialState = false, const wxString& tooltip = wxEmptyString);

	// SVG icon methods for theme-aware icons
	void AddButtonWithSVG(int id, const wxString& label, const wxString& iconName, const wxSize& iconSize, wxMenu* menu = nullptr, const wxString& tooltip = wxEmptyString);
	void AddToggleButtonWithSVG(int id, const wxString& label, const wxString& iconName, const wxSize& iconSize, bool initialState = false, const wxString& tooltip = wxEmptyString);
	void SetButtonSVGIcon(int id, const wxString& iconName, const wxSize& iconSize);
	void AddChoiceControl(int id, const wxString& label, const wxArrayString& choices, int initialSelection = 0, const wxString& tooltip = wxEmptyString);
	void AddSeparator();

	// Control state management
	void SetButtonChecked(int id, bool checked);
	bool IsButtonChecked(int id) const;
	void SetButtonEnabled(int id, bool enabled);
	bool IsButtonEnabled(int id) const;
	void SetButtonVisible(int id, bool visible);
	bool IsButtonVisible(int id) const;

	// Choice control specific methods
	void SetChoiceItems(int id, const wxArrayString& items);
	wxArrayString GetChoiceItems(int id) const;
	void SetChoiceSelection(int id, int selection);
	int GetChoiceSelection(int id) const;
	wxString GetChoiceValue(int id) const;

	// Radio button group management
	void SetRadioGroupSelection(int radioGroup, int selectedId);
	int GetRadioGroupSelection(int radioGroup) const;

	// Button value and properties
	void SetButtonValue(int id, const wxString& value);
	wxString GetButtonValue(int id) const;
	void SetButtonCustomColors(int id, const wxColour& bgColor, const wxColour& textColor = wxNullColour, const wxColour& borderColor = wxNullColour);

	// Add method to set tooltip for existing button
	void SetButtonTooltip(int id, const wxString& tooltip);

	// Remove button
	void RemoveButton(int id);
	void Clear();

	size_t GetButtonCount() const { return m_buttons.size(); }

	// === DISPLAY AND LAYOUT METHODS ===
	void SetDisplayStyle(ButtonDisplayStyle style);
	ButtonDisplayStyle GetDisplayStyle() const { return m_displayStyle; }

	void SetButtonStyle(ButtonStyle style);
	ButtonStyle GetButtonStyle() const { return m_buttonStyle; }

	void SetButtonBorderStyle(ButtonBorderStyle style);
	ButtonBorderStyle GetButtonBorderStyle() const { return m_buttonBorderStyle; }

	// === COLOUR AND STYLE SETTERS ===
	void SetButtonBackgroundColour(const wxColour& colour);
	wxColour GetButtonBackgroundColour() const { return m_buttonBgColour; }

	void SetButtonHoverBackgroundColour(const wxColour& colour);
	wxColour GetButtonHoverBackgroundColour() const { return m_buttonHoverBgColour; }

	void SetButtonPressedBackgroundColour(const wxColour& colour);
	wxColour GetButtonPressedBackgroundColour() const { return m_buttonPressedBgColour; }

	void SetButtonTextColour(const wxColour& colour);
	wxColour GetButtonTextColour() const { return m_buttonTextColour; }

	void SetButtonBorderColour(const wxColour& colour);
	wxColour GetButtonBorderColour() const { return m_buttonBorderColour; }

	void SetButtonBorderWidth(int width);
	int GetButtonBorderWidth() const { return m_buttonBorderWidth; }

	void SetButtonCornerRadius(int radius);
	int GetButtonCornerRadius() const { return m_buttonCornerRadius; }

	void SetButtonSpacing(int spacing);
	int GetButtonSpacing() const { return m_buttonSpacing; }

	void SetButtonPadding(int horizontal, int vertical);
	void GetButtonPadding(int& horizontal, int& vertical) const;

	void SetBtnBarBackgroundColour(const wxColour& colour);
	wxColour GetBtnBarBackgroundColour() const { return m_btnBarBgColour; }

	void SetBtnBarBorderColour(const wxColour& colour);
	wxColour GetBtnBarBorderColour() const { return m_btnBarBorderColour; }

	void SetBtnBarBorderWidth(int width);
	int GetBtnBarBorderWidth() const { return m_btnBarBorderWidth; }

	void SetHoverEffectsEnabled(bool enabled);
	bool GetHoverEffectsEnabled() const { return m_hoverEffectsEnabled; }

	// Event handlers
	void OnPaint(wxPaintEvent& evt);
	void OnMouseDown(wxMouseEvent& evt);
	void OnMouseMove(wxMouseEvent& evt);
	void OnMouseLeave(wxMouseEvent& evt);
	void OnSize(wxSizeEvent& evt);

	// Layout and drawing methods
	void RecalculateLayout();
	int CalculateButtonWidth(const ButtonInfo& button, wxDC& dc) const;
	void DrawButton(wxDC& dc, const ButtonInfo& button, int index);
	void DrawButtonBackground(wxDC& dc, const ButtonInfo& button, const wxRect& rect, bool isHovered, bool isPressed);
	void DrawButtonBorder(wxDC& dc, const ButtonInfo& button, const wxRect& rect, bool isHovered, bool isPressed);
	void DrawButtonIcon(wxDC& dc, const ButtonInfo& button, const wxRect& rect);
	void DrawButtonText(wxDC& dc, const ButtonInfo& button, const wxRect& rect);
	void DrawButtonDropdownArrow(wxDC& dc, const ButtonInfo& button, const wxRect& rect);
	void DrawButtonSeparator(wxDC& dc, const ButtonInfo& button, const wxRect& rect);

	// Specialized drawing methods for different button types
	void DrawToggleButton(wxDC& dc, const ButtonInfo& button, const wxRect& rect);
	void DrawCheckBox(wxDC& dc, const ButtonInfo& button, const wxRect& rect);
	void DrawRadioButton(wxDC& dc, const ButtonInfo& button, const wxRect& rect);
	void DrawChoiceControl(wxDC& dc, const ButtonInfo& button, const wxRect& rect);
	void DrawCheckBoxIndicator(wxDC& dc, const wxRect& rect, bool checked, bool enabled);
	void DrawRadioButtonIndicator(wxDC& dc, const wxRect& rect, bool checked, bool enabled);
	void DrawChoiceDropdownArrow(wxDC& dc, const wxRect& rect, bool enabled);

	// Button finding and utility methods
	ButtonInfo* FindButton(int id);
	const ButtonInfo* FindButton(int id) const;
	int FindButtonIndex(int id) const;
	wxRect GetCheckBoxIndicatorRect(const wxRect& buttonRect) const;
	wxRect GetRadioButtonIndicatorRect(const wxRect& buttonRect) const;
	wxRect GetChoiceDropdownRect(const wxRect& buttonRect) const;

	// Event handling methods for different button types
	void HandleToggleButton(ButtonInfo& button);
	void HandleCheckBox(ButtonInfo& button);
	void HandleRadioButton(ButtonInfo& button);
	void HandleChoiceControl(ButtonInfo& button, const wxPoint& mousePos);

private:
	// Button data
	std::vector<ButtonInfo> m_buttons;

	// Display and style settings
	ButtonDisplayStyle m_displayStyle;
	ButtonStyle m_buttonStyle;
	ButtonBorderStyle m_buttonBorderStyle;

	// Colors
	wxColour m_buttonBgColour;
	wxColour m_buttonHoverBgColour;
	wxColour m_buttonPressedBgColour;
	wxColour m_buttonTextColour;
	wxColour m_buttonBorderColour;

	// Dimensions and spacing
	int m_buttonBorderWidth;
	int m_buttonCornerRadius;
	int m_buttonSpacing;
	int m_buttonHorizontalPadding;
	int m_buttonVerticalPadding;

	// Bar properties
	wxColour m_btnBarBgColour;
	wxColour m_btnBarBorderColour;
	int m_btnBarBorderWidth;

	// Dropdown and separator properties
	int m_dropdownArrowWidth;
	int m_dropdownArrowHeight;
	int m_separatorWidth;
	int m_separatorPadding;
	int m_separatorMargin;
	int m_btnBarHorizontalMargin;

	// State
	bool m_hoverEffectsEnabled;
	int m_hoveredButtonIndex;
	int m_pressedButtonIndex;

	// Radio button group tracking
	std::map<int, std::vector<int>> m_radioGroups; // radioGroup -> button IDs

protected:
	int m_topMargin = 0;
public:
	int GetTopMargin() const { return m_topMargin; }
	// Override wxControl virtual method
	wxSize DoGetBestSize() const override;

	// Theme refresh method
	void RefreshTheme();
};

#endif // FLATUIBUTTONBAR_H