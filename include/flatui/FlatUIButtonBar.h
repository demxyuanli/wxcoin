#ifndef FLATUIBUTTONBAR_H
#define FLATUIBUTTONBAR_H

#include <wx/wx.h>
#include <wx/artprov.h>
#include <wx/vector.h>
#include <wx/menu.h>
#include <wx/dcbuffer.h>
#include "logger/Logger.h"

class FlatUIPanel;

enum class ButtonDisplayStyle {
    ICON_ONLY,
    TEXT_ONLY,
    ICON_TEXT_BESIDE,
    ICON_TEXT_BELOW
};

class FlatUIButtonBar : public wxControl
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

    FlatUIButtonBar(FlatUIPanel* parent);
    virtual ~FlatUIButtonBar();

    void AddButton(int id, const wxString& label, const wxBitmap& bitmap = wxNullBitmap, wxMenu* menu = nullptr);
    size_t GetButtonCount() const { return m_buttons.size(); }

    void SetDisplayStyle(ButtonDisplayStyle style);
    ButtonDisplayStyle GetDisplayStyle() const { return m_displayStyle; }

    void SetButtonStyle(ButtonStyle style);
    ButtonStyle GetButtonStyle() const { return m_buttonStyle; }

    void SetButtonBorderStyle(ButtonBorderStyle style);
    ButtonBorderStyle GetButtonBorderStyle() const { return m_buttonBorderStyle; }

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

    void OnPaint(wxPaintEvent& evt);
    void OnMouseDown(wxMouseEvent& evt);

protected:
    wxSize DoGetBestSize() const override;

private:
    struct ButtonInfo {
        int id;
        wxString label;
        wxBitmap icon;
        wxRect rect;
        wxMenu* menu = nullptr;
        bool isDropDown = false;
        bool hovered = false;
        bool pressed = false;
        wxSize textSize; // Cached text extent
    };
    wxVector<ButtonInfo> m_buttons;
    ButtonDisplayStyle m_displayStyle;
    ButtonStyle m_buttonStyle;
    ButtonBorderStyle m_buttonBorderStyle;
    wxColour m_buttonBgColour;
    wxColour m_buttonHoverBgColour;
    wxColour m_buttonPressedBgColour;
    wxColour m_buttonTextColour;
    wxColour m_buttonBorderColour;
    wxColour m_btnBarBgColour;
    wxColour m_btnBarBorderColour;
    int m_buttonBorderWidth;
    int m_buttonCornerRadius;
    int m_buttonSpacing;
    int m_buttonHorizontalPadding;
    int m_buttonVerticalPadding;
    int m_btnBarBorderWidth;
    int m_dropdownArrowWidth;
    int m_dropdownArrowHeight;
    int m_separatorWidth;
    int m_separatorPadding;
    int m_separatorMargin;
    int m_btnBarHorizontalMargin;
    bool m_hoverEffectsEnabled;
    int m_hoveredButtonIndex = -1;

    void RecalculateLayout();
    int CalculateButtonWidth(const ButtonInfo& button, wxDC& dc) const;
    void DrawButton(wxDC& dc, const ButtonInfo& button, int index);
    void DrawButtonBackground(wxDC& dc, const wxRect& rect, bool isHovered, bool isPressed);
    void DrawButtonBorder(wxDC& dc, const wxRect& rect, bool isHovered, bool isPressed);
    void DrawButtonIcon(wxDC& dc, const ButtonInfo& button, const wxRect& rect);
    void DrawButtonText(wxDC& dc, const ButtonInfo& button, const wxRect& rect);
    void DrawButtonDropdownArrow(wxDC& dc, const ButtonInfo& button, const wxRect& rect);
    void DrawButtonSeparator(wxDC& dc, const ButtonInfo& button, const wxRect& rect); // New method


    void OnMouseMove(wxMouseEvent& evt);
    void OnMouseLeave(wxMouseEvent& evt);
    void OnSize(wxSizeEvent& evt);
};

#endif // FLATUIBUTTONBAR_H