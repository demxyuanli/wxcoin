#include "flatui/FlatUIButtonBar.h"
#include "flatui/FlatUIPanel.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include "config/ThemeManager.h"

// Extended control addition methods
void FlatUIButtonBar::AddToggleButton(int id, const wxString& label, bool initialState, const wxBitmap& bitmap, const wxString& tooltip) {
    ButtonInfo button(id, ButtonType::TOGGLE);
    button.label = label;
    button.icon = bitmap;
    button.checked = initialState;
    button.tooltip = tooltip;
    
    m_buttons.push_back(button);
    RecalculateLayout();
    Refresh();
}

void FlatUIButtonBar::AddCheckBox(int id, const wxString& label, bool initialState, const wxString& tooltip) {
    ButtonInfo button(id, ButtonType::CHECKBOX);
    button.label = label;
    button.checked = initialState;
    button.tooltip = tooltip;
    
    m_buttons.push_back(button);
    RecalculateLayout();
    Refresh();
}

void FlatUIButtonBar::AddRadioButton(int id, const wxString& label, int radioGroup, bool initialState, const wxString& tooltip) {
    ButtonInfo button(id, ButtonType::RADIO);
    button.label = label;
    button.radioGroup = radioGroup;
    button.checked = initialState;
    button.tooltip = tooltip;
    
    // If this is initially checked, uncheck others in the same group
    if (initialState && radioGroup >= 0) {
        for (auto& existingButton : m_buttons) {
            if (existingButton.type == ButtonType::RADIO && existingButton.radioGroup == radioGroup) {
                existingButton.checked = false;
            }
        }
    }
    
    m_buttons.push_back(button);
    RecalculateLayout();
    Refresh();
}

void FlatUIButtonBar::AddChoiceControl(int id, const wxString& label, const wxArrayString& choices, int initialSelection, const wxString& tooltip) {
    ButtonInfo button(id, ButtonType::CHOICE);
    button.label = label;
    button.choiceItems = choices;
    button.selectedChoice = (initialSelection >= 0 && initialSelection < (int)choices.size()) ? initialSelection : -1;
    if (button.selectedChoice >= 0) {
        button.value = choices[button.selectedChoice];
    }
    button.tooltip = tooltip;
    
    m_buttons.push_back(button);
    RecalculateLayout();
    Refresh();
}

// Control state management
void FlatUIButtonBar::SetButtonChecked(int id, bool checked) {
    ButtonInfo* button = FindButton(id);
    if (button && (button->type == ButtonType::TOGGLE || 
                   button->type == ButtonType::CHECKBOX || 
                   button->type == ButtonType::RADIO)) {
        
        if (button->type == ButtonType::RADIO && checked && button->radioGroup >= 0) {
            // Uncheck all other radio buttons in the same group
            for (auto& otherButton : m_buttons) {
                if (otherButton.type == ButtonType::RADIO && 
                    otherButton.radioGroup == button->radioGroup) {
                    otherButton.checked = false;
                }
            }
        }
        
        button->checked = checked;
        Refresh();
    }
}

bool FlatUIButtonBar::IsButtonChecked(int id) const {
    const ButtonInfo* button = FindButton(id);
    return button ? button->checked : false;
}

void FlatUIButtonBar::SetButtonEnabled(int id, bool enabled) {
    ButtonInfo* button = FindButton(id);
    if (button) {
        button->enabled = enabled;
        Refresh();
    }
}

bool FlatUIButtonBar::IsButtonEnabled(int id) const {
    const ButtonInfo* button = FindButton(id);
    return button ? button->enabled : false;
}

void FlatUIButtonBar::SetButtonVisible(int id, bool visible) {
    ButtonInfo* button = FindButton(id);
    if (button && button->visible != visible) {
        button->visible = visible;
        RecalculateLayout();
        Refresh();
    }
}

bool FlatUIButtonBar::IsButtonVisible(int id) const {
    const ButtonInfo* button = FindButton(id);
    return button ? button->visible : false;
}

