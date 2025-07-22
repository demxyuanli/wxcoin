#include <wx/dcbuffer.h> // For wxAutoBufferedPaintDC
#include <wx/settings.h> // For system colours
#include <wx/display.h> 
#include "flatui/FlatUIHomeMenu.h" // For the custom menu
#include "flatui/FlatUIFrame.h"       // To get parent FlatFrame and content height
#include "flatui/FlatUIBar.h"         // To get FlatUIBar height
#include "config/ThemeManager.h"
#include "config/SvgIconManager.h"



// Known menu item IDs from FlatFrame (or define them in a shared constants header)
// These should match the IDs used in FlatFrame's event handlers
// wxID_EXIT is standard

FlatUIHomeSpace::FlatUIHomeSpace(wxWindow* parent, wxWindowID id)
    : wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE),
    m_hover(false),
    m_buttonWidth(CFG_INT("SystemButtonWidth")),
    m_activeHomeMenu(nullptr) // Initialize m_activeHomeMenu
{
    SetBackgroundStyle(wxBG_STYLE_PAINT); // Important for custom painting
    
    // Register theme change listener
    ThemeManager::getInstance().addThemeChangeListener(this, [this]() {
        RefreshTheme();
    });
}

FlatUIHomeSpace::~FlatUIHomeSpace()
{
    // m_menu is not owned by this class
    // Unregister theme change listener
    ThemeManager::getInstance().removeThemeChangeListener(this);
}

// void FlatUIHomeSpace::SetMenu(wxMenu* menu) { /* m_menu = menu; */ } // Removed

void FlatUIHomeSpace::SetIcon(const wxBitmap& icon)
{
    m_icon = icon;
    Refresh();
}

// SetButtonWidth and GetButtonWidth are inline in the header

void FlatUIHomeSpace::CalculateButtonRect(const wxSize& controlSize)
{
    // The button occupies the entire control area for FlatUIHomeSpace
    m_buttonRect = wxRect(0, 0, controlSize.GetWidth(), controlSize.GetHeight());
}

void FlatUIHomeSpace::OnPaint(wxPaintEvent& evt)
{
    wxAutoBufferedPaintDC dc(this);
    // dc.Clear(); // Clearing with default might not be what we want if aiming for transparency to parent.
                 // Instead, we will explicitly fill our buttonRect with appropriate color.

    CalculateButtonRect(GetClientSize());

    wxColour finalBgColorToDraw;

    if (m_hover) { // m_menu check removed as it's no longer relevant for hover indication
        finalBgColorToDraw = CFG_COLOUR("HomespaceHoverBgColour");
    }
    else {
        finalBgColorToDraw = CFG_COLOUR("BarBgColour"); // Use theme background color
    }

    // Fill the entire control area with the determined background color
    // This handles the "transparent" background for normal state and hover background
    dc.SetBrush(wxBrush(finalBgColorToDraw));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(0, 0, GetClientSize().GetWidth(), GetClientSize().GetHeight()); // Fill entire control

    // Draw icon on top
    if (m_icon.IsOk())
    {
        int x = (m_buttonRect.GetWidth() - m_icon.GetWidth()) / 2;
        int y = (m_buttonRect.GetHeight() - m_icon.GetHeight()) / 2;
        dc.DrawBitmap(m_icon, x, y, true /* use mask */);
    }
    else
    {
        // Try to get default home icon from SVG
        wxBitmap homeIcon = SVG_ICON("home", wxSize(16, 16));
        if (homeIcon.IsOk()) {
            int x = (m_buttonRect.GetWidth() - homeIcon.GetWidth()) / 2;
            int y = (m_buttonRect.GetHeight() - homeIcon.GetHeight()) / 2;
            dc.DrawBitmap(homeIcon, x, y, true);
        }
        else {
            // Draw default "hamburger" icon if no icon is set
            int lineCount = 3;
            int VMargin = m_buttonRect.GetHeight() / 4;
            int HMargin = m_buttonRect.GetWidth() / 5;
            int lineThickness = 2;
            int lineSpacing = (m_buttonRect.GetHeight() - 2 * VMargin - lineCount * lineThickness) / wxMax(1, lineCount - 1);

            dc.SetPen(wxPen(CFG_COLOUR("SystemButtonTextColour"), lineThickness));
            for (int i = 0; i < lineCount; ++i) {
                int yPos = VMargin + i * (lineThickness + lineSpacing);
                dc.DrawLine(m_buttonRect.GetLeft() + HMargin,
                    m_buttonRect.GetTop() + yPos,
                    m_buttonRect.GetRight() - HMargin,
                    m_buttonRect.GetTop() + yPos);
            }
        }
    }
}

