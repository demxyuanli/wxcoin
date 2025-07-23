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
#include "logger/Logger.h"
#include "STEPReader.h"
#include <wx/splitter.h>
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
#include "ShowEdgesListener.h"
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

#ifdef __WXMSW__
#define NOMINMAX
#include <windows.h>
#endif
// Note: Theme change events are now defined in FlatUIFrame

// These are now defined in FlatFrame.h as enum members.

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
    EVT_BUTTON(ID_FIX_NORMALS, FlatFrame::onCommand)
    EVT_BUTTON(ID_SET_TRANSPARENCY, FlatFrame::onCommand)
    EVT_BUTTON(ID_TOGGLE_WIREFRAME, FlatFrame::onCommand)
    EVT_BUTTON(ID_TOGGLE_SHADING, FlatFrame::onCommand)
    EVT_BUTTON(ID_TOGGLE_EDGES, FlatFrame::onCommand)
    EVT_BUTTON(ID_SHOW_FACES, FlatFrame::onCommand)

    EVT_BUTTON(ID_UNDO, FlatFrame::onCommand)
    EVT_BUTTON(ID_REDO, FlatFrame::onCommand)
    EVT_BUTTON(ID_NAVIGATION_CUBE_CONFIG, FlatFrame::onCommand)
    EVT_BUTTON(ID_ZOOM_SPEED, FlatFrame::onCommand)
    EVT_BUTTON(ID_MESH_QUALITY_DIALOG, FlatFrame::onCommand)
    EVT_BUTTON(ID_RENDERING_SETTINGS, FlatFrame::onCommand)
    EVT_BUTTON(ID_LIGHTING_SETTINGS, FlatFrame::onCommand)
    EVT_BUTTON(ID_EDGE_SETTINGS, FlatFrame::onCommand)
    EVT_BUTTON(wxID_ABOUT, FlatFrame::onCommand)
    EVT_BUTTON(ID_VIEW_SHOWEDGES, FlatFrame::onCommand)
    
    // Texture Mode Command Events
    EVT_BUTTON(ID_TEXTURE_MODE_DECAL, FlatFrame::onCommand)
    EVT_BUTTON(ID_TEXTURE_MODE_MODULATE, FlatFrame::onCommand)
    EVT_BUTTON(ID_TEXTURE_MODE_REPLACE, FlatFrame::onCommand)
    EVT_BUTTON(ID_TEXTURE_MODE_BLEND, FlatFrame::onCommand)
    EVT_BUTTON(ID_TOGGLE_COORDINATE_SYSTEM, FlatFrame::onCommand)
    
    
    EVT_CLOSE(FlatFrame::onClose)
    EVT_ACTIVATE(FlatFrame::onActivate)
    EVT_SIZE(FlatFrame::onSize)
wxEND_EVENT_TABLE()

// Fix the kEventTable to use only valid command types
static const std::unordered_map<int, cmd::CommandType> kEventTable = {
    {wxID_NEW, cmd::CommandType::FileNew},
    {wxID_OPEN, cmd::CommandType::FileOpen},
    {wxID_SAVE, cmd::CommandType::FileSave},
    {ID_SAVE_AS, cmd::CommandType::FileSaveAs},
    {ID_IMPORT_STEP, cmd::CommandType::ImportSTEP},
    {wxID_EXIT, cmd::CommandType::FileExit},
    {ID_CREATE_BOX, cmd::CommandType::CreateBox},
    {ID_CREATE_SPHERE, cmd::CommandType::CreateSphere},
    {ID_CREATE_CYLINDER, cmd::CommandType::CreateCylinder},
    {ID_CREATE_CONE, cmd::CommandType::CreateCone},
    {ID_CREATE_TORUS, cmd::CommandType::CreateTorus},
    {ID_CREATE_TRUNCATED_CYLINDER, cmd::CommandType::CreateTruncatedCylinder},
    {ID_CREATE_WRENCH, cmd::CommandType::CreateWrench},
    {ID_VIEW_ALL, cmd::CommandType::ViewAll},
    {ID_VIEW_TOP, cmd::CommandType::ViewTop},
    {ID_VIEW_FRONT, cmd::CommandType::ViewFront},
    {ID_VIEW_RIGHT, cmd::CommandType::ViewRight},
    {ID_VIEW_ISOMETRIC, cmd::CommandType::ViewIsometric},
    {ID_SHOW_NORMALS, cmd::CommandType::ShowNormals},
    {ID_FIX_NORMALS, cmd::CommandType::FixNormals},
    {ID_SET_TRANSPARENCY, cmd::CommandType::SetTransparency},
    {ID_TOGGLE_WIREFRAME, cmd::CommandType::ToggleWireframe},
    {ID_TOGGLE_SHADING, cmd::CommandType::ToggleShading},
    {ID_TOGGLE_EDGES, cmd::CommandType::ToggleEdges},
    {ID_SHOW_FACES, cmd::CommandType::ShowFaces},

    {ID_VIEW_SHOWEDGES, cmd::CommandType::ShowEdges},
    {ID_TEXTURE_MODE_DECAL, cmd::CommandType::TextureModeDecal},
    {ID_TEXTURE_MODE_MODULATE, cmd::CommandType::TextureModeModulate},
    {ID_TEXTURE_MODE_REPLACE, cmd::CommandType::TextureModeReplace},
    {ID_TEXTURE_MODE_BLEND, cmd::CommandType::TextureModeBlend},
    {ID_TOGGLE_COORDINATE_SYSTEM, cmd::CommandType::ToggleCoordinateSystem},
    {ID_UNDO, cmd::CommandType::Undo},
    {ID_REDO, cmd::CommandType::Redo},
    {ID_NAVIGATION_CUBE_CONFIG, cmd::CommandType::NavCubeConfig},
    {ID_ZOOM_SPEED, cmd::CommandType::ZoomSpeed},
    {ID_MESH_QUALITY_DIALOG, cmd::CommandType::MeshQualityDialog},
    {ID_RENDERING_SETTINGS, cmd::CommandType::RenderingSettings},
    {ID_EDGE_SETTINGS, cmd::CommandType::EdgeSettings},
    {ID_LIGHTING_SETTINGS, cmd::CommandType::LightingSettings},
    {wxID_ABOUT, cmd::CommandType::HelpAbout}
};

