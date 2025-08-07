#include "widgets/FlatButton.h"
#include <wx/dcgraph.h>
#include <wx/renderer.h>
#include <wx/graphics.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include "config/FontManager.h"

wxDEFINE_EVENT(wxEVT_FLAT_BUTTON_CLICKED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_BUTTON_HOVER, wxCommandEvent);

BEGIN_EVENT_TABLE(FlatButton, wxControl)
    EVT_PAINT(FlatButton::OnPaint)
    EVT_SIZE(FlatButton::OnSize)
    EVT_LEFT_DOWN(FlatButton::OnMouseDown)
    EVT_LEFT_UP(FlatButton::OnMouseUp)
    EVT_MOTION(FlatButton::OnMouseMove)
    EVT_LEAVE_WINDOW(FlatButton::OnMouseLeave)
    EVT_ENTER_WINDOW(FlatButton::OnMouseEnter)
    EVT_KEY_DOWN(FlatButton::OnKeyDown)
    EVT_KEY_UP(FlatButton::OnKeyUp)
    EVT_SET_FOCUS(FlatButton::OnFocus)
    EVT_KILL_FOCUS(FlatButton::OnKillFocus)
END_EVENT_TABLE()

FlatButton::FlatButton(wxWindow* parent, wxWindowID id, const wxString& label,
                       const wxPoint& pos, const wxSize& size, ButtonStyle style, long style_flags)
    : wxControl(parent, id, pos, size, style_flags | wxBORDER_NONE)
    , m_label(label)
    , m_buttonStyle(style)
    , m_state(ButtonState::NORMAL)
    , m_enabled(true)
    , m_isPressed(false)
    , m_isHovered(false)
    , m_hasFocus(false)
    , m_borderWidth(DEFAULT_BORDER_WIDTH)
    , m_cornerRadius(DEFAULT_CORNER_RADIUS)
    , m_iconTextSpacing(DEFAULT_ICON_TEXT_SPACING)
    , m_horizontalPadding(DEFAULT_PADDING_H)
    , m_verticalPadding(DEFAULT_PADDING_V)
    , m_iconSize(DEFAULT_ICON_SIZE, DEFAULT_ICON_SIZE)
    , m_animationProgress(0.0)
    , m_useConfigFont(true)
{
    // Initialize default colors based on style
    InitializeDefaultColors();
    
    // Initialize font from configuration
    ReloadFontFromConfig();
    
    // Set default size if not specified
    if (size == wxDefaultSize) {
        SetInitialSize(DoGetBestSize());
    }
    
    // Initialize animation timer
    m_animationTimer.SetOwner(this);
}

FlatButton::~FlatButton()
{
    if (m_animationTimer.IsRunning()) {
        m_animationTimer.Stop();
    }
}

void FlatButton::InitializeDefaultColors()
{
    // Fluent Design System inspired colors (based on PyQt-Fluent-Widgets)
    switch (m_buttonStyle) {
        case ButtonStyle::PRIMARY:
            // Primary button - Accent color
            m_backgroundColor = wxColour(32, 167, 232);  // Fluent Blue
            m_hoverColor = wxColour(50, 180, 240);       // Lighter blue on hover
            m_pressedColor = wxColour(20, 140, 200);      // Darker blue on press
            m_textColor = wxColour(255, 255, 255);      // White text
            m_borderColor = wxColour(32, 167, 232);      // Same as background
            break;
            
        case ButtonStyle::SECONDARY:
            // Secondary button - Subtle gray background
            m_backgroundColor = wxColour(240, 240, 240); // Light gray
            m_hoverColor = wxColour(230, 230, 230);      // Slightly darker on hover
            m_pressedColor = wxColour(220, 220, 220);    // Even darker on press
            m_textColor = wxColour(32, 32, 32);          // Dark gray text
            m_borderColor = wxColour(200, 200, 200);     // Light border
            break;
            
        case ButtonStyle::TRANSPARENTS:
            // Transparent button - No background
            m_backgroundColor = wxTransparentColour;     // Transparent
            m_hoverColor = wxColour(0, 0, 0, 20);       // Subtle hover effect
            m_pressedColor = wxColour(0, 0, 0, 40);     // More visible press effect
            m_textColor = wxColour(32, 32, 32);          // Dark text
            m_borderColor = wxTransparentColour;         // No border
            break;
            
        case ButtonStyle::OUTLINE:
            // Outline button - White background with colored border
            m_backgroundColor = wxColour(255, 255, 255); // White background
            m_hoverColor = wxColour(248, 248, 248);      // Very light gray on hover
            m_pressedColor = wxColour(240, 240, 240);    // Light gray on press
            m_textColor = wxColour(32, 167, 232);         // Blue text
            m_borderColor = wxColour(32, 167, 232);       // Blue border
            break;
            
        case ButtonStyle::TEXT:
            // Text button - No background, colored text
            m_backgroundColor = wxTransparentColour;     // Transparent
            m_hoverColor = wxColour(0, 0, 0, 20);       // Subtle hover effect
            m_pressedColor = wxColour(0, 0, 0, 40);     // More visible press effect
            m_textColor = wxColour(32, 167, 232);        // Blue text
            m_borderColor = wxTransparentColour;         // No border
            break;
            
        case ButtonStyle::ICON_ONLY:
        case ButtonStyle::ICON_WITH_TEXT:
            // Icon buttons - Transparent with subtle effects
            m_backgroundColor = wxTransparentColour;     // Transparent
            m_hoverColor = wxColour(0, 0, 0, 20);       // Subtle hover effect
            m_pressedColor = wxColour(0, 0, 0, 40);     // More visible press effect
            m_textColor = wxColour(32, 32, 32);          // Dark text/icon
            m_borderColor = wxTransparentColour;         // No border
            break;
    }
    
    // Disabled state color
    m_disabledColor = wxColour(240, 240, 240);
}

