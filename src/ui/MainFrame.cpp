#include "MainFrame.h"
#include "Canvas.h"
#include "PropertyPanel.h"
#include "ObjectTreePanel.h"
#include "MouseHandler.h"
#include "NavigationController.h"
#include "GeometryFactory.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "Command.h"
#include "Logger.h"
#include "STEPReader.h"
#include <wx/splitter.h>
#include <wx/artprov.h>
#include <wx/aboutdlg.h>
#include <wx/filedlg.h>
#include <wx/aui/aui.h>
#include <wx/toolbar.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <Inventor/SbVec3f.h>
#include "OCCViewer.h"
#include "CommandDispatcher.h"
#include "FileNewListener.h"
#include "FileOpenListener.h"
#include "FileSaveListener.h"
#include "ImportStepListener.h"
#include "CreateBoxListener.h"
#include "CreateSphereListener.h"
#include "CreateCylinderListener.h"
#include "CreateConeListener.h"
#include "CreateWrenchListener.h"
#include "ViewAllListener.h"
#include "ViewTopListener.h"
#include "ViewFrontListener.h"
#include "ViewRightListener.h"
#include "ViewIsometricListener.h"
#include "ShowNormalsListener.h"
#include "FixNormalsListener.h"
#include "ShowEdgesListener.h"
#include "UndoListener.h"
#include "RedoListener.h"
#include "HelpAboutListener.h"
#include "NavCubeConfigListener.h"
#include "ZoomSpeedListener.h"
#include "FileExitListener.h"
#include "CommandListenerManager.h"
#include "MeshQualityDialog.h"
#include "MeshQualityDialogListener.h"

static const std::unordered_map<int, cmd::CommandType> kEventTable = {
    {wxID_NEW, cmd::CommandType::FileNew},
    {wxID_OPEN, cmd::CommandType::FileOpen},
    {wxID_SAVE, cmd::CommandType::FileSave},
    {ID_IMPORT_STEP, cmd::CommandType::ImportSTEP},
    {wxID_EXIT, cmd::CommandType::FileExit},
    {ID_CREATE_BOX, cmd::CommandType::CreateBox},
    {ID_CREATE_SPHERE, cmd::CommandType::CreateSphere},
    {ID_CREATE_CYLINDER, cmd::CommandType::CreateCylinder},
    {ID_CREATE_CONE, cmd::CommandType::CreateCone},
    {ID_CREATE_WRENCH, cmd::CommandType::CreateWrench},
    {ID_VIEW_ALL, cmd::CommandType::ViewAll},
    {ID_VIEW_TOP, cmd::CommandType::ViewTop},
    {ID_VIEW_FRONT, cmd::CommandType::ViewFront},
    {ID_VIEW_RIGHT, cmd::CommandType::ViewRight},
    {ID_VIEW_ISOMETRIC, cmd::CommandType::ViewIsometric},
    {ID_SHOW_NORMALS, cmd::CommandType::ShowNormals},
    {ID_FIX_NORMALS, cmd::CommandType::FixNormals},
    {ID_VIEW_SHOWEDGES, cmd::CommandType::ShowEdges},
    {ID_UNDO, cmd::CommandType::Undo},
    {ID_REDO, cmd::CommandType::Redo},
    {ID_NAVIGATION_CUBE_CONFIG, cmd::CommandType::NavCubeConfig},
    {ID_ZOOM_SPEED, cmd::CommandType::ZoomSpeed},
    {ID_MESH_QUALITY_DIALOG, cmd::CommandType::MeshQualityDialog},
    {wxID_ABOUT, cmd::CommandType::HelpAbout},
};

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_MENU(wxID_NEW, MainFrame::onCommand)
EVT_MENU(wxID_OPEN, MainFrame::onCommand)
EVT_MENU(wxID_SAVE, MainFrame::onCommand)
EVT_MENU(ID_IMPORT_STEP, MainFrame::onCommand)
EVT_MENU(wxID_EXIT, MainFrame::onCommand)
EVT_MENU(ID_CREATE_BOX, MainFrame::onCommand)
EVT_MENU(ID_CREATE_SPHERE, MainFrame::onCommand)
EVT_MENU(ID_CREATE_CYLINDER, MainFrame::onCommand)
EVT_MENU(ID_CREATE_CONE, MainFrame::onCommand)
EVT_MENU(ID_CREATE_WRENCH, MainFrame::onCommand)
EVT_MENU(ID_VIEW_ALL, MainFrame::onCommand)
EVT_MENU(ID_VIEW_TOP, MainFrame::onCommand)
EVT_MENU(ID_VIEW_FRONT, MainFrame::onCommand)
EVT_MENU(ID_VIEW_RIGHT, MainFrame::onCommand)
EVT_MENU(ID_VIEW_ISOMETRIC, MainFrame::onCommand)
EVT_MENU(ID_SHOW_NORMALS, MainFrame::onCommand)
EVT_MENU(ID_FIX_NORMALS, MainFrame::onCommand)
EVT_MENU(ID_UNDO, MainFrame::onCommand)
EVT_MENU(ID_REDO, MainFrame::onCommand)
EVT_MENU(ID_NAVIGATION_CUBE_CONFIG, MainFrame::onCommand)
EVT_MENU(ID_ZOOM_SPEED, MainFrame::onCommand)
EVT_MENU(ID_MESH_QUALITY_DIALOG, MainFrame::onCommand)
EVT_MENU(wxID_ABOUT, MainFrame::onCommand)
EVT_CLOSE(MainFrame::onClose)
EVT_MENU(ID_VIEW_SHOWEDGES, MainFrame::onCommand)
EVT_ACTIVATE(MainFrame::onActivate)
END_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(1200, 800))
    , m_canvas(nullptr)
    , m_mouseHandler(nullptr)
    , m_geometryFactory(nullptr)
    , m_commandManager(new CommandManager())
    , m_occViewer(nullptr)
    , m_auiManager(this)
    , m_isFirstActivate(true)
{
    LOG_INF("MainFrame initializing with command pattern");
    createMenu();
    createToolbar();
    createPanels();
    setupCommandSystem();  // Setup command system after panels are created

    CreateStatusBar();
    SetStatusText("Ready - Command system initialized", 0);
}