// Choice control specific methods
void FlatUIButtonBar::SetChoiceItems(int id, const wxArrayString& items) {
    ButtonInfo* button = FindButton(id);
    if (button && button->type == ButtonType::CHOICE) {
        button->choiceItems = items;
        if (button->selectedChoice >= (int)items.size()) {
            button->selectedChoice = items.size() > 0 ? 0 : -1;
        }
        if (button->selectedChoice >= 0 && button->selectedChoice < (int)items.size()) {
            button->value = items[button->selectedChoice];
        }
        RecalculateLayout();
        Refresh();
    }
}

wxArrayString FlatUIButtonBar::GetChoiceItems(int id) const {
    const ButtonInfo* button = FindButton(id);
    return button && button->type == ButtonType::CHOICE ? button->choiceItems : wxArrayString();
}

void FlatUIButtonBar::SetChoiceSelection(int id, int selection) {
    ButtonInfo* button = FindButton(id);
    if (button && button->type == ButtonType::CHOICE) {
        if (selection >= 0 && selection < (int)button->choiceItems.size()) {
            button->selectedChoice = selection;
            button->value = button->choiceItems[selection];
            Refresh();
        }
    }
}

int FlatUIButtonBar::GetChoiceSelection(int id) const {
    const ButtonInfo* button = FindButton(id);
    return button && button->type == ButtonType::CHOICE ? button->selectedChoice : -1;
}

wxString FlatUIButtonBar::GetChoiceValue(int id) const {
    const ButtonInfo* button = FindButton(id);
    return button ? button->value : wxString();
}

// Radio button group management
void FlatUIButtonBar::SetRadioGroupSelection(int radioGroup, int selectedId) {
    for (auto& button : m_buttons) {
        if (button.type == ButtonType::RADIO && button.radioGroup == radioGroup) {
            button.checked = (button.id == selectedId);
        }
    }
    Refresh();
}

int FlatUIButtonBar::GetRadioGroupSelection(int radioGroup) const {
    for (const auto& button : m_buttons) {
        if (button.type == ButtonType::RADIO && button.radioGroup == radioGroup && button.checked) {
            return button.id;
        }
    }
    return -1;
}

// Button value and properties
void FlatUIButtonBar::SetButtonValue(int id, const wxString& value) {
    ButtonInfo* button = FindButton(id);
    if (button) {
        button->value = value;
        Refresh();
    }
}

wxString FlatUIButtonBar::GetButtonValue(int id) const {
    const ButtonInfo* button = FindButton(id);
    return button ? button->value : wxString();
}

void FlatUIButtonBar::SetButtonCustomColors(int id, const wxColour& bgColor, const wxColour& textColor, const wxColour& borderColor) {
    ButtonInfo* button = FindButton(id);
    if (button) {
        button->customBgColor = bgColor;
        button->customTextColor = textColor;
        button->customBorderColor = borderColor;
        Refresh();
    }
}