void FlatButton::SetLabel(const wxString& label)
{
    if (m_label != label) {
        m_label = label;
        InvalidateBestSize();
        Refresh();
    }
}

void FlatButton::SetIcon(const wxBitmap& icon)
{
    m_icon = icon;
    InvalidateBestSize();
    Refresh();
}

void FlatButton::SetIconSize(const wxSize& size)
{
    m_iconSize = size;
    InvalidateBestSize();
    Refresh();
}

void FlatButton::SetButtonStyle(ButtonStyle style)
{
    if (m_buttonStyle != style) {
        m_buttonStyle = style;
        InitializeDefaultColors();
        Refresh();
    }
}

void FlatButton::SetBackgroundColor(const wxColour& color)
{
    m_backgroundColor = color;
    Refresh();
}

void FlatButton::SetHoverColor(const wxColour& color)
{
    m_hoverColor = color;
    Refresh();
}

void FlatButton::SetPressedColor(const wxColour& color)
{
    m_pressedColor = color;
    Refresh();
}

void FlatButton::SetTextColor(const wxColour& color)
{
    m_textColor = color;
    Refresh();
}

void FlatButton::SetBorderColor(const wxColour& color)
{
    m_borderColor = color;
    Refresh();
}

void FlatButton::SetBorderWidth(int width)
{
    m_borderWidth = width;
    Refresh();
}

void FlatButton::SetCornerRadius(int radius)
{
    m_cornerRadius = radius;
    Refresh();
}

void FlatButton::SetIconTextSpacing(int spacing)
{
    m_iconTextSpacing = spacing;
    InvalidateBestSize();
    Refresh();
}

void FlatButton::SetPadding(int horizontal, int vertical)
{
    m_horizontalPadding = horizontal;
    m_verticalPadding = vertical;
    InvalidateBestSize();
    Refresh();
}

void FlatButton::GetPadding(int& horizontal, int& vertical) const
{
    horizontal = m_horizontalPadding;
    vertical = m_verticalPadding;
}

void FlatButton::SetEnabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        UpdateState(enabled ? ButtonState::NORMAL : ButtonState::DISABLED);
        Refresh();
    }
}

void FlatButton::SetPressed(bool pressed)
{
    if (m_isPressed != pressed) {
        m_isPressed = pressed;
        UpdateState(pressed ? ButtonState::PRESSED : ButtonState::NORMAL);
        Refresh();
    }
}

wxSize FlatButton::DoGetBestSize() const
{
    wxClientDC dc(const_cast<FlatButton*>(this));
    dc.SetFont(GetFont());
    
    wxSize textSize = dc.GetTextExtent(m_label);
    wxSize iconSize = m_icon.IsOk() ? m_iconSize : wxSize(0, 0);
    
    int width = m_horizontalPadding * 2;
    int height = m_verticalPadding * 2;
    
    if (m_icon.IsOk() && !m_label.IsEmpty()) {
        width += iconSize.GetWidth() + m_iconTextSpacing + textSize.GetWidth();
        height += wxMax(iconSize.GetHeight(), textSize.GetHeight());
    } else if (m_icon.IsOk()) {
        width += iconSize.GetWidth();
        height += iconSize.GetHeight();
    } else {
        width += textSize.GetWidth();
        height += textSize.GetHeight();
    }
    
    return wxSize(width, height);
}

