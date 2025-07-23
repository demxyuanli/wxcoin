#include "flatui/FlatUIFrame.h"
#include "flatui/FlatUIThemeAware.h" // For theme-aware component detection
#include "flatui/FlatUIHomeMenu.h" // For FlatUIHomeMenu::RefreshTheme
#include "flatui/FlatUIButtonBar.h" // For FlatUIButtonBar::RefreshTheme
#include "flatui/FlatUIPanel.h" // For FlatUIPanel::RefreshTheme
#include "flatui/FlatUIGallery.h" // For FlatUIGallery::RefreshTheme
#include "flatui/FlatUIPage.h" // For FlatUIPage::RefreshTheme
#include "flatui/FlatUIHomeSpace.h" // For FlatUIHomeSpace::RefreshTheme
#include "flatui/FlatUIProfileSpace.h" // For FlatUIProfileSpace::RefreshTheme
#include "flatui/FlatUISystemButtons.h" // For FlatUISystemButtons::RefreshTheme
#include "flatui/FlatUIBar.h"
#include "flatui/FlatUIPage.h"
#include "flatui/FlatUIPanel.h"
#include "flatui/FlatUIFunctionSpace.h"
#include "flatui/FlatUIProfileSpace.h"
#include "flatui/FlatUISystemButtons.h"
#include "flatui/FlatUISpacerControl.h"
#include "flatui/FlatUIFloatPanel.h"
#include "flatui/FlatUIFixPanel.h"
#include "flatui/FlatUITabDropdown.h"
#include "config/ThemeManager.h"
#include "logger/Logger.h"
#include <wx/sizer.h>
#include <wx/dcclient.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/display.h>
#include <wx/button.h>
#include <wx/menu.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>
#include <algorithm>
#include <functional>

