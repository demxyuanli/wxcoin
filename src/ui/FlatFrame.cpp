#include "FlatFrame.h"
#include "flatui/FlatUIPanel.h"
#include "flatui/FlatUIPage.h"
#include "flatui/FlatUIButtonBar.h"
#include "flatui/FlatUIGallery.h"
#include "flatui/FlatUIEventManager.h"
#include "flatui/FlatUIHomeSpace.h"
#include "flatui/FlatUIHomeMenu.h"
#include "flatui/FlatUIFunctionSpace.h"
#include "flatui/FlatUIProfileSpace.h"
#include "flatui/FlatUISystemButtons.h"
#include "flatui/FlatUICustomControl.h"
#include "flatui/UIHierarchyDebugger.h"
#include "config/ThemeManager.h"
#include "config/SvgIconManager.h"
#include <wx/display.h>
#include "logger/Logger.h"
#include <wx/dcbuffer.h>
#include <wx/splitter.h>
#include <wx/sizer.h>
#include <wx/stdpaths.h>  // Add this line for wxStandardPaths
#include <wx/filename.h>  // Add this line for wxFileName
#include <wx/bmpbndl.h>
#include <string>
#include "Canvas.h"
#include "PropertyPanel.h"
#include "ObjectTreePanel.h"
#include "MouseHandler.h"
#include "NavigationController.h"
#include "GeometryFactory.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "Command.h"
#include "OCCViewer.h"
#include "CommandDispatcher.h"
#include "CommandListenerManager.h"
#include "MeshQualityDialog.h"
#include "MeshQualityDialogListener.h"
#include "RenderingSettingsListener.h"
// Add other command listeners includes...
#include <unordered_map>
#include "CommandType.h"  // for cmd::CommandType
// removed duplicate logger include
#include "STEPReader.h"
// removed duplicate splitter include
#include <wx/artprov.h>
#include <wx/aboutdlg.h>
#include <wx/filedlg.h>
#include <wx/toolbar.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/nodes/SoTexture2.h>
#include "FileNewListener.h"
#include "FileOpenListener.h"
#include "FileSaveListener.h"
#include "FileSaveAsListener.h"
#include "ImportStepListener.h"
#include "CreateBoxListener.h"
#include "CreateSphereListener.h"
#include "CreateCylinderListener.h"
#include "CreateConeListener.h"
#include "CreateTorusListener.h"
#include "CreateTruncatedCylinderListener.h"
#include "CreateWrenchListener.h"
#include "ViewAllListener.h"
#include "ViewTopListener.h"
#include "ViewFrontListener.h"
#include "ViewRightListener.h"
#include "ViewIsometricListener.h"
#include "ShowNormalsListener.h"
#include "FixNormalsListener.h"
// #include "ShowSilhouetteEdgesListener.h" // removed
#include "SetTransparencyListener.h"
#include "TextureModeDecalListener.h"
#include "TextureModeModulateListener.h"
#include "TextureModeReplaceListener.h"
#include "TextureModeBlendListener.h"
#include "ViewModeListener.h"
#include "EdgeSettingsListener.h"
#include "LightingSettingsListener.h"
#include "CoordinateSystemVisibilityListener.h"

#include "UndoListener.h"
#include "RedoListener.h"
#include "HelpAboutListener.h"
#include "NavCubeConfigListener.h"
#include "ZoomSpeedListener.h"
#include "FileExitListener.h"
#include "config/RenderingConfig.h"
#include "ShowOriginalEdgesListener.h"
#include "ShowFeatureEdgesListener.h"
#include "ShowMeshEdgesListener.h"
#include "ShowWireFrameListener.h"
#include "ShowFaceNormalsListener.h"
#include "ShowSilhouetteEdgesListener.h"
#include "renderpreview/RenderPreviewDialog.h"
#include "RenderPreviewSystemListener.h"
#include "ShowFlatWidgetsExampleListener.h"
#include "widgets/FlatWidgetsExampleDialog.h"
#include "ReferenceGridToggleListener.h"
#include "ChessboardGridToggleListener.h"

#ifdef __WXMSW__
#define NOMINMAX
#include <windows.h>
#endif
// Note: Theme change events are now defined in FlatUIFrame

