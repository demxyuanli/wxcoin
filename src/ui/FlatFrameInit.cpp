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
#include "config/RenderingConfig.h"
#include "async/AsyncEngineIntegration.h"
#include <wx/display.h>
#include "logger/Logger.h"
#include <wx/dcbuffer.h>
#include <wx/splitter.h>
#include <wx/sizer.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/bmpbndl.h>
#include <string>
#include "Canvas.h"
#include "PropertyPanel.h"
#include "ObjectTreePanel.h"
#include "MouseHandler.h"
#include "NavigationController.h"
#include "NavigationModeManager.h"
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
#include <unordered_map>
#include "CommandType.h"
#include "STEPReader.h"
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
#include "renderpreview/RenderPreviewDialog.h"
#include "RenderPreviewSystemListener.h"
#include "ShowFlatWidgetsExampleListener.h"
#include "widgets/FlatWidgetsExampleDialog.h"
#include "widgets/ModernDockAdapter.h"
#include "widgets/ButtonGroup.h"
#include "ReferenceGridToggleListener.h"
#include "ChessboardGridToggleListener.h"
#include "widgets/FlatMessagePanel.h"
#include "ui/PerformancePanel.h"
#include "async/AsyncEngineIntegration.h"

void FlatFrame::InitializeUI(const wxSize& size)
{
	SetBackgroundColour(CFG_COLOUR("TitledPanelBgColour"));

	int barHeight = FlatUIBar::GetBarHeight();
	m_ribbon = new FlatUIBar(this, wxID_ANY, wxDefaultPosition, wxSize(-1, barHeight * 3));
	wxFont defaultFont = CFG_DEFAULTFONT();
	m_ribbon->SetDoubleBuffered(true);
	m_ribbon->SetTabStyle(FlatUIBar::TabStyle::DEFAULT);
	m_ribbon->SetTabBorderColour(CFG_COLOUR("BarTabBorderColour"));
	m_ribbon->SetActiveTabBackgroundColour(CFG_COLOUR("BarActiveTabBgColour"));
	m_ribbon->SetActiveTabTextColour(CFG_COLOUR("BarActiveTextColour"));
	m_ribbon->SetInactiveTabTextColour(CFG_COLOUR("BarInactiveTextColour"));
	m_ribbon->SetTabBorderStyle(FlatUIBar::TabBorderStyle::SOLID);
	m_ribbon->SetTabBorderWidths(2, 0, 1, 1);
	m_ribbon->SetTabBorderTopColour(CFG_COLOUR("BarTabBorderTopColour"));
	m_ribbon->SetTabCornerRadius(0);
	m_ribbon->SetHomeButtonWidth(CFG_INT("SystemButtonWidth"));

	FlatUIHomeSpace* homeSpace = m_ribbon->GetHomeSpace();
	if (homeSpace) {
		m_homeMenu = new FlatUIHomeMenu(homeSpace, this);
		m_homeMenu->AddMenuItem("&New Project...\tCtrl-N", ID_Menu_NewProject_MainFrame);
		m_homeMenu->AddSeparator();
		m_homeMenu->AddMenuItem("Show UI &Hierarchy\tCtrl-H", ID_ShowUIHierarchy);
		m_homeMenu->AddSeparator();
		m_homeMenu->AddMenuItem("Test &Widgets\tCtrl-W", ID_TEST_WIDGETS);
		m_homeMenu->AddSeparator();
		m_homeMenu->AddMenuItem("&Configuration Manager...\tCtrl+Shift+C", ID_CONFIG_MANAGER);
		m_homeMenu->AddSeparator();
		m_homeMenu->AddMenuItem("Print Frame All wxCtr", ID_Menu_PrintLayout_MainFrame);
		m_homeMenu->BuildMenuLayout();
		homeSpace->SetHomeMenu(m_homeMenu);
	}
	else {
		LOG_ERR("FlatUIHomeSpace is not available to attach the menu.", "FlatFrame");
	}

	m_ribbon->AddSpaceSeparator(FlatUIBar::SPACER_TAB_FUNCTION, 30, false, true, true);

	wxPanel* searchPanel = new wxPanel(m_ribbon);
	searchPanel->SetBackgroundColour(CFG_COLOUR("BarBgColour"));
	wxBoxSizer* searchSizer = new wxBoxSizer(wxHORIZONTAL);
	m_searchCtrl = new wxSearchCtrl(searchPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(240, -1), wxTE_PROCESS_ENTER);
	m_searchCtrl->SetFont(defaultFont);
	m_searchCtrl->SetBackgroundColour(CFG_COLOUR("SearchCtrlBgColour"));
	m_searchCtrl->SetForegroundColour(CFG_COLOUR("SearchCtrlFgColour"));
	m_searchCtrl->ShowSearchButton(true);
	m_searchCtrl->ShowCancelButton(true);
	m_searchButton = new wxBitmapButton(searchPanel, ID_SearchExecute, SvgIconManager::GetInstance().GetIconBitmap("search", wxSize(16, 16)));
	m_searchButton->SetBackgroundColour(CFG_COLOUR("BarBgColour"));
	searchSizer->Add(m_searchCtrl, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	searchSizer->Add(m_searchButton, 0, wxALIGN_CENTER_VERTICAL);
	searchPanel->SetSizer(searchSizer);
	searchPanel->SetFont(defaultFont);
	m_ribbon->SetFunctionSpaceControl(searchPanel, 270);

	wxPanel* profilePanel = new wxPanel(m_ribbon);
	profilePanel->SetBackgroundColour(CFG_COLOUR("BarBgColour"));
	wxBoxSizer* profileSizer = new wxBoxSizer(wxHORIZONTAL);
	m_userButton = new wxBitmapButton(profilePanel, ID_UserProfile, SvgIconManager::GetInstance().GetIconBitmap("user", wxSize(16, 16)));
	m_userButton->SetToolTip("User Profile");
	m_userButton->SetBackgroundColour(CFG_COLOUR("BarBgColour"));
	m_settingsButton = new wxBitmapButton(profilePanel, wxID_PREFERENCES, SvgIconManager::GetInstance().GetIconBitmap("settings", wxSize(16, 16)));
	m_settingsButton->SetToolTip("Settings");
	m_settingsButton->SetBackgroundColour(CFG_COLOUR("BarBgColour"));
	profileSizer->Add(m_userButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	profileSizer->Add(m_settingsButton, 0, wxALIGN_CENTER_VERTICAL);
	profilePanel->SetSizer(profileSizer);
	m_ribbon->SetProfileSpaceControl(profilePanel, 60);

	m_ribbon->AddSpaceSeparator(FlatUIBar::SPACER_FUNCTION_PROFILE, 30, false, true, true);

	m_searchPanel = searchPanel;
	m_profilePanel = profilePanel;

	FlatUIPage* page1 = new FlatUIPage(m_ribbon, "Project");
	FlatUIPanel* filePanel = new FlatUIPanel(page1, "File", wxHORIZONTAL);
	filePanel->SetFont(CFG_DEFAULTFONT());
	filePanel->SetPanelBorderWidths(0, 0, 0, 1);
	filePanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	filePanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	filePanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	filePanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* fileButtonBar = new FlatUIButtonBar(filePanel);
	fileButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	fileButtonBar->AddButtonWithSVG(wxID_NEW, "New", "new", wxSize(16, 16), nullptr, "Create a new project");
	fileButtonBar->AddButtonWithSVG(wxID_OPEN, "Open", "open", wxSize(16, 16), nullptr, "Open an existing project");
	fileButtonBar->AddButtonWithSVG(wxID_SAVE, "Save", "save", wxSize(16, 16), nullptr, "Save current project");
	fileButtonBar->AddButtonWithSVG(ID_SAVE_AS, "Save As", "saveas", wxSize(16, 16), nullptr, "Save project with a new name");
	fileButtonBar->AddButtonWithSVG(ID_IMPORT_STEP, "Import STEP", "import", wxSize(16, 16), nullptr, "Import STEP file");
	filePanel->AddButtonBar(fileButtonBar, 0, wxEXPAND | wxALL, 5);
	page1->AddPanel(filePanel);

	FlatUIPanel* createPanel = new FlatUIPanel(page1, "Create", wxHORIZONTAL);
	createPanel->SetFont(CFG_DEFAULTFONT());
	createPanel->SetPanelBorderWidths(0, 0, 0, 1);
	createPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	createPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	createPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	createPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* createButtonBar = new FlatUIButtonBar(createPanel);
	createButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	createButtonBar->AddButtonWithSVG(ID_CREATE_BOX, "Box", "cube", wxSize(16, 16), nullptr, "Create a box geometry");
	createButtonBar->AddButtonWithSVG(ID_CREATE_SPHERE, "Sphere", "sphere", wxSize(16, 16), nullptr, "Create a sphere geometry");
	createButtonBar->AddButtonWithSVG(ID_CREATE_CYLINDER, "Cylinder", "cylinder", wxSize(16, 16), nullptr, "Create a cylinder geometry");
	createButtonBar->AddButtonWithSVG(ID_CREATE_CONE, "Cone", "cone", wxSize(16, 16), nullptr, "Create a cone geometry");
	createButtonBar->AddButtonWithSVG(ID_CREATE_TORUS, "Torus", "torus", wxSize(16, 16), nullptr, "Create a torus geometry");
	createButtonBar->AddButtonWithSVG(ID_CREATE_TRUNCATED_CYLINDER, "trapezoid", "trapezoid", wxSize(16, 16), nullptr, "Create a truncated cylinder geometry");
	createButtonBar->AddButtonWithSVG(ID_CREATE_WRENCH, "Wrench", "wrench", wxSize(16, 16), nullptr, "Create a wrench geometry");
	createButtonBar->AddButtonWithSVG(ID_CREATE_NAV_CUBE, "Nav Cube", "m-cube", wxSize(16, 16), nullptr, "Create a navigation cube geometry");
	createPanel->AddButtonBar(createButtonBar, 0, wxEXPAND | wxALL, 5);
	page1->AddPanel(createPanel);
	m_ribbon->AddPage(page1);

	FlatUIPage* page2 = new FlatUIPage(m_ribbon, "WorkShop");
	FlatUIPanel* editPanel = new FlatUIPanel(page2, "Edit", wxHORIZONTAL);
	editPanel->SetFont(CFG_DEFAULTFONT());
	editPanel->SetPanelBorderWidths(0, 0, 0, 1);
	editPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	editPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	editPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	editPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* editButtonBar = new FlatUIButtonBar(editPanel);
	editButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	editButtonBar->AddButtonWithSVG(ID_UNDO, "Undo", "undo", wxSize(16, 16), nullptr, "Undo last operation");
	editButtonBar->AddButtonWithSVG(ID_REDO, "Redo", "redo", wxSize(16, 16), nullptr, "Redo last undone operation");
	editPanel->AddButtonBar(editButtonBar, 0, wxEXPAND | wxALL, 5);
	page2->AddPanel(editPanel);
	m_ribbon->AddPage(page2);

	FlatUIPage* page3 = new FlatUIPage(m_ribbon, "View");
	FlatUIPanel* viewPanel = new FlatUIPanel(page3, "Views", wxHORIZONTAL);
	viewPanel->SetFont(CFG_DEFAULTFONT());
	viewPanel->SetPanelBorderWidths(0, 0, 0, 1);
	viewPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	viewPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	viewPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	viewPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* viewButtonBar = new FlatUIButtonBar(viewPanel);
	viewButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	viewButtonBar->AddButtonWithSVG(ID_VIEW_ALL, "Fit All", "fitview", wxSize(16, 16), nullptr, "Fit all objects in view");
	viewButtonBar->AddButtonWithSVG(ID_VIEW_TOP, "Top", "topview", wxSize(16, 16), nullptr, "Switch to top view");
	viewButtonBar->AddButtonWithSVG(ID_VIEW_FRONT, "Front", "frontview", wxSize(16, 16), nullptr, "Switch to front view");
	viewButtonBar->AddButtonWithSVG(ID_VIEW_RIGHT, "Right", "rightview", wxSize(16, 16), nullptr, "Switch to right view");
	viewButtonBar->AddButtonWithSVG(ID_VIEW_ISOMETRIC, "Isometric", "isoview", wxSize(16, 16), nullptr, "Switch to isometric view");
	viewButtonBar->AddToggleButtonWithSVG(ID_SHOW_POINT_VIEW, "Point View", "pointview", wxSize(16, 16), false, "Toggle point view mode");
	viewPanel->AddButtonBar(viewButtonBar, 0, wxEXPAND | wxALL, 5);
	page3->AddPanel(viewPanel);

	FlatUIPanel* splitViewPanel = new FlatUIPanel(page3, "Split View", wxHORIZONTAL);
	splitViewPanel->SetFont(CFG_DEFAULTFONT());
	splitViewPanel->SetPanelBorderWidths(0, 0, 0, 1);
	splitViewPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	splitViewPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	splitViewPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	splitViewPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* splitViewButtonBar = new FlatUIButtonBar(splitViewPanel);
	splitViewButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	splitViewButtonBar->AddButtonWithSVG(ID_SPLIT_VIEW_SINGLE, "Single View", "one-split", wxSize(16, 16), nullptr, "Single viewport mode");
	splitViewButtonBar->AddButtonWithSVG(ID_SPLIT_VIEW_HORIZONTAL_2, "Horizontal Split", "h-split", wxSize(16, 16), nullptr, "Split viewport horizontally (2 views)");
	splitViewButtonBar->AddButtonWithSVG(ID_SPLIT_VIEW_VERTICAL_2, "Vertical Split", "v-split", wxSize(16, 16), nullptr, "Split viewport vertically (2 views)");
	splitViewButtonBar->AddButtonWithSVG(ID_SPLIT_VIEW_QUAD, "Quad View", "four-split", wxSize(16, 16), nullptr, "Quad viewport mode (4 views)");
	splitViewButtonBar->AddButtonWithSVG(ID_SPLIT_VIEW_SIX, "Six View", "six-split", wxSize(16, 16), nullptr, "Six viewport mode (6 views)");
	splitViewButtonBar->AddToggleButtonWithSVG(ID_SPLIT_VIEW_TOGGLE_SYNC, "Sync Views", "sync-view", wxSize(16, 16), true, "Toggle camera synchronization across split views");
	splitViewPanel->AddButtonBar(splitViewButtonBar, 0, wxEXPAND | wxALL, 5);
	page3->AddPanel(splitViewPanel);

	FlatUIPanel* assemblyPanel = new FlatUIPanel(page3, "Assembly", wxHORIZONTAL);
	assemblyPanel->SetFont(CFG_DEFAULTFONT());
	assemblyPanel->SetPanelBorderWidths(0, 0, 0, 1);
	assemblyPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	assemblyPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	assemblyPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	assemblyPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* assemblyButtonBar = new FlatUIButtonBar(assemblyPanel);
	assemblyButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	assemblyButtonBar->AddToggleButtonWithSVG(ID_EXPLODE_ASSEMBLY, "Explode", "explode", wxSize(16, 16), false, "Toggle exploded view for assemblies");
	// Extra: we will open a small slider when explode is active
	// Remove separate config button to match requirement: clicking Explode opens config first
	assemblyPanel->AddButtonBar(assemblyButtonBar, 0, wxEXPAND | wxALL, 5);
	page3->AddPanel(assemblyPanel);

	FlatUIPanel* assiPanel = new FlatUIPanel(page3, "Assistant", wxHORIZONTAL);
	assiPanel->SetFont(CFG_DEFAULTFONT());
	assiPanel->SetPanelBorderWidths(0, 0, 0, 1);
	assiPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	assiPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	assiPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	assiPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* AssiButtonBar = new FlatUIButtonBar(assiPanel);
	AssiButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	AssiButtonBar->AddButtonWithSVG(ID_SET_TRANSPARENCY, "Set Transparency", "transparency", wxSize(16, 16), nullptr, "Set object transparency");
	AssiButtonBar->AddToggleButtonWithSVG(ID_TOGGLE_COORDINATE_SYSTEM, "Toggle Coordinate System", "coordinate", wxSize(16, 16), false, "Toggle coordinate system display");
	AssiButtonBar->AddToggleButtonWithSVG(ID_TOGGLE_REFERENCE_GRID, "Reference Grid", "grid", wxSize(16, 16), false, "Toggle reference grid plane");
	AssiButtonBar->AddToggleButtonWithSVG(ID_TOGGLE_CHESSBOARD_GRID, "Chessboard Grid", "chessboard", wxSize(16, 16), false, "Toggle chessboard ground plane");
	assiPanel->AddButtonBar(AssiButtonBar, 0, wxEXPAND | wxALL, 5);
	page3->AddPanel(assiPanel);


	FlatUIPanel* displayPanel = new FlatUIPanel(page3, "Geom Display", wxHORIZONTAL);
	displayPanel->SetFont(CFG_DEFAULTFONT());
	displayPanel->SetPanelBorderWidths(0, 0, 0, 1);
	displayPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	displayPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	displayPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	displayPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* displayButtonBar = new FlatUIButtonBar(displayPanel);
	displayButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	displayButtonBar->AddToggleButtonWithSVG(ID_VIEW_SHOW_ORIGINAL_EDGES, "Original Edges", "ori-edge", wxSize(16, 16), false, "Toggle original edge display");
	displayButtonBar->AddButtonWithSVG(ID_CANCEL_INTERSECTION_COMPUTATION, "Cancel Intersection", "cancel", wxSize(16, 16), nullptr, "Cancel ongoing intersection computation");
	displayButtonBar->AddButtonWithSVG(ID_COMPUTE_INTERSECTIONS, "Compute Intersections", "intersection", wxSize(16, 16), nullptr, "Compute edge intersections asynchronously");
	displayButtonBar->AddToggleButtonWithSVG(ID_SHOW_FEATURE_EDGES, "Feature Edges", "feature-edges", wxSize(16, 16), false, "Toggle feature edge display");
	displayButtonBar->AddToggleButtonWithSVG(ID_TOGGLE_WIREFRAME, "Wireframe Mode", "wireframe-view", wxSize(16, 16), false, "Toggle wireframe rendering mode");
	displayButtonBar->AddToggleButtonWithSVG(ID_SHOW_MESH_EDGES, "Show Mesh Edges", "mesh", wxSize(16, 16), false, "Show/hide mesh edges overlay");
	displayButtonBar->AddToggleButtonWithSVG(ID_SHOW_NORMALS, "Show Normals", "point-normals", wxSize(16, 16), false, "Toggle normal vectors display");
	displayButtonBar->AddToggleButtonWithSVG(ID_SHOW_FACE_NORMALS, "Show Face Normals", "face-normals", wxSize(16, 16), false, "Toggle face normal vectors display");
	displayButtonBar->AddButtonWithSVG(ID_FIX_NORMALS, "Fix Normals", "fix-normals", wxSize(16, 16), nullptr, "Fix normal vectors orientation");
	displayButtonBar->AddButtonWithSVG(ID_NORMAL_FIX_DIALOG, "Normal Fix Dialog", "normals-settings", wxSize(16, 16), nullptr, "Open normal fix settings dialog");
	displayButtonBar->AddToggleButtonWithSVG(ID_TOGGLE_SLICE, "Slice", "slice", wxSize(16, 16), false, "Toggle slicing plane and drag to move");
	displayButtonBar->AddToggleButtonWithSVG(ID_TOGGLE_OUTLINE, "Outline", "outline", wxSize(16, 16), false, "Toggle geometry outline rendering");
	displayButtonBar->AddButtonWithSVG(ID_OUTLINE_SETTINGS, "Outline Settings", "outline-settings", wxSize(16, 16), nullptr, "Open outline settings");
	displayPanel->AddButtonBar(displayButtonBar, 0, wxEXPAND | wxALL, 5);
	page3->AddPanel(displayPanel);
	m_ribbon->AddPage(page3);

	// Create Navigator page with navigation tools
	FlatUIPage* navigatorPage = new FlatUIPage(m_ribbon, "Navigator");

	// Bookmarks panel
	FlatUIPanel* bookmarkPanel = new FlatUIPanel(navigatorPage, "Bookmarks", wxHORIZONTAL);
	bookmarkPanel->SetFont(CFG_DEFAULTFONT());
	bookmarkPanel->SetPanelBorderWidths(0, 0, 0, 1);
	bookmarkPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	bookmarkPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	bookmarkPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	bookmarkPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* bookmarkButtonBar = new FlatUIButtonBar(bookmarkPanel);
	bookmarkButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	bookmarkButtonBar->AddButtonWithSVG(ID_VIEW_BOOKMARK_SAVE, "Save Bookmark", "save", wxSize(16, 16), nullptr, "Save current view as bookmark");
	bookmarkButtonBar->AddButtonWithSVG(ID_VIEW_BOOKMARK_MANAGER, "Manage Bookmarks", "folder", wxSize(16, 16), nullptr, "Open bookmark manager");
	bookmarkPanel->AddButtonBar(bookmarkButtonBar, 0, wxEXPAND | wxALL, 5);
	navigatorPage->AddPanel(bookmarkPanel);

	// Standard views panel
	FlatUIPanel* standardViewPanel = new FlatUIPanel(navigatorPage, "Standard Views", wxHORIZONTAL);
	standardViewPanel->SetFont(CFG_DEFAULTFONT());
	standardViewPanel->SetPanelBorderWidths(0, 0, 0, 1);
	standardViewPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	standardViewPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	standardViewPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	standardViewPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* standardViewButtonBar = new FlatUIButtonBar(standardViewPanel);
	standardViewButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	standardViewButtonBar->AddButtonWithSVG(ID_VIEW_BOOKMARK_FRONT, "Front", "front-view", wxSize(16, 16), nullptr, "Animate to front view");
	standardViewButtonBar->AddButtonWithSVG(ID_VIEW_BOOKMARK_BACK, "Back", "back-view", wxSize(16, 16), nullptr, "Animate to back view");
	standardViewButtonBar->AddButtonWithSVG(ID_VIEW_BOOKMARK_LEFT, "Left", "left-view", wxSize(16, 16), nullptr, "Animate to left view");
	standardViewButtonBar->AddButtonWithSVG(ID_VIEW_BOOKMARK_RIGHT, "Right", "right-view", wxSize(16, 16), nullptr, "Animate to right view");
	standardViewButtonBar->AddButtonWithSVG(ID_VIEW_BOOKMARK_TOP, "Top", "top-view", wxSize(16, 16), nullptr, "Animate to top view");
	standardViewButtonBar->AddButtonWithSVG(ID_VIEW_BOOKMARK_BOTTOM, "Bottom", "bottom-view", wxSize(16, 16), nullptr, "Animate to bottom view");
	standardViewButtonBar->AddButtonWithSVG(ID_VIEW_BOOKMARK_ISOMETRIC, "Isometric", "iso-view", wxSize(16, 16), nullptr, "Animate to isometric view");
	standardViewPanel->AddButtonBar(standardViewButtonBar, 0, wxEXPAND | wxALL, 5);
	navigatorPage->AddPanel(standardViewPanel);

	// Zoom controls panel
	FlatUIPanel* zoomPanel = new FlatUIPanel(navigatorPage, "Zoom", wxHORIZONTAL);
	zoomPanel->SetFont(CFG_DEFAULTFONT());
	zoomPanel->SetPanelBorderWidths(0, 0, 0, 1);
	zoomPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	zoomPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	zoomPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	zoomPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* zoomButtonBar = new FlatUIButtonBar(zoomPanel);
	zoomButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	zoomButtonBar->AddButtonWithSVG(ID_ZOOM_IN, "Zoom In", "zoom-in", wxSize(16, 16), nullptr, "Zoom in");
	zoomButtonBar->AddButtonWithSVG(ID_ZOOM_OUT, "Zoom Out", "zoom-out", wxSize(16, 16), nullptr, "Zoom out");
	zoomButtonBar->AddButtonWithSVG(ID_ZOOM_RESET, "Reset Zoom", "zoom-reset", wxSize(16, 16), nullptr, "Reset zoom to 100%");
	zoomButtonBar->AddButtonWithSVG(ID_ZOOM_SETTINGS, "Zoom Settings", "zoom-settings", wxSize(16, 16), nullptr, "Configure zoom settings");
	zoomPanel->AddButtonBar(zoomButtonBar, 0, wxEXPAND | wxALL, 5);
	navigatorPage->AddPanel(zoomPanel);

	// Animation settings panel
	FlatUIPanel* animationPanel = new FlatUIPanel(navigatorPage, "Animation", wxHORIZONTAL);
	animationPanel->SetFont(CFG_DEFAULTFONT());
	animationPanel->SetPanelBorderWidths(0, 0, 0, 1);
	animationPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	animationPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	animationPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	animationPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* animationButtonBar = new FlatUIButtonBar(animationPanel);
	animationButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	animationButtonBar->AddButtonWithSVG(ID_ANIMATION_TYPE_LINEAR, "Linear", "linear", wxSize(16, 16), nullptr, "Set linear animation");
	animationButtonBar->AddButtonWithSVG(ID_ANIMATION_TYPE_SMOOTH, "Smooth", "smooth", wxSize(16, 16), nullptr, "Set smooth animation");
	animationButtonBar->AddButtonWithSVG(ID_ANIMATION_TYPE_EASE_IN, "Ease In", "ease-in", wxSize(16, 16), nullptr, "Set ease-in animation");
	animationButtonBar->AddButtonWithSVG(ID_ANIMATION_TYPE_EASE_OUT, "Ease Out", "ease-out", wxSize(16, 16), nullptr, "Set ease-out animation");
	animationButtonBar->AddButtonWithSVG(ID_ANIMATION_TYPE_BOUNCE, "Bounce", "bounce", wxSize(16, 16), nullptr, "Set bounce animation");
	animationPanel->AddButtonBar(animationButtonBar, 0, wxEXPAND | wxALL, 5);
	navigatorPage->AddPanel(animationPanel);

	m_ribbon->AddPage(navigatorPage);

	// Create Render page with rendering modes (similar to FreeCAD)
	FlatUIPage* renderPage = new FlatUIPage(m_ribbon, "Render");

	// Render modes panel
	FlatUIPanel* renderModePanel = new FlatUIPanel(renderPage, "Render Modes", wxHORIZONTAL);
	renderModePanel->SetFont(CFG_DEFAULTFONT());
	renderModePanel->SetPanelBorderWidths(0, 0, 0, 1);
	renderModePanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	renderModePanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	renderModePanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	renderModePanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* renderModeButtonBar = new FlatUIButtonBar(renderModePanel);
	renderModeButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);

	const struct RenderModeToggleDescriptor {
		int id;
		const char* label;
		const char* iconName;
		const char* tooltip;
	} renderModeButtons[] = {
		{ ID_RENDER_MODE_NO_SHADING, "No Shading", "cube", "No shading mode - uniform color like FreeCAD" },
		{ ID_RENDER_MODE_POINTS, "Points", "pointview", "Points mode - show only vertices" },
		{ ID_RENDER_MODE_WIREFRAME, "Wireframe", "wireframe-mod", "Wireframe mode - show only edges" },
		{ ID_RENDER_MODE_FLAT_LINES, "Flat Lines", "flat-shading", "Flat lines mode - flat shading with edges" },
		{ ID_RENDER_MODE_SHADED, "Shaded", "shaded", "Shaded mode - smooth shading with lighting" },
		{ ID_RENDER_MODE_SHADED_WIREFRAME, "Shaded+Wireframe", "wireframe-shading", "Shaded with wireframe overlay" },
		{ ID_RENDER_MODE_HIDDEN_LINE, "Hidden Line", "hidden-line", "Hidden line mode - edges with hidden line removal" },
	}; 

	constexpr int kRenderModeToggleGroup = 0;
	for (const auto& button : renderModeButtons) {
		renderModeButtonBar->AddToggleGroupButtonWithSVG(
			button.id,
			button.label,
			button.iconName,
			wxSize(16, 16),
			kRenderModeToggleGroup,
			false,
			button.tooltip);
	}

	const RenderingConfig& renderingConfig = RenderingConfig::getInstance();
	const auto displaySettings = renderingConfig.getDisplaySettings();
	const auto shadingSettings = renderingConfig.getShadingSettings();

	int selectedRenderModeId = ID_RENDER_MODE_SHADED;
	switch (displaySettings.displayMode) {
	case RenderingConfig::DisplayMode::NoShading:
		selectedRenderModeId = ID_RENDER_MODE_NO_SHADING;
		break;
	case RenderingConfig::DisplayMode::Points:
		selectedRenderModeId = ID_RENDER_MODE_POINTS;
		break;
	case RenderingConfig::DisplayMode::Wireframe:
		selectedRenderModeId = ID_RENDER_MODE_WIREFRAME;
		break;
	case RenderingConfig::DisplayMode::Solid:
		selectedRenderModeId = ID_RENDER_MODE_SHADED;
		break;
	case RenderingConfig::DisplayMode::SolidWireframe:
		if (shadingSettings.shadingMode == RenderingConfig::ShadingMode::Flat) {
			selectedRenderModeId = ID_RENDER_MODE_FLAT_LINES;
		} else {
			selectedRenderModeId = ID_RENDER_MODE_SHADED_WIREFRAME;
		}
		break;
	case RenderingConfig::DisplayMode::HiddenLine:
		selectedRenderModeId = ID_RENDER_MODE_HIDDEN_LINE;
		break;
	default:
		selectedRenderModeId = ID_RENDER_MODE_SHADED;
		break;
	}

	renderModeButtonBar->SetToggleGroupSelection(kRenderModeToggleGroup, selectedRenderModeId);
	renderModePanel->AddButtonBar(renderModeButtonBar, 0, wxEXPAND | wxALL, 5);
	renderPage->AddPanel(renderModePanel);

	// Create ButtonGroup to manage render mode buttons with mutual exclusivity
	std::vector<int> renderModeButtonIds;
	for (const auto& button : renderModeButtons) {
		renderModeButtonIds.push_back(button.id);
	}
	m_renderModeButtonGroup = new ButtonGroup(renderModeButtonBar, kRenderModeToggleGroup);
	m_renderModeButtonGroup->registerButtons(renderModeButtonIds);
	m_renderModeButtonGroup->setSelectedButton(selectedRenderModeId, false); // Set initial state without notification

	m_ribbon->AddPage(renderPage);

	FlatUIPage* page4 = new FlatUIPage(m_ribbon, "Tools");
	FlatUIPanel* toolsPanel = new FlatUIPanel(page4, "Tools", wxHORIZONTAL);
	toolsPanel->SetFont(CFG_DEFAULTFONT());
	toolsPanel->SetPanelBorderWidths(0, 0, 0, 1);
	toolsPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	toolsPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	toolsPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	toolsPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* toolsButtonBar = new FlatUIButtonBar(toolsPanel);
	toolsButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	toolsButtonBar->AddButtonWithSVG(ID_MESH_QUALITY_DIALOG, "Mesh Quality", "mesh", wxSize(16, 16), nullptr, "Open mesh quality dialog");
	toolsButtonBar->AddButtonWithSVG(ID_NAVIGATION_CUBE_CONFIG, "Nav Cube", "navi", wxSize(16, 16), nullptr, "Configure navigation cube");
	toolsButtonBar->AddButtonWithSVG(ID_ZOOM_SPEED, "Zoom Speed", "zoom-settings", wxSize(16, 16), nullptr, "Adjust zoom speed settings");
	toolsButtonBar->AddButtonWithSVG(ID_NAVIGATION_MODE, "Navigation Mode", "mouse-circle", wxSize(16, 16), nullptr, "Switch between Gesture and Inventor navigation modes");
	toolsButtonBar->AddButtonWithSVG(ID_RENDERING_SETTINGS, "Rendering Settings", "rendering", wxSize(16, 16), nullptr, "Configure material, lighting and texture settings");
	toolsButtonBar->AddButtonWithSVG(ID_LIGHTING_SETTINGS, "Lighting Settings", "light", wxSize(16, 16), nullptr, "Configure scene lighting and environment settings");
	toolsButtonBar->AddButtonWithSVG(ID_EDGE_SETTINGS, "Edge Settings", "edge-settings", wxSize(16, 16), nullptr, "Configure edge color, width and style settings");
	toolsButtonBar->AddButtonWithSVG(ID_RENDER_PREVIEW_SYSTEM, "Render Preview", "preview", wxSize(16, 16), nullptr, "Open render preview system");
	toolsPanel->AddButtonBar(toolsButtonBar, 0, wxEXPAND | wxALL, 5);
	page4->AddPanel(toolsPanel);

	FlatUIPanel* textureTestPanel = new FlatUIPanel(page4, "Texture Mode", wxHORIZONTAL);
	textureTestPanel->SetFont(CFG_DEFAULTFONT());
	textureTestPanel->SetPanelBorderWidths(0, 0, 0, 1);
	textureTestPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	textureTestPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	textureTestPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	textureTestPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* textureTestButtonBar = new FlatUIButtonBar(textureTestPanel);
	textureTestButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	textureTestButtonBar->AddButtonWithSVG(ID_TEXTURE_MODE_DECAL, "Decal", "decal", wxSize(16, 16), nullptr, "Switch to Decal texture mode");
	textureTestButtonBar->AddButtonWithSVG(ID_TEXTURE_MODE_MODULATE, "Modulate", "modulate", wxSize(16, 16), nullptr, "Switch to Modulate texture mode");
	textureTestButtonBar->AddButtonWithSVG(ID_TEXTURE_MODE_REPLACE, "Replace", "replace", wxSize(16, 16), nullptr, "Switch to Replace texture mode");
	textureTestButtonBar->AddButtonWithSVG(ID_TEXTURE_MODE_BLEND, "Blend", "blend", wxSize(16, 16), nullptr, "Switch to Blend texture mode");
	textureTestPanel->AddButtonBar(textureTestButtonBar, 0, wxEXPAND | wxALL, 5);
	page4->AddPanel(textureTestPanel);
	m_ribbon->AddPage(page4);

	// Create Editor page
	FlatUIPage* editorPage = new FlatUIPage(m_ribbon, "Editor");
	FlatUIPanel* editorPanel = new FlatUIPanel(editorPage, "Face Selection", wxHORIZONTAL);
	editorPanel->SetFont(CFG_DEFAULTFONT());
	editorPanel->SetPanelBorderWidths(0, 0, 0, 1);
	editorPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	editorPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	editorPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	editorPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* editorButtonBar = new FlatUIButtonBar(editorPanel);
	editorButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	editorButtonBar->AddToggleButtonWithSVG(ID_FACE_SELECTION_TOOL, "Face Selection", "select-face", wxSize(16, 16), false, "Select geometry faces - hover to highlight, click to select, right-click for menu");
	editorButtonBar->AddToggleButtonWithSVG(ID_EDGE_SELECTION_TOOL, "Edge Selection", "select-edge", wxSize(16, 16), false, "Select geometry edges - hover to highlight, click to select original edges");
	editorButtonBar->AddToggleButtonWithSVG(ID_VERTEX_SELECTION_TOOL, "Vertex Selection", "select-vertex", wxSize(16, 16), false, "Select geometry vertices - hover to highlight, click to select vertices");
	editorButtonBar->AddToggleButtonWithSVG(ID_FACE_QUERY_TOOL, "Face Query", "query-face", wxSize(16, 16), false, "Activate face query tool - left-click or middle-click on faces to view information");
	editorButtonBar->AddButtonWithSVG(ID_SELECTION_HIGHLIGHT_CONFIG, "Selection Highlight Config", "settings-highlight", wxSize(16, 16), nullptr, "Configure selection highlight colors and parameters");
	editorPanel->AddButtonBar(editorButtonBar, 0, wxEXPAND | wxALL, 5);
	editorPage->AddPanel(editorPanel);
	m_ribbon->AddPage(editorPage); 
	
	// Add Docking page
	FlatUIPage* dockingPage = new FlatUIPage(m_ribbon, "Docking");
	
	// Layout Configuration panel
	FlatUIPanel* layoutConfigPanel = new FlatUIPanel(dockingPage, "Layout Configuration", wxHORIZONTAL);
	layoutConfigPanel->SetFont(CFG_DEFAULTFONT());
	layoutConfigPanel->SetPanelBorderWidths(0, 0, 0, 1);
	layoutConfigPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	layoutConfigPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	layoutConfigPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	layoutConfigPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* layoutConfigBar = new FlatUIButtonBar(layoutConfigPanel);
	layoutConfigBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	layoutConfigBar->AddButtonWithSVG(ID_DOCK_LAYOUT_CONFIG, "Configure Layout", "settings", wxSize(16, 16), nullptr, "Configure dock panel sizes and layout");
	layoutConfigPanel->AddButtonBar(layoutConfigBar, 0, wxEXPAND | wxALL, 5);
	dockingPage->AddPanel(layoutConfigPanel);
	
	// Layout Management panel
	FlatUIPanel* layoutMgmtPanel = new FlatUIPanel(dockingPage, "Layout Management", wxHORIZONTAL);
	layoutMgmtPanel->SetFont(CFG_DEFAULTFONT());
	layoutMgmtPanel->SetPanelBorderWidths(0, 0, 0, 1);
	layoutMgmtPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	layoutMgmtPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	layoutMgmtPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	layoutMgmtPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* layoutMgmtBar = new FlatUIButtonBar(layoutMgmtPanel);
	layoutMgmtBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	layoutMgmtBar->AddButtonWithSVG(ID_DOCKING_SAVE_LAYOUT, "Save Layout", "save", wxSize(16, 16), nullptr, "Save current docking layout");
	layoutMgmtBar->AddButtonWithSVG(ID_DOCKING_LOAD_LAYOUT, "Load Layout", "open", wxSize(16, 16), nullptr, "Load saved docking layout");
	layoutMgmtBar->AddButtonWithSVG(ID_DOCKING_RESET_LAYOUT, "Reset Layout", "undo", wxSize(16, 16), nullptr, "Reset to default docking layout");
	layoutMgmtPanel->AddButtonBar(layoutMgmtBar, 0, wxEXPAND | wxALL, 5);
	dockingPage->AddPanel(layoutMgmtPanel);
	
	// Advanced Features panel
	FlatUIPanel* advancedPanel = new FlatUIPanel(dockingPage, "Advanced Features", wxHORIZONTAL);
	advancedPanel->SetFont(CFG_DEFAULTFONT());
	advancedPanel->SetPanelBorderWidths(0, 0, 0, 1);
	advancedPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	advancedPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	advancedPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	advancedPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* advancedBar = new FlatUIButtonBar(advancedPanel);
	advancedBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	advancedBar->AddButtonWithSVG(ID_DOCKING_MANAGE_PERSPECTIVES, "Perspectives", "layers", wxSize(16, 16), nullptr, "Manage saved layout perspectives");
	advancedBar->AddToggleButtonWithSVG(ID_DOCKING_TOGGLE_AUTOHIDE, "Auto-hide", "pin", wxSize(16, 16), false, "Toggle auto-hide for current panel");
	advancedPanel->AddButtonBar(advancedBar, 0, wxEXPAND | wxALL, 5);
	dockingPage->AddPanel(advancedPanel);
	
	// Panel Visibility panel
	FlatUIPanel* visibilityPanel = new FlatUIPanel(dockingPage, "Panel Visibility", wxHORIZONTAL);
	visibilityPanel->SetFont(CFG_DEFAULTFONT());
	visibilityPanel->SetPanelBorderWidths(0, 0, 0, 1);
	visibilityPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
	visibilityPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
	visibilityPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
	visibilityPanel->SetHeaderBorderWidths(0, 0, 0, 0);
	FlatUIButtonBar* visibilityBar = new FlatUIButtonBar(visibilityPanel);
	visibilityBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
	visibilityBar->AddToggleButton(ID_VIEW_OBJECT_TREE, "Object Tree", true, SVG_ICON("tree", wxSize(16, 16)), "Show/hide object tree panel");
	visibilityBar->AddToggleButton(ID_VIEW_PROPERTIES, "Properties", true, SVG_ICON("properties", wxSize(16, 16)), "Show/hide properties panel");
	visibilityBar->AddToggleButton(ID_VIEW_MESSAGE, "Message", true, SVG_ICON("message", wxSize(16, 16)), "Show/hide message output panel");
	visibilityBar->AddToggleButton(ID_VIEW_PERFORMANCE, "Performance", true, SVG_ICON("chart", wxSize(16, 16)), "Show/hide performance monitor panel");
	visibilityPanel->AddButtonBar(visibilityBar, 0, wxEXPAND | wxALL, 5);
	dockingPage->AddPanel(visibilityPanel);
	
	m_ribbon->AddPage(dockingPage);

	createPanels();
	// Setup command system after Canvas is created
	setupCommandSystem();

	SetClientSize(size);
	Layout();

	int panelTargetHeight = CFG_INT("PanelTargetHeight");
	if (panelTargetHeight <= 0) {
		panelTargetHeight = 80;
	}
	int ribbonMinHeight = FlatUIBar::GetBarHeight() + panelTargetHeight + 10;
	m_ribbon->SetMinSize(wxSize(-1, ribbonMinHeight));

	Layout();
}

void FlatFrame::createPanels() {
	wxBoxSizer* mainSizer = GetMainSizer();
	// Add ribbon with border margin to prevent covering the frame border
	mainSizer->Add(m_ribbon, 0, wxEXPAND | wxLEFT | wxTOP | wxRIGHT, 2);

	LOG_INF_S("Creating panels...");
	if (m_mainSplitter) { m_mainSplitter->Destroy(); m_mainSplitter = nullptr; }
	if (m_leftSplitter) { m_leftSplitter->Destroy(); m_leftSplitter = nullptr; }

	// Skip ModernDockAdapter creation if using new docking system
	if (IsUsingDockingSystem()) {
		LOG_INF_S("Using new docking system, skipping ModernDockAdapter");
		SetSizer(mainSizer);
		Layout();
		return;
	}

	// Use ModernDockAdapter to provide VS2022-style docking while maintaining compatibility
	auto* dock = new ModernDockAdapter(this);
	// Add dock with border margin to prevent covering the frame border
	mainSizer->Add(dock, 1, wxEXPAND | wxLEFT | wxBOTTOM | wxRIGHT, 2);

	m_objectTreePanel = new ObjectTreePanel(dock);
	m_objectTreePanel->SetName("Works");
	m_propertyPanel = new PropertyPanel(dock);
	m_propertyPanel->SetName("Properties");
	m_canvas = new Canvas(dock);
	m_canvas->SetName("Canvas");

	// Place panes: left top tree, left bottom properties, center canvas
	dock->AddPane(m_objectTreePanel, ModernDockAdapter::DockPos::LeftTop, 200);
	dock->AddPane(m_propertyPanel, ModernDockAdapter::DockPos::LeftBottom);
	dock->AddPane(m_canvas, ModernDockAdapter::DockPos::Center);

	// Message/Performance area
	// Message/Performance area as bottom dock
	// Bottom message container as a simple tab: Message + Performance tabs
	wxPanel* messagePage = new wxPanel(dock);
	wxBoxSizer* msgSizer = new wxBoxSizer(wxVERTICAL);
	auto* messageText = new wxTextCtrl(messagePage, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
		wxTE_READONLY | wxTE_MULTILINE | wxBORDER_NONE);
	msgSizer->Add(messageText, 1, wxEXPAND | wxALL, 2);
	messagePage->SetSizer(msgSizer);
	m_messageOutput = messageText;

	m_performancePanel = new PerformancePanel(dock);
	m_performancePanel->SetMinSize(wxSize(360, 140));

	// A container panel to host both pages will be created by dock manager; we just pass pages as panes
	messagePage->SetName("Message");
	m_performancePanel->SetName("Performance");
	dock->AddPane(messagePage, ModernDockAdapter::DockPos::Bottom, 160);
	dock->AddPane(m_performancePanel, ModernDockAdapter::DockPos::Bottom);

	SetSizer(mainSizer);
	Layout();

	m_progressTimer.SetOwner(this);
	Bind(wxEVT_TIMER, [this](wxTimerEvent&) {
		bool running = m_occViewer && m_occViewer->isFeatureEdgeGenerationRunning();
		bool justFinished = (!running && m_prevFeatureEdgesRunning);
		if (running) {
			int p = m_occViewer->getFeatureEdgeProgress();
			wxLogDebug("Progress timer: Feature edge generation running, progress: %d%%", p);
			if (auto* bar = GetFlatUIStatusBar()) {
				wxLogDebug("Progress timer: Got status bar, enabling progress gauge");
				bar->EnableProgressGauge(true);
				bar->SetGaugeRange(100);
				bar->SetGaugeValue(std::max(0, std::min(100, p)));
			}
			else {
				wxLogDebug("Progress timer: Failed to get status bar");
			}
			if (m_messageOutput) {
				wxString progressMsg = wxString::Format("Feature edge generation progress: %d%%", p);
				m_messageOutput->SetValue(progressMsg);
			}
			m_featureProgressHoldTicks = 4;
		}
		else {
			if (justFinished && m_messageOutput) {
				appendMessage("Feature edge generation completed.");
			}
			if (m_featureProgressHoldTicks > 0) {
				m_featureProgressHoldTicks--;
			}
			else {
				if (auto* bar = GetFlatUIStatusBar()) {
					bar->EnableProgressGauge(false);
				}
			}
		}
		m_prevFeatureEdgesRunning = running;
		});
	m_progressTimer.Start(50);

	if (m_messageOutput) {
		m_messageOutput->SetValue("Message output ready. Click 'Feature Edges' button to start parameter dialog.");
	}

	m_objectTreePanel->setPropertyPanel(m_propertyPanel);
	m_mouseHandler = new MouseHandler(m_canvas, m_objectTreePanel, m_propertyPanel, m_commandManager);
	m_canvas->getInputManager()->setMouseHandler(m_mouseHandler);
	
	// Create NavigationModeManager instead of direct NavigationController
	m_navigationModeManager = new NavigationModeManager(m_canvas, m_canvas->getSceneManager());
	m_mouseHandler->setNavigationModeManager(m_navigationModeManager);
	
	// Keep backward compatibility with direct NavigationController
	NavigationController* navController = m_navigationModeManager->getCurrentController();
	m_canvas->getInputManager()->setNavigationController(navController);
	m_mouseHandler->setNavigationController(navController);

	m_occViewer = new OCCViewer(m_canvas->getSceneManager());
	m_canvas->setOCCViewer(m_occViewer);
	m_canvas->getInputManager()->initializeStates();
	m_canvas->setObjectTreePanel(m_objectTreePanel);
	m_canvas->setCommandManager(m_commandManager);

	m_objectTreePanel->setOCCViewer(m_occViewer);
	m_geometryFactory = new GeometryFactory(
		m_canvas->getSceneManager()->getObjectRoot(),
		m_objectTreePanel,
		m_propertyPanel,
		m_commandManager,
		m_occViewer
	);
	
	// Initialize async compute engine (event handlers are bound inside AsyncEngineIntegration)
	m_asyncEngine = std::make_unique<async::AsyncEngineIntegration>(this);
	LOG_INF_S("Async compute engine initialized");

	if (m_canvas && m_canvas->getSceneManager()) {
		m_canvas->getSceneManager()->resetView();
		LOG_INF_S("Initial view set to isometric and fit to scene");
	}
	LOG_INF_S("Panels creation completed successfully");

	addStatusBar();
	if (auto* bar = GetFlatUIStatusBar()) {
		bar->SetFieldsCount(3);
		bar->SetStatusText("", 1);
		bar->EnableProgressGauge(false);
		bar->SetGaugeRange(100);
		bar->SetGaugeValue(0);
	}

	// Setup refresh listeners for UI state synchronization
	if (m_canvas && m_canvas->getRefreshManager()) {
		m_canvas->getRefreshManager()->addRefreshListener([this](ViewRefreshManager::RefreshReason reason) {
			this->OnPointViewToggled(reason);
		});
	}

	// Setup keyboard shortcuts
	SetupKeyboardShortcuts();
}

void FlatFrame::SetupKeyboardShortcuts()
{
	// Create accelerator entries
	wxAcceleratorEntry entries[] = {
		// Mesh quality dialog
		wxAcceleratorEntry(wxACCEL_CTRL, 'M', ID_MESH_QUALITY_DIALOG),
		
		// LOD controls
		wxAcceleratorEntry(wxACCEL_NORMAL, 'L', ID_TOGGLE_LOD),
		wxAcceleratorEntry(wxACCEL_SHIFT, 'L', ID_FORCE_ROUGH_LOD),
		wxAcceleratorEntry(wxACCEL_CTRL | wxACCEL_SHIFT, 'L', ID_FORCE_FINE_LOD),
		
		// Performance monitoring
		wxAcceleratorEntry(wxACCEL_NORMAL, WXK_F12, ID_TOGGLE_PERFORMANCE_MONITOR),
		
		// Quick presets (Alt + number)
		wxAcceleratorEntry(wxACCEL_ALT, '1', ID_PERFORMANCE_PRESET),
		wxAcceleratorEntry(wxACCEL_ALT, '2', ID_BALANCED_PRESET),
		wxAcceleratorEntry(wxACCEL_ALT, '3', ID_QUALITY_PRESET),
		
		// Existing shortcuts
		wxAcceleratorEntry(wxACCEL_CTRL, 'N', wxID_NEW),
		wxAcceleratorEntry(wxACCEL_CTRL, 'O', wxID_OPEN),
		wxAcceleratorEntry(wxACCEL_CTRL, 'S', wxID_SAVE),
		wxAcceleratorEntry(wxACCEL_CTRL, 'Z', ID_UNDO),
		wxAcceleratorEntry(wxACCEL_CTRL, 'Y', ID_REDO),
		wxAcceleratorEntry(wxACCEL_CTRL, 'H', ID_ShowUIHierarchy),

		// Navigation features shortcuts
		wxAcceleratorEntry(wxACCEL_CTRL, 'B', ID_VIEW_BOOKMARK_SAVE),     // Ctrl+B: Save bookmark
		wxAcceleratorEntry(wxACCEL_ALT, 'B', ID_VIEW_BOOKMARK_MANAGER),   // Alt+B: Bookmark manager
		wxAcceleratorEntry(wxACCEL_ALT, 'F', ID_VIEW_BOOKMARK_FRONT),     // Alt+F: Front view
		wxAcceleratorEntry(wxACCEL_ALT, 'R', ID_VIEW_BOOKMARK_RIGHT),     // Alt+R: Right view
		wxAcceleratorEntry(wxACCEL_ALT, 'T', ID_VIEW_BOOKMARK_TOP),       // Alt+T: Top view
		wxAcceleratorEntry(wxACCEL_ALT, 'I', ID_VIEW_BOOKMARK_ISOMETRIC), // Alt+I: Isometric view
		wxAcceleratorEntry(wxACCEL_ALT, 'L', ID_VIEW_BOOKMARK_LEFT),      // Alt+L: Left view
		wxAcceleratorEntry(wxACCEL_ALT, 'K', ID_VIEW_BOOKMARK_BACK),      // Alt+K: Back view
		wxAcceleratorEntry(wxACCEL_ALT, 'M', ID_VIEW_BOOKMARK_BOTTOM),    // Alt+M: Bottom view

		// Zoom shortcuts
		wxAcceleratorEntry(wxACCEL_CTRL, WXK_ADD, ID_ZOOM_IN),            // Ctrl++: Zoom in
		wxAcceleratorEntry(wxACCEL_CTRL, WXK_SUBTRACT, ID_ZOOM_OUT),      // Ctrl+-: Zoom out
		wxAcceleratorEntry(wxACCEL_CTRL, '0', ID_ZOOM_RESET),             // Ctrl+0: Reset zoom
		wxAcceleratorEntry(wxACCEL_ALT, 'Z', ID_ZOOM_SETTINGS),           // Alt+Z: Zoom settings

		// Animation shortcuts
		wxAcceleratorEntry(wxACCEL_ALT, '1', ID_ANIMATION_TYPE_LINEAR),   // Alt+1: Linear animation
		wxAcceleratorEntry(wxACCEL_ALT, '2', ID_ANIMATION_TYPE_SMOOTH),   // Alt+2: Smooth animation
		wxAcceleratorEntry(wxACCEL_ALT, '3', ID_ANIMATION_TYPE_EASE_IN),  // Alt+3: Ease-in animation
		wxAcceleratorEntry(wxACCEL_ALT, '4', ID_ANIMATION_TYPE_EASE_OUT), // Alt+4: Ease-out animation
		wxAcceleratorEntry(wxACCEL_ALT, '5', ID_ANIMATION_TYPE_BOUNCE)    // Alt+5: Bounce animation
	};
	
	wxAcceleratorTable accel(sizeof(entries) / sizeof(wxAcceleratorEntry), entries);
	SetAcceleratorTable(accel);
	
	LOG_INF_S("Keyboard shortcuts initialized");
}
