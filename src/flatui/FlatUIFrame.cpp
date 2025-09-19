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

	// Register theme change listener
	auto& themeManager = ThemeManager::getInstance();
	themeManager.addThemeChangeListener(this, [this]() {
		RefreshAllUI();
		});
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

	// HomeSpace width (use configured value instead of hardcoded 30px)
	if (ribbon->GetHomeSpace()) {
		minWidth += CFG_INT("SystemButtonWidth");
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

	// Remove frame-level debouncing since ThemeManager already handles debouncing
	// This prevents double-debouncing which can cause theme changes to be missed
	LOG_INF("Processing theme change event for theme: " + themeName.ToStdString(), "FlatUIFrame");

	// Perform comprehensive UI refresh without additional debouncing
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
	}
	else {
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
	// Synchronous update to ensure proper timing and avoid race conditions
	// ThemeManager already handles debouncing, so no need for additional debouncing here
	LOG_INF("Starting comprehensive UI refresh for theme change", "FlatUIFrame");

	// Perform immediate synchronous update for better reliability
	PerformSyncThemeUpdate();
}

void FlatUIFrame::PerformSyncThemeUpdate()
{
	// Phase 1: Freeze UI to prevent intermediate redraws
	Freeze();

	// Phase 2: Batch update all components without individual refreshes
	BatchUpdateAllComponents();

	// Phase 3: Single refresh at the end
	Thaw();
	InvalidateBestSize();
	Layout(); // Restored Layout() call - duplicate processing issues have been fixed
	Refresh(true);
	Update();

	LOG_INF("Synchronous theme refresh completed", "FlatUIFrame");
}

void FlatUIFrame::BatchUpdateAllComponents()
{
	// Collect all components that need theme updates
	std::vector<wxWindow*> allComponents;
	std::function<void(wxWindow*)> collectComponents = [&](wxWindow* window) {
		if (!window) return;
		allComponents.push_back(window);

		// Recursively collect all child controls
		wxWindowList& children = window->GetChildren();
		for (wxWindow* child : children) {
			collectComponents(child);
		}
		};

	collectComponents(this);

	// Batch update theme values without refreshing
	for (wxWindow* window : allComponents) {
		wxString className = window->GetClassInfo()->GetClassName();

		// Check if component implements FlatUIThemeAware (automatic theme handling)
		FlatUIThemeAware* themeAware = dynamic_cast<FlatUIThemeAware*>(window);
		if (themeAware) {
			// Skip components that inherit from FlatUIThemeAware as they handle theme changes automatically
			// These components already have their own theme listeners and will refresh themselves
			LOG_DBG("Skipping FlatUIThemeAware component: " + className.ToStdString(), "FlatUIFrame");
			continue;
		}

		// Update components that have RefreshTheme method but don't inherit from FlatUIThemeAware
		if (className == "FlatUIHomeMenu") {
			// Skip FlatUIHomeMenu - it has its own theme listener and RefreshTheme method
			LOG_DBG("Skipping FlatUIHomeMenu - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "FlatUIPage") {
			// Skip FlatUIPage - it has its own theme listener and RefreshTheme method
			LOG_DBG("Skipping FlatUIPage - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "FlatUIHomeSpace") {
			// Skip FlatUIHomeSpace - it has its own theme listener and RefreshTheme method
			// Duplicate processing here can cause layout issues like extra spacing around Home logo
			LOG_DBG("Skipping FlatUIHomeSpace - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "FlatUIProfileSpace") {
			// Skip FlatUIProfileSpace - it has its own theme listener and RefreshTheme method
			LOG_DBG("Skipping FlatUIProfileSpace - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "FlatUISystemButtons") {
			// Skip FlatUISystemButtons - it has its own theme listener and RefreshTheme method
			LOG_DBG("Skipping FlatUISystemButtons - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "FlatBarSpaceContainer") {
			// Skip FlatBarSpaceContainer - it has its own theme listener and RefreshTheme method
			LOG_DBG("Skipping FlatBarSpaceContainer - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "FlatUIFunctionSpace") {
			// Skip FlatUIFunctionSpace - it has its own theme listener and RefreshTheme method
			LOG_DBG("Skipping FlatUIFunctionSpace - has its own theme handling", "FlatUIFrame");
		}
		// Skip Flat Widget controls - they all have their own theme listeners
		else if (className == "FlatButton") {
			LOG_DBG("Skipping FlatButton - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "FlatCheckBox") {
			LOG_DBG("Skipping FlatCheckBox - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "FlatComboBox") {
			LOG_DBG("Skipping FlatComboBox - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "FlatLineEdit") {
			LOG_DBG("Skipping FlatLineEdit - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "FlatProgressBar") {
			LOG_DBG("Skipping FlatProgressBar - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "FlatRadioButton") {
			LOG_DBG("Skipping FlatRadioButton - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "FlatSlider") {
			LOG_DBG("Skipping FlatSlider - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "FlatSwitch") {
			LOG_DBG("Skipping FlatSwitch - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "FlatEnhancedButton") {
			LOG_DBG("Skipping FlatEnhancedButton - has its own theme handling", "FlatUIFrame");
		}
		// Update additional FlatUI controls
		else if (className == "FlatUIStatusBar") {
			window->Refresh(true);
			window->Update();
		}
		else if (className == "FlatUIUnpinButton") {
			window->Refresh(true);
			window->Update();
		}
		else if (className == "FlatUIPinControl") {
			window->Refresh(true);
			window->Update();
		}
		else if (className == "FlatUIPinButton") {
			window->Refresh(true);
			window->Update();
		}
		else if (className == "FlatUICustomControl") {
			window->Refresh(true);
			window->Update();
		}
		else if (className == "FlatUISpacerControl") {
			window->Refresh(true);
			window->Update();
		}
		else if (className == "FlatBarNotebook") {
			window->Refresh(true);
			window->Update();
		}
		// Update standard wxWidgets controls
		else if (className == "wxStaticText") {
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
		else if (className == "FlatUIBar") {
			// Skip FlatUIBar - it has its own theme listener and RefreshTheme method
			// Duplicate processing here can cause layout issues like extra top margin
			LOG_DBG("Skipping FlatUIBar - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "FlatUITabDropdown") {
			window->SetBackgroundColour(CFG_COLOUR("BarBackgroundColour"));
		}
		// Skip DockArea components - they all have their own theme listeners
		else if (className == "DockArea") {
			LOG_DBG("Skipping DockArea - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "DockAreaMergedTitleBar") {
			LOG_DBG("Skipping DockAreaMergedTitleBar - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "DockAreaTabBar") {
			LOG_DBG("Skipping DockAreaTabBar - has its own theme handling", "FlatUIFrame");
		}
		else if (className == "DockAreaTitleBar") {
			LOG_DBG("Skipping DockAreaTitleBar - has its own theme handling", "FlatUIFrame");
		}
	}

	// Note: FlatUIBar colors are handled by its own RefreshTheme method
	// to avoid duplicate processing and layout issues
	LOG_INF("Batch component theme update completed", "FlatUIFrame");
}