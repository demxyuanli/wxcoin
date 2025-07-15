#include "flatui/FlatUIButtonBar.h"
#include "flatui/FlatUIPanel.h"
#include <wx/dcbuffer.h>
#include "config/ThemeManager.h"

// Event handling methods
void FlatUIButtonBar::OnMouseMove(wxMouseEvent& evt) {
    wxPoint pos = evt.GetPosition();
    
    int newHoveredIndex = -1;
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        if (m_buttons[i].rect.Contains(pos) && m_buttons[i].visible && m_buttons[i].enabled) {
            newHoveredIndex = (int)i;
            break;
        }
    }
    
    if (newHoveredIndex != m_hoveredButtonIndex) {
        m_hoveredButtonIndex = newHoveredIndex;
        if (m_hoverEffectsEnabled) {
            Refresh();
        }
    }
    
    // Update tooltip
    if (m_hoveredButtonIndex >= 0 && m_hoveredButtonIndex < (int)m_buttons.size()) {
        const ButtonInfo& button = m_buttons[m_hoveredButtonIndex];
        if (!button.tooltip.IsEmpty()) {
            SetToolTip(button.tooltip);
        } else {
            UnsetToolTip();
        }
    } else {
        UnsetToolTip();
    }
    
    evt.Skip();
}

void FlatUIButtonBar::OnMouseLeave(wxMouseEvent& evt) {
    if (m_hoveredButtonIndex != -1) {
        m_hoveredButtonIndex = -1;
        if (m_hoverEffectsEnabled) {
            Refresh();
        }
    }
    UnsetToolTip();
    evt.Skip();
}

void FlatUIButtonBar::OnMouseDown(wxMouseEvent& evt) {
    wxPoint pos = evt.GetPosition();

    for (auto& button : m_buttons) {
        if (button.rect.Contains(pos) && button.visible && button.enabled) {
            // Handle different button types
            switch (button.type) {
                case ButtonType::TOGGLE:
                    HandleToggleButton(button);
                    break;
                    
                case ButtonType::CHECKBOX:
                    HandleCheckBox(button);
                    break;
                    
                case ButtonType::RADIO:
                    HandleRadioButton(button);
                    break;
                    
                case ButtonType::CHOICE:
                    HandleChoiceControl(button, pos);
                    break;
                    
                case ButtonType::SEPARATOR:
                    // Separators don't respond to clicks
                    break;
                    
                case ButtonType::NORMAL:
                default:
                    // Handle traditional buttons
                    if (button.menu) {
                        // Align menu with button's left-bottom corner
                        wxPoint menuPos = button.rect.GetBottomLeft();
                        menuPos.y += CFG_INT("ButtonbarMenuVerticalOffset");
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
            
            // Send appropriate events for new control types
            if (button.type != ButtonType::NORMAL && button.type != ButtonType::SEPARATOR) {
                wxCommandEvent event;
                
                switch (button.type) {
                    case ButtonType::TOGGLE:
                        event.SetEventType(wxEVT_COMMAND_BUTTON_CLICKED);
                        event.SetInt(button.checked ? 1 : 0);
                        break;
                        
                    case ButtonType::CHECKBOX:
                        event.SetEventType(wxEVT_COMMAND_BUTTON_CLICKED);
                        event.SetInt(button.checked ? 1 : 0);
                        break;
                        
                    case ButtonType::RADIO:
                        event.SetEventType(wxEVT_COMMAND_BUTTON_CLICKED);
                        event.SetInt(button.checked ? 1 : 0);
                        break;
                        
                    case ButtonType::CHOICE:
                        event.SetEventType(wxEVT_COMMAND_BUTTON_CLICKED);
                        event.SetInt(button.selectedChoice);
                        event.SetString(button.value);
                        break;
                        
                    default:
                        break;
                }
                
                if (event.GetEventType() != wxEVT_NULL) {
                    event.SetId(button.id);
                    event.SetEventObject(this);
                    GetParent()->ProcessWindowEvent(event);
                }
            }
            
            break;
        }
    }
}

void FlatUIButtonBar::OnSize(wxSizeEvent& evt) {
    RecalculateLayout();
}

// Button type handlers
void FlatUIButtonBar::HandleToggleButton(ButtonInfo& button) {
    button.checked = !button.checked;
    Refresh();
}

void FlatUIButtonBar::HandleCheckBox(ButtonInfo& button) {
    button.checked = !button.checked;
    Refresh();
}

void FlatUIButtonBar::HandleRadioButton(ButtonInfo& button) {
    if (!button.checked && button.radioGroup >= 0) {
        // Uncheck all other radio buttons in the same group
        for (auto& otherButton : m_buttons) {
            if (otherButton.type == ButtonType::RADIO && 
                otherButton.radioGroup == button.radioGroup) {
                otherButton.checked = false;
            }
        }
        // Check this button
        button.checked = true;
        Refresh();
    }
}

void FlatUIButtonBar::HandleChoiceControl(ButtonInfo& button, const wxPoint& mousePos) {
    // For now, just cycle through choices on click
    // In a real implementation, you might show a dropdown menu
    if (!button.choiceItems.empty()) {
        button.selectedChoice = (button.selectedChoice + 1) % (int)button.choiceItems.size();
        button.value = button.choiceItems[button.selectedChoice];
        Refresh();
    }
} 