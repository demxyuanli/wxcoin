#include "flatui/FlatUIButtonBar.h"
#include "flatui/FlatUIPanel.h"
#include "flatui/FlatUIEventManager.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/display.h>
#include <algorithm> // For std::min
#include "config/ThemeManager.h"  




int targetH = -1;

FlatUIButtonBar::FlatUIButtonBar(FlatUIPanel* parent)
    : wxControl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
    m_displayStyle(ButtonDisplayStyle::ICON_TEXT_BESIDE),
    m_buttonStyle(ButtonStyle::DEFAULT),
    m_buttonBorderStyle(ButtonBorderStyle::SOLID),
    m_buttonBgColour(CFG_COLOUR("ActBarBackgroundColour")),
    m_buttonHoverBgColour(CFG_COLOUR("ButtonbarDefaultHoverBgColour")),
    m_buttonPressedBgColour(CFG_COLOUR("ButtonbarDefaultPressedBgColour")),
    m_buttonTextColour(CFG_COLOUR("ButtonbarDefaultTextColour")),
    m_buttonBorderColour(CFG_COLOUR("ButtonbarDefaultBorderColour")),
    m_btnBarBgColour(CFG_COLOUR("ButtonbarDefaultBgColour")),
    m_btnBarBorderColour(CFG_COLOUR("ButtonbarDefaultBorderColour")), 
    m_buttonBorderWidth(CFG_INT("ButtonbarDefaultBorderWidth")),
    m_buttonCornerRadius(CFG_INT("ButtonbarDefaultCornerRadius")),
    m_buttonSpacing(CFG_INT("ButtonbarSpacing")),
    m_buttonHorizontalPadding(CFG_INT("ButtonbarHorizontalPadding")),
    m_buttonVerticalPadding(CFG_INT("ButtonbarInternalVerticalPadding")),
    m_btnBarBorderWidth(0),
    m_hoverEffectsEnabled(true)
{
    m_buttonBgColour = CFG_COLOUR("ActBarBackgroundColour");
    m_buttonHoverBgColour = CFG_COLOUR("ButtonbarDefaultHoverBgColour");
    m_buttonPressedBgColour = CFG_COLOUR("ButtonbarDefaultPressedBgColour");
    m_buttonTextColour = CFG_COLOUR("ButtonbarDefaultTextColour");
    m_buttonBorderColour = CFG_COLOUR("ButtonbarDefaultBorderColour");
    m_btnBarBgColour = CFG_COLOUR("ButtonbarDefaultBgColour");
    m_btnBarBorderColour = CFG_COLOUR("ButtonbarDefaultBorderColour");


    m_buttonBorderWidth = CFG_INT("ButtonbarDefaultBorderWidth");
    m_buttonCornerRadius = CFG_INT("ButtonbarDefaultCornerRadius");
    m_buttonSpacing = CFG_INT("ButtonbarSpacing");
    m_buttonHorizontalPadding = CFG_INT("ButtonbarHorizontalPadding");
    m_buttonVerticalPadding = CFG_INT("ButtonbarInternalVerticalPadding");


    m_dropdownArrowWidth = CFG_INT("ButtonbarDropdownArrowWidth");
    m_dropdownArrowHeight = CFG_INT("ButtonbarDropdownArrowHeight");


    m_separatorWidth = CFG_INT("ButtonbarSeparatorWidth");
    m_separatorPadding = CFG_INT("ButtonbarSeparatorPadding");
    m_separatorMargin = CFG_INT("ButtonbarSeparatorMargin");


    m_btnBarHorizontalMargin = CFG_INT("ButtonbarBarHorizontalMargin");

    targetH = CFG_INT("ButtonbarTargetHeight");

    SetFont(CFG_DEFAULTFONT());
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetMinSize(wxSize(targetH * 2, targetH));

    Bind(wxEVT_PAINT, &FlatUIButtonBar::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &FlatUIButtonBar::OnMouseDown, this);
    Bind(wxEVT_MOTION, &FlatUIButtonBar::OnMouseMove, this);
    Bind(wxEVT_LEAVE_WINDOW, &FlatUIButtonBar::OnMouseLeave, this);
    Bind(wxEVT_SIZE, &FlatUIButtonBar::OnSize, this);
}

