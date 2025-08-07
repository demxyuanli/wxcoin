#ifndef FLAT_PROGRESS_BAR_H
#define FLAT_PROGRESS_BAR_H

#include <wx/wx.h>
#include <wx/control.h>
#include <wx/bitmap.h>

// Forward declarations
class FlatProgressBar;

// Custom events
wxDECLARE_EVENT(wxEVT_FLAT_PROGRESS_BAR_VALUE_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_FLAT_PROGRESS_BAR_COMPLETED, wxCommandEvent);

class FlatProgressBar : public wxControl
{
public:
    // Progress bar styles inspired by PyQt-Fluent-Widgets
    enum class ProgressBarStyle {
        DEFAULT_STYLE,  // Normal progress bar
        INDETERMINATE,  // Indeterminate progress bar
        CIRCULAR,       // Circular progress bar
        STRIPED         // Striped progress bar
    };

    // Progress bar states
    enum class ProgressBarState {
        DEFAULT_STATE,
        PAUSED,
        HAS_ERROR,
        COMPLETED
    };

    FlatProgressBar(wxWindow* parent, 
                    wxWindowID id = wxID_ANY,
                    int value = 0,
                    int minValue = 0,
                    int maxValue = 100,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& size = wxDefaultSize,
                    ProgressBarStyle style = ProgressBarStyle::DEFAULT_STYLE,
                    long style_flags = 0);

    virtual ~FlatProgressBar();

    // Progress bar content
    void SetLabel(const wxString& label);
    wxString GetLabel() const;
    
    void SetIcon(const wxBitmap& icon);
    wxBitmap GetIcon() const { return m_icon; }

    // Progress bar style
    void SetProgressBarStyle(ProgressBarStyle style);
    ProgressBarStyle GetProgressBarStyle() const;

    // Progress bar values
    void SetValue(int value);
    int GetValue() const;
    
    void SetMinValue(int minValue);
    int GetMinValue() const { return m_minValue; }
    
    void SetMaxValue(int maxValue);
    int GetMaxValue() const { return m_maxValue; }
    
    void SetRange(int minValue, int maxValue);
    void GetRange(int& minValue, int& maxValue) const;
    
    double GetPercentage() const;

    // Colors
    void SetBackgroundColor(const wxColour& color);
    wxColour GetBackgroundColor() const;
    
    void SetProgressColor(const wxColour& color);
    wxColour GetProgressColor() const;
    
    void SetBorderColor(const wxColour& color);
    wxColour GetBorderColor() const;
    
    void SetTextColor(const wxColour& color);
    wxColour GetTextColor() const;
    
    void SetErrorColor(const wxColour& color);
    wxColour GetErrorColor() const { return m_errorColor; }
    
    void SetPausedColor(const wxColour& color);
    wxColour GetPausedColor() const { return m_pausedColor; }

    // Border and corner radius
    void SetBorderWidth(int width);
    int GetBorderWidth() const;
    
    void SetCornerRadius(int radius);
    int GetCornerRadius() const;

    // Progress bar height
    void SetBarHeight(int height);
    int GetBarHeight() const;

    // Progress bar properties
    void SetShowPercentage(bool show);
    bool IsShowPercentage() const;
    
    void SetShowValue(bool show);
    bool IsShowValue() const;
    
    void SetShowLabel(bool show);
    bool IsShowLabel() const;
    
    void SetStriped(bool striped);
    bool IsStriped() const;
    
    void SetAnimated(bool animated);
    bool IsAnimated() const;

    // State management
    void SetEnabled(bool enabled);
    bool IsEnabled() const;
    
    void SetState(ProgressBarState state);
    ProgressBarState GetState() const { return m_state; }

    // Size management
    virtual wxSize DoGetBestSize() const override;

protected:
    // Event handlers
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnTimer(wxTimerEvent& event);

    // Drawing methods
    void DrawBackground(wxDC& dc);
    void DrawProgress(wxDC& dc);
    void DrawText(wxDC& dc);
    void DrawIcon(wxDC& dc);
    void DrawRoundedRectangle(wxDC& dc, const wxRect& rect, int radius);
    void DrawStripes(wxDC& dc, const wxRect& rect);
    void DrawCircularProgress(wxDC& dc);
    void DrawProgressBar(wxDC& dc);

    // Helper methods
    void UpdateAnimation();
    void SendProgressEvent();
    wxRect GetProgressRect() const;
    wxRect GetTextRect() const;
    wxRect GetIconRect() const;
    wxColour GetCurrentProgressColor() const;
    wxColour GetCurrentBackgroundColor() const;
    wxColour GetCurrentBorderColor() const;
    wxColour GetCurrentTextColor() const;
    void InitializeDefaultColors();

private:
    // Progress bar content
    wxString m_label;
    wxBitmap m_icon;
    
    // Style and appearance
    ProgressBarStyle m_progressBarStyle;
    ProgressBarState m_state;
    bool m_enabled;
    
    // Progress bar values
    int m_value;
    int m_minValue;
    int m_maxValue;
    
    // Colors
    wxColour m_backgroundColor;
    wxColour m_progressColor;
    wxColour m_borderColor;
    wxColour m_textColor;
    wxColour m_errorColor;
    wxColour m_pausedColor;
    wxColour m_disabledColor;
    
    // Dimensions
    int m_borderWidth;
    int m_cornerRadius;
    int m_barHeight;
    
    // Display options
    bool m_showPercentage;
    bool m_showValue;
    bool m_showLabel;
    bool m_striped;
    bool m_animated;
    
    // Animation
    wxTimer m_animationTimer;
    double m_animationProgress;
    double m_stripeOffset;
    
    // Layout
    wxRect m_progressRect;
    wxRect m_textRect;
    wxRect m_iconRect;
    
    // Constants
    static const int DEFAULT_CORNER_RADIUS = 4;
    static const int DEFAULT_BORDER_WIDTH = 1;
    static const int DEFAULT_ANIMATION_INTERVAL = 50;
    static const int DEFAULT_BAR_HEIGHT = 20;

    wxDECLARE_EVENT_TABLE();
    wxDECLARE_NO_COPY_CLASS(FlatProgressBar);
};

#endif // FLAT_PROGRESS_BAR_H