// Event table for FlatFrame specific events
wxBEGIN_EVENT_TABLE(FlatFrame, FlatUIFrame) // Changed base class in macro
// Override theme change event to add custom logging behavior
EVT_COMMAND(wxID_ANY, wxEVT_THEME_CHANGED, FlatFrame::OnThemeChanged)
// Override pin state change event to handle main work area layout
EVT_COMMAND(wxID_ANY, wxEVT_PIN_STATE_CHANGED, FlatFrame::OnGlobalPinStateChanged)
// Note: Pin state changes are handled in FlatUIFrame base class
// Keep only FlatFrame specific event bindings here
// Mouse events (OnLeftDown, OnLeftUp, OnMotion) are handled by FlatUIFrame's table
// unless explicitly overridden and bound here with a different handler.
// If FlatFrame::OnLeftDown (etc.) are meant to override, they are called virtually by FlatUIFrame's handler.
// If they are completely different handlers for FlatFrame only, then they would need new EVT_LEFT_DOWN(FlatFrame::SpecificHandler)
EVT_BUTTON(wxID_NEW, FlatFrame::onCommand)
EVT_BUTTON(wxID_OPEN, FlatFrame::onCommand)
EVT_BUTTON(wxID_SAVE, FlatFrame::onCommand)
EVT_BUTTON(ID_SAVE_AS, FlatFrame::onCommand)
EVT_BUTTON(ID_IMPORT_STEP, FlatFrame::onCommand)
EVT_BUTTON(wxID_EXIT, FlatFrame::onCommand)
EVT_BUTTON(ID_CREATE_BOX, FlatFrame::onCommand)
EVT_BUTTON(ID_CREATE_SPHERE, FlatFrame::onCommand)
EVT_BUTTON(ID_CREATE_CYLINDER, FlatFrame::onCommand)
EVT_BUTTON(ID_CREATE_CONE, FlatFrame::onCommand)
EVT_BUTTON(ID_CREATE_TORUS, FlatFrame::onCommand)
EVT_BUTTON(ID_CREATE_TRUNCATED_CYLINDER, FlatFrame::onCommand)
EVT_BUTTON(ID_CREATE_WRENCH, FlatFrame::onCommand)
EVT_BUTTON(ID_VIEW_ALL, FlatFrame::onCommand)
EVT_BUTTON(ID_VIEW_TOP, FlatFrame::onCommand)
EVT_BUTTON(ID_VIEW_FRONT, FlatFrame::onCommand)
EVT_BUTTON(ID_VIEW_RIGHT, FlatFrame::onCommand)
EVT_BUTTON(ID_VIEW_ISOMETRIC, FlatFrame::onCommand)
EVT_BUTTON(ID_SHOW_NORMALS, FlatFrame::onCommand)
EVT_BUTTON(ID_SHOW_FACE_NORMALS, FlatFrame::onCommand)
EVT_BUTTON(ID_FIX_NORMALS, FlatFrame::onCommand)
EVT_BUTTON(ID_SET_TRANSPARENCY, FlatFrame::onCommand)
EVT_BUTTON(ID_TOGGLE_WIREFRAME, FlatFrame::onCommand)
// Removed Toggle Shading event handler
EVT_BUTTON(ID_TOGGLE_EDGES, FlatFrame::onCommand)
// Removed Show Faces event handler - functionality interferes with edge testing
// EVT_BUTTON(ID_SHOW_FACES, FlatFrame::onCommand)
EVT_BUTTON(ID_VIEW_SHOW_ORIGINAL_EDGES, FlatFrame::onCommand)
EVT_BUTTON(ID_SHOW_FEATURE_EDGES, FlatFrame::onCommand)
EVT_BUTTON(ID_SHOW_MESH_EDGES, FlatFrame::onCommand)
EVT_BUTTON(ID_OUTLINE_SETTINGS, FlatFrame::onCommand)
EVT_BUTTON(ID_TOGGLE_OUTLINE, FlatFrame::onCommand)

