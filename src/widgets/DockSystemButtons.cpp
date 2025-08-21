#include "widgets/DockSystemButtons.h"
#include "widgets/ModernDockPanel.h"
#include "config/ThemeManager.h"
#include "config/SvgIconManager.h"
#include "logger/Logger.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/artprov.h>

wxBEGIN_EVENT_TABLE(DockSystemButtons, wxPanel)
    EVT_PAINT(DockSystemButtons::OnPaint)
    EVT_SIZE(DockSystemButtons::OnSize)
    EVT_LEFT_DOWN(DockSystemButtons::OnButtonDown)
    EVT_LEFT_UP(DockSystemButtons::OnButtonUp)
    EVT_MOTION(DockSystemButtons::OnButtonHover)
    EVT_LEAVE_WINDOW(DockSystemButtons::OnButtonLeave)
wxEND_EVENT_TABLE()

#ifndef SVG_ICON
#define SVG_ICON(name, size) SvgIconManager::GetInstance().GetBitmapBundle(name, size).GetBitmap(size)
#endif

DockSystemButtons::DockSystemButtons(ModernDockPanel* parent, wxWindowID id)
    : wxPanel(parent, id), m_parent(parent), m_hoveredButtonIndex(-1), m_pressedButtonIndex(-1)
{
    // Set background style for custom painting
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetDoubleBuffered(true);
    
    // Initialize layout
    m_buttonSize = DEFAULT_BUTTON_SIZE;
    m_buttonSpacing = DEFAULT_BUTTON_SPACING;
    m_margin = DEFAULT_MARGIN;
    
    // Initialize colors
    UpdateThemeColors();
    
    // Initialize default buttons
    InitializeButtons();
    
    // Set minimum size (3 buttons: Pin toggle, Min/Max toggle, Close)
    SetMinSize(wxSize(m_buttonSize * 3 + m_buttonSpacing * 2 + m_margin * 2, m_buttonSize + m_margin * 2));
}

DockSystemButtons::~DockSystemButtons()
{
}

void DockSystemButtons::InitializeButtons()
{
    // Add default system buttons in order: Pin/Unpin Toggle, Min/Max Toggle, Close
    AddButton(DockSystemButtonType::PIN, "Pin/Unpin Panel");
    AddButton(DockSystemButtonType::MINIMIZE, "Minimize/Maximize Panel");
    AddButton(DockSystemButtonType::CLOSE, "Close Panel");
    
    // Debug: Log button creation
    LOG_INF("Created " + std::to_string(m_buttons.size()) + " system buttons", "DockSystemButtons");
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        LOG_INF("Button " + std::to_string(i) + ": " + 
                std::to_string(static_cast<int>(m_buttons[i].type)) + 
                " (visible: " + std::to_string(m_buttons[i].visible) + ")", "DockSystemButtons");
    }
    
    UpdateLayout();
}

