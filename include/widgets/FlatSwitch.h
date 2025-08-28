#ifndef FLAT_SWITCH_H
#define FLAT_SWITCH_H

#include <wx/wx.h>
#include <wx/control.h>
#include <wx/bitmap.h>

// Forward declarations
class FlatSwitch;

// Custom events
wxDECLARE_EVENT(wxEVT_FLAT_SWITCH_TOGGLED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_FLAT_SWITCH_STATE_CHANGED, wxCommandEvent);

class FlatSwitch : public wxControl
{
public:
	// Switch styles inspired by PyQt-Fluent-Widgets
	enum class SwitchStyle {
		DEFAULT_STYLE,  // Normal switch
		ROUND,          // Round switch
		SQUARE,         // Square switch
		CUSTOM          // Custom styled switch
	};

	// Switch states
	enum class SwitchState {
		DEFAULT_STATE,
		HOVERED,
		PRESSED,
		DISABLED
	};

	FlatSwitch(wxWindow* parent,
		wxWindowID id = wxID_ANY,
		bool value = false,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		SwitchStyle style = SwitchStyle::DEFAULT_STYLE,
		long style_flags = 0);

	virtual ~FlatSwitch();

	// Switch content
	void SetLabel(const wxString& label);
	wxString GetLabel() const;

	void SetIcon(const wxBitmap& icon);
	wxBitmap GetIcon() const { return m_icon; }

	// Switch style
	void SetSwitchStyle(SwitchStyle style);
	SwitchStyle GetSwitchStyle() const;

	// Switch state
	void SetValue(bool value);
	bool GetValue() const;

	void Toggle();
	bool IsOn() const { return m_value; }
	bool IsOff() const { return !m_value; }

	// Colors
	void SetBackgroundColor(const wxColour& color);
	wxColour GetBackgroundColor() const;

	void SetOnColor(const wxColour& color);
	wxColour GetOnColor() const { return m_onColor; }

	void SetOffColor(const wxColour& color);
	wxColour GetOffColor() const { return m_offColor; }

	void SetBorderColor(const wxColour& color);
	wxColour GetBorderColor() const;

	void SetTextColor(const wxColour& color);
	wxColour GetTextColor() const;

	void SetThumbColor(const wxColour& color);
	wxColour GetThumbColor() const { return m_thumbColor; }

	void SetHoverColor(const wxColour& color);
	wxColour GetHoverColor() const { return m_hoverColor; }

	// Border and corner radius
	void SetBorderWidth(int width);
	int GetBorderWidth() const;

	void SetCornerRadius(int radius);
	int GetCornerRadius() const;

	// Switch properties
	void SetThumbSize(const wxSize& size);
	wxSize GetThumbSize() const;

	void SetTrackHeight(int height);
	int GetTrackHeight() const;

	void SetAnimationDuration(int duration);
	int GetAnimationDuration() const;

	// State management
	bool Enable(bool enabled) override;
	bool IsEnabled() const;

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
	void OnTimer(wxTimerEvent& event);

	// Drawing methods
	void DrawBackground(wxDC& dc);
	void DrawTrack(wxDC& dc);
	void DrawThumb(wxDC& dc);
	void DrawText(wxDC& dc);
	void DrawIcon(wxDC& dc);
	void DrawRoundedRectangle(wxDC& dc, const wxRect& rect, int radius);
	void DrawSwitch(wxDC& dc);

	// Helper methods
	void UpdateState(SwitchState newState);
	void ToggleValue();
	void SendSwitchEvent();
	void StartAnimation();
	void UpdateAnimation();
	wxRect GetTrackRect() const;
	wxRect GetThumbRect() const;
	wxRect GetTextRect() const;
	wxRect GetIconRect() const;
	wxColour GetCurrentBackgroundColor() const;
	wxColour GetCurrentBorderColor() const;
	wxColour GetCurrentTextColor() const;
	wxColour GetCurrentThumbColor() const;
	void InitializeDefaultColors();

private:
	// Switch content
	wxString m_label;
	wxBitmap m_icon;

	// Style and appearance
	SwitchStyle m_switchStyle;
	SwitchState m_state;
	bool m_enabled;
	bool m_value;

	// Colors
	wxColour m_backgroundColor;
	wxColour m_onColor;
	wxColour m_offColor;
	wxColour m_borderColor;
	wxColour m_textColor;
	wxColour m_thumbColor;
	wxColour m_hoverColor;
	wxColour m_disabledColor;
	wxColour m_checkedColor;

	// Dimensions
	int m_borderWidth;
	int m_cornerRadius;
	wxSize m_thumbSize;
	int m_trackHeight;

	// Animation
	wxTimer m_animationTimer;
	double m_animationProgress;
	bool m_animating;
	int m_animationDuration;

	// State tracking
	bool m_isHovered;
	bool m_isPressed;
	bool m_hasFocus;

	// Layout
	wxRect m_trackRect;
	wxRect m_thumbRect;
	wxRect m_textRect;
	wxRect m_iconRect;

	// Constants
	static const int DEFAULT_CORNER_RADIUS = 12;
	static const int DEFAULT_BORDER_WIDTH = 1;
	static const int DEFAULT_THUMB_SIZE = 20;
	static const int DEFAULT_TRACK_HEIGHT = 24;
	static const int DEFAULT_ANIMATION_DURATION = 200;

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(FlatSwitch);
};

#endif // FLAT_SWITCH_H
