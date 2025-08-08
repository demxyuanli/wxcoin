#include "widgets/FlatButton.h"
#include <wx/dcgraph.h>
#include <wx/renderer.h>
#include <wx/graphics.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include "config/FontManager.h"
#include "config/ThemeManager.h"
#include <algorithm>

wxDEFINE_EVENT(wxEVT_FLAT_BUTTON_CLICKED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_FLAT_BUTTON_HOVER, wxCommandEvent);

BEGIN_EVENT_TABLE(FlatButton, wxControl)
    EVT_PAINT(FlatButton::OnPaint)
    EVT_SIZE(FlatButton::OnSize)
    EVT_ERASE_BACKGROUND(FlatButton::OnEraseBackground)
    EVT_LEFT_DOWN(FlatButton::OnMouseDown)
    EVT_LEFT_UP(FlatButton::OnMouseUp)
    EVT_MOTION(FlatButton::OnMouseMove)
    EVT_LEAVE_WINDOW(FlatButton::OnMouseLeave)
    EVT_ENTER_WINDOW(FlatButton::OnMouseEnter)
    EVT_KEY_DOWN(FlatButton::OnKeyDown)
    EVT_KEY_UP(FlatButton::OnKeyUp)
    EVT_SET_FOCUS(FlatButton::OnFocus)
    EVT_KILL_FOCUS(FlatButton::OnKillFocus)
    EVT_TIMER(wxID_ANY, FlatButton::OnAnimationTimer)
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

    // Add a theme change listener
    ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
        InitializeDefaultColors();
        Refresh();
    });

    // Use transparent background so parent paints beneath; we draw our content on top
    SetBackgroundStyle(wxBG_STYLE_PAINT); 
    SetDoubleBuffered(true);
}

FlatButton::~FlatButton()
{
    if (m_animationTimer.IsRunning()) {
        m_animationTimer.Stop();
    }
    ThemeManager::getInstance().removeThemeChangeListener(this);
}

void FlatButton::InitializeDefaultColors()
{
    // Fluent Design System inspired colors from ThemeManager
    switch (m_buttonStyle) {
    case ButtonStyle::PRIMARY:
        m_backgroundColor = CFG_COLOUR("AccentColour");
        m_hoverColor = CFG_COLOUR("HighlightColour");
        m_pressedColor = CFG_COLOUR("AccentColour");
        m_textColor = CFG_COLOUR("PrimaryTextColour");
        m_borderColor = CFG_COLOUR("AccentColour");
        break;

    case ButtonStyle::SECONDARY:
        m_backgroundColor = CFG_COLOUR("SecondaryBackgroundColour");
        m_hoverColor = CFG_COLOUR("HomespaceHoverBgColour");
        m_pressedColor = CFG_COLOUR("ButtonbarDefaultPressedBgColour");
        m_textColor = CFG_COLOUR("PrimaryTextColour");
        m_borderColor = CFG_COLOUR("ButtonBorderColour");
        break;

    case ButtonStyle::DEFAULT_TRANSPARENT:
        m_backgroundColor = wxTransparentColour;
        m_hoverColor = CFG_COLOUR("HomespaceHoverBgColour");
        m_pressedColor = CFG_COLOUR("ButtonbarDefaultPressedBgColour");
        m_textColor = CFG_COLOUR("PrimaryTextColour");
        m_borderColor = wxTransparentColour;
        break;

    case ButtonStyle::OUTLINE:
        m_backgroundColor = CFG_COLOUR("SecondaryBackgroundColour");
        m_hoverColor = CFG_COLOUR("HomespaceHoverBgColour");
        m_pressedColor = CFG_COLOUR("ButtonbarDefaultPressedBgColour");
        m_textColor = CFG_COLOUR("AccentColour");
        m_borderColor = CFG_COLOUR("AccentColour");
        break;

    case ButtonStyle::TEXT:
        m_backgroundColor = wxTransparentColour;
        m_hoverColor = CFG_COLOUR("HomespaceHoverBgColour");
        m_pressedColor = CFG_COLOUR("ButtonbarDefaultPressedBgColour");
        m_textColor = CFG_COLOUR("AccentColour");
        m_borderColor = wxTransparentColour;
        break;

    case ButtonStyle::ICON_ONLY:
    case ButtonStyle::ICON_WITH_TEXT:
        m_backgroundColor = wxTransparentColour;
        m_hoverColor = CFG_COLOUR("HomespaceHoverBgColour");
        m_pressedColor = CFG_COLOUR("ButtonbarDefaultPressedBgColour");
        m_textColor = CFG_COLOUR("PrimaryTextColour");
        m_borderColor = wxTransparentColour;
        break;
    }

    // Disabled state color
    m_disabledColor = CFG_COLOUR("PanelDisabledBgColour");
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
    }
    else if (m_icon.IsOk()) {
        width += iconSize.GetWidth();
        height += iconSize.GetHeight();
    }
    else {
        width += textSize.GetWidth();
        height += textSize.GetHeight();
    }

    return wxSize(width, height);
}