void DockSystemButtons::AddButton(DockSystemButtonType type, const wxString& tooltip)
{
    // Check if button already exists
    if (GetButtonIndex(type) >= 0) {
        return;
    }
    
    // Create button configuration
    DockSystemButtonConfig config(type, tooltip);
    
    // Set default icons based on type using SVG icons (12x12 size)
    switch (type) {
        case DockSystemButtonType::PIN:
            // PIN toggle: pinned state (thumbtack) <-> unpinned state (unpin)
            config.icon = SVG_ICON("thumbtack", wxSize(12, 12));    // Default: pinned
            config.altIcon = SVG_ICON("unpin", wxSize(12, 12));     // Toggled: unpinned
            break;
        case DockSystemButtonType::MINIMIZE:
            // MIN/MAX toggle: minimized state (minimize) <-> maximized state (maximize)
            config.icon = SVG_ICON("minimize", wxSize(12, 12));     // Default: minimize
            config.altIcon = SVG_ICON("maximize", wxSize(12, 12));  // Toggled: maximize
            break;
        case DockSystemButtonType::CLOSE:
            // CLOSE button: not a toggle, single action
            config.icon = SVG_ICON("close", wxSize(12, 12));
            break;
        case DockSystemButtonType::MAXIMIZE:
        case DockSystemButtonType::FLOAT:
        case DockSystemButtonType::DOCK:
            // These types are not used in the simplified 3-button design
            break;
    }
    
    // If SVG icon loading failed, use wxArtProvider as fallback
    if (!config.icon.IsOk()) {
        LOG_INF("SVG icon loading failed for type " + std::to_string(static_cast<int>(type)) + 
                ", using wxArtProvider fallback", "DockSystemButtons");
        
        switch (type) {
            case DockSystemButtonType::PIN:
                config.icon = wxArtProvider::GetBitmap(wxART_TICK_MARK);
                break;
            case DockSystemButtonType::FLOAT:
                config.icon = wxArtProvider::GetBitmap(wxART_FIND_AND_REPLACE);
                break;
            case DockSystemButtonType::CLOSE:
                config.icon = wxArtProvider::GetBitmap(wxART_CLOSE);
                break;
            default:
                config.icon = wxArtProvider::GetBitmap(wxART_CLOSE);
                break;
        }
    } else {
        LOG_INF("SVG icon loaded successfully for type " + std::to_string(static_cast<int>(type)) + 
                ", size: " + std::to_string(config.icon.GetWidth()) + "x" + 
                std::to_string(config.icon.GetHeight()), "DockSystemButtons");
    }
    
    // Create hover and pressed icons with theme support
    // For now, use the same icon for all states
    wxBitmap hoverIcon = config.icon;
    wxBitmap pressedIcon = config.icon;
    wxBitmap altHoverIcon = config.altIcon;
    wxBitmap altPressedIcon = config.altIcon;
    
    // Use themed icons if available, otherwise use the main icon
    config.hoverIcon = hoverIcon.IsOk() ? hoverIcon : config.icon;
    config.pressedIcon = pressedIcon.IsOk() ? pressedIcon : config.icon;
    config.altHoverIcon = altHoverIcon.IsOk() ? altHoverIcon : config.altIcon;
    config.altPressedIcon = altPressedIcon.IsOk() ? altPressedIcon : config.altIcon;
    
    m_buttons.push_back(config);
    
    UpdateLayout();
}

void DockSystemButtons::RemoveButton(DockSystemButtonType type)
{
    int index = GetButtonIndex(type);
    if (index >= 0) {
        // Remove button configuration
        m_buttons.erase(m_buttons.begin() + index);
        
        UpdateLayout();
    }
}

void DockSystemButtons::SetButtonEnabled(DockSystemButtonType type, bool enabled)
{
    int index = GetButtonIndex(type);
    if (index >= 0) {
        m_buttons[index].enabled = enabled;
        Refresh();
    }
}

void DockSystemButtons::SetButtonVisible(DockSystemButtonType type, bool visible)
{
    int index = GetButtonIndex(type);
    if (index >= 0) {
        m_buttons[index].visible = visible;
        UpdateLayout();
    }
}

void DockSystemButtons::SetButtonIcon(DockSystemButtonType type, const wxBitmap& icon)
{
    int index = GetButtonIndex(type);
    if (index >= 0) {
        m_buttons[index].icon = icon;
        Refresh();
    }
}

void DockSystemButtons::SetButtonTooltip(DockSystemButtonType type, const wxString& tooltip)
{
    int index = GetButtonIndex(type);
    if (index >= 0) {
        m_buttons[index].tooltip = tooltip;
    }
}

void DockSystemButtons::SetButtonToggled(DockSystemButtonType type, bool toggled)
{
    int index = GetButtonIndex(type);
    if (index >= 0) {
        m_buttons[index].isToggled = toggled;
        Refresh();
    }
}

bool DockSystemButtons::IsButtonToggled(DockSystemButtonType type) const
{
    int index = GetButtonIndex(type);
    if (index >= 0) {
        return m_buttons[index].isToggled;
    }
    return false;
}

