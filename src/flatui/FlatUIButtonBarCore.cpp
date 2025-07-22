#include "flatui/FlatUIButtonBar.h"
#include "flatui/FlatUIPanel.h"
#include "flatui/FlatUIEventManager.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/display.h>
#include <algorithm>
#include "config/ThemeManager.h"

int targetH = -1;

FlatUIButtonBar::FlatUIButtonBar(FlatUIPanel* parent)
    : FlatUIThemeAware(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
    m_displayStyle(ButtonDisplayStyle::ICON_TEXT_BESIDE),
    m_buttonStyle(ButtonStyle::DEFAULT),
    m_buttonBorderStyle(ButtonBorderStyle::SOLID),
    m_buttonBgColour(GetThemeColour("ActBarBackgroundColour")),
    m_buttonHoverBgColour(GetThemeColour("ActBarHoverBackgroundColour")),
    m_buttonPressedBgColour(GetThemeColour("ActBarPressedBackgroundColour")),
    m_buttonTextColour(GetThemeColour("ActBarTextColour")),
    m_buttonBorderColour(GetThemeColour("ActBarBorderColour")),
    m_buttonBorderWidth(GetThemeInt("ActBarBorderWidth")),
    m_buttonCornerRadius(GetThemeInt("ActBarCornerRadius")),
    m_buttonSpacing(GetThemeInt("ActBarButtonSpacing")),
    m_buttonHorizontalPadding(GetThemeInt("ActBarButtonPaddingHorizontal")),
    m_buttonVerticalPadding(GetThemeInt("ActBarButtonPaddingVertical")),
    m_btnBarBgColour(GetThemeColour("ActBarBackgroundColour")),
    m_btnBarBorderColour(GetThemeColour("ActBarBorderColour")),
    m_btnBarBorderWidth(GetThemeInt("ActBarBorderWidth")),
    m_dropdownArrowWidth(GetThemeInt("ButtonbarDropdownArrowWidth")),
    m_dropdownArrowHeight(GetThemeInt("ButtonbarDropdownArrowHeight")),
    m_separatorWidth(GetThemeInt("ButtonbarSeparatorWidth")),
    m_separatorPadding(GetThemeInt("ButtonbarSeparatorPadding")),
    m_separatorMargin(GetThemeInt("ButtonbarSeparatorMargin")),
    m_btnBarHorizontalMargin(GetThemeInt("ButtonbarHorizontalMargin")),
    m_hoverEffectsEnabled(true),
    m_hoveredButtonIndex(-1),
    m_pressedButtonIndex(-1)
{
    // Additional configuration setup (missing in refactor)
    m_buttonBgColour = GetThemeColour("ActBarBackgroundColour");
    m_buttonHoverBgColour = GetThemeColour("ButtonbarDefaultHoverBgColour");
    m_buttonPressedBgColour = GetThemeColour("ButtonbarDefaultPressedBgColour");
    m_buttonTextColour = GetThemeColour("ButtonbarDefaultTextColour");
    m_buttonBorderColour = GetThemeColour("ButtonbarDefaultBorderColour");
    m_btnBarBgColour = GetThemeColour("ButtonbarDefaultBgColour");
    m_btnBarBorderColour = GetThemeColour("ButtonbarDefaultBorderColour");
    
    m_buttonBorderWidth = GetThemeInt("ButtonbarDefaultBorderWidth");
    m_buttonCornerRadius = GetThemeInt("ButtonbarDefaultCornerRadius");
    m_buttonSpacing = GetThemeInt("ButtonbarSpacing");
    m_buttonHorizontalPadding = GetThemeInt("ButtonbarHorizontalPadding");
    m_buttonVerticalPadding = GetThemeInt("ButtonbarInternalVerticalPadding");
    
    m_dropdownArrowWidth = GetThemeInt("ButtonbarDropdownArrowWidth");
    m_dropdownArrowHeight = GetThemeInt("ButtonbarDropdownArrowHeight");
    m_separatorWidth = GetThemeInt("ButtonbarSeparatorWidth");
    m_separatorPadding = GetThemeInt("ButtonbarSeparatorPadding");
    m_separatorMargin = GetThemeInt("ButtonbarSeparatorMargin");
    m_btnBarHorizontalMargin = GetThemeInt("ButtonbarBarHorizontalMargin");
    
    targetH = GetThemeInt("ButtonbarTargetHeight");
    
    SetFont(GetThemeFont());
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(m_btnBarBgColour);
    SetMinSize(wxSize(targetH * 2, targetH));
    
    Bind(wxEVT_PAINT, &FlatUIButtonBar::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &FlatUIButtonBar::OnMouseDown, this);
    Bind(wxEVT_MOTION, &FlatUIButtonBar::OnMouseMove, this);
    Bind(wxEVT_LEAVE_WINDOW, &FlatUIButtonBar::OnMouseLeave, this);
    Bind(wxEVT_SIZE, &FlatUIButtonBar::OnSize, this);
}

FlatUIButtonBar::~FlatUIButtonBar() {
    // Clean up any resources if needed
}

void FlatUIButtonBar::OnThemeChanged()
{
    // Update all theme-based colors and settings
    m_buttonBgColour = GetThemeColour("ActBarBackgroundColour");
    m_buttonHoverBgColour = GetThemeColour("ButtonbarDefaultHoverBgColour");
    m_buttonPressedBgColour = GetThemeColour("ButtonbarDefaultPressedBgColour");
    m_buttonTextColour = GetThemeColour("ButtonbarDefaultTextColour");
    m_buttonBorderColour = GetThemeColour("ButtonbarDefaultBorderColour");
    m_btnBarBgColour = GetThemeColour("ButtonbarDefaultBgColour");
    m_btnBarBorderColour = GetThemeColour("ButtonbarDefaultBorderColour");
    
    m_buttonBorderWidth = GetThemeInt("ButtonbarDefaultBorderWidth");
    m_buttonCornerRadius = GetThemeInt("ButtonbarDefaultCornerRadius");
    m_buttonSpacing = GetThemeInt("ButtonbarSpacing");
    m_buttonHorizontalPadding = GetThemeInt("ButtonbarHorizontalPadding");
    m_buttonVerticalPadding = GetThemeInt("ButtonbarInternalVerticalPadding");
    
    m_dropdownArrowWidth = GetThemeInt("ButtonbarDropdownArrowWidth");
    m_dropdownArrowHeight = GetThemeInt("ButtonbarDropdownArrowHeight");
    m_separatorWidth = GetThemeInt("ButtonbarSeparatorWidth");
    m_separatorPadding = GetThemeInt("ButtonbarSeparatorPadding");
    m_separatorMargin = GetThemeInt("ButtonbarSeparatorMargin");
    m_btnBarHorizontalMargin = GetThemeInt("ButtonbarBarHorizontalMargin");
    
    targetH = GetThemeInt("ButtonbarTargetHeight");
    
    // Update control properties
    SetFont(GetThemeFont());
    SetBackgroundColour(m_btnBarBgColour);
    SetMinSize(wxSize(targetH * 2, targetH));
    
    // Force refresh
    Refresh(true);
    Update();
}

// Basic button management
void FlatUIButtonBar::AddButton(int id, const wxString& label, const wxBitmap& bitmap, wxMenu* menu, const wxString& tooltip) {
    Freeze();
    ButtonInfo button(id, ButtonType::NORMAL);
    button.label = label;
    button.icon = bitmap;
    button.menu = menu;
    button.isDropDown = (menu != nullptr);
    button.tooltip = tooltip;
    
    // Calculate text size immediately (from original)
    wxClientDC dc(this);
    dc.SetFont(CFG_DEFAULTFONT());
    button.textSize = dc.GetTextExtent(label);
    
    m_buttons.push_back(button);
    RecalculateLayout();
    
    Thaw();
    Refresh();
}

void FlatUIButtonBar::AddButton(int id, ButtonType type, const wxString& label, const wxBitmap& bitmap, const wxString& tooltip) {
    ButtonInfo button(id, type);
    button.label = label;
    button.icon = bitmap;
    button.tooltip = tooltip;
    
    m_buttons.push_back(button);
    RecalculateLayout();
    Refresh();
}

void FlatUIButtonBar::AddSeparator() {
    ButtonInfo separator(wxID_SEPARATOR, ButtonType::SEPARATOR);
    separator.label = wxEmptyString;
    
    m_buttons.push_back(separator);
    RecalculateLayout();
    Refresh();
}

void FlatUIButtonBar::RemoveButton(int id) {
    for (auto it = m_buttons.begin(); it != m_buttons.end(); ++it) {
        if (it->id == id) {
            m_buttons.erase(it);
            RecalculateLayout();
            Refresh();
            break;
        }
    }
}

void FlatUIButtonBar::Clear() {
    m_buttons.clear();
    m_hoveredButtonIndex = -1;
    RecalculateLayout();
    Refresh();
}

void FlatUIButtonBar::SetButtonTooltip(int id, const wxString& tooltip) {
    ButtonInfo* button = FindButton(id);
    if (button) {
        button->tooltip = tooltip;
    }
}

// Layout calculations
int FlatUIButtonBar::CalculateButtonWidth(const ButtonInfo& button, wxDC& dc) const {
    if (button.type == ButtonType::SEPARATOR) {
        return m_separatorWidth + m_separatorPadding * 2;
    }

    int width = m_buttonHorizontalPadding * 2;
    
    if (m_displayStyle == ButtonDisplayStyle::ICON_ONLY) {
        if (button.icon.IsOk()) {
            width += button.icon.GetWidth();
        } else {
            width += 16; // Default icon size
        }
    }
    else if (m_displayStyle == ButtonDisplayStyle::TEXT_ONLY) {
        if (!button.label.IsEmpty()) {
            width += dc.GetTextExtent(button.label).GetWidth();
        }
    }
    else if (m_displayStyle == ButtonDisplayStyle::ICON_TEXT_BESIDE) {
        if (button.icon.IsOk()) {
            width += button.icon.GetWidth();
            if (!button.label.IsEmpty()) {
                width += CFG_INT("ActBarIconTextSpacing");
            }
        }
        if (!button.label.IsEmpty()) {
            width += dc.GetTextExtent(button.label).GetWidth();
        }
    }
    else if (m_displayStyle == ButtonDisplayStyle::ICON_TEXT_BELOW) {
        int iconWidth = button.icon.IsOk() ? button.icon.GetWidth() : 0;
        int textWidth = !button.label.IsEmpty() ? dc.GetTextExtent(button.label).GetWidth() : 0;
        width += wxMax(iconWidth, textWidth);
    }

    // Additional width for dropdown arrow
    if (button.isDropDown) {
        width += m_dropdownArrowWidth + CFG_INT("ActBarDropdownArrowSpacing");
    }
    
    // Additional width for choice control dropdown arrow
    if (button.type == ButtonType::CHOICE) {
        width += 20; // Space for dropdown arrow
    }

    return wxMax(width, CFG_INT("ActBarMinButtonWidth"));
}

void FlatUIButtonBar::RecalculateLayout() {
    wxClientDC dc(this);
    wxSize clientSize = GetClientSize();
    
    int x = m_btnBarHorizontalMargin;
    int y = 0;
    int maxHeight = targetH;
    
    for (auto& button : m_buttons) {
        if (!button.visible) {
            button.rect = wxRect(0, 0, 0, 0);
            continue;
        }
        
        int buttonWidth = CalculateButtonWidth(button, dc);
        int buttonHeight = maxHeight;
        
        button.rect = wxRect(x, y, buttonWidth, buttonHeight);
        x += buttonWidth + m_buttonSpacing;
        
        // Cache text size for faster drawing
        if (!button.label.IsEmpty()) {
            button.textSize = dc.GetTextExtent(button.label);
        }
    }
    
    // Update minimum size
    int totalWidth = x - m_buttonSpacing + m_btnBarHorizontalMargin;
    SetMinSize(wxSize(totalWidth, maxHeight));
}

void FlatUIButtonBar::SetDisplayStyle(ButtonDisplayStyle style) {
    if (m_displayStyle != style) {
        m_displayStyle = style;
        RecalculateLayout();
        Refresh();
    }
}

wxSize FlatUIButtonBar::DoGetBestSize() const {
    if (m_buttons.empty()) {
        return wxSize(100, targetH);
    }
    
    wxClientDC dc(const_cast<FlatUIButtonBar*>(this));
    int totalWidth = m_btnBarHorizontalMargin;
    int maxHeight = targetH;
    
    for (const auto& button : m_buttons) {
        if (button.visible) {
            totalWidth += CalculateButtonWidth(button, dc) + m_buttonSpacing;
        }
    }
    
    totalWidth = totalWidth - m_buttonSpacing + m_btnBarHorizontalMargin;
    
    return wxSize(totalWidth, maxHeight);
}

// Helper methods
FlatUIButtonBar::ButtonInfo* FlatUIButtonBar::FindButton(int id) {
    for (auto& button : m_buttons) {
        if (button.id == id) {
            return &button;
        }
    }
    return nullptr;
}

const FlatUIButtonBar::ButtonInfo* FlatUIButtonBar::FindButton(int id) const {
    for (const auto& button : m_buttons) {
        if (button.id == id) {
            return &button;
        }
    }
    return nullptr;
}

int FlatUIButtonBar::FindButtonIndex(int id) const {
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        if (m_buttons[i].id == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
} 