MainFrame::~MainFrame()
{
    m_auiManager.UnInit();
    delete m_commandManager;
    LOG_INF("MainFrame destroyed");
}

void MainFrame::setupCommandSystem()
{
    LOG_INF("Setting up command system");
    
    // Create command dispatcher
    m_commandDispatcher = std::make_unique<CommandDispatcher>();
    
    // Create command listeners
    auto createBoxListener = std::make_shared<CreateBoxListener>(m_mouseHandler);
    auto createSphereListener = std::make_shared<CreateSphereListener>(m_mouseHandler);
    auto createCylinderListener = std::make_shared<CreateCylinderListener>(m_mouseHandler);
    auto createConeListener = std::make_shared<CreateConeListener>(m_mouseHandler);
    auto createWrenchListener = std::make_shared<CreateWrenchListener>(m_geometryFactory);
    
    // Register geometry command listeners
    m_listenerManager = std::make_unique<CommandListenerManager>();
    m_listenerManager->registerListener(cmd::CommandType::CreateBox, createBoxListener);
    m_listenerManager->registerListener(cmd::CommandType::CreateSphere, createSphereListener);
    m_listenerManager->registerListener(cmd::CommandType::CreateCylinder, createCylinderListener);
    m_listenerManager->registerListener(cmd::CommandType::CreateCone, createConeListener);
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
    
    // Register view command listeners
    m_listenerManager->registerListener(cmd::CommandType::ViewAll, viewAllListener);
    m_listenerManager->registerListener(cmd::CommandType::ViewTop, viewTopListener);
    m_listenerManager->registerListener(cmd::CommandType::ViewFront, viewFrontListener);
    m_listenerManager->registerListener(cmd::CommandType::ViewRight, viewRightListener);
    m_listenerManager->registerListener(cmd::CommandType::ViewIsometric, viewIsoListener);
    m_listenerManager->registerListener(cmd::CommandType::ShowNormals, showNormalsListener);
    m_listenerManager->registerListener(cmd::CommandType::FixNormals, fixNormalsListener);
    m_listenerManager->registerListener(cmd::CommandType::ShowEdges, showEdgesListener);
    
    // Register file command listeners
    auto fileNewListener = std::make_shared<FileNewListener>(m_canvas, m_commandManager);
    auto fileOpenListener = std::make_shared<FileOpenListener>(this);
    auto fileSaveListener = std::make_shared<FileSaveListener>(this);
    auto importStepListener = std::make_shared<ImportStepListener>(this, m_canvas, m_occViewer);
    m_listenerManager->registerListener(cmd::CommandType::FileNew, fileNewListener);
    m_listenerManager->registerListener(cmd::CommandType::FileOpen, fileOpenListener);
    m_listenerManager->registerListener(cmd::CommandType::FileSave, fileSaveListener);
    m_listenerManager->registerListener(cmd::CommandType::ImportSTEP, importStepListener);
    auto undoListener = std::make_shared<UndoListener>(m_commandManager, m_canvas);
    auto redoListener = std::make_shared<RedoListener>(m_commandManager, m_canvas);
    auto helpAboutListener = std::make_shared<HelpAboutListener>(this);
    auto navCubeConfigListener = std::make_shared<NavCubeConfigListener>(m_canvas);
    auto zoomSpeedListener = std::make_shared<ZoomSpeedListener>(this, m_canvas);
    auto fileExitListener = std::make_shared<FileExitListener>(this);
    auto meshQualityDialogListener = std::make_shared<MeshQualityDialogListener>(this, m_occViewer);
    m_listenerManager->registerListener(cmd::CommandType::Undo, undoListener);
    m_listenerManager->registerListener(cmd::CommandType::Redo, redoListener);
    m_listenerManager->registerListener(cmd::CommandType::HelpAbout, helpAboutListener);
    m_listenerManager->registerListener(cmd::CommandType::NavCubeConfig, navCubeConfigListener);
    m_listenerManager->registerListener(cmd::CommandType::ZoomSpeed, zoomSpeedListener);
    m_listenerManager->registerListener(cmd::CommandType::FileExit, fileExitListener);
    m_listenerManager->registerListener(cmd::CommandType::MeshQualityDialog, meshQualityDialogListener);
    
    // Set UI feedback handler
    m_commandDispatcher->setUIFeedbackHandler(
        [this](const CommandResult& result) {
            this->onCommandFeedback(result);
        }
    );
    
    LOG_INF("Command system setup completed");
}