FlatUIButtonBar::~FlatUIButtonBar() = default;

void FlatUIButtonBar::AddButton(int id, const wxString& label, const wxBitmap& bitmap, wxMenu* menu)
{
    Freeze();
    ButtonInfo button;
    button.id = id;
    button.label = label;
    int imgw = CFG_INT("ButtonbarIconSize");
    button.icon = bitmap;
    button.menu = menu;
    button.isDropDown = (menu != nullptr);

    wxClientDC dc(this);
    dc.SetFont(CFG_DEFAULTFONT());
    button.textSize = dc.GetTextExtent(label);

    m_buttons.push_back(button);
    RecalculateLayout();

    Thaw();
    Refresh();
}

int FlatUIButtonBar::CalculateButtonWidth(const ButtonInfo& button, wxDC& dc) const
{
    int buttonWidth = 0;
    int iconWidth = button.icon.IsOk() ? button.icon.GetWidth() : 0;
    const int STANDARD_BUTTON_SIZE = 24; // Standard button size 24x24

    switch (m_displayStyle) {
    case ButtonDisplayStyle::ICON_ONLY:
        // Fixed 24x24 button size regardless of icon size
        buttonWidth = STANDARD_BUTTON_SIZE;
        break;
    case ButtonDisplayStyle::TEXT_ONLY:
        // Text width + padding
        buttonWidth = !button.label.empty() ? button.textSize.GetWidth() + 2 * m_buttonHorizontalPadding
            : 2 * m_buttonHorizontalPadding;
        break;
    case ButtonDisplayStyle::ICON_TEXT_BELOW:
        // Button width is 24 + text width (whichever is larger) + padding
        buttonWidth = m_buttonHorizontalPadding;
        if (button.icon.IsOk() && !button.label.empty()) {
            buttonWidth += wxMax(STANDARD_BUTTON_SIZE, button.textSize.GetWidth());
        }
        else if (button.icon.IsOk()) {
            buttonWidth += STANDARD_BUTTON_SIZE;
        }
        else if (!button.label.empty()) {
            buttonWidth += button.textSize.GetWidth();
        }
        buttonWidth += m_buttonHorizontalPadding;
        break;
    case ButtonDisplayStyle::ICON_TEXT_BESIDE:
        // 24 (button area) + text width + padding
        buttonWidth = STANDARD_BUTTON_SIZE;
        if (!button.label.empty()) {
            buttonWidth += m_buttonHorizontalPadding + button.textSize.GetWidth();
        }
        buttonWidth += m_buttonHorizontalPadding;
        break;
    }

    // Add dropdown arrow and separator width if needed
    if (button.isDropDown) {
        buttonWidth += m_buttonHorizontalPadding + m_dropdownArrowWidth;
        buttonWidth += m_separatorWidth + 2 * m_separatorPadding;
    }

    return wxMax(buttonWidth, STANDARD_BUTTON_SIZE);
}