void DockSystemButtons::ToggleButton(DockSystemButtonType type)
{
    int index = GetButtonIndex(type);
    if (index >= 0) {
        m_buttons[index].isToggled = !m_buttons[index].isToggled;
        Refresh();
    }
}

void DockSystemButtons::UpdateLayout()
{
    if (m_buttons.empty()) return;
    
    // Calculate total width needed
    int totalWidth = m_buttonSize * static_cast<int>(m_buttons.size()) + 
                     m_buttonSpacing * (static_cast<int>(m_buttons.size()) - 1) + 
                     m_margin * 2;
    
    // Ensure panel has enough width for all buttons
    int panelWidth = GetSize().GetWidth();
    if (panelWidth < totalWidth) {
        panelWidth = totalWidth;
    }
    
    // Debug: Log layout update
    LOG_INF("UpdateLayout: " + std::to_string(m_buttons.size()) + " buttons, panel width: " + 
            std::to_string(panelWidth) + ", total width: " + std::to_string(totalWidth), "DockSystemButtons");
    
    Refresh();
}

wxSize DockSystemButtons::GetBestSize() const
{
    if (m_buttons.empty()) {
        return wxSize(m_margin * 2, m_buttonSize + m_margin * 2);
    }
    
    int visibleCount = 0;
    for (const auto& button : m_buttons) {
        if (button.visible) visibleCount++;
    }
    
    int width = m_buttonSize * visibleCount + 
                m_buttonSpacing * (visibleCount - 1) + 
                m_margin * 2;
    int height = m_buttonSize + m_margin * 2;
    
    return wxSize(width, height);
}

void DockSystemButtons::UpdateThemeColors()
{
    // Get colors from theme manager
    m_backgroundColor = CFG_COLOUR("SystemButtonBgColour");
    m_buttonBgColor = CFG_COLOUR("SystemButtonBgColour");
    m_buttonHoverColor = wxColour(128, 128, 128); // Light gray for hover
    m_buttonPressedColor = CFG_COLOUR("SystemButtonPressedColour");
    m_buttonBorderColor = wxColour(0, 0, 0, 0); // Transparent border (no border)
    m_buttonTextColor = CFG_COLOUR("PanelTextColour");
    
    Refresh();
}

void DockSystemButtons::OnThemeChanged()
{
    UpdateThemeColors();
}

void DockSystemButtons::OnButtonClick(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    int buttonIndex = GetButtonAtPosition(pos);
    
    if (buttonIndex >= 0 && buttonIndex < static_cast<int>(m_buttons.size())) {
        DockSystemButtonType type = m_buttons[buttonIndex].type;
        ExecuteButtonAction(type);
    }
}

void DockSystemButtons::OnButtonDown(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    int buttonIndex = GetButtonAtPosition(pos);
    
    if (buttonIndex >= 0 && buttonIndex < static_cast<int>(m_buttons.size())) {
        m_pressedButtonIndex = buttonIndex;
        Refresh();
    }
}

void DockSystemButtons::OnButtonUp(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    int buttonIndex = GetButtonAtPosition(pos);
    
    if (m_pressedButtonIndex >= 0 && buttonIndex == m_pressedButtonIndex) {
        // Button was pressed and released on the same button
        if (buttonIndex < static_cast<int>(m_buttons.size())) {
            DockSystemButtonType type = m_buttons[buttonIndex].type;
            ExecuteButtonAction(type);
        }
    }
    
    m_pressedButtonIndex = -1;
    Refresh();
}

void DockSystemButtons::OnButtonHover(wxMouseEvent& event)
{
    wxPoint pos = event.GetPosition();
    
    // Find which button is being hovered
    int newHoveredIndex = GetButtonAtPosition(pos);
    
    if (newHoveredIndex != m_hoveredButtonIndex) {
        m_hoveredButtonIndex = newHoveredIndex;
        Refresh();
    }
}