void MainFrame::onCommand(wxCommandEvent& event)
{
    auto it = kEventTable.find(event.GetId());
    if (it == kEventTable.end()) {
        LOG_WRN("Unknown command ID: " + std::to_string(event.GetId()));
        return;
    }
    cmd::CommandType commandType = it->second;
    std::unordered_map<std::string, std::string> parameters;
    if (commandType == cmd::CommandType::ShowNormals || commandType == cmd::CommandType::ShowEdges) {
        parameters["toggle"] = "true";
    }
    if (m_listenerManager && m_listenerManager->hasListener(commandType)) {
        CommandResult result = m_listenerManager->dispatch(commandType, parameters);
        onCommandFeedback(result);
    } else {
        LOG_ERR("No listener registered for command");
        SetStatusText("Error: No listener registered", 0);
    }
}

void MainFrame::onCommandFeedback(const CommandResult& result)
{
    // Update UI based on command execution result
    if (result.success) {
        SetStatusText(result.message.empty() ? "Command executed successfully" : result.message, 0);
        LOG_INF("Command executed: " + result.commandId);
    } else {
        SetStatusText("Error: " + result.message, 0);
        LOG_ERR("Command failed: " + result.commandId + " - " + result.message);
        
        // Show error dialog for critical failures
        if (!result.message.empty() && result.commandId != "UNKNOWN") {
            wxMessageBox(result.message, "Command Error", wxOK | wxICON_ERROR, this);
        }
    }
    
    // Update menu/toolbar states for toggle commands
    if (result.commandId == cmd::to_string(cmd::CommandType::ShowNormals) && result.success && m_occViewer) {
        GetMenuBar()->Check(ID_SHOW_NORMALS, m_occViewer->isShowNormals());
    }
    else if (result.commandId == cmd::to_string(cmd::CommandType::ShowEdges) && result.success && m_occViewer) {
        GetMenuBar()->Check(ID_VIEW_SHOWEDGES, m_occViewer->isShowEdges());
    }
    
    // Refresh canvas if needed
    if (m_canvas && (result.commandId.find("VIEW_") == 0 || 
                     result.commandId.find("SHOW_") == 0 ||
                     result.commandId == "FIX_NORMALS")) {
        m_canvas->Refresh();
    }
}