EVT_BUTTON(ID_UNDO, FlatFrame::onCommand)
EVT_BUTTON(ID_REDO, FlatFrame::onCommand)
EVT_BUTTON(ID_NAVIGATION_CUBE_CONFIG, FlatFrame::onCommand)
EVT_BUTTON(ID_ZOOM_SPEED, FlatFrame::onCommand)
EVT_BUTTON(ID_MESH_QUALITY_DIALOG, FlatFrame::onCommand)
EVT_BUTTON(ID_RENDERING_SETTINGS, FlatFrame::onCommand)
EVT_BUTTON(ID_LIGHTING_SETTINGS, FlatFrame::onCommand)
EVT_BUTTON(ID_EDGE_SETTINGS, FlatFrame::onCommand)
EVT_BUTTON(ID_RENDER_PREVIEW_SYSTEM, FlatFrame::onCommand)
EVT_BUTTON(ID_SHOW_FLAT_WIDGETS_EXAMPLE, FlatFrame::onCommand)
EVT_BUTTON(wxID_ABOUT, FlatFrame::onCommand)
// EVT_BUTTON(ID_VIEW_SHOWSILHOUETTEEDGES, FlatFrame::onCommand) // removed
EVT_BUTTON(ID_TOGGLE_SLICE, FlatFrame::onCommand)

// Texture Mode Command Events
EVT_BUTTON(ID_TEXTURE_MODE_DECAL, FlatFrame::onCommand)
EVT_BUTTON(ID_TEXTURE_MODE_MODULATE, FlatFrame::onCommand)
EVT_BUTTON(ID_TEXTURE_MODE_REPLACE, FlatFrame::onCommand)
EVT_BUTTON(ID_TEXTURE_MODE_BLEND, FlatFrame::onCommand)
EVT_BUTTON(ID_TOGGLE_COORDINATE_SYSTEM, FlatFrame::onCommand)
EVT_BUTTON(ID_TOGGLE_REFERENCE_GRID, FlatFrame::onCommand)
EVT_BUTTON(ID_TOGGLE_CHESSBOARD_GRID, FlatFrame::onCommand)
EVT_BUTTON(ID_EXPLODE_ASSEMBLY, FlatFrame::onCommand)

// Message output control button events
EVT_BUTTON(ID_MESSAGE_OUTPUT_FLOAT, FlatFrame::OnMessageOutputFloat)
EVT_BUTTON(ID_MESSAGE_OUTPUT_MINIMIZE, FlatFrame::OnMessageOutputMinimize)
EVT_BUTTON(ID_MESSAGE_OUTPUT_CLOSE, FlatFrame::OnMessageOutputClose)

// Performance shortcuts
EVT_MENU(ID_TOGGLE_LOD, FlatFrame::OnToggleLOD)
EVT_MENU(ID_FORCE_ROUGH_LOD, FlatFrame::OnForceRoughLOD)
EVT_MENU(ID_FORCE_FINE_LOD, FlatFrame::OnForceFineLoD)
EVT_MENU(ID_TOGGLE_PERFORMANCE_MONITOR, FlatFrame::OnTogglePerformanceMonitor)
EVT_MENU(ID_PERFORMANCE_PRESET, FlatFrame::OnPerformancePreset)
EVT_MENU(ID_BALANCED_PRESET, FlatFrame::OnBalancedPreset)
EVT_MENU(ID_QUALITY_PRESET, FlatFrame::OnQualityPreset)

EVT_CLOSE(FlatFrame::onClose)
EVT_ACTIVATE(FlatFrame::onActivate)
EVT_SIZE(FlatFrame::onSize)
wxEND_EVENT_TABLE()

