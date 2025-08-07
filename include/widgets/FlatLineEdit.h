#ifndef FLAT_LINE_EDIT_H
#define FLAT_LINE_EDIT_H

#include <wx/wx.h>
#include <wx/control.h>
#include <wx/textctrl.h>
#include <wx/bitmap.h>

// Forward declarations
class FlatLineEdit;

// Custom events
wxDECLARE_EVENT(wxEVT_FLAT_LINE_EDIT_TEXT_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_FLAT_LINE_EDIT_FOCUS_GAINED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_FLAT_LINE_EDIT_FOCUS_LOST, wxCommandEvent);

class FlatLineEdit : public wxControl
{
public:
    // Line edit styles inspired by PyQt-Fluent-Widgets
    enum class LineEditStyle {
        DEFAULT_STYLE,  // Normal text input
        SEARCH,         // Search input with search icon
        PASSWORD,       // Password input with show/hide toggle
        CLEARABLE       // Clearable input with clear button
    };

    // Line edit states
    enum class LineEditState {
        NORMAL,
        FOCUSED,
        HOVERED,
        DISABLED,
        HAS_ERROR
    };

    FlatLineEdit(wxWindow* parent, 
                 wxWindowID id = wxID_ANY,
                 const wxString& value = wxEmptyString,
                 const wxPoint& pos = wxDefaultPosition,
                 const wxSize& size = wxDefaultSize,
                 LineEditStyle style = LineEditStyle::DEFAULT_STYLE,
                 long style_flags = 0);

    virtual ~FlatLineEdit();

    // Text content
    void SetValue(const wxString& value);
    wxString GetValue() const;
    
    void SetPlaceholderText(const wxString& placeholder);
    wxString GetPlaceholderText() const { return m_placeholderText; }

    // Line edit style
    void SetLineEditStyle(LineEditStyle style);
    LineEditStyle GetLineEditStyle() const { return m_lineEditStyle; }

    // Colors
    void SetBackgroundColor(const wxColour& color);
    wxColour GetBackgroundColor() const { return m_backgroundColor; }
    
    void SetFocusedColor(const wxColour& color);
    wxColour GetFocusedColor() const { return m_focusedColor; }
    
    void SetBorderColor(const wxColour& color);
    wxColour GetBorderColor() const { return m_borderColor; }
    
    void SetTextColor(const wxColour& color);
    wxColour GetTextColor() const { return m_textColor; }
    
    void SetPlaceholderColor(const wxColour& color);
    wxColour GetPlaceholderColor() const { return m_placeholderColor; }

    // Border and corner radius
    void SetBorderWidth(int width);
    int GetBorderWidth() const { return m_borderWidth; }
    
    void SetCornerRadius(int radius);
    int GetCornerRadius() const { return m_cornerRadius; }
    
    void SetPadding(int horizontal, int vertical);
    void GetPadding(int& horizontal, int& vertical) const;

    // Icons
    void SetLeftIcon(const wxBitmap& icon);
    wxBitmap GetLeftIcon() const { return m_leftIcon; }
    
    void SetRightIcon(const wxBitmap& icon);
    wxBitmap GetRightIcon() const { return m_rightIcon; }

    // Password mode
    void SetPasswordMode(bool password);
    bool IsPasswordMode() const { return m_passwordMode; }
    
    void SetPasswordVisible(bool visible);
    bool IsPasswordVisible() const { return m_passwordVisible; }

    // Clear functionality
    void SetClearable(bool clearable);
    bool IsClearable() const { return m_clearable; }
    
    void Clear();

    // Font configuration
    void SetCustomFont(const wxFont& font);
    wxFont GetCustomFont() const { return m_customFont; }
    void UseConfigFont(bool useConfig = true);
    bool IsUsingConfigFont() const { return m_useConfigFont; }
    void ReloadFontFromConfig();

    // State management
    void SetEnabled(bool enabled);
    bool IsEnabled() const { return m_enabled; }
    
    void SetError(bool error);
    bool HasError() const { return m_hasError; }

    // Size management
    virtual wxSize DoGetBestSize() const override;

    // Text selection
    void SelectAll();
    void SelectText(long from, long to);
    wxString GetSelectedText() const;

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
    void OnTextChanged(wxCommandEvent& event);
    void OnFocusGained(wxFocusEvent& event);
    void OnFocusLost(wxFocusEvent& event);

    // Drawing methods
    void DrawBackground(wxDC& dc);
    void DrawBorder(wxDC& dc);
    void DrawText(wxDC& dc);
    void DrawPlaceholder(wxDC& dc);
    void DrawIcons(wxDC& dc);
    void DrawRoundedRectangle(wxDC& dc, const wxRect& rect, int radius);

    // Helper methods
    void UpdateState(LineEditState newState);
    void UpdateTextRect();
    void HandleIconClick(const wxPoint& pos);
    wxRect GetTextRect() const;
    wxRect GetLeftIconRect() const;
    wxRect GetRightIconRect() const;
    wxColour GetCurrentBackgroundColor() const;
    wxColour GetCurrentBorderColor() const;
    wxColour GetCurrentTextColor() const;
    void InitializeDefaultColors();

private:
    // Text content
    wxString m_value;
    wxString m_placeholderText;
    wxString m_displayValue; // For password mode
    
    // Style and appearance
    LineEditStyle m_lineEditStyle;
    LineEditState m_state;
    bool m_enabled;
    bool m_hasError;
    
    // Colors
    wxColour m_backgroundColor;
    wxColour m_focusedColor;
    wxColour m_borderColor;
    wxColour m_textColor;
    wxColour m_placeholderColor;
    wxColour m_errorColor;
    wxColour m_errorBorderColor;
    wxColour m_hoverColor;
    wxColour m_focusBorderColor;
    
    // Dimensions
    int m_borderWidth;
    int m_cornerRadius;
    int m_padding;
    int m_iconSpacing;
    int m_horizontalPadding;
    int m_verticalPadding;
    
    // Font configuration
    wxFont m_customFont;
    bool m_useConfigFont;
    
    // Icons
    wxBitmap m_leftIcon;
    wxBitmap m_rightIcon;
    wxBitmap m_clearIcon;
    wxBitmap m_passwordShowIcon;
    wxBitmap m_passwordHideIcon;
    wxBitmap m_searchIcon;
    
    // State tracking
    bool m_isFocused;
    bool m_isHovered;
    bool m_isPressed;
    bool m_passwordMode;
    bool m_passwordVisible;
    bool m_clearable;
    bool m_showClearButton;
    
    // Text selection
    long m_selectionStart;
    long m_selectionEnd;
    bool m_hasSelection;
    
    // Layout
    wxRect m_textRect;
    wxRect m_leftIconRect;
    wxRect m_rightIconRect;
    
    // Constants
    static const int DEFAULT_CORNER_RADIUS = 6;
    static const int DEFAULT_BORDER_WIDTH = 1;
    static const int DEFAULT_PADDING = 8;
    static const int DEFAULT_ICON_SPACING = 4;
    static const int DEFAULT_ICON_SIZE = 16;

    wxDECLARE_EVENT_TABLE();
    wxDECLARE_NO_COPY_CLASS(FlatLineEdit);
};

#endif // FLAT_LINE_EDIT_H