void FlatUIButtonBar::RecalculateLayout()
{
    Freeze();
    wxClientDC dc(this);
    dc.SetFont(CFG_DEFAULTFONT());
    int currentX = m_btnBarHorizontalMargin;
    const int STANDARD_BUTTON_HEIGHT = 24; // Standard button height

    for (auto& button : m_buttons) {
        if (button.textSize != dc.GetTextExtent(button.label)) {
            button.textSize = dc.GetTextExtent(button.label);
        }
        int buttonWidth = CalculateButtonWidth(button, dc);

        // Button height calculation based on display style
        int buttonHeight = STANDARD_BUTTON_HEIGHT;
        if (m_displayStyle == ButtonDisplayStyle::ICON_TEXT_BELOW) {
            // Icon above text mode needs extra height for text
            if (button.icon.IsOk() && !button.label.empty()) {
                buttonHeight = STANDARD_BUTTON_HEIGHT + button.textSize.GetHeight() + CFG_INT("IconTextBelowSpacing");
            }
        }

        int buttonY = CFG_INT("ButtonbarVerticalMargin");
        button.rect = wxRect(currentX, buttonY, buttonWidth, buttonHeight);
        currentX += buttonWidth;
        if (&button != &m_buttons.back()) {
            currentX += m_buttonSpacing;
        }
    }
    currentX += 2 * m_btnBarHorizontalMargin;

    // Calculate overall button bar height using Gallery's target height for consistency
    int galleryTargetHeight = CFG_INT("GalleryTargetHeight");
    int totalBarHeight = galleryTargetHeight + 2 * CFG_INT("ButtonbarVerticalMargin"); 

    wxSize currentMinSize = GetMinSize();
    if (currentMinSize.GetWidth() != currentX || currentMinSize.GetHeight() != totalBarHeight) {
        SetMinSize(wxSize(currentX, totalBarHeight));
        InvalidateBestSize();
        if (auto* parentPanel = dynamic_cast<FlatUIPanel*>(GetParent())) {
            parentPanel->UpdatePanelSize();
        }
        else {
            GetParent()->Layout();
        }
    }
    Thaw();
    Refresh();
}

void FlatUIButtonBar::SetDisplayStyle(ButtonDisplayStyle style)
{
    if (m_displayStyle != style) {
        m_displayStyle = style;
        RecalculateLayout();
        Refresh();
    }
}

wxSize FlatUIButtonBar::DoGetBestSize() const
{
    wxMemoryDC dc;
    dc.SetFont(CFG_DEFAULTFONT());
    int totalWidth = m_btnBarHorizontalMargin;

    for (const auto& button : m_buttons) {
        totalWidth += CalculateButtonWidth(button, dc);
        if (&button != &m_buttons.back()) {
            totalWidth += m_buttonSpacing;
        }
    }
    totalWidth += m_btnBarHorizontalMargin;

    if (m_buttons.empty()) {
        totalWidth = m_btnBarHorizontalMargin * 2;
        if (totalWidth == 0) totalWidth = 10;
    }

    // Use same height calculation as Gallery for consistency
    int galleryTargetHeight = CFG_INT("GalleryTargetHeight");
    int totalHeight = galleryTargetHeight + 2 * CFG_INT("ButtonbarVerticalMargin");
    return wxSize(totalWidth, totalHeight);
}

void FlatUIButtonBar::OnPaint(wxPaintEvent& evt)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(m_btnBarBgColour);
    dc.Clear();

    if (m_buttons.empty() && IsShown()) {
        dc.SetTextForeground(CFG_COLOUR("ButtonTextPlaceholderColour"));
        wxString text = "BtnBar";
        wxSize textSize = dc.GetTextExtent(text);
        dc.DrawText(text, (GetSize().GetWidth() - textSize.GetWidth()) / 2,
            (GetSize().GetHeight() - textSize.GetHeight()) / 2);
        return;
    }

    dc.SetFont(CFG_DEFAULTFONT());
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        DrawButton(dc, m_buttons[i], i);
    }
}

void FlatUIButtonBar::DrawButton(wxDC& dc, const ButtonInfo& button, int index)
{
    bool isHovered = m_hoverEffectsEnabled && index == m_hoveredButtonIndex;
    bool isPressed = button.pressed;

    if (m_buttonStyle != ButtonStyle::GHOST || isHovered || isPressed) {
        DrawButtonBackground(dc, button.rect, isHovered, isPressed);
    }

    if (m_buttonStyle == ButtonStyle::OUTLINED ||
        m_buttonStyle == ButtonStyle::RAISED ||
        (m_buttonStyle == ButtonStyle::DEFAULT && (isHovered || isPressed))) {
        DrawButtonBorder(dc, button.rect, isHovered, isPressed);
    }

    dc.SetTextForeground(m_buttonTextColour);
    DrawButtonIcon(dc, button, button.rect);
    DrawButtonText(dc, button, button.rect);
    if (button.isDropDown) {
        DrawButtonSeparator(dc, button, button.rect); // Draw separator before arrow
        DrawButtonDropdownArrow(dc, button, button.rect);
    }
}