// Drawing methods for extended controls
void FlatUIButtonBar::DrawToggleButton(wxDC& dc, const ButtonInfo& button, const wxRect& rect) {
    bool isHovered = m_hoverEffectsEnabled && FindButtonIndex(button.id) == m_hoveredButtonIndex;
    bool isPressed = button.pressed;
    
    // Use different colors for toggled state
    wxColour bgColour;
    if (button.customBgColor.IsOk()) {
        bgColour = button.customBgColor;
    } else if (button.checked) {
        bgColour = m_buttonPressedBgColour; // Use pressed color for checked state
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
    
    // Draw border - always draw for checked toggle buttons to show state
    if (m_buttonBorderWidth > 0 || button.checked) {
        wxColour borderColour = button.customBorderColor.IsOk() ? button.customBorderColor : m_buttonBorderColour;
        int borderWidth = button.checked ? (m_buttonBorderWidth > 0 ? m_buttonBorderWidth + 1 : 2) : m_buttonBorderWidth;
        dc.SetPen(wxPen(borderColour, borderWidth));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        
        if (m_buttonStyle == ButtonStyle::PILL || m_buttonCornerRadius > 0) {
            dc.DrawRoundedRectangle(rect, m_buttonCornerRadius);
        } else {
            dc.DrawRectangle(rect);
        }
    }
    
    // Draw icon and text
    DrawButtonIcon(dc, button, rect);
    DrawButtonText(dc, button, rect);
}

void FlatUIButtonBar::DrawCheckBox(wxDC& dc, const ButtonInfo& button, const wxRect& rect) {
    bool isHovered = m_hoverEffectsEnabled && FindButtonIndex(button.id) == m_hoveredButtonIndex;
    
    // Draw button background
    DrawButtonBackground(dc, button, rect, isHovered, false);
    DrawButtonBorder(dc, button, rect, isHovered, false);
    
    // Draw checkbox indicator
    wxRect checkRect = GetCheckBoxIndicatorRect(rect);
    DrawCheckBoxIndicator(dc, checkRect, button.checked, button.enabled);
    
    // Draw text (shifted to make room for checkbox)
    if (!button.label.IsEmpty()) {
        wxColour textColour = button.customTextColor.IsOk() ? button.customTextColor : m_buttonTextColour;
        dc.SetTextForeground(textColour);
        dc.SetFont(GetFont());
        
        wxRect textRect = rect;
        textRect.x += 24; // Space for checkbox + margin
        textRect.width -= 24;
        textRect.Deflate(m_buttonHorizontalPadding, m_buttonVerticalPadding);
        
        dc.DrawText(button.label, textRect.x, textRect.y + (textRect.height - button.textSize.GetHeight()) / 2);
    }
}

void FlatUIButtonBar::DrawRadioButton(wxDC& dc, const ButtonInfo& button, const wxRect& rect) {
    bool isHovered = m_hoverEffectsEnabled && FindButtonIndex(button.id) == m_hoveredButtonIndex;
    
    // Draw button background
    DrawButtonBackground(dc, button, rect, isHovered, false);
    DrawButtonBorder(dc, button, rect, isHovered, false);
    
    // Draw radio button indicator
    wxRect radioRect = GetRadioButtonIndicatorRect(rect);
    DrawRadioButtonIndicator(dc, radioRect, button.checked, button.enabled);
    
    // Draw text (shifted to make room for radio button)
    if (!button.label.IsEmpty()) {
        wxColour textColour = button.customTextColor.IsOk() ? button.customTextColor : m_buttonTextColour;
        dc.SetTextForeground(textColour);
        dc.SetFont(GetFont());
        
        wxRect textRect = rect;
        textRect.x += 24; // Space for radio button + margin
        textRect.width -= 24;
        textRect.Deflate(m_buttonHorizontalPadding, m_buttonVerticalPadding);
        
        dc.DrawText(button.label, textRect.x, textRect.y + (textRect.height - button.textSize.GetHeight()) / 2);
    }
}

void FlatUIButtonBar::DrawChoiceControl(wxDC& dc, const ButtonInfo& button, const wxRect& rect) {
    bool isHovered = m_hoverEffectsEnabled && FindButtonIndex(button.id) == m_hoveredButtonIndex;
    
    // Draw button background
    DrawButtonBackground(dc, button, rect, isHovered, false);
    DrawButtonBorder(dc, button, rect, isHovered, false);
    
    // Draw current selection text
    wxString displayText = button.value;
    if (displayText.IsEmpty() && !button.choiceItems.empty() && button.selectedChoice >= 0) {
        displayText = button.choiceItems[button.selectedChoice];
    }
    if (displayText.IsEmpty()) {
        displayText = button.label;
    }
    
    if (!displayText.IsEmpty()) {
        wxColour textColour = button.customTextColor.IsOk() ? button.customTextColor : m_buttonTextColour;
        dc.SetTextForeground(textColour);
        dc.SetFont(GetFont());
        
        wxRect textRect = rect;
        textRect.width -= 20; // Space for dropdown arrow
        textRect.Deflate(m_buttonHorizontalPadding, m_buttonVerticalPadding);
        
        // Clip text if it's too long
        wxString clippedText = displayText;
        wxSize textSize = dc.GetTextExtent(clippedText);
        while (textSize.GetWidth() > textRect.GetWidth() && clippedText.length() > 3) {
            clippedText = clippedText.Left(clippedText.length() - 4) + "...";
            textSize = dc.GetTextExtent(clippedText);
        }
        
        dc.DrawText(clippedText, textRect.x, textRect.y + (textRect.height - textSize.GetHeight()) / 2);
    }
    
    // Draw dropdown arrow
    wxRect arrowRect = GetChoiceDropdownRect(rect);
    DrawChoiceDropdownArrow(dc, arrowRect, button.enabled);
}

// Drawing helper methods for indicators
void FlatUIButtonBar::DrawCheckBoxIndicator(wxDC& dc, const wxRect& rect, bool checked, bool enabled) {
    wxColour borderColor = enabled ? m_buttonBorderColour : m_buttonBorderColour.ChangeLightness(150);
    wxColour fillColor = enabled ? *wxWHITE : wxColour(240, 240, 240);
    
    // Draw checkbox border
    dc.SetPen(wxPen(borderColor, 1));
    dc.SetBrush(wxBrush(fillColor));
    dc.DrawRectangle(rect);
    
    // Draw check mark if checked
    if (checked) {
        wxColour checkColor = enabled ? *wxBLACK : wxColour(128, 128, 128);
        dc.SetPen(wxPen(checkColor, 2));
        
        // Draw check mark
        wxPoint points[3];
        points[0] = wxPoint(rect.x + 3, rect.y + rect.height / 2);
        points[1] = wxPoint(rect.x + rect.width / 2, rect.y + rect.height - 4);
        points[2] = wxPoint(rect.x + rect.width - 3, rect.y + 3);
        
        dc.DrawLine(points[0], points[1]);
        dc.DrawLine(points[1], points[2]);
    }
}

void FlatUIButtonBar::DrawRadioButtonIndicator(wxDC& dc, const wxRect& rect, bool checked, bool enabled) {
    wxColour borderColor = enabled ? m_buttonBorderColour : m_buttonBorderColour.ChangeLightness(150);
    wxColour fillColor = enabled ? *wxWHITE : wxColour(240, 240, 240);
    
    // Draw radio button circle
    dc.SetPen(wxPen(borderColor, 1));
    dc.SetBrush(wxBrush(fillColor));
    dc.DrawEllipse(rect);
    
    // Draw inner fill if checked
    if (checked) {
        wxColour checkColor = enabled ? *wxBLACK : wxColour(128, 128, 128);
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush(checkColor));
        
        wxRect innerRect = rect;
        innerRect.Deflate(4);
        dc.DrawEllipse(innerRect);
    }
}

