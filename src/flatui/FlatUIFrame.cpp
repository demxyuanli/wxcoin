#include "flatui/FlatUIFrame.h"
#include "flatui/FlatUIPage.h" // For FlatUIPage in RefreshAllUI
#include "flatui/FlatUIHomeMenu.h" // For FlatUIHomeMenu::RefreshTheme
#include <wx/dcbuffer.h> // For wxScreenDC
#include <wx/display.h>  // For wxDisplay
#include <functional>   // For std::function
#include "config/ThemeManager.h"
#include "logger/Logger.h"

#ifdef __WXMSW__
#include <windows.h>     // For Windows specific GDI calls for rubber band
#endif

// Define custom events
wxDEFINE_EVENT(wxEVT_THEME_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_PIN_STATE_CHANGED, wxCommandEvent);

// Event table for FlatUIFrame global events
wxBEGIN_EVENT_TABLE(FlatUIFrame, BorderlessFrameLogic)
    EVT_COMMAND(wxID_ANY, wxEVT_THEME_CHANGED, FlatUIFrame::OnThemeChanged)
    EVT_COMMAND(wxID_ANY, wxEVT_PIN_STATE_CHANGED, FlatUIFrame::OnGlobalPinStateChanged)
wxEND_EVENT_TABLE()

// FlatUIFrame now has its own event table for global events (theme changes, pin state changes)
// Mouse events are handled by overriding BorderlessFrameLogic's virtual handlers

FlatUIFrame::FlatUIFrame(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style)
    : BorderlessFrameLogic(parent, id, title, pos, size, style), // Call new base class constructor
      m_isPseudoMaximized(false)
      // m_dragging, m_resizing, m_resizeMode, m_rubberBandVisible, m_borderThreshold are initialized by BorderlessFrameLogic
{
    InitFrameStyle(); // Specific styling for FlatUIFrame
}

FlatUIFrame::~FlatUIFrame()
{
    LOG_DBG("FlatUIFrame destruction started.", "FlatUIFrame");
    LOG_DBG("FlatUIFrame destruction completed.", "FlatUIFrame");
}

void FlatUIFrame::InitFrameStyle()
{
    // Set specific FlatUIFrame styles (e.g., background, etc.)
    // BorderlessFrameLogic constructor already sets DoubleBuffered.
    SetBackgroundColour(CFG_COLOUR("FrameAppWorkspaceColour"));
    // m_borderThreshold is set in BorderlessFrameLogic constructor, can be overridden here if needed for FlatUIFrame
    // e.g., this->m_borderThreshold = 10; // If FlatUIFrame needs a different threshold
}

void FlatUIFrame::OnLeftDown(wxMouseEvent& event)
{
    // Check for conditions that should prevent dragging/resizing (e.g., pseudo-maximized state)
    if (m_isPseudoMaximized) {
        event.Skip(); // Don't initiate drag/resize if maximized
        return;
    }

    // Call the base class implementation to handle the actual dragging/resizing logic
    BorderlessFrameLogic::OnLeftDown(event);
}

void FlatUIFrame::OnLeftUp(wxMouseEvent& event)
{
    // If FlatUIFrame needs specific behavior on mouse up after dragging/resizing,
    // it can add it here. Otherwise, just call base.
    BorderlessFrameLogic::OnLeftUp(event);
}

void FlatUIFrame::OnMotion(wxMouseEvent& event)
{
    // Check for conditions that should prevent cursor changes or rubber banding
    if (m_isPseudoMaximized) {
        // Still call base class to update cursor, but prevent actual resizing/dragging
        BorderlessFrameLogic::OnMotion(event);
        return;
    }

    // Call the base class implementation for cursor updates and rubber banding logic
    BorderlessFrameLogic::OnMotion(event);
}

void FlatUIFrame::PseudoMaximize()
{
    if (m_isPseudoMaximized) return;

    m_preMaximizeRect = GetScreenRect();
    int displayIndex = wxDisplay::GetFromWindow(this);
    wxDisplay display(displayIndex != wxNOT_FOUND ? displayIndex : 0);
    wxRect workArea = display.GetClientArea(); // Use client area to avoid taskbar

    SetSize(workArea);
    Move(workArea.GetPosition());
    m_isPseudoMaximized = true;
    Refresh();
    Update();
}