#ifdef __WXMSW__
#define NOMINMAX
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
    : BorderlessFrameLogic(parent, id, title, pos, size, style), m_isPseudoMaximized(false)
{
    InitFrameStyle();

    // 自动添加自定义主题状态栏到frame最低层
    // wxBoxSizer* sizer = dynamic_cast<wxBoxSizer*>(GetSizer());
    // if (!sizer) {
    //     sizer = new wxBoxSizer(wxVERTICAL);
    //     SetSizer(sizer);
    // }
    // m_statusBar = new FlatUIStatusBar(this);
    // sizer->AddStretchSpacer(1); // 保证状态栏永远在最底部
    // sizer->Add(m_statusBar, 0, wxEXPAND);
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
    
    // Perform comprehensive UI refresh with debouncing
    static wxDateTime lastThemeChange = wxDateTime::Now();
    wxDateTime now = wxDateTime::Now();
    
    // Debounce theme changes - ignore if less than 100ms since last change
    if ((now - lastThemeChange).GetMilliseconds() < 100) {
        LOG_DBG("Theme change debounced - too soon since last change", "FlatUIFrame");
        return;
    }
    
    lastThemeChange = now;
    
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
    // Show a brief progress indicator
    wxBusyCursor busyCursor;
    
    // Collect all theme-aware components that need refresh
    std::vector<FlatUIThemeAware*> themeAwareComponents;
    std::vector<wxWindow*> otherComponents;
    
    // Recursive function to collect all components
    std::function<void(wxWindow*)> collectComponents = [&](wxWindow* window) {
        if (!window) return;
        
        // Check if the window is a theme-aware component
        FlatUIThemeAware* themeAware = dynamic_cast<FlatUIThemeAware*>(window);
        if (themeAware && themeAware->NeedsThemeRefresh()) {
            themeAwareComponents.push_back(themeAware);
        } else {
            // For non-theme-aware components, collect for later processing
            otherComponents.push_back(window);
        }
        
        // Recursively collect all child controls
        wxWindowList& children = window->GetChildren();
        for (wxWindow* child : children) {
            collectComponents(child);
        }
    };
    
    // Phase 1: Collect all components
    collectComponents(this);
    
    // Phase 2: Update theme values for all theme-aware components
    for (FlatUIThemeAware* component : themeAwareComponents) {
        component->UpdateThemeValues();
    }
    
    // Phase 3: Update non-theme-aware components
    for (wxWindow* window : otherComponents) {
        wxString className = window->GetClassInfo()->GetClassName();
        
        if (className == "wxStaticText") {
            window->SetForegroundColour(CFG_COLOUR("DefaultTextColour"));
        }
        else if (className == "wxTextCtrl") {
            window->SetBackgroundColour(CFG_COLOUR("TextCtrlBgColour"));
            window->SetForegroundColour(CFG_COLOUR("TextCtrlFgColour"));
        }
        else if (className == "wxSearchCtrl") {
            window->SetBackgroundColour(CFG_COLOUR("SearchCtrlBgColour"));
            window->SetForegroundColour(CFG_COLOUR("SearchCtrlFgColour"));
        }
        else if (className == "wxPanel") {
                window->SetBackgroundColour(CFG_COLOUR("SearchPanelBgColour"));
            }
        else if (className == "wxScrolledWindow") {
                window->SetBackgroundColour(CFG_COLOUR("SvgPanelBgColour"));
            }
        else if (className == "wxStaticBitmap") {
                window->SetBackgroundColour(CFG_COLOUR("IconPanelBgColour"));
        }
        else if (className == "wxScrolledWindow") {
            window->SetBackgroundColour(CFG_COLOUR("ScrolledWindowBgColour"));
        }
        else if (className == "wxStaticText" && window->GetName() == "error") {
                window->SetForegroundColour(CFG_COLOUR("ErrorTextColour"));
            }
        else if (className == "wxStaticText" && window->GetName() == "placeholder") {
                window->SetForegroundColour(CFG_COLOUR("PlaceholderTextColour"));
            }
        else if (className == "wxStaticText" && window->GetName() == "default") {
                window->SetForegroundColour(CFG_COLOUR("DefaultTextColour"));
            }
        else if (className == "FlatUIBar") {
            window->SetBackgroundColour(CFG_COLOUR("BarBackgroundColour"));
        }
        else if (className == "FlatUIPanel") {
            window->SetBackgroundColour(CFG_COLOUR("BarBackgroundColour"));
        }
        else if (className == "wxScrolledWindow") {
            window->SetBackgroundColour(CFG_COLOUR("ScrolledWindowBgColour"));
        }
        else if (className == "FlatUIHomeMenu") {
            // Cast to FlatUIHomeMenu and call RefreshTheme
            FlatUIHomeMenu* homeMenu = static_cast<FlatUIHomeMenu*>(window);
            homeMenu->RefreshTheme();
        }
        else if (className == "FlatUIButtonBar") {
            // Cast to FlatUIButtonBar and call RefreshTheme
            FlatUIButtonBar* buttonBar = static_cast<FlatUIButtonBar*>(window);
            buttonBar->RefreshTheme();
        }
        else if (className == "FlatUIPanel") {
            // Cast to FlatUIPanel and call RefreshTheme
            FlatUIPanel* panel = static_cast<FlatUIPanel*>(window);
            panel->RefreshTheme();
        }
        else if (className == "FlatUIGallery") {
            // Cast to FlatUIGallery and call RefreshTheme
            FlatUIGallery* gallery = static_cast<FlatUIGallery*>(window);
            gallery->RefreshTheme();
        }
        else if (className == "FlatUIPage") {
            // Cast to FlatUIPage and call RefreshTheme
            FlatUIPage* page = static_cast<FlatUIPage*>(window);
            page->RefreshTheme();
        }
        else if (className == "FlatUIHomeSpace") {
            // Cast to FlatUIHomeSpace and call RefreshTheme
            FlatUIHomeSpace* homeSpace = static_cast<FlatUIHomeSpace*>(window);
            homeSpace->RefreshTheme();
        }
        else if (className == "FlatUIProfileSpace") {
            // Cast to FlatUIProfileSpace and call RefreshTheme
            FlatUIProfileSpace* profileSpace = static_cast<FlatUIProfileSpace*>(window);
            profileSpace->RefreshTheme();
        }
        else if (className == "FlatUISystemButtons") {
            // Cast to FlatUISystemButtons and call RefreshTheme
            FlatUISystemButtons* systemButtons = static_cast<FlatUISystemButtons*>(window);
            systemButtons->RefreshTheme();
        }
        else if (className == "FlatUITabDropdown") {
            window->SetBackgroundColour(CFG_COLOUR("BarBackgroundColour"));
        }
    }
    
    // Phase 4: Update the ribbon with new theme colors
    FlatUIBar* ribbon = GetUIBar();
    if (ribbon) {
        ribbon->SetTabBorderColour(CFG_COLOUR("BarTabBorderColour"));
        ribbon->SetActiveTabBackgroundColour(CFG_COLOUR("BarActiveTabBgColour"));
        ribbon->SetActiveTabTextColour(CFG_COLOUR("BarActiveTextColour"));
        ribbon->SetInactiveTabTextColour(CFG_COLOUR("BarInactiveTextColour"));
        ribbon->SetTabBorderTopColour(CFG_COLOUR("BarTabBorderTopColour"));
    }
    
    // Phase 5: Perform a single batch refresh to avoid flickering
    // Freeze the frame to prevent intermediate redraws
    Freeze();
    
    // Refresh all theme-aware components
    for (FlatUIThemeAware* component : themeAwareComponents) {
        component->Refresh(true);
    }
    
    // Refresh all other components
    for (wxWindow* window : otherComponents) {
        window->Refresh(true);
    }
    
    // Refresh the ribbon
    if (ribbon) {
        ribbon->Refresh(true);
    }
    
    // Thaw and update the frame
    Thaw();
    
    // Force a complete refresh of the frame
    Refresh(true);
    Update();
}