void FlatButton::OnPaint(wxPaintEvent& event)
{
    wxUnusedVar(event);
    wxPaintDC dc(this);
    // Base clear with parent's background to avoid black artifacts when using transparent styles
    wxColour baseBg = GetParent() ? GetParent()->GetBackgroundColour() : wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    dc.SetBackground(wxBrush(baseBg));
    dc.Clear();

    // Use wxGraphicsContext for high-quality, anti-aliased rendering
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);

    if (gc)
    {
        // Set the font for drawing
        wxFont currentFont = GetFont();
        if (currentFont.IsOk()) {
            gc->SetFont(currentFont, GetCurrentTextColor());
        }

        // Draw in the correct order: shadow -> background -> border -> icon -> text
        DrawBackground(*gc);
        DrawBorder(*gc);
        DrawIcon(*gc);
        DrawText(*gc);

        delete gc;
    }
}

void FlatButton::OnSize(wxSizeEvent& event)
{
    wxUnusedVar(event);
    Refresh();
    event.Skip();
}

void FlatButton::OnEraseBackground(wxEraseEvent& event)
{
    // Prevent background erase to reduce flicker during hover/press redraws
    wxUnusedVar(event);
}

void FlatButton::OnMouseDown(wxMouseEvent& event)
{
    wxUnusedVar(event);
    if (!m_enabled) return;

    m_isPressed = true;
    UpdateState(ButtonState::PRESSED);
    CaptureMouse();
    m_animationProgress = 0.0;
    m_animationTimer.Start(10);
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

        m_animationProgress = 0.0;
        m_animationTimer.Start(10);
    }
}

// Helper function for color interpolation
wxColour InterpolateColour(const wxColour& start, const wxColour& end, double progress)
{
    unsigned char r = start.Red() + (end.Red() - start.Red()) * progress;
    unsigned char g = start.Green() + (end.Green() - start.Green()) * progress;
    unsigned char b = start.Blue() + (end.Blue() - start.Blue()) * progress;
    unsigned char a = start.Alpha() + (end.Alpha() - start.Alpha()) * progress;
    return wxColour(r, g, b, a);
}

void FlatButton::OnAnimationTimer(wxTimerEvent& event)
{
    wxUnusedVar(event);
    m_animationProgress += 0.1;
    if (m_animationProgress >= 1.0) {
        m_animationProgress = 1.0;
        m_animationTimer.Stop();
    }
    Refresh();
}

void FlatButton::OnMouseMove(wxMouseEvent& event)
{
    if (!m_enabled) return;

    wxPoint mousePos = event.GetPosition();
    bool isOverButton = GetClientRect().Contains(mousePos);

    if (isOverButton && !m_isHovered) {
        OnMouseEnter(event);
    }
    else if (!isOverButton && m_isHovered) {
        OnMouseLeave(event);
    }
}

void FlatButton::OnMouseLeave(wxMouseEvent& event)
{
    wxUnusedVar(event);
    if (!m_enabled) return;

    m_isHovered = false;
    UpdateState(ButtonState::NORMAL);
    m_animationProgress = 0.0;
    m_animationTimer.Start(10);
}