void FlatButton::OnPaint(wxPaintEvent& event)
{
    wxUnusedVar(event);
    wxPaintDC dc(this);
    
    // Enable high-quality rendering
    dc.SetLogicalFunction(wxCOPY);
    
    // Set the font for drawing
    wxFont currentFont = GetFont();
    if (currentFont.IsOk()) {
        dc.SetFont(currentFont);
    }
    
    // Draw in the correct order: shadow -> background -> border -> icon -> text
    DrawBackground(dc);
    DrawBorder(dc);
    DrawIcon(dc);
    DrawText(dc);
}

void FlatButton::OnSize(wxSizeEvent& event)
{
    wxUnusedVar(event);
    Refresh();
    event.Skip();
}

void FlatButton::OnMouseDown(wxMouseEvent& event)
{
    wxUnusedVar(event);
    if (!m_enabled) return;
    
    m_isPressed = true;
    UpdateState(ButtonState::PRESSED);
    CaptureMouse();
    Refresh();
}

void FlatButton::OnMouseUp(wxMouseEvent& event)
{
    wxUnusedVar(event);
    if (!m_enabled) return;
    
    if (m_isPressed) {
        m_isPressed = false;
        UpdateState(ButtonState::HOVERED);
        
        if (HasCapture()) {
            ReleaseMouse();
        }
        
        // Check if mouse is still over the button
        wxPoint mousePos = ScreenToClient(wxGetMousePosition());
        if (GetClientRect().Contains(mousePos)) {
            SendButtonEvent();
        }
        
        Refresh();
    }
}

void FlatButton::OnMouseMove(wxMouseEvent& event)
{
    if (!m_enabled) return;
    
    wxPoint mousePos = event.GetPosition();
    bool isOverButton = GetClientRect().Contains(mousePos);
    
    if (isOverButton && !m_isHovered) {
        m_isHovered = true;
        UpdateState(ButtonState::HOVERED);
        Refresh();
    } else if (!isOverButton && m_isHovered) {
        m_isHovered = false;
        UpdateState(ButtonState::NORMAL);
        Refresh();
    }
}

void FlatButton::OnMouseLeave(wxMouseEvent& event)
{
    wxUnusedVar(event);
    if (!m_enabled) return;
    
    m_isHovered = false;
    UpdateState(ButtonState::NORMAL);
    Refresh();
}

void FlatButton::OnMouseEnter(wxMouseEvent& event)
{
    wxUnusedVar(event);
    if (!m_enabled) return;
    
    m_isHovered = true;
    UpdateState(ButtonState::HOVERED);
    Refresh();
}

void FlatButton::OnKeyDown(wxKeyEvent& event)
{
    if (!m_enabled) return;
    
    if (event.GetKeyCode() == WXK_SPACE || event.GetKeyCode() == WXK_RETURN) {
        m_isPressed = true;
        UpdateState(ButtonState::PRESSED);
        Refresh();
    }
    
    event.Skip();
}

void FlatButton::OnKeyUp(wxKeyEvent& event)
{
    if (!m_enabled) return;
    
    if (event.GetKeyCode() == WXK_SPACE || event.GetKeyCode() == WXK_RETURN) {
        if (m_isPressed) {
            m_isPressed = false;
            UpdateState(ButtonState::NORMAL);
            SendButtonEvent();
            Refresh();
        }
    }
    
    event.Skip();
}

void FlatButton::OnFocus(wxFocusEvent& event)
{
    m_hasFocus = true;
    Refresh();
    event.Skip();
}

void FlatButton::OnKillFocus(wxFocusEvent& event)
{
    m_hasFocus = false;
    m_isPressed = false;
    UpdateState(ButtonState::NORMAL);
    Refresh();
    event.Skip();
}

void FlatButton::DrawBackground(wxDC& dc)
{
    wxRect rect = GetClientRect();
    wxColour bgColor = GetCurrentBackgroundColor();
    
    // Draw shadow first (if needed)
    if ((m_buttonStyle == ButtonStyle::PRIMARY || m_buttonStyle == ButtonStyle::SECONDARY) && 
        m_state == ButtonState::NORMAL) {
        DrawSubtleShadow(dc, rect);
    }
    
    // Draw background
    if (bgColor != wxTransparentColour && bgColor.Alpha() > 0) {
        // Set up the brush and pen for drawing
        dc.SetBrush(wxBrush(bgColor));
        dc.SetPen(*wxTRANSPARENT_PEN);
        
        if (m_cornerRadius > 0) {
            // Draw rounded rectangle with better quality
            DrawRoundedRectangle(dc, rect, m_cornerRadius);
        } else {
            dc.DrawRectangle(rect);
        }
    }
}