void FlatUIFrame::RestoreFromPseudoMaximize()
{
    if (!m_isPseudoMaximized) return;

    SetSize(m_preMaximizeRect.GetSize());
    Move(m_preMaximizeRect.GetPosition());
    m_isPseudoMaximized = false;
    Refresh();
    Update();
}

// LogUILayout method remains the same as it was, no changes needed for this refactor.
void FlatUIFrame::LogUILayout(wxWindow* window, int depth)
{
    if (!window) {
        window = this; // Default to logging the FlatUIFrame itself if no window is provided
        LOG_DBG("--- UI Layout Tree for Frame: " + GetTitle().ToStdString() + " ---", "FlatUIFrame");
    }

    wxString indent(depth * 2, ' '); // Create indentation string
    wxString windowClass = window->GetClassInfo()->GetClassName();
    wxString windowName = window->GetName();
    wxString windowLabel = window->GetLabel(); // May be empty for non-control windows
    wxRect windowRect = window->GetRect();
    wxSize windowSize = window->GetSize();
    wxPoint windowPos = window->GetPosition();

    wxString logMsg = wxString::Format(wxT("%s%s (Name: %s, Label: '%s') - Pos: (%d,%d), Size: (%dx%d)"),
        indent, windowClass, windowName, windowLabel, windowPos.x, windowPos.y, windowSize.x, windowSize.y);

    LOG_DBG(logMsg.ToStdString(), "FlatUIFrame");

    // Recursively log children
    wxWindowList children = window->GetChildren();
    for (wxWindowList::Node* node = children.GetFirst(); node; node = node->GetNext()) {
        wxWindow* child = (wxWindow*)node->GetData();
        LogUILayout(child, depth + 1);
    }

    if (depth == 0) {
        LOG_DBG("--- End of UI Layout Tree ---", "FlatUIFrame");
    }
}

int FlatUIFrame::GetMinWidth() const
{
    return CalculateMinimumWidth();
}

int FlatUIFrame::GetMinHeight() const
{
    return CalculateMinimumHeight();
}

int FlatUIFrame::CalculateMinimumWidth() const
{
    FlatUIBar* ribbon = GetUIBar();
    if (!ribbon) return 400; // Fallback minimum width

    int minWidth = 0;

    // HomeSpace width (30px)
    if (ribbon->GetHomeSpace()) {
        minWidth += 30;
    }

    // System buttons width (estimate 3 buttons * 30px each)
    minWidth += 90;

    // One Tab width (estimate 80px)
    minWidth += 80;

    // Scroll button width (20px)
    minWidth += 20;

    // Add margins and spacing
    minWidth += 20; // Left and right margins

    return minWidth; // Approximately 240px minimum
}

int FlatUIFrame::CalculateMinimumHeight() const
{
    FlatUIBar* ribbon = GetUIBar();
    if (!ribbon) return 200; // Fallback minimum height

    // Get the actual FlatUIBar height (which is 3 times the base height)
    int actualBarHeight = ribbon->GetSize().GetHeight();
    if (actualBarHeight <= 0) {
        // If bar hasn't been sized yet, calculate expected height
        int baseHeight = FlatUIBar::GetBarHeight();
        actualBarHeight = baseHeight * 3; // 90 pixels
    }

    // Minimum height should be bar height plus some content area
    // For example: bar height + 150px for content
    return actualBarHeight + 150; // Approximately 240px minimum
}

void FlatUIFrame::SetSize(const wxRect& rect)
{
    BorderlessFrameLogic::SetSize(rect);
    HandleAdaptiveUIVisibility(rect.GetSize());
}

void FlatUIFrame::SetSize(const wxSize& size)
{
    BorderlessFrameLogic::SetSize(size);
    HandleAdaptiveUIVisibility(size);
}