void FlatUIButtonBar::DrawButtonIcon(wxDC& dc, const ButtonInfo& button, const wxRect& rect)
{
    if (!button.icon.IsOk()) return;

    int iconWidth = button.icon.GetWidth();
    int iconHeight = button.icon.GetHeight();
    const int STANDARD_BUTTON_SIZE = 24;

    switch (m_displayStyle) {
    case ButtonDisplayStyle::ICON_ONLY:
    {
        // Center the original-sized icon within the 24x24 button area
        int iconX = rect.GetLeft() + (STANDARD_BUTTON_SIZE - iconWidth) / 2;
        int iconY = rect.GetTop() + (STANDARD_BUTTON_SIZE - iconHeight) / 2;
        dc.DrawBitmap(button.icon, iconX, iconY, true);
        break;
    }
    case ButtonDisplayStyle::ICON_TEXT_BELOW:
    {
        // Center the icon horizontally within the button width
        int iconX = rect.GetLeft() + (rect.GetWidth() - iconWidth) / 2;
        int iconY = rect.GetTop() + CFG_INT("IconTextBelowTopMargin");
        dc.DrawBitmap(button.icon, iconX, iconY, true);
        break;
    }
    case ButtonDisplayStyle::ICON_TEXT_BESIDE:
    {
        // Center the icon within the 24px button area on the left
        int iconX = rect.GetLeft() + (STANDARD_BUTTON_SIZE - iconWidth) / 2;
        int iconY = rect.GetTop() + (rect.GetHeight() - iconHeight) / 2;
        dc.DrawBitmap(button.icon, iconX, iconY, true);
        break;
    }
    default:
        break;
    }
}

void FlatUIButtonBar::DrawButtonText(wxDC& dc, const ButtonInfo& button, const wxRect& rect)
{
    if (button.label.empty()) return;

    const int STANDARD_BUTTON_SIZE = 24;

    switch (m_displayStyle) {
    case ButtonDisplayStyle::TEXT_ONLY:
    {
        int textX = rect.GetLeft() + (rect.GetWidth() - button.textSize.GetWidth()) / 2;
        int textY = rect.GetTop() + (rect.GetHeight() - button.textSize.GetHeight()) / 2;
        dc.DrawText(button.label, textX, textY);
        break;
    }
    case ButtonDisplayStyle::ICON_TEXT_BELOW:
    {
        int textX = rect.GetLeft() + (rect.GetWidth() - button.textSize.GetWidth()) / 2;
        int textY = rect.GetTop() + CFG_INT("IconTextBelowTopMargin") +
            (button.icon.IsOk() ? button.icon.GetHeight() + CFG_INT("IconTextBelowSpacing") : 0);
        if (textY + button.textSize.GetHeight() <= rect.GetBottom()) {
            dc.DrawText(button.label, textX, textY);
        }
        break;
    }
    case ButtonDisplayStyle::ICON_TEXT_BESIDE:
    {
        // Text starts after the 24px button area
        int textX = rect.GetLeft() + STANDARD_BUTTON_SIZE + m_buttonHorizontalPadding;
        int textY = rect.GetTop() + (rect.GetHeight() - button.textSize.GetHeight()) / 2;
        dc.DrawText(button.label, textX, textY);
        break;
    }
    default:
        break;
    }
}

void FlatUIButtonBar::DrawButtonDropdownArrow(wxDC& dc, const ButtonInfo& button, const wxRect& rect)
{
    int arrowX = rect.GetRight() - m_buttonHorizontalPadding - m_dropdownArrowWidth;
    int arrowY = rect.GetTop() + (rect.GetHeight() - m_dropdownArrowHeight) / 2;
    wxPoint pts[3] = {
        {arrowX, arrowY},
        {arrowX + m_dropdownArrowWidth, arrowY},
        {arrowX + m_dropdownArrowWidth / 2, arrowY + m_dropdownArrowHeight}
    };
    dc.SetBrush(wxBrush(m_buttonTextColour));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawPolygon(3, pts);
}

