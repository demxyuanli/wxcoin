#include "flatui/FlatUIButtonBar.h"
#include "flatui/FlatUIPanel.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include "config/ThemeManager.h"

// Main drawing methods
void FlatUIButtonBar::OnPaint(wxPaintEvent& evt) {
    wxAutoBufferedPaintDC dc(this);
    PrepareDC(dc);
    
    wxSize clientSize = GetClientSize();
    
    // Draw background
    dc.SetBrush(wxBrush(m_btnBarBgColour));
    if (m_btnBarBorderWidth > 0) {
        dc.SetPen(wxPen(m_btnBarBorderColour, m_btnBarBorderWidth));
    } else {
        dc.SetPen(*wxTRANSPARENT_PEN);
    }
    dc.DrawRectangle(0, 0, clientSize.x, clientSize.y);
    
    // Draw buttons
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        if (m_buttons[i].visible) {
            DrawButton(dc, m_buttons[i], (int)i);
        }
    }
}

void FlatUIButtonBar::DrawButton(wxDC& dc, const ButtonInfo& button, int index) {
    if (button.type == ButtonType::SEPARATOR) {
        DrawButtonSeparator(dc, button, button.rect);
        return;
    }
    
    bool isHovered = m_hoverEffectsEnabled && index == m_hoveredButtonIndex;
    bool isPressed = button.pressed;
    
    // Draw button based on type
    switch (button.type) {
        case ButtonType::TOGGLE:
            DrawToggleButton(dc, button, button.rect);
            break;
        case ButtonType::CHECKBOX:
            DrawCheckBox(dc, button, button.rect);
            break;
        case ButtonType::RADIO:
            DrawRadioButton(dc, button, button.rect);
            break;
        case ButtonType::CHOICE:
            DrawChoiceControl(dc, button, button.rect);
            break;
        default:
            // Draw normal button
            DrawButtonBackground(dc, button, button.rect, isHovered, isPressed);
            DrawButtonBorder(dc, button, button.rect, isHovered, isPressed);
            DrawButtonIcon(dc, button, button.rect);
            DrawButtonText(dc, button, button.rect);
            if (button.isDropDown) {
                DrawButtonDropdownArrow(dc, button, button.rect);
            }
            break;
    }
}

void FlatUIButtonBar::DrawButtonBackground(wxDC& dc, const ButtonInfo& button, const wxRect& rect, bool isHovered, bool isPressed) {
    wxColour bgColour;
    
    if (button.customBgColor.IsOk()) {
        bgColour = button.customBgColor;
    } else if (isPressed) {
        bgColour = m_buttonPressedBgColour;
    } else if (isHovered) {
        bgColour = m_buttonHoverBgColour;
    } else {
        bgColour = m_buttonBgColour;
    }
    
    dc.SetBrush(wxBrush(bgColour));
    dc.SetPen(*wxTRANSPARENT_PEN);
    
    if (m_buttonStyle == ButtonStyle::PILL || m_buttonCornerRadius > 0) {
        dc.DrawRoundedRectangle(rect, m_buttonCornerRadius);
    } else {
        dc.DrawRectangle(rect);
    }
}

void FlatUIButtonBar::DrawButtonBorder(wxDC& dc, const ButtonInfo& button, const wxRect& rect, bool isHovered, bool isPressed) {
    if (m_buttonBorderWidth <= 0) return; // Only draw border if width > 0
    
    wxColour borderColour = button.customBorderColor.IsOk() ? button.customBorderColor : m_buttonBorderColour;
    
    switch (m_buttonBorderStyle) {
    case ButtonBorderStyle::SOLID:
        dc.SetPen(wxPen(borderColour, m_buttonBorderWidth));
        break;
    case ButtonBorderStyle::DASHED:
        dc.SetPen(wxPen(borderColour, m_buttonBorderWidth, wxPENSTYLE_LONG_DASH));
        break;
    case ButtonBorderStyle::DOTTED:
        dc.SetPen(wxPen(borderColour, m_buttonBorderWidth, wxPENSTYLE_DOT));
        break;
    case ButtonBorderStyle::DOUBLE:
        dc.SetPen(wxPen(borderColour, m_buttonBorderWidth));
        break;
    case ButtonBorderStyle::ROUNDED:
        dc.SetPen(wxPen(borderColour, m_buttonBorderWidth));
        break;
    }

    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    if (m_buttonStyle == ButtonStyle::PILL || m_buttonCornerRadius > 0) {
        dc.DrawRoundedRectangle(rect, m_buttonCornerRadius);
    }
    else {
        dc.DrawRectangle(rect);
    }
}