FlatFrame::FlatFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
	: FlatUIFrame(NULL, wxID_ANY, title, pos, size, wxBORDER_NONE),
	m_ribbon(nullptr),
	m_messageOutput(nullptr),
	m_searchCtrl(nullptr),
	m_homeMenu(nullptr),
	m_searchPanel(nullptr),
	m_profilePanel(nullptr),
	m_performancePanel(nullptr),
	m_canvas(nullptr),
	m_propertyPanel(nullptr),
	m_objectTreePanel(nullptr),
	m_mouseHandler(nullptr),
	m_geometryFactory(nullptr),
	m_occViewer(nullptr),
	m_isFirstActivate(true),
	m_startupTimerFired(false),  // Initialize startup timer flag
	m_mainSplitter(nullptr),
	m_leftSplitter(nullptr),
	m_commandManager(new CommandManager()),
	m_prevFeatureEdgesRunning(false),
	m_featureProgressHoldTicks(0)
{
	wxInitAllImageHandlers();
	// PlatUIFrame::InitFrameStyle() is called by base constructor.
	// FlatFrame specific UI initialization
	InitializeUI(size);

	// Event bindings specific to FlatFrame controls
	auto& eventManager = FlatUIEventManager::getInstance();
	eventManager.bindFrameEvents(this); // General frame events (close, etc.)

	// Button events (Open, Save, etc. are specific to FlatFrame's UI)
	eventManager.bindButtonEvent(this, &FlatFrame::OnButtonClick, wxID_OPEN);
	eventManager.bindButtonEvent(this, &FlatFrame::OnButtonClick, wxID_SAVE);
	eventManager.bindButtonEvent(this, &FlatFrame::OnButtonClick, wxID_COPY);
	eventManager.bindButtonEvent(this, &FlatFrame::OnButtonClick, wxID_PASTE);
	eventManager.bindButtonEvent(this, &FlatFrame::OnButtonClick, wxID_FIND);

	eventManager.bindButtonEvent(this, &FlatFrame::OnButtonClick, wxID_ABOUT);
	eventManager.bindButtonEvent(this, &FlatFrame::OnButtonClick, wxID_STOP);

	// Events for search, profile, settings (specific to FlatFrame's UI)
	eventManager.bindButtonEvent(this, &FlatFrame::OnSearchExecute, ID_SearchExecute);
	eventManager.bindButtonEvent(this, &FlatFrame::OnUserProfile, ID_UserProfile);
	eventManager.bindButtonEvent(this, &FlatFrame::OnSettings, wxID_PREFERENCES);
	eventManager.bindButtonEvent(this, &FlatFrame::OnToggleFunctionSpace, ID_ToggleFunctionSpace);
	eventManager.bindButtonEvent(this, &FlatFrame::OnToggleProfileSpace, ID_ToggleProfileSpace);

	if (m_searchCtrl) {
		m_searchCtrl->Bind(wxEVT_COMMAND_TEXT_ENTER, &FlatFrame::OnSearchTextEnter, this);
	}

	// Menu events (specific to FlatFrame's menu items)
	eventManager.bindMenuEvent(this, &FlatFrame::OnMenuNewProject, ID_Menu_NewProject_MainFrame);
	eventManager.bindMenuEvent(this, &FlatFrame::OnMenuOpenProject, ID_Menu_OpenProject_MainFrame);
	eventManager.bindMenuEvent(this, &FlatFrame::OnShowUIHierarchy, ID_ShowUIHierarchy);
	eventManager.bindMenuEvent(this, &FlatFrame::PrintUILayout, ID_Menu_PrintLayout_MainFrame);
	eventManager.bindMenuEvent(this, &FlatFrame::OnMenuExit, wxID_EXIT);

	// Ensure timer is not already running
	if (m_startupTimer.IsRunning()) {
		m_startupTimer.Stop();
	}

	// Ensure proper timer ownership and binding
	m_startupTimer.SetOwner(this);

	// Unbind any existing timer events to prevent duplicate binding
	this->Unbind(wxEVT_TIMER, &FlatFrame::OnStartupTimer, this);

	// Bind the timer event
	this->Bind(wxEVT_TIMER, &FlatFrame::OnStartupTimer, this);

	m_startupTimer.StartOnce(100); // Reduced for faster startup if UI is complex
}

FlatFrame::~FlatFrame()
{
	// m_homeMenu is a child window, wxWidgets handles its deletion.
	// Other child controls (m_ribbon, m_messageOutput, m_searchCtrl) are also managed by wxWidgets.
	LOG_DBG("FlatFrame destruction started.", "FlatFrame");

	// Stop timers to prevent any remaining timer events
	m_startupTimer.Stop();
	m_progressTimer.Stop();

	// Unbind events to prevent access violations
	auto& eventManager = FlatUIEventManager::getInstance();
	eventManager.unbindFrameEvents(this);
	if (m_ribbon) {
		eventManager.unbindBarEvents(m_ribbon);
		FlatUIHomeSpace* homeSpace = m_ribbon->GetHomeSpace();
		if (homeSpace) {
			eventManager.unbindHomeSpaceEvents(homeSpace);
		}
	}

	LOG_DBG("FlatFrame destruction completed.", "FlatFrame");
	delete m_commandManager;
}

wxWindow* FlatFrame::GetFunctionSpaceControl() const
{
	return m_searchPanel;
}

wxWindow* FlatFrame::GetProfileSpaceControl() const
{
	return m_profilePanel;
}

FlatUIBar* FlatFrame::GetUIBar() const
{
	return m_ribbon;
}