void MainFrame::createMenu()
{
    wxMenuBar* menuBar = new wxMenuBar;

    wxMenu* fileMenu = new wxMenu;
    fileMenu->Append(wxID_NEW, "&New\tCtrl+N", "Create a new project");
    fileMenu->Append(wxID_OPEN, "&Open...\tCtrl+O", "Open an existing project");
    fileMenu->Append(wxID_SAVE, "&Save\tCtrl+S", "Save the current project");
    fileMenu->AppendSeparator();
    fileMenu->Append(ID_IMPORT_STEP, "&Import STEP...\tCtrl+I", "Import STEP/STP CAD file");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4", "Exit the application");
    menuBar->Append(fileMenu, "&File");

    wxMenu* createMenu = new wxMenu;
    createMenu->Append(ID_CREATE_BOX, "&Box", "Create a box");
    createMenu->Append(ID_CREATE_SPHERE, "&Sphere", "Create a sphere");
    createMenu->Append(ID_CREATE_CYLINDER, "&Cylinder", "Create a cylinder");
    createMenu->Append(ID_CREATE_CONE, "&Cone", "Create a cone");
    createMenu->Append(ID_CREATE_WRENCH, "&Wrench", "Create a wrench");
    menuBar->Append(createMenu, "&Create");

    // View menu
    wxMenu* viewMenu = new wxMenu;
    viewMenu->Append(ID_VIEW_ALL, _("Fit &All\tCtrl+A"), _("Fit all objects in view"));
    viewMenu->AppendSeparator();
    viewMenu->Append(ID_VIEW_TOP, _("&Top\tCtrl+1"), _("Top view"));
    viewMenu->Append(ID_VIEW_FRONT, _("&Front\tCtrl+2"), _("Front view"));
    viewMenu->Append(ID_VIEW_RIGHT, _("&Right\tCtrl+3"), _("Right view"));
    viewMenu->Append(ID_VIEW_ISOMETRIC, _("&Isometric\tCtrl+4"), _("Isometric view"));
    viewMenu->AppendSeparator();
    viewMenu->AppendCheckItem(ID_SHOW_NORMALS, _("Show &Normals"), _("Show/hide surface normals"));
    viewMenu->AppendCheckItem(ID_VIEW_SHOWEDGES, _("Show &Edges"), _("Show/hide object edges"));
    viewMenu->Append(ID_FIX_NORMALS, "&Fix Normals", "Automatically fix incorrect face normals");
    viewMenu->AppendSeparator();
    viewMenu->Append(ID_MESH_QUALITY_DIALOG, "&Mesh Quality Control...", "Control mesh precision and LOD settings");
    viewMenu->AppendSeparator();
    viewMenu->Append(ID_NAVIGATION_CUBE_CONFIG, "&Navigation Cube Config...", "Configure navigation cube settings");
    viewMenu->AppendSeparator();
    viewMenu->Append(ID_ZOOM_SPEED, _("Zoom &Speed...\tCtrl+Shift+Z"), _("Set mouse scroll zoom speed"));
    menuBar->Append(viewMenu, "&View");

    wxMenu* editMenu = new wxMenu;
    editMenu->Append(ID_UNDO, "&Undo\tCtrl+Z", "Undo the last action");
    editMenu->Append(ID_REDO, "&Redo\tCtrl+Y", "Redo the last undone action");
    menuBar->Append(editMenu, "&Edit");

    wxMenu* helpMenu = new wxMenu;
    helpMenu->Append(wxID_ABOUT, "&About...", "Show about dialog");
    menuBar->Append(helpMenu, "&Help");

    SetMenuBar(menuBar);
}

void MainFrame::createToolbar()
{
    wxToolBar* toolbar = CreateToolBar();
    toolbar->AddTool(wxID_NEW, "New", wxArtProvider::GetBitmap(wxART_NEW), "Create a new project");
    toolbar->AddTool(wxID_OPEN, "Open", wxArtProvider::GetBitmap(wxART_FILE_OPEN), "Open an existing project");
    toolbar->AddTool(wxID_SAVE, "Save", wxArtProvider::GetBitmap(wxART_FILE_SAVE), "Save the current project");
    toolbar->AddTool(ID_IMPORT_STEP, "Import STEP", wxArtProvider::GetBitmap(wxART_FOLDER_OPEN), "Import STEP/STP CAD file");
    toolbar->AddSeparator();
    toolbar->AddTool(ID_CREATE_BOX, "Box", wxArtProvider::GetBitmap(wxART_HELP_BOOK), "Create a box");
    toolbar->AddTool(ID_CREATE_SPHERE, "Sphere", wxArtProvider::GetBitmap(wxART_HELP_PAGE), "Create a sphere");
    toolbar->AddTool(ID_CREATE_CYLINDER, "Cylinder", wxArtProvider::GetBitmap(wxART_TIP), "Create a cylinder");
    toolbar->AddTool(ID_CREATE_CONE, "Cone", wxArtProvider::GetBitmap(wxART_INFORMATION), "Create a cone");
    toolbar->AddTool(ID_CREATE_WRENCH, "Wrench", wxArtProvider::GetBitmap(wxART_PLUS), "Create a wrench");
    toolbar->AddSeparator();
    toolbar->AddTool(ID_VIEW_ALL, "Fit All", wxArtProvider::GetBitmap(wxART_FULL_SCREEN), "Fit all objects in view");
    toolbar->AddTool(ID_VIEW_TOP, "Top", wxArtProvider::GetBitmap(wxART_GO_UP), "Set top view");
    toolbar->AddTool(ID_VIEW_FRONT, "Front", wxArtProvider::GetBitmap(wxART_GO_FORWARD), "Set front view");
    toolbar->AddTool(ID_VIEW_RIGHT, "Right", wxArtProvider::GetBitmap(wxART_GO_TO_PARENT), "Set right view");
    toolbar->AddTool(ID_VIEW_ISOMETRIC, "Isometric", wxArtProvider::GetBitmap(wxART_HELP_SETTINGS), "Set isometric view");
    toolbar->AddSeparator();
    toolbar->AddCheckTool(ID_SHOW_NORMALS, "Show Normals", wxArtProvider::GetBitmap(wxART_LIST_VIEW), wxNullBitmap, "Show/hide face normals");
    toolbar->AddSeparator();
    toolbar->AddTool(ID_UNDO, "Undo", wxArtProvider::GetBitmap(wxART_UNDO), "Undo the last action");
    toolbar->AddTool(ID_REDO, "Redo", wxArtProvider::GetBitmap(wxART_REDO), "Redo the last undone action");
    toolbar->AddSeparator();
    toolbar->AddTool(ID_NAVIGATION_CUBE_CONFIG, "Nav Cube Config", wxArtProvider::GetBitmap(wxART_HELP_SIDE_PANEL), "Configure navigation cube");
    toolbar->Realize();
}