void FlatUIButtonBar::DrawButtonSeparator(wxDC& dc, const ButtonInfo& button, const wxRect& rect)
{
    int sepX = rect.GetRight() - m_buttonHorizontalPadding - m_dropdownArrowWidth
        - m_separatorPadding - m_separatorWidth;
    int topY = rect.GetTop() + m_separatorMargin;
    int botY = rect.GetBottom() - m_separatorMargin;
    dc.SetPen(wxPen(m_buttonBorderColour, m_separatorWidth));
    dc.DrawLine(sepX, topY, sepX, botY);
}

void FlatUIButtonBar::DrawButtonBackground(wxDC& dc, const wxRect& rect, bool isHovered, bool isPressed)
{
    wxColour bgColour = isPressed && m_hoverEffectsEnabled ? m_buttonPressedBgColour :
        isHovered && m_hoverEffectsEnabled ? m_buttonHoverBgColour :
        m_buttonBgColour;

    dc.SetBrush(wxBrush(bgColour));
    dc.SetPen(*wxTRANSPARENT_PEN);

    if (m_buttonStyle == ButtonStyle::PILL ||
        (m_buttonBorderStyle == ButtonBorderStyle::ROUNDED && m_buttonCornerRadius > 0)) {
        dc.DrawRoundedRectangle(rect, m_buttonCornerRadius);
    }
    else {
        dc.DrawRectangle(rect);
    }

    if (m_buttonStyle == ButtonStyle::RAISED && !isPressed) {
        wxColour shadowColour = bgColour.ChangeLightness(70);
        dc.SetPen(wxPen(shadowColour, 1));
        dc.DrawLine(rect.GetLeft() + 1, rect.GetBottom(), rect.GetRight(), rect.GetBottom());
        dc.DrawLine(rect.GetRight(), rect.GetTop() + 1, rect.GetRight(), rect.GetBottom());
    }
}

void FlatUIButtonBar::DrawButtonBorder(wxDC& dc, const wxRect& rect, bool isHovered, bool isPressed)
{
    wxColour borderColour = isHovered && m_hoverEffectsEnabled ? m_buttonBorderColour.ChangeLightness(80)
        : m_buttonBorderColour;
    wxRect innerRect = rect;

    switch (m_buttonBorderStyle) {
    case ButtonBorderStyle::SOLID:
        dc.SetPen(wxPen(borderColour, m_buttonBorderWidth));
        break;
    case ButtonBorderStyle::DASHED:
        dc.SetPen(wxPen(borderColour, m_buttonBorderWidth, wxPENSTYLE_SHORT_DASH));
        break;
    case ButtonBorderStyle::DOTTED:
        dc.SetPen(wxPen(borderColour, m_buttonBorderWidth, wxPENSTYLE_DOT));
        break;
    case ButtonBorderStyle::DOUBLE:
        dc.SetPen(wxPen(borderColour, 1));
        dc.DrawRectangle(rect);
        innerRect.Deflate(2);
        dc.DrawRectangle(innerRect);
        return;
    case ButtonBorderStyle::ROUNDED:
        dc.SetPen(wxPen(borderColour, m_buttonBorderWidth));
        break;
    }

    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    if (m_buttonStyle == ButtonStyle::PILL ||
        (m_buttonBorderStyle == ButtonBorderStyle::ROUNDED && m_buttonCornerRadius > 0)) {
        dc.DrawRoundedRectangle(rect, m_buttonCornerRadius);
    }
    else {
        dc.DrawRectangle(rect);
    }
}

void FlatUIButtonBar::OnMouseMove(wxMouseEvent& evt)
{
    if (!m_hoverEffectsEnabled) return;

    int oldHoveredIndex = m_hoveredButtonIndex;
    m_hoveredButtonIndex = -1;
    wxPoint pos = evt.GetPosition();

    for (size_t i = 0; i < m_buttons.size(); ++i) {
        if (m_buttons[i].rect.Contains(pos)) {
            m_hoveredButtonIndex = i;
            break;
        }
    }

    if (oldHoveredIndex != m_hoveredButtonIndex) {
        Refresh();
    }
}

