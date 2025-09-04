// DockStyle.cpp - Docking style configuration and helper functions

#include "docking/DockArea.h"
#include "config/ThemeManager.h"
#include "config/SvgIconManager.h"
#include <wx/dcbuffer.h>
#include <wx/settings.h>

// ThemeManager macros for consistent usage
#define DOCK_COLOUR(key) CFG_COLOUR(key)
#define DOCK_FONT() CFG_DEFAULTFONT()
#define DOCK_INT(key) CFG_INT(key)

namespace ads {

// Global style configuration instance
static DockStyleConfig g_dockStyleConfig;

// Helper function to ensure theme manager is initialized
void EnsureThemeManagerInitialized() {
    static bool themeInitialized = false;
    if (!themeInitialized) {
        try {
            g_dockStyleConfig.InitializeFromThemeManager();
            themeInitialized = true;
            wxLogDebug("DockStyleConfig: Initialized from ThemeManager");
        } catch (...) {
            wxLogDebug("DockStyleConfig: ThemeManager not available, using defaults");
        }
    }
}

// Style helper functions

// Draw a styled rectangle with flat background and selective borders for active tabs
void DrawStyledRect(wxDC& dc, const wxRect& rect, const DockStyleConfig& style,
                   bool isActive, bool isHovered, bool isTitleBar) {
    // Determine background color - flat style, no gradients or 3D effects
    wxColour bgColor = wxColour(0, 0, 0, 0);  // Transparent by default
    if (isActive && !isTitleBar) {
        bgColor = style.activeBackgroundColour;
    } else if (!isTitleBar) {
        // Inactive tabs have no background color
        bgColor = wxColour(0, 0, 0, 0);
    } else {
        // Title bar background
        bgColor = style.backgroundColour;
    }

    // Fill background only if not transparent
    if (bgColor.IsOk() && bgColor != wxColour(0, 0, 0, 0)) {
        dc.SetBrush(wxBrush(bgColor));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(rect);
    }

    // For active tabs: draw 2px top line and 1px left/right borders
    if (isActive && !isTitleBar) {
        dc.SetPen(wxPen(style.borderTopColour, 2));  // 2px top line
        dc.DrawLine(rect.GetLeft() - 1, rect.GetTop(), rect.GetRight() - 1, rect.GetTop());

        dc.SetPen(wxPen(style.borderLeftColour, 1));  // 1px left border
        dc.DrawLine(rect.GetLeft(), rect.GetTop(), rect.GetLeft(), rect.GetBottom());

        dc.SetPen(wxPen(style.borderRightColour, 1));  // 1px right border
        dc.DrawLine(rect.GetRight() - 1, rect.GetTop(), rect.GetRight() - 1, rect.GetBottom());
    }

    // For title bar: draw bottom border line
    if (isTitleBar) {
        dc.SetPen(wxPen(style.borderBottomColour, 1));
        dc.DrawLine(rect.GetLeft(), rect.GetBottom() - 1, rect.GetRight(), rect.GetBottom() - 1);
    }
}

// Set text color based on state
void SetStyledTextColor(wxDC& dc, const DockStyleConfig& style, bool isActive) {
    if (isActive) {
        dc.SetTextForeground(style.activeTextColour);
    } else {
        dc.SetTextForeground(style.inactiveTextColour);
    }
}

// Draw a close button with style (legacy fallback)
void DrawCloseButton(wxDC& dc, const wxRect& rect, const DockStyleConfig& style, bool isHovered) {
    // Draw button background if hovered
    if (isHovered) {
        dc.SetBrush(wxBrush(style.hoverBackgroundColour));
        dc.SetPen(wxPen(style.borderTopColour, 1));
        dc.DrawRectangle(rect);
    }

    // Draw X
    dc.SetPen(wxPen(isHovered ? style.activeTextColour : style.inactiveTextColour, 1));
    int margin = 3;
    dc.DrawLine(rect.GetLeft() + margin, rect.GetTop() + margin,
              rect.GetRight() - margin, rect.GetBottom() - margin);
    dc.DrawLine(rect.GetRight() - margin, rect.GetTop() + margin,
              rect.GetLeft() + margin, rect.GetBottom() - margin);
}

// Draw a button with SVG icon (no hover effects, fixed 12x12 size)
void DrawSvgButton(wxDC& dc, const wxRect& rect, const wxString& iconName,
                  const DockStyleConfig& style, bool isHovered) {
    // No hover background - completely flat button
    // Remove background drawing entirely for flat appearance

    // Try to draw SVG icon
    if (style.useSvgIcons) {
        try {
            // Use fixed 12x12 size as specified
            wxSize iconSize(12, 12);
            wxBitmap iconBitmap = SvgIconManager::GetInstance().GetIconBitmap(iconName, iconSize);

            if (iconBitmap.IsOk()) {
                // Center the icon in the button rect
                int x = rect.GetLeft() + (rect.GetWidth() - iconSize.GetWidth()) / 2;
                int y = rect.GetTop() + (rect.GetHeight() - iconSize.GetHeight()) / 2;
                dc.DrawBitmap(iconBitmap, x, y, true);
                return;
            }
        }
        catch (...) {
            // Fall through to fallback drawing
        }
    }

    // Fallback: draw simple X for close button (no hover effects)
    dc.SetPen(wxPen(style.inactiveTextColour, 1));
    int margin = 3;
    dc.DrawLine(rect.GetLeft() + margin, rect.GetTop() + margin,
              rect.GetRight() - margin, rect.GetBottom() - margin);
    dc.DrawLine(rect.GetRight() - margin, rect.GetTop() + margin,
              rect.GetLeft() + margin, rect.GetBottom() - margin);
}

// Static methods for style configuration
void DockArea::SetDockStyle(DockStyle style) {
    g_dockStyleConfig.SetStyle(style);
    EnsureThemeManagerInitialized();
}

void DockArea::SetDockStyleConfig(const DockStyleConfig& config) {
    g_dockStyleConfig = config;
    EnsureThemeManagerInitialized();
}

const DockStyleConfig& DockArea::GetDockStyleConfig() {
    EnsureThemeManagerInitialized();
    return g_dockStyleConfig;
}

// Helper function to get dock style config with theme initialization
const DockStyleConfig& GetDockStyleConfig() {
    EnsureThemeManagerInitialized();
    return g_dockStyleConfig;
}

// Implementation of InitializeFromThemeManager
void DockStyleConfig::InitializeFromThemeManager() {
    // Use standard ThemeManager macros consistent with the rest of the project
    try {
        // Update colors from existing theme configuration
        // Use appropriate existing colors for docking elements
        backgroundColour = DOCK_COLOUR("MainBackgroundColour");
        activeBackgroundColour = DOCK_COLOUR("SecondaryBackgroundColour");
        hoverBackgroundColour = DOCK_COLOUR("HighlightColour");

        // Use border colors from theme (use specific tab border colors)
        borderTopColour = DOCK_COLOUR("TabBorderTopColour");
        borderBottomColour = DOCK_COLOUR("TabBorderBottomColour");
        borderLeftColour = DOCK_COLOUR("TabBorderLeftColour");
        borderRightColour = DOCK_COLOUR("TabBorderRightColour");

        // Use text colors from theme
        textColour = DOCK_COLOUR("DefaultTextColour");
        activeTextColour = DOCK_COLOUR("DefaultTextColour");
        inactiveTextColour = DOCK_COLOUR("DefaultTextColour");

        // Update font from theme
        font = DOCK_FONT();

        // Set reasonable defaults for sizes (can be overridden by theme)
        tabHeight = DOCK_INT("TabHeight");  // Use specific docking tab height
        if (tabHeight <= 0) tabHeight = 24;  // Default to 24 as specified

        tabTopMargin = DOCK_INT("TabTopMargin");  // Get from theme
        if (tabTopMargin <= 0) tabTopMargin = 4;  // Default to 4 as specified

        titleBarHeight = DOCK_INT("TitleBarHeight");  // Use specific title bar height
        if (titleBarHeight <= 0) titleBarHeight = 30;  // Default to 30 as specified

        borderWidth = DOCK_INT("BorderWidth");  // Use specific border width
        if (borderWidth <= 0) borderWidth = 1;

        buttonSize = DOCK_INT("ButtonSize");  // Get button size from theme
        if (buttonSize <= 0) buttonSize = 12;  // Default to 12 as specified

        tabSpacing = DOCK_INT("TabSpacing");  // Get tab spacing from theme
        if (tabSpacing <= 0) tabSpacing = 4;   // Default to 4 as specified

        contentMargin = DOCK_INT("ContentMargin");  // Get content margin from theme
        if (contentMargin <= 0) contentMargin = 2; // Default to 2 as specified

        wxLogDebug("DockStyleConfig: Successfully initialized from ThemeManager");
    } catch (...) {
        wxLogDebug("DockStyleConfig: ThemeManager not available, using defaults");
        // Keep existing default values if ThemeManager fails
    }
}

} // namespace ads