void FlatUIHomeSpace::SetHomeMenu(FlatUIHomeMenu* menu)
{
    m_activeHomeMenu = menu;
}

void FlatUIHomeSpace::OnMouseDown(wxMouseEvent& evt)
{
    if (m_buttonRect.Contains(evt.GetPosition()))
    {
        if (m_activeHomeMenu) {
            if (m_show) {
                m_activeHomeMenu->Close();
                m_hover = false;
                m_show = false;
                Refresh();
                return;
            }
            else {
                wxPoint menuPos = ClientToScreen(wxPoint(-2, m_buttonRect.GetBottom() + 1));

                int menuContentHeight = 420; // Default height
                FlatUIFrame* mainFrame = m_activeHomeMenu->GetEventSinkFrame();
                if (mainFrame) {
                    // Get frame's screen position and size
                    wxPoint framePos = mainFrame->GetScreenPosition();
                    wxSize frameSize = mainFrame->GetSize();
                    int frameBottom = framePos.y + frameSize.GetHeight();
                    
                    // Calculate available height from menu position to frame bottom
                    int availableHeight = frameBottom - menuPos.y; 
                    
                    int frameHeight = mainFrame->GetClientSize().GetHeight();
                    int calculatedHeight = frameHeight - CFG_INT("ButtonBarTargetHeight") - CFG_INT("BarTopMargin");
                    
                    // Use the smaller of calculated height and available height
                    menuContentHeight = wxMin(calculatedHeight, availableHeight);
                    
                    if (menuContentHeight < 50) { // Ensure a minimum height
                        menuContentHeight = 50;
                        // If minimum height still exceeds available space, move menu up
                        if (menuPos.y + menuContentHeight > frameBottom - 10) {
                            menuPos.y = frameBottom - menuContentHeight - 10;
                            // Make sure menu doesn't go above the button
                            wxPoint buttonScreenPos = ClientToScreen(wxPoint(0, 0));
                            if (menuPos.y < buttonScreenPos.y + m_buttonRect.GetHeight()) {
                                menuPos.y = buttonScreenPos.y + m_buttonRect.GetHeight() + 3;
                                menuContentHeight = frameBottom - menuPos.y;
                            }
                        }
                    }
                }
                else {
                    wxLogWarning("Could not get main frame to calculate menu height.");
                }

                m_activeHomeMenu->ShowAt(menuPos, menuContentHeight, m_show);
                m_hover = false;
                Refresh();
                return;
            }
        }
    }
    evt.Skip();
}

void FlatUIHomeSpace::OnMouseMove(wxMouseEvent& evt)
{
    bool oldHover = m_hover;
    // Allow hover state regardless of menu, visual feedback for clickability
    m_hover = m_buttonRect.Contains(evt.GetPosition());

    if (m_hover != oldHover) {
        Refresh();
    }
    evt.Skip();
}

void FlatUIHomeSpace::OnMouseLeave(wxMouseEvent& evt)
{
    if (m_hover) {
        m_hover = false;
        Refresh();
    }
    evt.Skip();
}

void FlatUIHomeSpace::OnHomeMenuClosed(FlatUIHomeMenu* closedMenu)
{
    if (m_activeHomeMenu == closedMenu) {
        m_hover = false;
        m_show = false;
        Refresh();
    }
}

void FlatUIHomeSpace::RefreshTheme()
{
    // Update theme-based settings
    m_buttonWidth = CFG_INT("SystemButtonWidth");
    
    // Update control properties
    SetFont(CFG_DEFAULTFONT());
    
    // Force refresh to redraw with new theme colors
    Refresh(true);
    Update();
}