void FlatUIButtonBar::OnMouseLeave(wxMouseEvent& evt)
{
    if (m_hoveredButtonIndex != -1) {
        m_hoveredButtonIndex = -1;
        Refresh();
    }
}

void FlatUIButtonBar::OnMouseDown(wxMouseEvent& evt)
{
    wxPoint pos = evt.GetPosition();

    for (const auto& button : m_buttons) {
        if (button.rect.Contains(pos)) {
            if (button.menu) {
                // Align menu with button's left-bottom corner
                wxPoint menuPos = button.rect.GetBottomLeft();
                menuPos.y += CFG_INT("ButtonbarMenuVerticalOffset");
                // Log client and screen coordinates
                wxPoint screenMenuPos = ClientToScreen(menuPos);
                PopupMenu(button.menu, menuPos);
            }
            else {
                wxCommandEvent event(wxEVT_BUTTON, button.id);
                event.SetEventObject(this);
                GetParent()->ProcessWindowEvent(event);
            }
            break;
        }
    }
}

void FlatUIButtonBar::OnSize(wxSizeEvent& evt)
{
    RecalculateLayout();
}

void FlatUIButtonBar::SetButtonStyle(ButtonStyle style)
{
    if (m_buttonStyle != style) {
        m_buttonStyle = style;
        Refresh();
    }
}

void FlatUIButtonBar::SetButtonBorderStyle(ButtonBorderStyle style)
{
    if (m_buttonBorderStyle != style) {
        m_buttonBorderStyle = style;
        Refresh();
    }
}

void FlatUIButtonBar::SetButtonBackgroundColour(const wxColour& colour)
{
    m_buttonBgColour = colour;
    Refresh();
}

void FlatUIButtonBar::SetButtonHoverBackgroundColour(const wxColour& colour)
{
    m_buttonHoverBgColour = colour;
    Refresh();
}

void FlatUIButtonBar::SetButtonPressedBackgroundColour(const wxColour& colour)
{
    m_buttonPressedBgColour = colour;
    Refresh();
}

void FlatUIButtonBar::SetButtonTextColour(const wxColour& colour)
{
    m_buttonTextColour = colour;
    Refresh();
}

void FlatUIButtonBar::SetButtonBorderColour(const wxColour& colour)
{
    m_buttonBorderColour = colour;
    Refresh();
}

void FlatUIButtonBar::SetButtonBorderWidth(int width)
{
    m_buttonBorderWidth = width;
    Refresh();
}

void FlatUIButtonBar::SetButtonCornerRadius(int radius)
{
    m_buttonCornerRadius = radius;
    Refresh();
}

void FlatUIButtonBar::SetButtonSpacing(int spacing)
{
    if (m_buttonSpacing != spacing) {
        m_buttonSpacing = spacing;
        RecalculateLayout();
    }
}

void FlatUIButtonBar::SetButtonPadding(int horizontal, int vertical)
{
    if (m_buttonHorizontalPadding != horizontal || m_buttonVerticalPadding != vertical) {
        m_buttonHorizontalPadding = horizontal;
        m_buttonVerticalPadding = vertical;
        RecalculateLayout();
    }
}

void FlatUIButtonBar::GetButtonPadding(int& horizontal, int& vertical) const
{
    horizontal = m_buttonHorizontalPadding;
    vertical = m_buttonVerticalPadding;
}

void FlatUIButtonBar::SetBtnBarBackgroundColour(const wxColour& colour)
{
    m_btnBarBgColour = colour;
    Refresh();
}

void FlatUIButtonBar::SetBtnBarBorderColour(const wxColour& colour)
{
    m_btnBarBorderColour = colour;
    Refresh();
}

void FlatUIButtonBar::SetBtnBarBorderWidth(int width)
{
    m_btnBarBorderWidth = width;
    Refresh();
}

void FlatUIButtonBar::SetHoverEffectsEnabled(bool enabled)
{
    if (m_hoverEffectsEnabled != enabled) {
        m_hoverEffectsEnabled = enabled;
        m_hoveredButtonIndex = -1;
        Refresh();
    }
}