void FlatUIButtonBar::DrawButtonIcon(wxDC& dc, const ButtonInfo& button, const wxRect& rect) {
    if (!button.icon.IsOk() || 
        m_displayStyle == ButtonDisplayStyle::TEXT_ONLY) {
        return;
    }
    
    wxRect iconRect;
    int iconWidth = button.icon.GetWidth();
    int iconHeight = button.icon.GetHeight();
    
    if (m_displayStyle == ButtonDisplayStyle::ICON_ONLY) {
        iconRect.x = rect.x + (rect.width - iconWidth) / 2;
        iconRect.y = rect.y + (rect.height - iconHeight) / 2;
    }
    else if (m_displayStyle == ButtonDisplayStyle::ICON_TEXT_BESIDE) {
        int totalWidth = iconWidth;
        if (!button.label.IsEmpty()) {
            totalWidth += CFG_INT("ActBarIconTextSpacing") + button.textSize.GetWidth();
        }
        
        int startX = rect.x + (rect.width - totalWidth) / 2;
        iconRect.x = startX;
        iconRect.y = rect.y + (rect.height - iconHeight) / 2;
    }
    else if (m_displayStyle == ButtonDisplayStyle::ICON_TEXT_BELOW) {
        iconRect.x = rect.x + (rect.width - iconWidth) / 2;
        
        int totalHeight = iconHeight;
        if (!button.label.IsEmpty()) {
            totalHeight += CFG_INT("ActBarIconTextSpacing") + button.textSize.GetHeight();
        }
        
        int startY = rect.y + (rect.height - totalHeight) / 2;
        iconRect.y = startY;
    }
    
    iconRect.width = iconWidth;
    iconRect.height = iconHeight;
    
    dc.DrawBitmap(button.icon, iconRect.x, iconRect.y, true);
}

void FlatUIButtonBar::DrawButtonText(wxDC& dc, const ButtonInfo& button, const wxRect& rect) {
    if (button.label.IsEmpty() || 
        m_displayStyle == ButtonDisplayStyle::ICON_ONLY) {
        return;
    }
    
    wxColour textColour = button.customTextColor.IsOk() ? button.customTextColor : m_buttonTextColour;
    dc.SetTextForeground(textColour);
    dc.SetFont(GetFont());
    
    wxRect textRect;
    
    if (m_displayStyle == ButtonDisplayStyle::TEXT_ONLY) {
        textRect = rect;
        textRect.Deflate(m_buttonHorizontalPadding, m_buttonVerticalPadding);
    }
    else if (m_displayStyle == ButtonDisplayStyle::ICON_TEXT_BESIDE) {
        int iconWidth = button.icon.IsOk() ? button.icon.GetWidth() : 0;
        int totalWidth = iconWidth;
        if (iconWidth > 0) {
            totalWidth += CFG_INT("ActBarIconTextSpacing");
        }
        totalWidth += button.textSize.GetWidth();
        
        int startX = rect.x + (rect.width - totalWidth) / 2;
        if (iconWidth > 0) {
            startX += iconWidth + CFG_INT("ActBarIconTextSpacing");
        }
        
        textRect.x = startX;
        textRect.y = rect.y + (rect.height - button.textSize.GetHeight()) / 2;
        textRect.width = button.textSize.GetWidth();
        textRect.height = button.textSize.GetHeight();
    }
    else if (m_displayStyle == ButtonDisplayStyle::ICON_TEXT_BELOW) {
        int iconHeight = button.icon.IsOk() ? button.icon.GetHeight() : 0;
        int totalHeight = iconHeight;
        if (iconHeight > 0) {
            totalHeight += CFG_INT("ActBarIconTextSpacing");
        }
        totalHeight += button.textSize.GetHeight();
        
        int startY = rect.y + (rect.height - totalHeight) / 2;
        if (iconHeight > 0) {
            startY += iconHeight + CFG_INT("ActBarIconTextSpacing");
        }
        
        textRect.x = rect.x + (rect.width - button.textSize.GetWidth()) / 2;
        textRect.y = startY;
        textRect.width = button.textSize.GetWidth();
        textRect.height = button.textSize.GetHeight();
    }
    
    dc.DrawText(button.label, textRect.x, textRect.y);
}

void FlatUIButtonBar::DrawButtonDropdownArrow(wxDC& dc, const ButtonInfo& button, const wxRect& rect) {
    int arrowX = rect.x + rect.width - m_dropdownArrowWidth - CFG_INT("ActBarDropdownArrowMargin");
    int arrowY = rect.y + (rect.height - m_dropdownArrowHeight) / 2;
    
    wxPoint points[3];
    points[0] = wxPoint(arrowX, arrowY);
    points[1] = wxPoint(arrowX + m_dropdownArrowWidth, arrowY);
    points[2] = wxPoint(arrowX + m_dropdownArrowWidth / 2, arrowY + m_dropdownArrowHeight);
    
    dc.SetBrush(wxBrush(m_buttonTextColour));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawPolygon(3, points);
}

void FlatUIButtonBar::DrawButtonSeparator(wxDC& dc, const ButtonInfo& button, const wxRect& rect) {
    int separatorX = rect.x + rect.width / 2;
    int separatorY1 = rect.y + m_separatorMargin;
    int separatorY2 = rect.y + rect.height - m_separatorMargin;
    
    dc.SetPen(wxPen(m_buttonBorderColour, 1));
    dc.DrawLine(separatorX, separatorY1, separatorX, separatorY2);
} 