void FlatUIFrame::HandleAdaptiveUIVisibility(const wxSize& newSize)
{
    FlatUIBar* ribbon = GetUIBar();
    if (!ribbon) return;

    int availableWidth = newSize.GetWidth();
    int baseWidth = CalculateMinimumWidth();

    // Calculate required width for different configurations
    int withFunctionSpace = baseWidth + 270; // FunctionSpace width
    int withProfileSpace = baseWidth + 60;   // ProfileSpace width
    int withBothSpaces = baseWidth + 270 + 60;

    // Adaptive visibility logic
    if (availableWidth >= withBothSpaces) {
        // Show both FunctionSpace and ProfileSpace
        ribbon->SetFunctionSpaceControl(GetFunctionSpaceControl(), 270);
        ribbon->SetProfileSpaceControl(GetProfileSpaceControl(), 60);
        ShowTabFunctionSpacer(true);
        ShowFunctionProfileSpacer(true);
    }
    else if (availableWidth >= withProfileSpace) {
        // Hide FunctionSpace, show ProfileSpace
        ribbon->SetFunctionSpaceControl(nullptr, 0);
        ribbon->SetProfileSpaceControl(GetProfileSpaceControl(), 60);
        ShowTabFunctionSpacer(false);
        ShowFunctionProfileSpacer(false);
    }
    else {
        // Hide both FunctionSpace and ProfileSpace
        ribbon->SetFunctionSpaceControl(nullptr, 0);
        ribbon->SetProfileSpaceControl(nullptr, 0);
        ShowTabFunctionSpacer(false);
        ShowFunctionProfileSpacer(false);
    }
}

void FlatUIFrame::ShowTabFunctionSpacer(bool show)
{
    FlatUIBar* ribbon = GetUIBar();
    if (ribbon && ribbon->GetTabFunctionSpacer()) {
        ribbon->GetTabFunctionSpacer()->Show(show);
    }
}

void FlatUIFrame::ShowFunctionProfileSpacer(bool show)
{
    FlatUIBar* ribbon = GetUIBar();
    if (ribbon && ribbon->GetFunctionProfileSpacer()) {
        ribbon->GetFunctionProfileSpacer()->Show(show);
    }
}

void FlatUIFrame::OnThemeChanged(wxCommandEvent& event)
{
    wxString themeName = event.GetString();
    
    // Update config file
    auto& themeManager = ThemeManager::getInstance();
    themeManager.saveCurrentTheme();
    
    // Perform comprehensive UI refresh
    RefreshAllUI();
    
    event.Skip();
}

void FlatUIFrame::OnGlobalPinStateChanged(wxCommandEvent& event)
{
    // Base implementation - only handle basic layout if no derived class overrides
    FlatUIBar* ribbon = GetUIBar();
    if (!ribbon) {
        event.Skip();
        return;
    }

    bool isPinned = event.GetInt() != 0;

    if (isPinned) {
        // Restore original min height for ribbon
        int ribbonMinHeight = FlatUIBar::GetBarHeight() + CFG_INT("PanelTargetHeight") + 10;
        ribbon->SetMinSize(wxSize(-1, ribbonMinHeight));
    } else {
        // Set collapsed min height for ribbon
        int unpinnedHeight = CFG_INT("BarUnpinnedHeight");
        ribbon->SetMinSize(wxSize(-1, unpinnedHeight));
    }

    // Basic layout update
    if (GetSizer()) {
        GetSizer()->Layout();
    }
    
    Layout();
    Refresh();
    Update();
    
    event.Skip();
}