void MainFrame::createPanels()
{
    m_canvas = new Canvas(this);
    if (!m_canvas) {
        LOG_ERR("Failed to create Canvas");
        return;
    }

    PropertyPanel* propertyPanel = new PropertyPanel(this);
    if (!propertyPanel) {
        LOG_ERR("Failed to create PropertyPanel");
        return;
    }

    ObjectTreePanel* objectTreePanel = new ObjectTreePanel(this);
    if (!objectTreePanel) {
        LOG_ERR("Failed to create ObjectTreePanel");
        return;
    }

    objectTreePanel->setPropertyPanel(propertyPanel);

    m_mouseHandler = new MouseHandler(m_canvas, objectTreePanel, propertyPanel, m_commandManager);
    if (!m_mouseHandler) {
        LOG_ERR("Failed to create MouseHandler");
        return;
    }

    m_canvas->getInputManager()->setMouseHandler(m_mouseHandler);

    NavigationController* navController = new NavigationController(m_canvas, m_canvas->getSceneManager());
    if (!navController) {
        LOG_ERR("Failed to create NavigationController");
        return;
    }
    m_canvas->getInputManager()->setNavigationController(navController);
    m_mouseHandler->setNavigationController(navController);

    m_occViewer = new OCCViewer(m_canvas->getSceneManager());

    // Set OCCViewer reference in Canvas for LOD interaction detection
    m_canvas->setOCCViewer(m_occViewer);

    // Now that all handlers are set, initialize the input manager's states
    m_canvas->getInputManager()->initializeStates();

    m_canvas->setObjectTreePanel(objectTreePanel);
    m_canvas->setCommandManager(m_commandManager);

    m_geometryFactory = new GeometryFactory(
        m_canvas->getSceneManager()->getObjectRoot(),
        objectTreePanel,
        propertyPanel,
        m_commandManager,
        m_occViewer
    );
    if (!m_geometryFactory) {
        LOG_ERR("Failed to create GeometryFactory");
        return;
    }

    m_auiManager.AddPane(m_canvas, wxAuiPaneInfo().Name("Canvas").CenterPane().Caption("Canvas"));
    m_auiManager.AddPane(objectTreePanel, wxAuiPaneInfo().Name("Objects").Left().Caption("Objects View").MinSize(wxSize(250, 400)).Layer(1));
    m_auiManager.AddPane(propertyPanel, wxAuiPaneInfo().Name("Properties").Left().Caption("Properties View").MinSize(wxSize(250, 200)).Layer(1));

    m_auiManager.Update();
    
    if (m_canvas && m_canvas->getSceneManager()) {
        m_canvas->getSceneManager()->resetView(); 
        LOG_INF("Initial view set to isometric and fit to scene");
    }
}

void MainFrame::onClose(wxCloseEvent& event)
{
    LOG_INF("Closing application");
    Destroy();
}

void MainFrame::onActivate(wxActivateEvent& event)
{
    if (event.GetActive() && m_isFirstActivate) {
        m_isFirstActivate = false;
        // Synchronize UI state now that the window is active and ready
        if (m_occViewer) {
            GetMenuBar()->Check(ID_VIEW_SHOWEDGES, m_occViewer->isShowEdges());
        }
    }
    event.Skip();
}