void FlatButton::DrawSubtleShadow(wxDC& dc, const wxRect& rect)
{
    // Draw a subtle shadow effect for elevated buttons
    wxColour shadowColor = wxColour(0, 0, 0, 15); // Very subtle shadow
    
    // Create shadow rectangle (slightly offset)
    wxRect shadowRect = rect;
    shadowRect.x += 2;
    shadowRect.y += 2;
    
    // Draw shadow with rounded corners if needed
    dc.SetBrush(wxBrush(shadowColor));
    dc.SetPen(*wxTRANSPARENT_PEN);
    
    if (m_cornerRadius > 0) {
        DrawRoundedRectangle(dc, shadowRect, m_cornerRadius);
    } else {
        dc.DrawRectangle(shadowRect);
    }
}

void FlatButton::DrawBorder(wxDC& dc)
{
    if (m_borderWidth > 0) {
        wxRect rect = GetClientRect();
        wxColour borderColor = GetCurrentBorderColor();
        
        if (borderColor != wxTransparentColour) {
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.SetPen(wxPen(borderColor, m_borderWidth));
            
            if (m_cornerRadius > 0) {
                DrawRoundedRectangle(dc, rect, m_cornerRadius);
            } else {
                dc.DrawRectangle(rect);
            }
        }
    }
}

void FlatButton::DrawText(wxDC& dc)
{
    if (m_label.IsEmpty()) return;
    
    wxRect textRect = GetTextRect();
    wxColour textColor = GetCurrentTextColor();
    
    // Set the font for drawing
    wxFont currentFont = GetFont();
    if (currentFont.IsOk()) {
        dc.SetFont(currentFont);
    }
    
    dc.SetTextForeground(textColor);
    dc.SetTextBackground(wxTransparentColour);
    
    // Center text vertically and horizontally
    wxSize textSize = dc.GetTextExtent(m_label);
    int x = textRect.x + (textRect.width - textSize.GetWidth()) / 2;
    int y = textRect.y + (textRect.height - textSize.GetHeight()) / 2;
    
    dc.DrawText(m_label, x, y);
}

void FlatButton::DrawIcon(wxDC& dc)
{
    if (!m_icon.IsOk()) return;
    
    wxRect iconRect = GetIconRect();
    
    // Create a memory DC for the icon
    wxMemoryDC memDC;
    memDC.SelectObject(m_icon);
    
    // Draw the icon
    dc.Blit(iconRect.x, iconRect.y, iconRect.width, iconRect.height,
            &memDC, 0, 0, wxCOPY, true);
}

void FlatButton::DrawRoundedRectangle(wxDC& dc, const wxRect& rect, int radius)
{
    // High-quality rounded rectangle implementation using precise arc drawing
    if (radius <= 0) {
        dc.DrawRectangle(rect);
        return;
    }
    
    // Ensure radius doesn't exceed half the width or height
    int maxRadius = wxMin(rect.width, rect.height) / 2;
    radius = wxMin(radius, maxRadius);
    
    // Calculate corner positions
    int x = rect.x;
    int y = rect.y;
    int width = rect.width;
    int height = rect.height;
    
    // Check if we're drawing a border by checking if the brush is transparent
    bool isDrawingBorder = (dc.GetBrush().GetColour() == wxTransparentColour);
    
    if (isDrawingBorder) {
        // Draw border outline using a more precise approach
        // Draw the four straight edges
        if (width > 2 * radius) {
            dc.DrawLine(x + radius, y, x + width - radius, y); // Top
            dc.DrawLine(x + radius, y + height, x + width - radius, y + height); // Bottom
        }
        if (height > 2 * radius) {
            dc.DrawLine(x, y + radius, x, y + height - radius); // Left
            dc.DrawLine(x + width, y + radius, x + width, y + height - radius); // Right
        }
        
        // Draw the four corner arcs using precise parameters
        // Top-left corner (90-degree arc from top to left)
        dc.DrawArc(x + radius, y + radius, x + radius, y, x, y + radius);
        
        // Top-right corner (90-degree arc from right to top)
        dc.DrawArc(x + width - radius, y + radius, x + width, y + radius, x + width - radius, y);
        
        // Bottom-right corner (90-degree arc from bottom to right)
        dc.DrawArc(x + width - radius, y + height - radius, x + width - radius, y + height, x + width, y + height - radius);
        
        // Bottom-left corner (90-degree arc from left to bottom)
        dc.DrawArc(x + radius, y + height - radius, x, y + height - radius, x + radius, y + height);
    } else {
        // Fill the rounded rectangle using a more robust approach
        // This approach draws the rounded rectangle as a series of filled shapes
        
        // Fill the center rectangle
        if (width > 2 * radius) {
            dc.DrawRectangle(x + radius, y, width - 2 * radius, height);
        }
        
        // Fill the left rectangle
        if (height > 2 * radius) {
            dc.DrawRectangle(x, y + radius, radius, height - 2 * radius);
        }
        
        // Fill the right rectangle
        if (height > 2 * radius) {
            dc.DrawRectangle(x + width - radius, y + radius, radius, height - 2 * radius);
        }
        
        // Fill the top rectangle
        if (width > 2 * radius) {
            dc.DrawRectangle(x + radius, y, width - 2 * radius, radius);
        }
        
        // Fill the bottom rectangle
        if (width > 2 * radius) {
            dc.DrawRectangle(x + radius, y + height - radius, width - 2 * radius, radius);
        }
        
        // Draw the corner arcs to fill the corners
        // We need to use a different approach for filling arcs
        // Create filled pie segments for the corners
        
        // Top-left corner
        dc.DrawArc(x + radius, y + radius, x + radius, y, x, y + radius);
        
        // Top-right corner
        dc.DrawArc(x + width - radius, y + radius, x + width, y + radius, x + width - radius, y);
        
        // Bottom-right corner
        dc.DrawArc(x + width - radius, y + height - radius, x + width - radius, y + height, x + width, y + height - radius);
        
        // Bottom-left corner
        dc.DrawArc(x + radius, y + height - radius, x, y + height - radius, x + radius, y + height);
    }
}

