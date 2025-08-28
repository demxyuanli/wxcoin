#ifndef FLAT_SLIDER_H
#define FLAT_SLIDER_H

#include <wx/wx.h>
#include <wx/control.h>
#include <wx/bitmap.h>

// Forward declarations
class FlatSlider;

// Custom events
wxDECLARE_EVENT(wxEVT_FLAT_SLIDER_VALUE_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_FLAT_SLIDER_THUMB_DRAGGED, wxCommandEvent);

class FlatSlider : public wxControl
{
public:
	// Slider styles inspired by PyQt-Fluent-Widgets
	enum class SliderStyle {
		NORMAL,         // Normal slider
		PROGRESS,       // Progress bar style
		RANGE,          // Range slider with two thumbs
		VERTICAL        // Vertical slider
	};

	// Slider states
	enum class SliderState {
		DEFAULT_STATE,
		HOVERED,
		DRAGGING,
		FOCUSED,
		DISABLED,
		NORMAL
	};

	FlatSlider(wxWindow* parent,
		wxWindowID id = wxID_ANY,
		int value = 0,
		int minValue = 0,
		int maxValue = 100,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		SliderStyle style = SliderStyle::NORMAL,
		long style_flags = 0);

	virtual ~FlatSlider();

	// Slider content
	void SetLabel(const wxString& label);
	wxString GetLabel() const { return m_label; }

	void SetIcon(const wxBitmap& icon);
	wxBitmap GetIcon() const { return m_icon; }

	// Slider style
	void SetSliderStyle(SliderStyle style);
	SliderStyle GetSliderStyle() const { return m_sliderStyle; }

	// Slider values
	void SetValue(int value);
	int GetValue() const { return m_value; }

	void SetMinValue(int minValue);
	int GetMinValue() const { return m_minValue; }

	void SetMaxValue(int maxValue);
	int GetMaxValue() const { return m_maxValue; }

	void SetRange(int minValue, int maxValue);
	void GetRange(int& minValue, int& maxValue) const;

	// Colors
	void SetBackgroundColor(const wxColour& color);
	wxColour GetBackgroundColor() const { return m_backgroundColor; }

	void SetProgressColor(const wxColour& color);
	wxColour GetProgressColor() const { return m_progressColor; }

	void SetBorderColor(const wxColour& color);
	wxColour GetBorderColor() const { return m_borderColor; }

	void SetTextColor(const wxColour& color);
	wxColour GetTextColor() const { return m_textColor; }

	void SetThumbColor(const wxColour& color);
	wxColour GetThumbColor() const { return m_thumbColor; }

	void SetHoverColor(const wxColour& color);
	wxColour GetHoverColor() const { return m_hoverColor; }

	// Border and corner radius
	void SetBorderWidth(int width);
	int GetBorderWidth() const { return m_borderWidth; }

	void SetCornerRadius(int radius);
	int GetCornerRadius() const { return m_cornerRadius; }

	// Slider properties
	void SetThumbSize(const wxSize& size);
	wxSize GetThumbSize() const { return m_thumbSize; }

	void SetTrackHeight(int height);
	int GetTrackHeight() const { return m_trackHeight; }

	void SetShowValue(bool show);
	bool IsShowValue() const { return m_showValue; }

	void SetShowTicks(bool show);
	bool IsShowTicks() const { return m_showTicks; }

	void SetTickCount(int count);
	int GetTickCount() const { return m_tickCount; }

	// State management
	bool Enable(bool enabled) override;
	bool IsEnabled() const { return m_enabled; }

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
	void DrawBackground(wxDC& dc);
	void DrawTrack(wxDC& dc);
	void DrawProgress(wxDC& dc);
	void DrawThumb(wxDC& dc);
	void DrawTicks(wxDC& dc);
	void DrawText(wxDC& dc);
	void DrawIcon(wxDC& dc);
	void DrawRoundedRectangle(wxDC& dc, const wxRect& rect, int radius);

	// Helper methods
	void UpdateState(SliderState newState);
	void UpdateValueFromPosition(const wxPoint& pos);
	void SendSliderEvent();
	wxRect GetTrackRect() const;
	wxRect GetThumbRect() const;
	wxRect GetTextRect() const;
	wxRect GetIconRect() const;
	wxColour GetCurrentBackgroundColor() const;
	wxColour GetCurrentBorderColor() const;
	wxColour GetCurrentTextColor() const;
	wxColour GetCurrentThumbColor() const;
	double GetValueFromPosition(int pos) const;
	int GetPositionFromValue(double value) const;
	void InitializeDefaultColors();

private:
	// Slider content
	wxString m_label;
	wxBitmap m_icon;

	// Style and appearance
	SliderStyle m_sliderStyle;
	SliderState m_state;
	bool m_enabled;

	// Slider values
	int m_value;
	int m_minValue;
	int m_maxValue;

	// Colors
	wxColour m_backgroundColor;
	wxColour m_progressColor;
	wxColour m_borderColor;
	wxColour m_textColor;
	wxColour m_thumbColor;
	wxColour m_hoverColor;
	wxColour m_disabledColor;

	// Dimensions
	int m_borderWidth;
	int m_cornerRadius;
	wxSize m_thumbSize;
	int m_trackHeight;

	// Display options
	bool m_showValue;
	bool m_showTicks;
	int m_tickCount;

	// State tracking
	bool m_isHovered;
	bool m_isDragging;
	bool m_hasFocus;
	wxPoint m_dragStartPos;
	int m_dragStartValue;

	// Layout
	wxRect m_trackRect;
	wxRect m_thumbRect;
	wxRect m_textRect;
	wxRect m_iconRect;

	// Constants
	static const int DEFAULT_CORNER_RADIUS = 4;
	static const int DEFAULT_BORDER_WIDTH = 1;
	static const int DEFAULT_THUMB_SIZE = 16;
	static const int DEFAULT_TRACK_HEIGHT = 4;
	static const int DEFAULT_TICK_COUNT = 10;

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(FlatSlider);
};

#endif // FLAT_SLIDER_H