void DockSystemButtons::OnButtonLeave(wxMouseEvent& event)
{
    wxUnusedVar(event);
    m_hoveredButtonIndex = -1;
    m_pressedButtonIndex = -1;
    Refresh();
}

void DockSystemButtons::OnPaint(wxPaintEvent& event)
{
    wxUnusedVar(event);
    
    wxAutoBufferedPaintDC dc(this);
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    
    if (gc) {
        // Draw normal background
        gc->SetBrush(wxBrush(m_backgroundColor));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRectangle(0, 0, GetSize().GetWidth(), GetSize().GetHeight());
        
        // Debug: Log paint event
        LOG_INF("OnPaint: " + std::to_string(m_buttons.size()) + " buttons", "DockSystemButtons");
        
        // Draw buttons
        for (size_t i = 0; i < m_buttons.size(); ++i) {
            if (m_buttons[i].visible) {
                wxRect buttonRect = GetButtonRect(i);
                bool hovered = (static_cast<int>(i) == m_hoveredButtonIndex);
                bool pressed = (static_cast<int>(i) == m_pressedButtonIndex);
                
                LOG_INF("Drawing button " + std::to_string(i) + " at rect (" + 
                        std::to_string(buttonRect.x) + ", " + std::to_string(buttonRect.y) + 
                        ", " + std::to_string(buttonRect.width) + ", " + std::to_string(buttonRect.height) + ")", "DockSystemButtons");
                
                RenderButton(gc, buttonRect, m_buttons[i], hovered, pressed);
            }
        }
        
        delete gc;
    }
}

void DockSystemButtons::OnSize(wxSizeEvent& event)
{
    event.Skip();
    UpdateLayout();
}

void DockSystemButtons::RenderButton(wxGraphicsContext* gc, const wxRect& rect, 
                                    const DockSystemButtonConfig& config, bool hovered, bool pressed)
{
    // Determine button colors based on state
    wxColour bgColor = m_buttonBgColor;
    wxColour borderColor = m_buttonBorderColor;
    
    if (pressed) {
        bgColor = m_buttonPressedColor;
    } else if (hovered) {
        bgColor = m_buttonHoverColor;
    }
    
    // Draw button background with theme colors (no border)
    gc->SetBrush(wxBrush(bgColor));
    gc->SetPen(*wxTRANSPARENT_PEN);
    
    // Draw rounded rectangle for button
    gc->DrawRoundedRectangle(rect.x, rect.y, rect.width, rect.height, 2);
    
    // Draw icon if available
    if (config.icon.IsOk()) {
        wxBitmap icon;
        
        // Select icon based on toggle state and button state
        if (config.isToggled) {
            // Use alternative icon for toggled state
            icon = pressed ? config.altPressedIcon : (hovered ? config.altHoverIcon : config.altIcon);
        } else {
            // Use normal icon for untoggled state
            icon = pressed ? config.pressedIcon : (hovered ? config.hoverIcon : config.icon);
        }
        
        // Center icon in button
        int iconX = rect.x + (rect.width - icon.GetWidth()) / 2;
        int iconY = rect.y + (rect.height - icon.GetHeight()) / 2;
        
        LOG_INF("Drawing icon at (" + std::to_string(iconX) + ", " + std::to_string(iconY) + 
                ") with size " + std::to_string(icon.GetWidth()) + "x" + 
                std::to_string(icon.GetHeight()) + "), button rect: (" + 
                std::to_string(rect.x) + ", " + std::to_string(rect.y) + ", " + 
                std::to_string(rect.width) + ", " + std::to_string(rect.height) + 
                "), toggled: " + std::to_string(config.isToggled), "DockSystemButtons");
        
        // Draw the actual icon
        gc->DrawBitmap(icon, iconX, iconY, icon.GetWidth(), icon.GetHeight());
    } else {
        LOG_INF("Icon not available for button type " + std::to_string(static_cast<int>(config.type)) + 
                ", icon size: " + std::to_string(config.icon.GetWidth()) + "x" + 
                std::to_string(config.icon.GetWidth()) + ")", "DockSystemButtons");
    }
}