void FlatUIFrame::RefreshAllUI()
{
    // Recursive function to refresh all child controls
    std::function<void(wxWindow*)> refreshRecursive = [&](wxWindow* window) {
        if (!window) return;
        
        // Special handling for specific control types
        wxString className = window->GetClassInfo()->GetClassName();
        
        // Re-apply theme colors for specific controls
        if (className == "wxSearchCtrl") {
            window->SetBackgroundColour(CFG_COLOUR("SearchCtrlBgColour"));
            window->SetForegroundColour(CFG_COLOUR("SearchCtrlFgColour"));
        }
        else if (className == "wxTextCtrl") {
            window->SetBackgroundColour(CFG_COLOUR("TextCtrlBgColour"));
            window->SetForegroundColour(CFG_COLOUR("TextCtrlFgColour"));
        }
        else if (className == "wxPanel") {
            // Check for specific panel types by name
            wxString name = window->GetName();
            if (name.Contains("search") || name.Contains("Search")) {
                window->SetBackgroundColour(CFG_COLOUR("SearchPanelBgColour"));
            }
            else if (name.Contains("svg") || name.Contains("SVG")) {
                window->SetBackgroundColour(CFG_COLOUR("SvgPanelBgColour"));
            }
            else if (name.Contains("icon") || name.Contains("Icon")) {
                window->SetBackgroundColour(CFG_COLOUR("IconPanelBgColour"));
            }
        }
        else if (className == "wxScrolledWindow") {
            window->SetBackgroundColour(CFG_COLOUR("ScrolledWindowBgColour"));
        }
        else if (className == "wxStaticText") {
            // Re-apply text colors based on usage
            wxString label = static_cast<wxStaticText*>(window)->GetLabel();
            if (label.Contains("Error") || label.Contains("error")) {
                window->SetForegroundColour(CFG_COLOUR("ErrorTextColour"));
            }
            else if (label.Contains("Not Found") || label.Contains("Missing")) {
                window->SetForegroundColour(CFG_COLOUR("PlaceholderTextColour"));
            }
            else {
                window->SetForegroundColour(CFG_COLOUR("DefaultTextColour"));
            }
        }
        else if (className == "FlatUIHomeMenu") {
            // Cast to FlatUIHomeMenu and call RefreshTheme
            FlatUIHomeMenu* homeMenu = static_cast<FlatUIHomeMenu*>(window);
            homeMenu->RefreshTheme();
        }
        else if (className == "FlatUITabDropdown") {
            window->SetBackgroundColour(CFG_COLOUR("BarBackgroundColour"));
        }
        else if (className == "FlatUIFixPanel") {
            window->SetBackgroundColour(CFG_COLOUR("BarBackgroundColour"));
        }
        else if (className == "FlatUIFloatPanel") {
            window->SetBackgroundColour(CFG_COLOUR("ScrolledWindowBgColour"));
        }
        
        // Force refresh
        window->Refresh(true);
        window->Update();
        
        // Recursively refresh all children
        wxWindowList& children = window->GetChildren();
        for (wxWindow* child : children) {
            refreshRecursive(child);
        }
    };
    
    // Start with this frame
    SetBackgroundColour(CFG_COLOUR("FrameAppWorkspaceColour"));
    
    // Refresh ribbon with theme colors
    FlatUIBar* ribbon = GetUIBar();
    if (ribbon) {
        ribbon->SetTabBorderColour(CFG_COLOUR("BarTabBorderColour"));
        ribbon->SetActiveTabBackgroundColour(CFG_COLOUR("BarActiveTabBgColour"));
        ribbon->SetActiveTabTextColour(CFG_COLOUR("BarActiveTextColour"));
        ribbon->SetInactiveTabTextColour(CFG_COLOUR("BarInactiveTextColour"));
        ribbon->SetTabBorderTopColour(CFG_COLOUR("BarTabBorderTopColour"));
        
        // Refresh all ribbon pages and panels
        for (size_t i = 0; i < ribbon->GetPageCount(); ++i) {
            FlatUIPage* page = ribbon->GetPage(i);
            if (page) {
                refreshRecursive(page);
            }
        }
    }
    
    // Refresh function and profile space controls
    wxWindow* functionSpaceControl = GetFunctionSpaceControl();
    if (functionSpaceControl) {
        refreshRecursive(functionSpaceControl);
    }
    
    wxWindow* profileSpaceControl = GetProfileSpaceControl();
    if (profileSpaceControl) {
        refreshRecursive(profileSpaceControl);
    }
    
    // Refresh all other children recursively
    refreshRecursive(this);
    
    // Force complete relayout
    if (GetSizer()) {
        GetSizer()->Layout();
    }
    Layout();
    
    // Final refresh
    Refresh(true);
    Update();
}
