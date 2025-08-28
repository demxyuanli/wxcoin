#ifndef FLAT_RADIO_BUTTON_H
#define FLAT_RADIO_BUTTON_H

#include <wx/wx.h>
#include <wx/control.h>
#include <wx/bitmap.h>

// Forward declarations
class FlatRadioButton;

// Custom events
wxDECLARE_EVENT(wxEVT_FLAT_RADIO_BUTTON_CLICKED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_FLAT_RADIO_BUTTON_STATE_CHANGED, wxCommandEvent);

class FlatRadioButton : public wxControl
{
public:
	FlatRadioButton(wxWindow* parent,
		wxWindowID id = wxID_ANY,
		const wxString& label = wxEmptyString,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style_flags = 0);

	virtual ~FlatRadioButton();

	// Radio button content
	void SetLabel(const wxString& label);
	wxString GetLabel() const { return m_label; }

	// Radio button state
	void SetValue(bool checked);
	bool GetValue() const { return m_checked; }

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

	// Spacing and sizing
	void SetRadioButtonSize(const wxSize& size);
	wxSize GetRadioButtonSize() const { return m_radioButtonSize; }

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
	void OnKeyUp(wxKeyEvent& event);
	void OnFocus(wxFocusEvent& event);
	void OnKillFocus(wxFocusEvent& event);

	// Drawing methods
	void DrawBackground(wxGraphicsContext& gc);
	void DrawRadioButton(wxGraphicsContext& gc);
	void DrawText(wxGraphicsContext& gc);

	// Helper methods
	void UpdateState(bool checked);
	void SendRadioButtonEvent();
	wxRect GetRadioButtonRect() const;
	wxRect GetTextRect() const;
	wxColour GetCurrentBackgroundColor() const;
	wxColour GetCurrentBorderColor() const;
	wxColour GetCurrentTextColor() const;
	wxColour GetCurrentCheckColor() const;
	void InitializeDefaultColors();

private:
	// Radio button content
	wxString m_label;

	// Style and appearance
	bool m_enabled;
	bool m_checked;

	// Colors
	wxColour m_backgroundColor;
	wxColour m_checkedColor;
	wxColour m_borderColor;
	wxColour m_textColor;
	wxColour m_hoverColor;
	wxColour m_disabledColor;

	// Dimensions
	int m_borderWidth;
	wxSize m_radioButtonSize;
	int m_labelSpacing;

	// Font configuration
	wxFont m_customFont;
	bool m_useConfigFont;

	// State tracking
	bool m_isHovered;
	bool m_isPressed;
	bool m_hasFocus;

	// Layout
	wxRect m_radioButtonRect;
	wxRect m_textRect;

	// Constants
	static const int DEFAULT_BORDER_WIDTH = 1;
	static const int DEFAULT_RADIO_BUTTON_SIZE = 16;
	static const int DEFAULT_LABEL_SPACING = 8;

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(FlatRadioButton);
};

#endif // FLAT_RADIO_BUTTON_H