void FlatButton::UpdateState(ButtonState newState)
{
    if (m_state != newState) {
        m_state = newState;
        Refresh();
    }
}

void FlatButton::SendButtonEvent()
{
    wxCommandEvent event(wxEVT_FLAT_BUTTON_CLICKED, GetId());
    event.SetEventObject(this);
    event.SetString(m_label);
    ProcessWindowEvent(event);
}

wxRect FlatButton::GetTextRect() const
{
    wxRect rect = GetClientRect();
    rect.Deflate(m_horizontalPadding, m_verticalPadding);
    
    if (m_icon.IsOk() && !m_label.IsEmpty()) {
        // Adjust for icon
        rect.x += m_iconSize.GetWidth() + m_iconTextSpacing;
        rect.width -= m_iconSize.GetWidth() + m_iconTextSpacing;
    }
    
    return rect;
}

wxRect FlatButton::GetIconRect() const
{
    if (!m_icon.IsOk()) return wxRect();
    
    wxRect rect = GetClientRect();
    rect.Deflate(m_horizontalPadding, m_verticalPadding);
    
    // Center the icon
    int x = rect.x + (rect.width - m_iconSize.GetWidth()) / 2;
    int y = rect.y + (rect.height - m_iconSize.GetHeight()) / 2;
    
    return wxRect(x, y, m_iconSize.GetWidth(), m_iconSize.GetHeight());
}

wxColour FlatButton::GetCurrentBackgroundColor() const
{
    if (!m_enabled) return m_disabledColor;
    
    switch (m_state) {
        case ButtonState::PRESSED: return m_pressedColor;
        case ButtonState::HOVERED: return m_hoverColor;
        default: return m_backgroundColor;
    }
}

wxColour FlatButton::GetCurrentTextColor() const
{
    if (!m_enabled) return wxColour(128, 128, 128);
    return m_textColor;
}

wxColour FlatButton::GetCurrentBorderColor() const
{
    if (!m_enabled) return wxColour(200, 200, 200);
    if (m_borderColor == wxTransparentColour) return wxTransparentColour;
    return m_borderColor;
}

// Font configuration methods
void FlatButton::SetCustomFont(const wxFont& font)
{
    m_customFont = font;
    m_useConfigFont = false;
    SetFont(font);
    InvalidateBestSize();
    Refresh();
}

void FlatButton::UseConfigFont(bool useConfig)
{
    m_useConfigFont = useConfig;
    if (useConfig) {
        ReloadFontFromConfig();
    }
}

void FlatButton::ReloadFontFromConfig()
{
    if (m_useConfigFont) {
        try {
            FontManager& fontManager = FontManager::getInstance();
            wxFont configFont = fontManager.getButtonFont();
            if (configFont.IsOk()) {
                SetFont(configFont);
                InvalidateBestSize();
                Refresh();
            }
        } catch (...) {
            // If font manager is not available, use default font
            wxFont defaultFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
            SetFont(defaultFont);
        }
    }
}
