#include "flatui/FlatUIButtonBar.h"
#include "flatui/FlatUIPanel.h"
#include "config/ThemeManager.h"

// Style configuration methods
void FlatUIButtonBar::SetButtonStyle(ButtonStyle style) {
    if (m_buttonStyle != style) {
        m_buttonStyle = style;
        Refresh();
    }
}

void FlatUIButtonBar::SetButtonBorderStyle(ButtonBorderStyle style) {
    if (m_buttonBorderStyle != style) {
        m_buttonBorderStyle = style;
        Refresh();
    }
}

// Color configuration methods
void FlatUIButtonBar::SetButtonBackgroundColour(const wxColour& colour) {
    m_buttonBgColour = colour;
    Refresh();
}

void FlatUIButtonBar::SetButtonHoverBackgroundColour(const wxColour& colour) {
    m_buttonHoverBgColour = colour;
    Refresh();
}

void FlatUIButtonBar::SetButtonPressedBackgroundColour(const wxColour& colour) {
    m_buttonPressedBgColour = colour;
    Refresh();
}

void FlatUIButtonBar::SetButtonTextColour(const wxColour& colour) {
    m_buttonTextColour = colour;
    Refresh();
}

void FlatUIButtonBar::SetButtonBorderColour(const wxColour& colour) {
    m_buttonBorderColour = colour;
    Refresh();
}

// Size and spacing configuration methods
void FlatUIButtonBar::SetButtonBorderWidth(int width) {
    if (m_buttonBorderWidth != width) {
        m_buttonBorderWidth = width;
        Refresh();
    }
}

void FlatUIButtonBar::SetButtonCornerRadius(int radius) {
    if (m_buttonCornerRadius != radius) {
        m_buttonCornerRadius = radius;
        Refresh();
    }
}

void FlatUIButtonBar::SetButtonSpacing(int spacing) {
    if (m_buttonSpacing != spacing) {
        m_buttonSpacing = spacing;
        RecalculateLayout();
        Refresh();
    }
}

void FlatUIButtonBar::SetButtonPadding(int horizontal, int vertical) {
    if (m_buttonHorizontalPadding != horizontal || m_buttonVerticalPadding != vertical) {
        m_buttonHorizontalPadding = horizontal;
        m_buttonVerticalPadding = vertical;
        RecalculateLayout();
        Refresh();
    }
}

void FlatUIButtonBar::GetButtonPadding(int& horizontal, int& vertical) const {
    horizontal = m_buttonHorizontalPadding;
    vertical = m_buttonVerticalPadding;
}

// ButtonBar styling methods
void FlatUIButtonBar::SetBtnBarBackgroundColour(const wxColour& colour) {
    m_btnBarBgColour = colour;
    SetBackgroundColour(colour);
    Refresh();
}

void FlatUIButtonBar::SetBtnBarBorderColour(const wxColour& colour) {
    m_btnBarBorderColour = colour;
    Refresh();
}

void FlatUIButtonBar::SetBtnBarBorderWidth(int width) {
    if (m_btnBarBorderWidth != width) {
        m_btnBarBorderWidth = width;
        Refresh();
    }
}

// Interaction configuration
void FlatUIButtonBar::SetHoverEffectsEnabled(bool enabled) {
    if (m_hoverEffectsEnabled != enabled) {
        m_hoverEffectsEnabled = enabled;
        if (!enabled) {
            m_hoveredButtonIndex = -1;
        }
        Refresh();
    }
} 