void FlatFrame::OnGlobalPinStateChanged(wxCommandEvent& event)
{
	// Call base class implementation first
	FlatUIFrame::OnGlobalPinStateChanged(event);

	FlatUIBar* ribbon = GetUIBar();
	if (!ribbon) {
		return;
	}

	bool isPinned = event.GetInt() != 0;

	if (!isPinned) {
		// Force ribbon to immediately resize to unpinned height
		int unpinnedHeight = CFG_INT("BarUnpinnedHeight");
		wxSize currentSize = ribbon->GetSize();
		wxSize newSize = wxSize(currentSize.GetWidth(), unpinnedHeight);
		ribbon->SetSize(newSize);
	}

	// Force ribbon to update its size immediately
	if (ribbon) {
		ribbon->Layout();
		ribbon->Refresh();
		ribbon->Update();
	}

	// Force main splitter to recalculate its size and position
	if (m_mainSplitter) {
		m_mainSplitter->Layout();
		m_mainSplitter->Refresh();
		m_mainSplitter->Update();
	}

	// Add a deferred layout update to ensure proper space allocation after all changes
	CallAfter([this]() {
		// Force complete layout recalculation
		if (GetSizer()) {
			GetSizer()->Layout();
		}

		// Force main splitter to recalculate its size and position
		if (m_mainSplitter) {
			m_mainSplitter->Layout();
			m_mainSplitter->Refresh();
			m_mainSplitter->Update();
		}

		// Force frame to recalculate its layout and ensure main work area fills remaining space
		Layout();
		Refresh();
		Update();

		// Additional deferred update to ensure proper space allocation
		CallAfter([this]() {
			if (GetSizer()) {
				GetSizer()->Layout();
			}
			Layout();
			Refresh();
			Update();
			});
		});
}