void FlatUIButtonBar::DrawChoiceDropdownArrow(wxDC& dc, const wxRect& rect, bool enabled) {
    wxColour arrowColor = enabled ? m_buttonTextColour : m_buttonTextColour.ChangeLightness(150);
    
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(arrowColor));
    
    // Draw down arrow
    wxPoint points[3];
    points[0] = wxPoint(rect.x, rect.y);
    points[1] = wxPoint(rect.x + rect.width, rect.y);
    points[2] = wxPoint(rect.x + rect.width / 2, rect.y + rect.height);
    
    dc.DrawPolygon(3, points);
}

// Helper methods for control layout
wxRect FlatUIButtonBar::GetCheckBoxIndicatorRect(const wxRect& buttonRect) const {
    return wxRect(buttonRect.x + 4, buttonRect.y + (buttonRect.height - 16) / 2, 16, 16);
}

wxRect FlatUIButtonBar::GetRadioButtonIndicatorRect(const wxRect& buttonRect) const {
    return wxRect(buttonRect.x + 4, buttonRect.y + (buttonRect.height - 16) / 2, 16, 16);
}

wxRect FlatUIButtonBar::GetChoiceDropdownRect(const wxRect& buttonRect) const {
    return wxRect(buttonRect.x + buttonRect.width - 16, buttonRect.y + (buttonRect.height - 8) / 2, 12, 8);
} 