FlatFrame::FlatFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
    : FlatUIFrame(NULL, wxID_ANY, title, pos, size, wxBORDER_NONE),
    m_ribbon(nullptr),
    m_messageOutput(nullptr),
    m_searchCtrl(nullptr),
    m_homeMenu(nullptr),
    m_searchPanel(nullptr),
    m_profilePanel(nullptr),
    m_canvas(nullptr),
    m_propertyPanel(nullptr),
    m_objectTreePanel(nullptr),
    m_mouseHandler(nullptr),
    m_geometryFactory(nullptr),
    m_occViewer(nullptr),
    m_isFirstActivate(true),
    m_mainSplitter(nullptr),
    m_leftSplitter(nullptr),
    m_statusBar(nullptr),
    m_commandManager(new CommandManager())
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

    // Startup timer (could be base, but often specific UI needs to be ready)
    wxTimer* startupTimer = new wxTimer(this);
    this->Bind(wxEVT_TIMER, &FlatFrame::OnStartupTimer, this);
    startupTimer->StartOnce(100); // Reduced for faster startup if UI is complex
}

FlatFrame::~FlatFrame()
{
    // m_homeMenu is a child window, wxWidgets handles its deletion.
    // Other child controls (m_ribbon, m_messageOutput, m_searchCtrl) are also managed by wxWidgets.
    LOG_DBG("FlatFrame destruction started.", "FlatFrame");
    
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
    m_ribbon->SetHomeButtonWidth(30);

    FlatUIHomeSpace* homeSpace = m_ribbon->GetHomeSpace();
    if (homeSpace) {
        m_homeMenu = new FlatUIHomeMenu(homeSpace, this);
        m_homeMenu->AddMenuItem("&New Project...\tCtrl-N", ID_Menu_NewProject_MainFrame);
        m_homeMenu->AddSeparator();
        m_homeMenu->AddMenuItem("Show UI &Hierarchy\tCtrl-H", ID_ShowUIHierarchy);
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
    searchPanel->SetBackgroundColour(CFG_COLOUR("BarBgColour")); // Use consistent theme background
    wxBoxSizer* searchSizer = new wxBoxSizer(wxHORIZONTAL);
    m_searchCtrl = new wxSearchCtrl(searchPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(240, -1), wxTE_PROCESS_ENTER);
    m_searchCtrl->SetFont(defaultFont);
    m_searchCtrl->SetBackgroundColour(CFG_COLOUR("SearchCtrlBgColour"));
    m_searchCtrl->SetForegroundColour(CFG_COLOUR("SearchCtrlFgColour"));
    m_searchCtrl->ShowSearchButton(true);
    m_searchCtrl->ShowCancelButton(true);
    wxBitmapButton* searchButton = new wxBitmapButton(searchPanel, ID_SearchExecute, SVG_ICON("search", wxSize(16, 16)));
    searchButton->SetBackgroundColour(CFG_COLOUR("BarBgColour")); // Set consistent background
    searchSizer->Add(m_searchCtrl, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
    searchSizer->Add(searchButton, 0, wxALIGN_CENTER_VERTICAL);
    searchPanel->SetSizer(searchSizer);
    searchPanel->SetFont(defaultFont);
    m_ribbon->SetFunctionSpaceControl(searchPanel, 270);

    wxPanel* profilePanel = new wxPanel(m_ribbon);
    profilePanel->SetBackgroundColour(CFG_COLOUR("BarBgColour")); // Set theme background color
    wxBoxSizer* profileSizer = new wxBoxSizer(wxHORIZONTAL);
    wxBitmapButton* userButton = new wxBitmapButton(profilePanel, ID_UserProfile, SVG_ICON("user",wxSize(16, 16)));
    userButton->SetToolTip("User Profile");
    userButton->SetBackgroundColour(CFG_COLOUR("BarBgColour")); // Set consistent background
    wxBitmapButton* settingsButton = new wxBitmapButton(profilePanel, wxID_PREFERENCES, SVG_ICON("settings", wxSize(16, 16)));
    settingsButton->SetToolTip("Settings");
    settingsButton->SetBackgroundColour(CFG_COLOUR("BarBgColour")); // Set consistent background 
    profileSizer->Add(userButton, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    profileSizer->Add(settingsButton, 0, wxALIGN_CENTER_VERTICAL);
    profilePanel->SetSizer(profileSizer);
    m_ribbon->SetProfileSpaceControl(profilePanel, 60);

    m_ribbon->AddSpaceSeparator(FlatUIBar::SPACER_FUNCTION_PROFILE, 30, false, true, true);

    // Store reference to search panel
    m_searchPanel = searchPanel;
    // Store reference to profile panel  
    m_profilePanel = profilePanel;

    FlatUIPage* page1 = new FlatUIPage(m_ribbon, "Project");
    
    // File Operations Panel
    FlatUIPanel* filePanel = new FlatUIPanel(page1, "File", wxHORIZONTAL);
    filePanel->SetFont(CFG_DEFAULTFONT()); 
    filePanel->SetPanelBorderWidths(0, 0, 0, 1);
    filePanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
    filePanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
    filePanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
    filePanel->SetHeaderBorderWidths(0, 0, 0, 0);
    FlatUIButtonBar* fileButtonBar = new FlatUIButtonBar(filePanel);
    fileButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
    fileButtonBar->AddButton(wxID_NEW, "New", SVG_ICON("new", wxSize(16, 16)), nullptr, "Create a new project");
    fileButtonBar->AddButton(wxID_OPEN, "Open", SVG_ICON("open", wxSize(16, 16)), nullptr, "Open an existing project");
    fileButtonBar->AddButton(wxID_SAVE, "Save", SVG_ICON("save", wxSize(16, 16)), nullptr, "Save current project");
    fileButtonBar->AddButton(ID_SAVE_AS, "Save As", SVG_ICON("saveas", wxSize(16, 16)), nullptr, "Save project with a new name");
    fileButtonBar->AddButton(ID_IMPORT_STEP, "Import STEP", SVG_ICON("import", wxSize(16, 16)), nullptr, "Import STEP file");
    filePanel->AddButtonBar(fileButtonBar, 0, wxEXPAND | wxALL, 5);
    page1->AddPanel(filePanel);

    // Create Geometry Panel
    FlatUIPanel* createPanel = new FlatUIPanel(page1, "Create", wxHORIZONTAL);
    createPanel->SetFont(CFG_DEFAULTFONT());
    createPanel->SetPanelBorderWidths(0, 0, 0, 1);
    createPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
    createPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
    createPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
    createPanel->SetHeaderBorderWidths(0, 0, 0, 0);
    FlatUIButtonBar* createButtonBar = new FlatUIButtonBar(createPanel);
    createButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
    createButtonBar->AddButton(ID_CREATE_BOX, "Box", SVG_ICON("cube", wxSize(16, 16)), nullptr, "Create a box geometry");
    createButtonBar->AddButton(ID_CREATE_SPHERE, "Sphere", SVG_ICON("circle", wxSize(16, 16)), nullptr, "Create a sphere geometry");
    createButtonBar->AddButton(ID_CREATE_CYLINDER, "Cylinder", SVG_ICON("cylinder", wxSize(16, 16)), nullptr, "Create a cylinder geometry");
    createButtonBar->AddButton(ID_CREATE_CONE, "Cone", SVG_ICON("cone", wxSize(16, 16)), nullptr, "Create a cone geometry");
    createButtonBar->AddButton(ID_CREATE_TORUS, "Torus", SVG_ICON("circle", wxSize(16, 16)), nullptr, "Create a torus geometry");
    createButtonBar->AddButton(ID_CREATE_TRUNCATED_CYLINDER, "Truncated Cylinder", SVG_ICON("cylinder", wxSize(16, 16)), nullptr, "Create a truncated cylinder geometry");
    createButtonBar->AddButton(ID_CREATE_WRENCH, "Wrench", SVG_ICON("wrench", wxSize(16, 16)), nullptr, "Create a wrench geometry");
    createPanel->AddButtonBar(createButtonBar, 0, wxEXPAND | wxALL, 5);
    page1->AddPanel(createPanel);
    m_ribbon->AddPage(page1);

    // Edit Page
    FlatUIPage* page2 = new FlatUIPage(m_ribbon, "Edit");
    FlatUIPanel* editPanel = new FlatUIPanel(page2, "Edit", wxHORIZONTAL);
    editPanel->SetFont(CFG_DEFAULTFONT());
    editPanel->SetPanelBorderWidths(0, 0, 0, 1);
    editPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
    editPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
    editPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
    editPanel->SetHeaderBorderWidths(0, 0, 0, 0);
    FlatUIButtonBar* editButtonBar = new FlatUIButtonBar(editPanel);
    editButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);

    editButtonBar->AddButton(ID_UNDO, "Undo", SVG_ICON("undo", wxSize(16, 16)), nullptr, "Undo last operation");
    editButtonBar->AddButton(ID_REDO, "Redo", SVG_ICON("redo", wxSize(16, 16)), nullptr, "Redo last undone operation");
    editPanel->AddButtonBar(editButtonBar, 0, wxEXPAND | wxALL, 5);
    page2->AddPanel(editPanel);
    m_ribbon->AddPage(page2);

    // View Page  
    FlatUIPage* page3 = new FlatUIPage(m_ribbon, "View");
    
    // View Controls Panel
    FlatUIPanel* viewPanel = new FlatUIPanel(page3, "Views", wxHORIZONTAL);
    viewPanel->SetFont(CFG_DEFAULTFONT());
    viewPanel->SetPanelBorderWidths(0, 0, 0, 1);
    viewPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
    viewPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
    viewPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
    viewPanel->SetHeaderBorderWidths(0, 0, 0, 0);
    FlatUIButtonBar* viewButtonBar = new FlatUIButtonBar(viewPanel);
    viewButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
    viewButtonBar->AddButton(ID_VIEW_ALL, "Fit All", SVG_ICON("fitview", wxSize(16, 16)), nullptr, "Fit all objects in view");
    viewButtonBar->AddButton(ID_VIEW_TOP, "Top", SVG_ICON("topview", wxSize(16, 16)), nullptr, "Switch to top view");
    viewButtonBar->AddButton(ID_VIEW_FRONT, "Front", SVG_ICON("frontview", wxSize(16, 16)), nullptr, "Switch to front view");
    viewButtonBar->AddButton(ID_VIEW_RIGHT, "Right", SVG_ICON("rightview", wxSize(16, 16)), nullptr, "Switch to right view");
    viewButtonBar->AddButton(ID_VIEW_ISOMETRIC, "Isometric", SVG_ICON("isoview", wxSize(16, 16)), nullptr, "Switch to isometric view");
    viewPanel->AddButtonBar(viewButtonBar, 0, wxEXPAND | wxALL, 5);
    page3->AddPanel(viewPanel);

    // Display Options Panel
    FlatUIPanel* displayPanel = new FlatUIPanel(page3, "Display", wxHORIZONTAL); 
    displayPanel->SetFont(CFG_DEFAULTFONT());
    displayPanel->SetPanelBorderWidths(0, 0, 0, 1);
    displayPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
    displayPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
    displayPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
    displayPanel->SetHeaderBorderWidths(0, 0, 0, 0);
    FlatUIButtonBar* displayButtonBar = new FlatUIButtonBar(displayPanel);
    displayButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
    displayButtonBar->AddToggleButton(ID_VIEW_SHOWEDGES, "Toggle Edges", false, SVG_ICON("edges", wxSize(16, 16)), "Toggle edge display");
    displayButtonBar->AddToggleButton(ID_TOGGLE_WIREFRAME, "Toggle Wireframe", false, SVG_ICON("triangle", wxSize(16, 16)), "Toggle wireframe display mode");
    displayButtonBar->AddToggleButton(ID_TOGGLE_SHADING, "Toggle Shading", false, SVG_ICON("circle", wxSize(16, 16)), "Toggle shading display mode");
    displayButtonBar->AddToggleButton(ID_SHOW_FACES, "Show Faces", true, SVG_ICON("faces", wxSize(16, 16)), "Toggle face/solid display");
    displayButtonBar->AddToggleButton(ID_SHOW_NORMALS, "Show Normals", false, SVG_ICON("normals", wxSize(16, 16)), "Toggle normal vectors display");
    displayButtonBar->AddButton(ID_FIX_NORMALS, "Fix Normals", SVG_ICON("fixnormals", wxSize(16, 16)), nullptr, "Fix normal vectors orientation");
    displayButtonBar->AddButton(ID_SET_TRANSPARENCY, "Set Transparency", SVG_ICON("transparency", wxSize(16, 16)), nullptr, "Set object transparency");
    displayButtonBar->AddToggleButton(ID_TOGGLE_COORDINATE_SYSTEM, "Toggle Coordinate System", false, SVG_ICON("grid", wxSize(16, 16)), "Toggle coordinate system display");
    displayPanel->AddButtonBar(displayButtonBar, 0, wxEXPAND | wxALL, 5);
    page3->AddPanel(displayPanel);
    m_ribbon->AddPage(page3);

    // Tools Page
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
    toolsButtonBar->AddButton(ID_MESH_QUALITY_DIALOG, "Mesh Quality", SVG_ICON("mesh", wxSize(16, 16)), nullptr, "Open mesh quality dialog");
    toolsButtonBar->AddButton(ID_NAVIGATION_CUBE_CONFIG, "Nav Cube", SVG_ICON("cube", wxSize(16, 16)), nullptr, "Configure navigation cube");
    toolsButtonBar->AddButton(ID_ZOOM_SPEED, "Zoom Speed", SVG_ICON("pulse", wxSize(16, 16)), nullptr, "Adjust zoom speed settings");
    toolsButtonBar->AddButton(ID_RENDERING_SETTINGS, "Rendering Settings", SVG_ICON("palette", wxSize(16, 16)), nullptr, "Configure material, lighting and texture settings");
    toolsButtonBar->AddButton(ID_LIGHTING_SETTINGS, "Lighting Settings", SVG_ICON("light", wxSize(16, 16)), nullptr, "Configure scene lighting and environment settings");
    toolsButtonBar->AddButton(ID_EDGE_SETTINGS, "Edge Settings", SVG_ICON("edges", wxSize(16, 16)), nullptr, "Configure edge color, width and style settings");
    toolsPanel->AddButtonBar(toolsButtonBar, 0, wxEXPAND | wxALL, 5);
    page4->AddPanel(toolsPanel);
    
    // Texture Mode Test Panel
    FlatUIPanel* textureTestPanel = new FlatUIPanel(page4, "Texture Mode", wxHORIZONTAL);
    textureTestPanel->SetFont(CFG_DEFAULTFONT());
    textureTestPanel->SetPanelBorderWidths(0, 0, 0, 1);
    textureTestPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
    textureTestPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
    textureTestPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
    textureTestPanel->SetHeaderBorderWidths(0, 0, 0, 0);
    FlatUIButtonBar* textureTestButtonBar = new FlatUIButtonBar(textureTestPanel);
    textureTestButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
    textureTestButtonBar->AddButton(ID_TEXTURE_MODE_DECAL, "Decal", SVG_ICON("decal", wxSize(16, 16)), nullptr, "Switch to Decal texture mode");
    textureTestButtonBar->AddButton(ID_TEXTURE_MODE_MODULATE, "Modulate", SVG_ICON("modulate", wxSize(16, 16)), nullptr, "Switch to Modulate texture mode");
    textureTestButtonBar->AddButton(ID_TEXTURE_MODE_REPLACE, "Replace", SVG_ICON("replace", wxSize(16, 16)), nullptr, "Switch to Replace texture mode");
    textureTestButtonBar->AddButton(ID_TEXTURE_MODE_BLEND, "Blend", SVG_ICON("blend", wxSize(16, 16)), nullptr, "Switch to Blend texture mode");
    textureTestPanel->AddButtonBar(textureTestButtonBar, 0, wxEXPAND | wxALL, 5);
    page4->AddPanel(textureTestPanel);
    m_ribbon->AddPage(page4);

    // Help Page
    FlatUIPage* page5 = new FlatUIPage(m_ribbon, "Help");
    FlatUIPanel* helpPanel = new FlatUIPanel(page5, "Help", wxHORIZONTAL);
    helpPanel->SetFont(CFG_DEFAULTFONT());
    helpPanel->SetPanelBorderWidths(0, 0, 0, 1);
    helpPanel->SetHeaderStyle(PanelHeaderStyle::BOTTOM_CENTERED);
    helpPanel->SetHeaderColour(CFG_COLOUR("PanelHeaderColour"));
    helpPanel->SetHeaderTextColour(CFG_COLOUR("PanelHeaderTextColour"));
    helpPanel->SetHeaderBorderWidths(0, 0, 0, 0);
    FlatUIButtonBar* helpButtonBar = new FlatUIButtonBar(helpPanel);
    helpButtonBar->SetDisplayStyle(ButtonDisplayStyle::ICON_ONLY);
    helpButtonBar->AddButton(wxID_ABOUT, "About", SVG_ICON("about", wxSize(16, 16)), nullptr, "Show application information");
    helpButtonBar->AddButton(ID_ShowUIHierarchy, "UI Debug", SVG_ICON("tree", wxSize(16, 16)), nullptr, "Show UI hierarchy debugger");
    // Add separator between icon buttons and text buttons
    helpButtonBar->AddSeparator();
    // Add toggle buttons to the same button bar
    helpButtonBar->AddToggleButton(ID_ToggleFunctionSpace, "ToggleFunc", true, SVG_ICON("find", wxSize(16, 16)),  "Toggle function space visibility");
    helpButtonBar->AddToggleButton(ID_ToggleProfileSpace, "ToggleProf", true, SVG_ICON("user", wxSize(16, 16)),  "Toggle profile space visibility");
    helpPanel->AddButtonBar(helpButtonBar, 0, wxEXPAND | wxALL, 5);
    
    page5->AddPanel(helpPanel); 
    m_ribbon->AddPage(page5);

    // Create main layout with vertical sizer
    // The layout will be: Ribbon at top, splitter in middle, status bar at bottom
    // The splitter will be created in createPanels() method

    createPanels(); // Create canvas and panels with proper layout

    setupCommandSystem();

    // Create status bar manually and add to main sizer
    // m_statusBar = new FlatUIStatusBar(this);
    // m_statusBar->SetStatusText("Ready - Command system initialized", 0);
    
    // Add status bar to main sizer
    // if (GetSizer()) {
    //     GetSizer()->Add(m_statusBar, 0, wxEXPAND | wxALL, 0);
    // }

    SetClientSize(size); // Default size
    Layout();

    int panelTargetHeight = CFG_INT("PanelTargetHeight");
    if (panelTargetHeight <= 0) {
        panelTargetHeight = 80; // Default value if not configured
    }
    int ribbonMinHeight = FlatUIBar::GetBarHeight() + panelTargetHeight + 10; 
    m_ribbon->SetMinSize(wxSize(-1, ribbonMinHeight));

    Layout();
}

// Complete createPanels method
void FlatFrame::createPanels() {

    wxBoxSizer* mainSizer = GetMainSizer();
    mainSizer->Add(m_ribbon, 0, wxEXPAND | wxALL, 1);
    
    LOG_INF_S("Creating panels...");
    if (m_mainSplitter) { m_mainSplitter->Destroy(); m_mainSplitter = nullptr; }
    if (m_leftSplitter) { m_leftSplitter->Destroy(); m_leftSplitter = nullptr; }

    // Create main horizontal splitter (left panels vs right canvas)
    m_mainSplitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
    m_mainSplitter->SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_mainSplitter->SetBackgroundColour(CFG_COLOUR("PanelBgColour"));
    m_mainSplitter->SetDoubleBuffered(true);
    m_mainSplitter->SetSashGravity(0.0); // Fixed position for left side
    m_mainSplitter->SetMinimumPaneSize(200); // Left side minimum width

    // Create left vertical splitter (object tree vs property panel)
    m_leftSplitter = new wxSplitterWindow(m_mainSplitter, wxID_ANY);
    m_leftSplitter->SetBackgroundStyle(wxBG_STYLE_PAINT);
    m_leftSplitter->SetBackgroundColour(CFG_COLOUR("PanelBgColour"));
    m_leftSplitter->SetDoubleBuffered(true);
    m_leftSplitter->SetSashGravity(0.0); // Fixed position for property panel

    m_leftSplitter->SetMinimumPaneSize(200); // Property panel minimum height

    // Create object tree panel (top of left side)
    m_objectTreePanel = new ObjectTreePanel(m_leftSplitter);
    
    // Create property panel (bottom of left side)
    m_propertyPanel = new PropertyPanel(m_leftSplitter);
    
    // Split left side horizontally: object tree on top, property panel on bottom
    m_leftSplitter->SplitHorizontally(m_objectTreePanel, m_propertyPanel);

    // Create canvas (right side)
    m_canvas = new Canvas(m_mainSplitter);
    
    // Split main window vertically: left panels vs right canvas
    m_mainSplitter->SplitVertically(m_leftSplitter, m_canvas);
    m_mainSplitter->SetSashPosition(200);
    // Create main vertical sizer: ribbon at top, splitter in middle
    mainSizer->Add(m_mainSplitter, 1, wxEXPAND | wxALL, 2);
    SetSizer(mainSizer);
    Layout();

    // Initial split positions will be set in onSize event when window is first shown

    // Setup connections between panels
    m_objectTreePanel->setPropertyPanel(m_propertyPanel);
    
    // Initialize mouse handler and navigation
    m_mouseHandler = new MouseHandler(m_canvas, m_objectTreePanel, m_propertyPanel, m_commandManager);
    m_canvas->getInputManager()->setMouseHandler(m_mouseHandler);
    NavigationController* navController = new NavigationController(m_canvas, m_canvas->getSceneManager());
    m_canvas->getInputManager()->setNavigationController(navController);
    m_mouseHandler->setNavigationController(navController);
    
    // Setup OCC viewer and geometry factory
    m_occViewer = new OCCViewer(m_canvas->getSceneManager());
    m_canvas->setOCCViewer(m_occViewer);
    m_canvas->getInputManager()->initializeStates();
    m_canvas->setObjectTreePanel(m_objectTreePanel);
    m_canvas->setCommandManager(m_commandManager);
    
    // Set up bidirectional connections
    m_objectTreePanel->setOCCViewer(m_occViewer);
    m_geometryFactory = new GeometryFactory(
        m_canvas->getSceneManager()->getObjectRoot(),
        m_objectTreePanel,
        m_propertyPanel,
        m_commandManager,
        m_occViewer
    );
    
    // Set initial view
    if (m_canvas && m_canvas->getSceneManager()) {
        m_canvas->getSceneManager()->resetView();
        LOG_INF_S("Initial view set to isometric and fit to scene");
    }
    LOG_INF_S("Panels creation completed successfully");

    addStatusBar();
}

void FlatFrame::setupCommandSystem() {
    LOG_INF_S("Setting up command system"); 
    
    // Create command dispatcher
    m_commandDispatcher = std::make_unique<CommandDispatcher>();
    
    // Create command listeners with proper constructors
    auto createBoxListener = std::make_shared<CreateBoxListener>(m_mouseHandler);
    auto createSphereListener = std::make_shared<CreateSphereListener>(m_mouseHandler);
    auto createCylinderListener = std::make_shared<CreateCylinderListener>(m_mouseHandler);
    auto createConeListener = std::make_shared<CreateConeListener>(m_mouseHandler);
    auto createTorusListener = std::make_shared<CreateTorusListener>(m_mouseHandler);
    auto createTruncatedCylinderListener = std::make_shared<CreateTruncatedCylinderListener>(m_mouseHandler);
    auto createWrenchListener = std::make_shared<CreateWrenchListener>(m_mouseHandler, m_geometryFactory);
    
    // Register geometry command listeners
    m_listenerManager = std::make_unique<CommandListenerManager>();
    m_listenerManager->registerListener(cmd::CommandType::CreateBox, createBoxListener);
    m_listenerManager->registerListener(cmd::CommandType::CreateSphere, createSphereListener);
    m_listenerManager->registerListener(cmd::CommandType::CreateCylinder, createCylinderListener);
    m_listenerManager->registerListener(cmd::CommandType::CreateCone, createConeListener);
    m_listenerManager->registerListener(cmd::CommandType::CreateTorus, createTorusListener);
    m_listenerManager->registerListener(cmd::CommandType::CreateTruncatedCylinder, createTruncatedCylinderListener);
    m_listenerManager->registerListener(cmd::CommandType::CreateWrench, createWrenchListener);
    
    // View listeners
    auto viewAllListener = std::make_shared<ViewAllListener>(m_canvas->getInputManager()->getNavigationController());
    auto viewTopListener = std::make_shared<ViewTopListener>(m_canvas->getInputManager()->getNavigationController());
    auto viewFrontListener = std::make_shared<ViewFrontListener>(m_canvas->getInputManager()->getNavigationController());
    auto viewRightListener = std::make_shared<ViewRightListener>(m_canvas->getInputManager()->getNavigationController());
    auto viewIsoListener = std::make_shared<ViewIsometricListener>(m_canvas->getInputManager()->getNavigationController());
    auto showNormalsListener = std::make_shared<ShowNormalsListener>(m_occViewer);
    auto fixNormalsListener = std::make_shared<FixNormalsListener>(m_occViewer);
    auto showEdgesListener = std::make_shared<ShowEdgesListener>(m_occViewer);
    auto setTransparencyListener = std::make_shared<SetTransparencyListener>(this, m_occViewer);
    auto viewModeListener = std::make_shared<ViewModeListener>(m_occViewer);

    
    // Register view command listeners
    m_listenerManager->registerListener(cmd::CommandType::ViewAll, viewAllListener);
    m_listenerManager->registerListener(cmd::CommandType::ViewTop, viewTopListener);
    m_listenerManager->registerListener(cmd::CommandType::ViewFront, viewFrontListener);
    m_listenerManager->registerListener(cmd::CommandType::ViewRight, viewRightListener);
    m_listenerManager->registerListener(cmd::CommandType::ViewIsometric, viewIsoListener);
    m_listenerManager->registerListener(cmd::CommandType::ShowNormals, showNormalsListener);
    m_listenerManager->registerListener(cmd::CommandType::FixNormals, fixNormalsListener);
    m_listenerManager->registerListener(cmd::CommandType::ShowEdges, showEdgesListener);
    m_listenerManager->registerListener(cmd::CommandType::SetTransparency, setTransparencyListener);
    m_listenerManager->registerListener(cmd::CommandType::ToggleWireframe, viewModeListener);
    m_listenerManager->registerListener(cmd::CommandType::ToggleShading, viewModeListener);
    m_listenerManager->registerListener(cmd::CommandType::ToggleEdges, viewModeListener);
    
    // Register texture mode listeners
    auto textureModeDecalListener = std::make_shared<TextureModeDecalListener>(this, m_occViewer);
    m_listenerManager->registerListener(cmd::CommandType::TextureModeDecal, textureModeDecalListener);
    
    auto textureModeModulateListener = std::make_shared<TextureModeModulateListener>(this, m_occViewer);
    m_listenerManager->registerListener(cmd::CommandType::TextureModeModulate, textureModeModulateListener);
    
    auto textureModeReplaceListener = std::make_shared<TextureModeReplaceListener>(this, m_occViewer);
    m_listenerManager->registerListener(cmd::CommandType::TextureModeReplace, textureModeReplaceListener);
    
    auto textureModeBlendListener = std::make_shared<TextureModeBlendListener>(this, m_occViewer);
    m_listenerManager->registerListener(cmd::CommandType::TextureModeBlend, textureModeBlendListener);

    
    // Register file command listeners
    auto fileNewListener = std::make_shared<FileNewListener>(m_canvas, m_commandManager);
    auto fileOpenListener = std::make_shared<FileOpenListener>(this);
    auto fileSaveListener = std::make_shared<FileSaveListener>(this);
    auto fileSaveAsListener = std::make_shared<FileSaveAsListener>(this);
    auto importStepListener = std::make_shared<ImportStepListener>(this, m_canvas, m_occViewer);
    m_listenerManager->registerListener(cmd::CommandType::FileNew, fileNewListener);
    m_listenerManager->registerListener(cmd::CommandType::FileOpen, fileOpenListener);
    m_listenerManager->registerListener(cmd::CommandType::FileSave, fileSaveListener);
    m_listenerManager->registerListener(cmd::CommandType::FileSaveAs, fileSaveAsListener);
    m_listenerManager->registerListener(cmd::CommandType::ImportSTEP, importStepListener);
    
    auto undoListener = std::make_shared<UndoListener>(m_commandManager, m_canvas);
    auto redoListener = std::make_shared<RedoListener>(m_commandManager, m_canvas);
    auto helpAboutListener = std::make_shared<HelpAboutListener>(this);
    auto navCubeConfigListener = std::make_shared<NavCubeConfigListener>(m_canvas);
    auto zoomSpeedListener = std::make_shared<ZoomSpeedListener>(this, m_canvas);
    auto fileExitListener = std::make_shared<FileExitListener>(this);
    auto meshQualityDialogListener = std::make_shared<MeshQualityDialogListener>(this, m_occViewer);
    auto renderingSettingsListener = std::make_shared<RenderingSettingsListener>(m_occViewer, m_canvas->getRenderingEngine());
    auto edgeSettingsListener = std::make_shared<EdgeSettingsListener>(this, m_occViewer);
    auto lightingSettingsListener = std::make_shared<LightingSettingsListener>(this);
    auto coordinateSystemVisibilityListener = std::make_shared<CoordinateSystemVisibilityListener>(this, m_canvas->getSceneManager());
    
    m_listenerManager->registerListener(cmd::CommandType::Undo, undoListener);
    m_listenerManager->registerListener(cmd::CommandType::Redo, redoListener);
    m_listenerManager->registerListener(cmd::CommandType::HelpAbout, helpAboutListener);
    m_listenerManager->registerListener(cmd::CommandType::NavCubeConfig, navCubeConfigListener);
    m_listenerManager->registerListener(cmd::CommandType::ZoomSpeed, zoomSpeedListener);
    m_listenerManager->registerListener(cmd::CommandType::FileExit, fileExitListener);
    m_listenerManager->registerListener(cmd::CommandType::MeshQualityDialog, meshQualityDialogListener);
    m_listenerManager->registerListener(cmd::CommandType::RenderingSettings, renderingSettingsListener);
    m_listenerManager->registerListener(cmd::CommandType::EdgeSettings, edgeSettingsListener);
    m_listenerManager->registerListener(cmd::CommandType::LightingSettings, lightingSettingsListener);
    m_listenerManager->registerListener(cmd::CommandType::ToggleCoordinateSystem, coordinateSystemVisibilityListener);
    
    // Set UI feedback handler
    m_commandDispatcher->setUIFeedbackHandler(
        [this](const CommandResult& result) {
            this->onCommandFeedback(result);
        }
    );
    
    LOG_INF_S("Command system setup completed"); 
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

// Override OnLeftDown to prevent dragging if home menu is shown
void FlatFrame::OnLeftDown(wxMouseEvent& event)
{
    if (m_homeMenu && m_homeMenu->IsShown()) {
        event.Skip(); // Allow menu to handle event, prevent frame dragging/resizing
        return;
    }
    // Call base class implementation for actual dragging/resizing logic
    FlatUIFrame::OnLeftDown(event);
}

// Override OnMotion to prevent cursor changes if home menu is shown
void FlatFrame::OnMotion(wxMouseEvent& event)
{
    if (m_homeMenu && m_homeMenu->IsShown()) {
        SetCursor(wxCursor(wxCURSOR_ARROW)); // Ensure default cursor
        event.Skip();
        return;
    }
    // Call base class implementation for cursor updates and rubber banding
    FlatUIFrame::OnMotion(event);
}

// OnLeftUp can often use the base class implementation directly if no special conditions
// void FlatFrame::OnLeftUp(wxMouseEvent& event) { PlatUIFrame::OnLeftUp(event); }

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
    // Example: Forcing a refresh on the first page of the ribbon
    if (m_ribbon) {
        m_ribbon->Refresh();
        if (m_ribbon->GetPageCount() > 0) {
            FlatUIPage* page = m_ribbon->GetPage(0);
            if (page) {
                page->Show();
                page->Layout();
                page->Refresh();
                LOG_DBG_S("Force refreshed first page: " + page->GetLabel().ToStdString());
            }
        }
    }
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

// Note: OnGlobalPinStateChanged, OnThemeChanged, and RefreshAllUI methods
// have been moved to FlatUIFrame base class for reusability

void FlatFrame::OnThemeChanged(wxCommandEvent& event)
{
    // Add custom behavior: log theme change to status bar
    wxString themeName = event.GetString();
    SetStatusText("Theme changed to: " + themeName, 0);
    
    // Call base class implementation for actual theme change handling
    FlatUIFrame::OnThemeChanged(event);
    
    // Re-apply specific colors for FlatFrame controls
    if (m_searchPanel) {
        m_searchPanel->SetBackgroundColour(CFG_COLOUR("SearchPanelBgColour"));
    }
    
    if (m_searchCtrl) {
        m_searchCtrl->SetBackgroundColour(CFG_COLOUR("SearchCtrlBgColour"));
        m_searchCtrl->SetForegroundColour(CFG_COLOUR("SearchCtrlFgColour"));
    }
    
    // Re-apply ribbon colors
    if (m_ribbon) {
        m_ribbon->SetTabBorderColour(CFG_COLOUR("BarTabBorderColour"));
        m_ribbon->SetActiveTabBackgroundColour(CFG_COLOUR("BarActiveTabBgColour"));
        m_ribbon->SetActiveTabTextColour(CFG_COLOUR("BarActiveTextColour"));
        m_ribbon->SetInactiveTabTextColour(CFG_COLOUR("BarInactiveTextColour"));
        m_ribbon->SetTabBorderTopColour(CFG_COLOUR("BarTabBorderTopColour"));
        
        // Force ribbon to update its display
        m_ribbon->Refresh(true);
        m_ribbon->Update();
    }
    
    // Additional full refresh to ensure all changes take effect
    Refresh(true);
    Update();
}

void FlatFrame::onCommand(wxCommandEvent& event) {
    auto it = kEventTable.find(event.GetId());
    if (it == kEventTable.end()) { LOG_WRN_S("Unknown command ID: " + std::to_string(event.GetId())); return; }
    cmd::CommandType commandType = it->second;
    std::unordered_map<std::string, std::string> parameters;
    if (commandType == cmd::CommandType::ShowNormals || commandType == cmd::CommandType::ShowEdges) { parameters["toggle"] = "true"; }
    if (m_listenerManager && m_listenerManager->hasListener(commandType)) {
        CommandResult result = m_listenerManager->dispatch(commandType, parameters);
        onCommandFeedback(result);
    } else {
        SetStatusText("Error: No listener registered", 0);
        LOG_ERR_S("No listener registered for command");
    }
}

void FlatFrame::onCommandFeedback(const CommandResult& result) {
    if (result.success) {
        SetStatusText(result.message.empty() ? "Command executed successfully" : result.message, 0);
        LOG_INF_S("Command executed: " + result.commandId);
    } else {
        SetStatusText("Error: " + result.message, 0);
        LOG_ERR_S("Command failed: " + result.commandId + " - " + result.message);
        if (!result.message.empty() && result.commandId != "UNKNOWN") { wxMessageBox(result.message, "Command Error", wxOK | wxICON_ERROR, this); }
    }
    
    // Update UI state for toggle commands (since no menu bar in FlatFrame)
    if (result.commandId == cmd::to_string(cmd::CommandType::ShowNormals) && result.success && m_occViewer) {
        // Could update button states here if needed
        LOG_INF_S("Show normals state updated: " + std::string(m_occViewer->isShowNormals() ? "shown" : "hidden"));
    }
    else if (result.commandId == cmd::to_string(cmd::CommandType::ShowEdges) && result.success && m_occViewer) {
        // Could update button states here if needed
        LOG_INF_S("Show edges state updated: " + std::string(m_occViewer->isShowEdges() ? "shown" : "hidden"));
    }
    
    // Refresh canvas if needed - ensure all view and display commands trigger refresh
    if (m_canvas && (
            result.commandId.find("VIEW_") == 0 ||
            result.commandId.find("SHOW_") == 0 ||
            result.commandId == "FIX_NORMALS" ||
            result.commandId.find("CREATE_") == 0 ||
            result.commandId == "TOGGLE_COORDINATE_SYSTEM"
        )) {
        m_canvas->Refresh();
        LOG_INF_S("Canvas refreshed for command: " + result.commandId);
    }
}

void FlatFrame::onClose(wxCloseEvent& event) {
    LOG_INF_S("Closing application"); Destroy();
}

void FlatFrame::onActivate(wxActivateEvent& event) {
    if (event.GetActive() && m_isFirstActivate) {
        m_isFirstActivate = false;
        if (m_occViewer) {
            // Update UI state, perhaps check buttons instead of menu
        }
    }
    event.Skip();
}

void FlatFrame::onSize(wxSizeEvent& event) {
    // Call base class implementation first
    event.Skip();
    
    // Only set initial split positions once when the window is first shown
    static bool firstSize = true;
    if (firstSize && m_mainSplitter && m_mainSplitter->IsShown()) {
        firstSize = false;
        
        // Set main splitter position (fixed 160 pixels for left side)
        if (m_mainSplitter->GetSize().GetWidth() > 160) {
            m_mainSplitter->SetSashPosition(160);
        }
        
        // Set left splitter position (fixed 200 pixels for property panel)
        if (m_leftSplitter && m_leftSplitter->IsShown()) {
            int leftHeight = m_leftSplitter->GetSize().GetHeight();
            if (leftHeight > 200) {
                int treeHeight = leftHeight - 200; // Object tree gets remaining height
                m_leftSplitter->SetSashPosition(treeHeight);
            }
        }
    }
}