wxRect DockSystemButtons::GetButtonRect(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_buttons.size())) {
        return wxRect();
    }
    
    // Calculate total width needed for visible buttons
    int visibleCount = 0;
    for (const auto& button : m_buttons) {
        if (button.visible) visibleCount++;
    }
    
    int totalWidth = m_buttonSize * visibleCount + 
                     m_buttonSpacing * (visibleCount - 1) + 
                     m_margin * 2;
    
    // Ensure panel has enough width for all buttons
    int panelWidth = GetSize().GetWidth();
    if (panelWidth < totalWidth) {
        panelWidth = totalWidth;
    }
    
    // Calculate button position from left to right
    // Start from the leftmost position
    int x = m_margin;
    int y = m_margin;
    
    // Count visible buttons before this index to calculate offset
    int visibleBeforeIndex = 0;
    for (int i = 0; i < index; ++i) {
        if (m_buttons[i].visible) {
            visibleBeforeIndex++;
        }
    }
    
    // Move right by the number of visible buttons before this index
    x += visibleBeforeIndex * (m_buttonSize + m_buttonSpacing);
    
    LOG_INF("Button " + std::to_string(index) + " position: (" + 
            std::to_string(x) + ", " + std::to_string(y) + 
            "), visible before: " + std::to_string(visibleBeforeIndex) + 
            ", panel width: " + std::to_string(panelWidth), "DockSystemButtons");
    
    return wxRect(x, y, m_buttonSize, m_buttonSize);
}

int DockSystemButtons::GetButtonIndex(DockSystemButtonType type) const
{
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        if (m_buttons[i].type == type) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int DockSystemButtons::GetButtonAtPosition(const wxPoint& pos) const
{
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        if (m_buttons[i].visible) {
            wxRect buttonRect = GetButtonRect(i);
            if (buttonRect.Contains(pos)) {
                return static_cast<int>(i);
            }
        }
    }
    return -1;
}

void DockSystemButtons::ExecuteButtonAction(DockSystemButtonType type)
{
    // Handle button action based on type
    LOG_INF("System button clicked: " + std::to_string(static_cast<int>(type)), "DockSystemButtons");
    
    // Handle toggle buttons automatically
    switch (type) {
        case DockSystemButtonType::PIN:
            // PIN toggle: pinned <-> unpinned
            ToggleButton(type);
            LOG_INF("Toggled PIN button to state: " + 
                    std::string(IsButtonToggled(type) ? "unpinned" : "pinned"), "DockSystemButtons");
            break;
            
        case DockSystemButtonType::MINIMIZE:
            // MIN/MAX toggle: minimize <-> maximize
            ToggleButton(type);
            LOG_INF("Toggled MIN/MAX button to state: " + 
                    std::string(IsButtonToggled(type) ? "maximize" : "minimize"), "DockSystemButtons");
            break;
            
        case DockSystemButtonType::CLOSE:
            // Close button is not a toggle
            LOG_INF("Close button clicked", "DockSystemButtons");
            break;
            
        default:
            break;
    }
    
    // Notify parent panel about button action
    // This would typically be handled by the parent ModernDockPanel
    // TODO: Implement actual button actions
}

wxString DockSystemButtons::GetIconName(DockSystemButtonType type) const
{
    switch (type) {
        case DockSystemButtonType::MINIMIZE:
            return "minimize";
        case DockSystemButtonType::MAXIMIZE:
            return "maximize";
        case DockSystemButtonType::CLOSE:
            return "close";
        case DockSystemButtonType::PIN:
            return "thumbtack";
        case DockSystemButtonType::FLOAT:
            return "maximize";
        case DockSystemButtonType::DOCK:
            return "unpin";
        default:
            return "close";
    }
}