void FlatFrame::LoadSVGIcons(wxWindow* parent, wxSizer* sizer)
{
	// Get the executable directory for SVG file paths
	wxString exePath = wxStandardPaths::Get().GetExecutablePath();
	wxFileName exeFile(exePath);
	wxString exeDir = exeFile.GetPath();

	// List of SVG files to load
	wxArrayString svgFiles;
	svgFiles.Add("config/icons/svg/home.svg");
	svgFiles.Add("config/icons/svg/settings.svg");
	svgFiles.Add("config/icons/svg/user.svg");
	svgFiles.Add("config/icons/svg/file.svg");
	svgFiles.Add("config/icons/svg/folder.svg");
	svgFiles.Add("config/icons/svg/search.svg");

	for (const wxString& svgFile : svgFiles)
	{
		wxString fullPath = exeDir + wxFILE_SEP_PATH + svgFile;

		// Create a panel for each SVG icon
		wxPanel* iconPanel = new wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(80, 100));
		iconPanel->SetBackgroundColour(CFG_COLOUR("IconPanelBgColour"));
		wxBoxSizer* iconSizer = new wxBoxSizer(wxVERTICAL);

		// Try to load SVG file
		if (wxFileExists(fullPath))
		{
			try
			{
				wxBitmapBundle svgBundle = wxBitmapBundle::FromSVGFile(fullPath, wxSize(16, 16));
				wxStaticBitmap* bitmap = new wxStaticBitmap(iconPanel, wxID_ANY, svgBundle);
				iconSizer->Add(bitmap, 0, wxALIGN_CENTER | wxALL, 5);

				wxFileName fn(fullPath);
				wxStaticText* label = new wxStaticText(iconPanel, wxID_ANY, fn.GetName());
				label->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
				label->SetForegroundColour(CFG_COLOUR("DefaultTextColour"));
				iconSizer->Add(label, 0, wxALIGN_CENTER | wxALL, 2);
			}
			catch (const std::exception& e)
			{
				// Create error placeholder
				wxStaticText* errorText = new wxStaticText(iconPanel, wxID_ANY, "Error\nLoading\nSVG");
				errorText->SetForegroundColour(CFG_COLOUR("ErrorTextColour"));
				iconSizer->Add(errorText, 1, wxALIGN_CENTER | wxALL, 5);

				LOG_ERR_S("Failed to load SVG: " + fullPath.ToStdString() + " - " + std::string(e.what()));
			}
		}
		else
		{
			// Create placeholder for missing file
			wxStaticText* missingText = new wxStaticText(iconPanel, wxID_ANY, "SVG\nNot\nFound");
			missingText->SetForegroundColour(CFG_COLOUR("PlaceholderTextColour"));
			iconSizer->Add(missingText, 1, wxALIGN_CENTER | wxALL, 5);

			wxFileName fn(fullPath);
			wxStaticText* label = new wxStaticText(iconPanel, wxID_ANY, fn.GetName());
			label->SetFont(wxFont(8, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
			label->SetForegroundColour(CFG_COLOUR("PlaceholderTextColour"));
			iconSizer->Add(label, 0, wxALIGN_CENTER | wxALL, 2);
		}

		iconPanel->SetSizer(iconSizer);
		sizer->Add(iconPanel, 0, wxALL, 5);
	}
}

wxBitmap FlatFrame::LoadHighQualityBitmap(const wxString& resourceName, const wxSize& targetSize) {
	wxBitmap bitmap(resourceName, wxBITMAP_TYPE_PNG_RESOURCE);
	if (bitmap.IsOk() && (bitmap.GetWidth() != targetSize.x || bitmap.GetHeight() != targetSize.y)) {
		wxImage image = bitmap.ConvertToImage();
		image = image.Scale(targetSize.x, targetSize.y, wxIMAGE_QUALITY_HIGH);
		return wxBitmap(image);
	}
	return bitmap;
}

void FlatFrame::OnButtonClick(wxCommandEvent& event)
{
	// Handle ribbon button clicks by dispatching to appropriate command handlers
	// Most CAD commands are handled through the event table mapping to specific handlers
	// This method handles general UI buttons that don't have specific handlers

	switch (event.GetId())
	{
	case wxID_ABOUT:
		// About dialog is handled by HelpAboutListener
		break;
	case ID_ShowUIHierarchy:
		ShowUIHierarchy();
		break;
		// removed silhouette entry; use ID_TOGGLE_OUTLINE instead
	case ID_VIEW_SHOW_ORIGINAL_EDGES:
		if (m_occViewer) m_occViewer->setShowOriginalEdges(event.IsChecked());
		break;
	case ID_TOGGLE_WIREFRAME:
		if (m_occViewer) m_occViewer->setWireframeMode(event.IsChecked());
		return;
	case ID_SHOW_MESH_EDGES:
		if (m_occViewer) m_occViewer->setShowMeshEdges(event.IsChecked());
		return;
	case ID_TOGGLE_OUTLINE:
		if (m_occViewer) m_occViewer->setOutlineEnabled(event.IsChecked());
		return;
	default:
		// For other buttons, let the event propagate to be handled by command system
		event.Skip();
		break;
	}
}

void FlatFrame::OnMenuNewProject(wxCommandEvent& event)
{
	// Handle new project - delegate to file new listener
	wxCommandEvent newEvent(wxEVT_COMMAND_MENU_SELECTED, wxID_NEW);
	ProcessEvent(newEvent);
}

void FlatFrame::OnMenuOpenProject(wxCommandEvent& event)
{
	// Handle open project - delegate to file open listener
	wxCommandEvent openEvent(wxEVT_COMMAND_MENU_SELECTED, wxID_OPEN);
	ProcessEvent(openEvent);
}

void FlatFrame::OnMenuExit(wxCommandEvent& event)
{
	Close(true);
}

void FlatFrame::OnStartupTimer(wxTimerEvent& event)
{
	// Prevent multiple executions of startup timer
	if (m_startupTimerFired) {
		return;
	}

	// Additional safety check - ensure timer is actually running
	if (!m_startupTimer.IsRunning()) {
		LOG_DBG_S("Startup timer not running, ignoring event");
		return;
	}

	LOG_DBG_S("Startup timer executing - first time only");
	m_startupTimerFired = true;

	// Example: Forcing a refresh on the first page of the ribbon
	if (m_ribbon) {
		m_ribbon->Refresh();
		if (m_ribbon->GetPageCount() > 0) {
			FlatUIPage* page = m_ribbon->GetPage(0);
			if (page) {
				page->Show();
				page->Layout();
				page->Refresh();
			}
		}
	}

	// Ensure timer is stopped and cannot fire again
	m_startupTimer.Stop();

	// Initial UI Hierarchy debug log (optional)
	// UIHierarchyDebugger debugger;
	// debugger.PrintUIHierarchy(this);
}

void FlatFrame::OnSearchExecute(wxCommandEvent& event)
{
	if (!m_searchCtrl) return;
	wxString searchText = m_searchCtrl->GetValue();
	if (!searchText.IsEmpty())
	{
		// TODO: Implement search functionality for CAD objects
		SetStatusText("Searching for: " + searchText, 0);
	}
	else
	{
		SetStatusText("Please enter search terms", 0);
	}
}

void FlatFrame::OnSearchTextEnter(wxCommandEvent& event)
{
	OnSearchExecute(event); // Simply call the other handler
}

void FlatFrame::OnUserProfile(wxCommandEvent& event)
{
	// TODO: Implement user profile dialog
	SetStatusText("User Profile - Not implemented yet", 0);
}

void FlatFrame::OnSettings(wxCommandEvent& event)
{
	// TODO: Implement settings dialog
	SetStatusText("Settings - Not implemented yet", 0);
}

void FlatFrame::OnToggleFunctionSpace(wxCommandEvent& event)
{
	if (m_ribbon) m_ribbon->ToggleFunctionSpaceVisibility();
}

void FlatFrame::OnToggleProfileSpace(wxCommandEvent& event)
{
	if (m_ribbon) m_ribbon->ToggleProfileSpaceVisibility();
}

void FlatFrame::OnShowUIHierarchy(wxCommandEvent& event)
{
	ShowUIHierarchy();
}

void FlatFrame::ShowUIHierarchy()
{
	// Create a debug dialog to show UI hierarchy
	wxDialog* debugDialog = new wxDialog(this, wxID_ANY, "UI Hierarchy Debug",
		wxDefaultPosition, wxSize(600, 400));

	wxTextCtrl* debugText = new wxTextCtrl(debugDialog, wxID_ANY, "",
		wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxTE_READONLY);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(debugText, 1, wxEXPAND | wxALL, 5);

	wxButton* closeBtn = new wxButton(debugDialog, wxID_OK, "Close");
	sizer->Add(closeBtn, 0, wxALIGN_CENTER | wxALL, 5);

	debugDialog->SetSizer(sizer);

	UIHierarchyDebugger debugger;
	debugger.SetLogTextCtrl(debugText);
	debugger.PrintUIHierarchy(this);

	debugDialog->ShowModal();
	debugDialog->Destroy();
}

void FlatFrame::PrintUILayout(wxCommandEvent& event)
{
	// Create a debug dialog to show UI layout details
	wxDialog* layoutDialog = new wxDialog(this, wxID_ANY, "UI Layout Details",
		wxDefaultPosition, wxSize(600, 400));

	wxTextCtrl* layoutText = new wxTextCtrl(layoutDialog, wxID_ANY, "",
		wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE | wxTE_READONLY);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(layoutText, 1, wxEXPAND | wxALL, 5);

	wxButton* closeBtn = new wxButton(layoutDialog, wxID_OK, "Close");
	sizer->Add(closeBtn, 0, wxALIGN_CENTER | wxALL, 5);

	layoutDialog->SetSizer(sizer);

	wxLog* oldLog = wxLog::SetActiveTarget(new wxLogTextCtrl(layoutText));
	LogUILayout(this);
	wxLog::SetActiveTarget(oldLog);
	if (oldLog != wxLog::GetActiveTarget()) {
		delete oldLog;
	}

	layoutDialog->ShowModal();
	layoutDialog->Destroy();
}

// Message output control button event handlers
void FlatFrame::OnMessageOutputFloat(wxCommandEvent& event)
{
	// Find the message output panel and make it float
	if (m_messageOutput) {
		wxWindow* msgPanel = m_messageOutput->GetParent();
		if (msgPanel) {
			// Create a floating frame for the message output
			wxFrame* floatFrame = new wxFrame(this, wxID_ANY, "Message Output",
				wxDefaultPosition, wxSize(600, 400));

			// Move the message output to the floating frame
			msgPanel->Reparent(floatFrame);

			// Set up the floating frame
			wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
			sizer->Add(msgPanel, 1, wxEXPAND | wxALL, 5);
			floatFrame->SetSizer(sizer);

			// Show the floating frame
			floatFrame->Show();

			// Hide the original message output in main frame
			msgPanel->Hide();

			SetStatusText("Message Output window is now floating", 0);
		}
	}
}

void FlatFrame::OnMessageOutputMinimize(wxCommandEvent& event)
{
	// Find the message output panel and minimize it
	if (m_messageOutput) {
		wxWindow* msgPanel = m_messageOutput->GetParent();
		if (msgPanel) {
			// Toggle visibility to simulate minimize/restore
			if (msgPanel->IsShown()) {
				msgPanel->Hide();
				SetStatusText("Message Output window minimized", 0);
			}
			else {
				msgPanel->Show();
				SetStatusText("Message Output window restored", 0);
			}

			// Refresh layout
			Layout();
		}
	}
}

void FlatFrame::OnMessageOutputClose(wxCommandEvent& event)
{
	// Find the message output panel and close it
	if (m_messageOutput) {
		wxWindow* msgPanel = m_messageOutput->GetParent();
		if (msgPanel) {
			// Hide the message output panel
			msgPanel->Hide();
			SetStatusText("Message Output window closed", 0);

			// Refresh layout
			Layout();
		}
	}
}

void FlatFrame::OnKeyDown(wxKeyEvent& event)
{
	// Check for message output shortcuts first
	if (event.ControlDown() && event.ShiftDown()) {
		switch (event.GetKeyCode()) {
		case 'F':
		case 'f':
			OnMessageOutputFloat(wxCommandEvent());
			return;
		case 'M':
		case 'm':
			OnMessageOutputMinimize(wxCommandEvent());
			return;
		case 'C':
		case 'c':
			OnMessageOutputClose(wxCommandEvent());
			return;
		}
	}

	// Let other handlers process the event
	event.Skip();
}

// Performance shortcut handlers
void FlatFrame::OnToggleLOD(wxCommandEvent& event)
{
	if (m_occViewer) {
		bool lodEnabled = m_occViewer->isLODEnabled();
		m_occViewer->setLODEnabled(!lodEnabled);
		appendMessage(wxString::Format("LOD %s", lodEnabled ? "disabled" : "enabled"));
	}
}

void FlatFrame::OnForceRoughLOD(wxCommandEvent& event)
{
	if (m_occViewer) {
		m_occViewer->setLODMode(true); // true = rough mode
		appendMessage("Forced rough LOD mode");
	}
}

void FlatFrame::OnForceFineLoD(wxCommandEvent& event)
{
	if (m_occViewer) {
		m_occViewer->setLODMode(false); // false = fine mode
		appendMessage("Forced fine LOD mode");
	}
}

void FlatFrame::OnTogglePerformanceMonitor(wxCommandEvent& event)
{
	// Toggle performance panel visibility
	if (m_performancePanel) {
		bool visible = m_performancePanel->IsShown();
		m_performancePanel->Show(!visible);
		Layout();
		appendMessage(wxString::Format("Performance monitor %s", visible ? "hidden" : "shown"));
	}
}

void FlatFrame::OnPerformancePreset(wxCommandEvent& event)
{
	if (m_occViewer) {
		// Apply performance preset
		m_occViewer->setMeshDeflection(2.0, true);
		m_occViewer->setLODEnabled(true);
		m_occViewer->setLODRoughDeflection(3.0);
		m_occViewer->setLODFineDeflection(1.0);
		m_occViewer->setParallelProcessing(true);
		appendMessage("Applied Performance Preset (Alt+1)");
	}
}

void FlatFrame::OnBalancedPreset(wxCommandEvent& event)
{
	if (m_occViewer) {
		// Apply balanced preset
		m_occViewer->setMeshDeflection(1.0, true);
		m_occViewer->setLODEnabled(true);
		m_occViewer->setLODRoughDeflection(1.5);
		m_occViewer->setLODFineDeflection(0.5);
		m_occViewer->setParallelProcessing(true);
		appendMessage("Applied Balanced Preset (Alt+2)");
	}
}

void FlatFrame::OnQualityPreset(wxCommandEvent& event)
{
	if (m_occViewer) {
		// Apply quality preset
		m_occViewer->setMeshDeflection(0.2, true);
		m_occViewer->setLODEnabled(true);
		m_occViewer->setLODRoughDeflection(0.5);
		m_occViewer->setLODFineDeflection(0.1);
		m_occViewer->setParallelProcessing(true);
		appendMessage("Applied Quality Preset (Alt+3)");
	}
}

void FlatFrame::EnsurePanelsCreated()
{
	// Ensure panels are created if not already done
	if (!m_canvas || !m_propertyPanel || !m_objectTreePanel) {
		createPanels();
	}
}