void FlatButton::OnMouseEnter(wxMouseEvent& event)
{
    wxUnusedVar(event);
    if (!m_enabled) return;

    m_isHovered = true;
    UpdateState(ButtonState::HOVERED);
    m_animationProgress = 0.0;
    m_animationTimer.Start(10);
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

void FlatButton::DrawBackground(wxGraphicsContext& gc)
{
    wxRect rect = GetClientRect();
    wxColour bgColor = GetCurrentBackgroundColor();

    // Draw shadow first (if needed)
    if ((m_buttonStyle == ButtonStyle::PRIMARY || m_buttonStyle == ButtonStyle::SECONDARY) &&
        m_state == ButtonState::NORMAL) {
        DrawSubtleShadow(gc, rect);
    }

    // Draw background
    if (bgColor != wxTransparentColour && bgColor.IsOk() && bgColor.Alpha() > 0) {
        gc.SetBrush(wxBrush(bgColor));
        gc.SetPen(*wxTRANSPARENT_PEN);

        if (m_cornerRadius > 0) {
            gc.DrawRoundedRectangle(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight(), m_cornerRadius);
        }
        else {
            gc.DrawRectangle(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
        }
    }
}

void FlatButton::DrawSubtleShadow(wxGraphicsContext& gc, const wxRect& rect)
{
    // Draw a subtle shadow effect for elevated buttons
    wxColour shadowColor = wxColour(0, 0, 0, 15); // Very subtle shadow

    // Create shadow rectangle (slightly offset)
    wxRect shadowRect = rect;
    shadowRect.x += 2;
    shadowRect.y += 2;

    // Draw shadow with rounded corners if needed
    gc.SetBrush(wxBrush(shadowColor));
    gc.SetPen(*wxTRANSPARENT_PEN);

    if (m_cornerRadius > 0) {
        gc.DrawRoundedRectangle(shadowRect.GetX(), shadowRect.GetY(), shadowRect.GetWidth(), shadowRect.GetHeight(), m_cornerRadius);
    }
    else {
        gc.DrawRectangle(shadowRect.GetX(), shadowRect.GetY(), shadowRect.GetWidth(), shadowRect.GetHeight());
    }
}

void FlatButton::DrawBorder(wxGraphicsContext& gc)
{
    if (m_borderWidth <= 0) return;

    wxRect rect = GetClientRect();
    wxColour borderColor = GetCurrentBorderColor();
    if (borderColor == wxTransparentColour || !borderColor.IsOk()) return;

    // Deflate by at least 1px on all sides to avoid clipping (or half pen width if larger)
    const double strokeWidth = static_cast<double>(m_borderWidth);
    const double inset = std::max(1.0, strokeWidth * 0.5);
    const double x = static_cast<double>(rect.GetX()) + inset;
    const double y = static_cast<double>(rect.GetY()) + inset;
    const double w = static_cast<double>(rect.GetWidth()) - 2.0 * inset;
    const double h = static_cast<double>(rect.GetHeight()) - 2.0 * inset;

    gc.SetBrush(*wxTRANSPARENT_BRUSH);
    gc.SetPen(wxPen(borderColor, m_borderWidth));

    if (m_cornerRadius > 0) {
        gc.DrawRoundedRectangle(x, y, w, h, m_cornerRadius);
    } else {
        gc.DrawRectangle(x, y, w, h);
    }
}

void FlatButton::DrawText(wxGraphicsContext& gc)
{
    if (m_label.IsEmpty()) return;

    wxRect textRect = GetTextRect();
    wxColour textColor = GetCurrentTextColor();

    // Set the font for drawing
    wxFont currentFont = GetFont();
    if (currentFont.IsOk()) {
        gc.SetFont(currentFont, textColor);
    }
    else {
        gc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT), textColor);
    }

    // Center text vertically and horizontally
    double textWidth, textHeight;
    gc.GetTextExtent(m_label, &textWidth, &textHeight);

    double x = textRect.x + (textRect.width - textWidth) / 2;
    double y = textRect.y + (textRect.height - textHeight) / 2;

    gc.DrawText(m_label, x, y);
}

void FlatButton::DrawIcon(wxGraphicsContext& gc)
{
    if (!m_icon.IsOk()) return;

    wxRect iconRect = GetIconRect();

    // Draw the icon
    gc.DrawBitmap(m_icon, iconRect.GetX(), iconRect.GetY(), iconRect.GetWidth(), iconRect.GetHeight());
}

void FlatButton::DrawRoundedRectangle(wxGraphicsContext& gc, const wxRect& rect, int radius)
{
    if (radius <= 0) {
        gc.DrawRectangle(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight());
        return;
    }
    gc.DrawRoundedRectangle(rect.GetX(), rect.GetY(), rect.GetWidth(), rect.GetHeight(), radius);
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

    // Also emit standard button event for compatibility
    wxCommandEvent stdEvent(wxEVT_BUTTON, GetId());
    stdEvent.SetEventObject(this);
    stdEvent.SetString(m_label);
    ProcessWindowEvent(stdEvent);
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

    wxColour startColor = m_backgroundColor;
    wxColour endColor = m_backgroundColor;

    switch (m_state) {
    case ButtonState::PRESSED:
        startColor = m_hoverColor;
        endColor = m_pressedColor;
        break;
    case ButtonState::HOVERED:
        startColor = m_backgroundColor;
        endColor = m_hoverColor;
        break;
    default:
        startColor = m_hoverColor;
        endColor = m_backgroundColor;
        break;
    }

    return InterpolateColour(startColor, endColor, m_animationProgress);
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
        }
        catch (...) {
            // If font manager is not available, use default font
            wxFont defaultFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
            SetFont(defaultFont);
        }
    }
}
