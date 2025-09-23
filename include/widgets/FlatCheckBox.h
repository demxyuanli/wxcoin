#ifndef FLAT_CHECK_BOX_H
#define FLAT_CHECK_BOX_H

#include <wx/wx.h>
#include <wx/control.h>
#include <wx/bitmap.h>

// Forward declarations
class FlatCheckBox;

// Custom events
wxDECLARE_EVENT(wxEVT_FLAT_CHECK_BOX_CLICKED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_FLAT_CHECK_BOX_STATE_CHANGED, wxCommandEvent);

class FlatCheckBox : public wxControl
{
public:
	// Check box styles inspired by PyQt-Fluent-Widgets
	enum class CheckBoxStyle {
		DEFAULT_STYLE,  // Normal checkbox
		SWITCH,         // Switch-style checkbox
		RADIO,          // Radio-style checkbox
		CUSTOM          // Custom styled checkbox
	};

	// Check box states
	enum class CheckBoxState {
		UNCHECKED,
		CHECKED,
		PARTIALLY_CHECKED,
		HOVERED,
		PRESSED,
		DISABLED
	};

	FlatCheckBox(wxWindow* parent,
		wxWindowID id = wxID_ANY,
		const wxString& label = wxEmptyString,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		CheckBoxStyle style = CheckBoxStyle::DEFAULT_STYLE,
		long style_flags = 0);

	virtual ~FlatCheckBox();

	// Check box content
	void SetLabel(const wxString& label);
	wxString GetLabel() const { return m_label; }

	void SetIcon(const wxBitmap& icon);
	wxBitmap GetIcon() const { return m_icon; }

	// Check box style
	void SetCheckBoxStyle(CheckBoxStyle style);
	CheckBoxStyle GetCheckBoxStyle() const { return m_checkBoxStyle; }

	// Check box state
	void SetValue(bool checked);
	bool GetValue() const { return m_checked; }

	void Set3StateValue(wxCheckBoxState state);
	wxCheckBoxState Get3StateValue() const;

	void SetTriState(bool triState);
	bool IsTriState() const { return m_triState; }

	// Colors
	void SetBackgroundColor(const wxColour& color);
	wxColour GetBackgroundColor() const { return m_backgroundColor; }

	void SetCheckedColor(const wxColour& color);
	wxColour GetCheckedColor() const { return m_checkedColor; }

	void SetBorderColor(const wxColour& color);
	wxColour GetBorderColor() const { return m_borderColor; }

	void SetTextColor(const wxColour& color);
	wxColour GetTextColor() const { return m_textColor; }

	void SetHoverColor(const wxColour& color);
	wxColour GetHoverColor() const { return m_hoverColor; }

	// Border and corner radius
	void SetBorderWidth(int width);
	int GetBorderWidth() const { return m_borderWidth; }

	void SetCornerRadius(int radius);
	int GetCornerRadius() const { return m_cornerRadius; }

	// Spacing and sizing
	void SetCheckBoxSize(const wxSize& size);
	wxSize GetCheckBoxSize() const { return m_checkBoxSize; }

	void SetLabelSpacing(int spacing);
	int GetLabelSpacing() const { return m_labelSpacing; }

	// State management
	bool Enable(bool enabled) override;
	bool IsEnabled() const { return m_enabled; }

	// Font configuration
	void SetCustomFont(const wxFont& font);
	wxFont GetCustomFont() const { return m_customFont; }
	void UseConfigFont(bool useConfig = true);
	bool IsUsingConfigFont() const { return m_useConfigFont; }
	void ReloadFontFromConfig();

	// Size management
	virtual wxSize DoGetBestSize() const override;

protected:
	// Event handlers
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnMouseDown(wxMouseEvent& event);
	void OnMouseUp(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	void OnMouseLeave(wxMouseEvent& event);
	void OnMouseEnter(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnKeyUp(wxKeyEvent& event);
	void OnFocus(wxFocusEvent& event);
	void OnKillFocus(wxFocusEvent& event);

	// Drawing methods
	void DrawBackground(wxGraphicsContext& gc);
	void DrawCheckBox(wxGraphicsContext& gc);
	void DrawText(wxGraphicsContext& gc);
	void DrawRoundedRectangle(wxGraphicsContext& gc, const wxRect& rect, int radius);
	void DrawCheckMark(wxGraphicsContext& gc, const wxRect& rect);
	void DrawSwitch(wxGraphicsContext& gc, const wxRect& rect);

	// Helper methods
	void UpdateState(CheckBoxState newState);
	void ToggleValue();
	void SendCheckBoxEvent();
	wxRect GetCheckBoxRect() const;
	wxRect GetTextRect() const;
	wxColour GetCurrentBackgroundColor() const;
	wxColour GetCurrentBorderColor() const;
	wxColour GetCurrentTextColor() const;
	wxColour GetCurrentCheckColor() const;
	void InitializeDefaultColors();

private:
	// Check box content
	wxString m_label;
	wxBitmap m_icon;

	// Style and appearance
	CheckBoxStyle m_checkBoxStyle;
	CheckBoxState m_state;
	bool m_enabled;
	bool m_checked;
	bool m_triState;
	bool m_partiallyChecked;

	// Colors
	wxColour m_backgroundColor;
	wxColour m_checkedColor;
	wxColour m_borderColor;
	wxColour m_textColor;
	wxColour m_hoverColor;
	wxColour m_disabledColor;

	// Dimensions
	int m_borderWidth;
	int m_cornerRadius;
	wxSize m_checkBoxSize;
	int m_labelSpacing;

	// Font configuration
	wxFont m_customFont;
	bool m_useConfigFont;

	// State tracking
	bool m_isHovered;
	bool m_isPressed;
	bool m_hasFocus;

	// Layout
	wxRect m_checkBoxRect;
	wxRect m_textRect;
	wxRect m_iconRect;

	// Constants
	static const int DEFAULT_CORNER_RADIUS = 4;
	static const int DEFAULT_BORDER_WIDTH = 1;
	static const int DEFAULT_CHECK_BOX_SIZE = 16;
	static const int DEFAULT_LABEL_SPACING = 8;

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(FlatCheckBox);
};

#endif // FLAT_CHECK_BOX_H
