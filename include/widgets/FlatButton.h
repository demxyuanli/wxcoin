#ifndef FLAT_BUTTON_H
#define FLAT_BUTTON_H

#include <wx/wx.h>
#include <wx/control.h>
#include <wx/bitmap.h>
#include <wx/timer.h>

// Forward declarations
class FlatButton;

// Custom events
wxDECLARE_EVENT(wxEVT_FLAT_BUTTON_CLICKED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_FLAT_BUTTON_HOVER, wxCommandEvent);

class FlatButton : public wxControl
{
public:
    // Button styles inspired by PyQt-Fluent-Widgets
    enum class ButtonStyle {
        PRIMARY,        // Primary button with accent color
        SECONDARY,      // Secondary button with subtle background
        DEFAULT_TRANSPARENT,    // Transparent button
        OUTLINE,        // Outlined button
        TEXT,           // Text-only button
        ICON_ONLY,      // Icon-only button
        ICON_WITH_TEXT  // Icon with text button
    };

    // Button states
    enum class ButtonState {
        NORMAL,
        HOVERED,
        PRESSED,
        DISABLED
    };

    FlatButton(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxString& label = wxEmptyString,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        ButtonStyle style = ButtonStyle::PRIMARY,
        long style_flags = 0);

    virtual ~FlatButton();

    // Button content
    void SetLabel(const wxString& label);
    wxString GetLabel() const { return m_label; }

    void SetIcon(const wxBitmap& icon);
    wxBitmap GetIcon() const { return m_icon; }

    void SetIconSize(const wxSize& size);
    wxSize GetIconSize() const { return m_iconSize; }

    // Button style
    void SetButtonStyle(ButtonStyle style);
    ButtonStyle GetButtonStyle() const { return m_buttonStyle; }

    // Colors
    void SetBackgroundColor(const wxColour& color);
    wxColour GetBackgroundColor() const { return m_backgroundColor; }

    void SetHoverColor(const wxColour& color);
    wxColour GetHoverColor() const { return m_hoverColor; }

    void SetPressedColor(const wxColour& color);
    wxColour GetPressedColor() const { return m_pressedColor; }

    void SetTextColor(const wxColour& color);
    wxColour GetTextColor() const { return m_textColor; }

    void SetBorderColor(const wxColour& color);
    wxColour GetBorderColor() const { return m_borderColor; }

    // Border and corner radius
    void SetBorderWidth(int width);
    int GetBorderWidth() const { return m_borderWidth; }

    void SetCornerRadius(int radius);
    int GetCornerRadius() const { return m_cornerRadius; }

    // Spacing and padding
    void SetIconTextSpacing(int spacing);
    int GetIconTextSpacing() const { return m_iconTextSpacing; }

    void SetPadding(int horizontal, int vertical);
    void GetPadding(int& horizontal, int& vertical) const;

    // Font configuration
    void SetCustomFont(const wxFont& font);
    wxFont GetCustomFont() const { return m_customFont; }
    void UseConfigFont(bool useConfig = true);
    bool IsUsingConfigFont() const { return m_useConfigFont; }
    void ReloadFontFromConfig();

    // State management
    void SetEnabled(bool enabled);
    bool IsEnabled() const { return m_enabled; }

    void SetPressed(bool pressed);
    bool IsPressed() const { return m_state == ButtonState::PRESSED; }

    // Size management
    virtual wxSize DoGetBestSize() const override;

protected:
    // Event handlers
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnEraseBackground(wxEraseEvent& event);
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
    void DrawBorder(wxGraphicsContext& gc);
    void DrawText(wxGraphicsContext& gc);
    void DrawIcon(wxGraphicsContext& gc);
    void DrawRoundedRectangle(wxGraphicsContext& gc, const wxRect& rect, int radius);
    void DrawSubtleShadow(wxGraphicsContext& gc, const wxRect& rect);

    // Helper methods
    void UpdateState(ButtonState newState);
    void SendButtonEvent();
    wxRect GetTextRect() const;
    wxRect GetIconRect() const;
    wxColour GetCurrentBackgroundColor() const;
    wxColour GetCurrentTextColor() const;
    wxColour GetCurrentBorderColor() const;
    void InitializeDefaultColors();

private:
    // Button content
    wxString m_label;
    wxBitmap m_icon;
    wxSize m_iconSize;

    // Style and appearance
    ButtonStyle m_buttonStyle;
    ButtonState m_state;
    bool m_enabled;

    // Colors
    wxColour m_backgroundColor;
    wxColour m_hoverColor;
    wxColour m_pressedColor;
    wxColour m_textColor;
    wxColour m_borderColor;
    wxColour m_disabledColor;

    // Dimensions
    int m_borderWidth;
    int m_cornerRadius;
    int m_iconTextSpacing;
    int m_horizontalPadding;
    int m_verticalPadding;

    // Font configuration
    wxFont m_customFont;
    bool m_useConfigFont;

    // State tracking
    bool m_isPressed;
    bool m_isHovered;
    bool m_hasFocus;

    // Animation support
    void OnAnimationTimer(wxTimerEvent& event);
    wxTimer m_animationTimer;
    double m_animationProgress;

    // Constants
    static const int DEFAULT_CORNER_RADIUS = 6;
    static const int DEFAULT_BORDER_WIDTH = 1;
    static const int DEFAULT_ICON_TEXT_SPACING = 8;
    static const int DEFAULT_PADDING_H = 16;
    static const int DEFAULT_PADDING_V = 8;
    static const int DEFAULT_ICON_SIZE = 16;

    wxDECLARE_EVENT_TABLE();
    wxDECLARE_NO_COPY_CLASS(FlatButton);
};

#endif // FLAT_BUTTON_H
