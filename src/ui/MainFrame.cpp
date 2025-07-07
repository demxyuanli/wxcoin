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
#include <Inventor/SbVec3f.h>
#include "OCCViewer.h"

enum
{
    ID_CreateBox = wxID_HIGHEST + 100,
    ID_CreateSphere,
    ID_CreateCylinder,
    ID_CreateCone,
    ID_CreateWrench,
    ID_ImportSTEP,
    ID_ViewAll,
    ID_ViewTop,
    ID_ViewFront,
    ID_ViewRight,
    ID_ViewIsometric,
    ID_ShowNormals,
    ID_FixNormals,
    ID_Undo,
    ID_Redo,
    ID_NavigationCubeConfig
};

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_MENU(wxID_NEW, MainFrame::onNew)
EVT_MENU(wxID_OPEN, MainFrame::onOpen)
EVT_MENU(wxID_SAVE, MainFrame::onSave)
EVT_MENU(ID_ImportSTEP, MainFrame::onImportSTEP)
EVT_MENU(wxID_EXIT, MainFrame::onExit)
EVT_MENU(ID_CreateBox, MainFrame::onCreateBox)
EVT_MENU(ID_CreateSphere, MainFrame::onCreateSphere)
EVT_MENU(ID_CreateCylinder, MainFrame::onCreateCylinder)
EVT_MENU(ID_CreateCone, MainFrame::onCreateCone)
EVT_MENU(ID_CreateWrench, MainFrame::onCreateWrench)
EVT_MENU(ID_ViewAll, MainFrame::onViewAll)
EVT_MENU(ID_ViewTop, MainFrame::onViewTop)
EVT_MENU(ID_ViewFront, MainFrame::onViewFront)
EVT_MENU(ID_ViewRight, MainFrame::onViewRight)
EVT_MENU(ID_ViewIsometric, MainFrame::onViewIsometric)
EVT_MENU(ID_ShowNormals, MainFrame::onShowNormals)
EVT_MENU(ID_FixNormals, MainFrame::onFixNormals)
EVT_MENU(ID_Undo, MainFrame::onUndo)
EVT_MENU(ID_Redo, MainFrame::onRedo)
EVT_MENU(ID_NavigationCubeConfig, MainFrame::onNavigationCubeConfig)
EVT_MENU(wxID_ABOUT, MainFrame::onAbout)
EVT_CLOSE(MainFrame::onClose)
END_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title)
    : wxFrame(nullptr, wxID_ANY, title, wxDefaultPosition, wxSize(1200, 800))
    , m_canvas(nullptr)
    , m_mouseHandler(nullptr)
    , m_geometryFactory(nullptr)
    , m_commandManager(new CommandManager())
    , m_occViewer(nullptr)
    , m_auiManager(this)
{
    LOG_INF("MainFrame initializing");
    createMenu();
    createToolbar();
    createPanels();
    CreateStatusBar();
    SetStatusText("Ready", 0);
}

MainFrame::~MainFrame()
{
    m_auiManager.UnInit();
    delete m_commandManager;
    LOG_INF("MainFrame destroyed");
}

void MainFrame::createMenu()
{
    wxMenuBar* menuBar = new wxMenuBar;

    wxMenu* fileMenu = new wxMenu;
    fileMenu->Append(wxID_NEW, "&New\tCtrl+N", "Create a new project");
    fileMenu->Append(wxID_OPEN, "&Open...\tCtrl+O", "Open an existing project");
    fileMenu->Append(wxID_SAVE, "&Save\tCtrl+S", "Save the current project");
    fileMenu->AppendSeparator();
    fileMenu->Append(ID_ImportSTEP, "&Import STEP...\tCtrl+I", "Import STEP/STP CAD file");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4", "Exit the application");
    menuBar->Append(fileMenu, "&File");

    wxMenu* createMenu = new wxMenu;
    createMenu->Append(ID_CreateBox, "&Box", "Create a box");
    createMenu->Append(ID_CreateSphere, "&Sphere", "Create a sphere");
    createMenu->Append(ID_CreateCylinder, "&Cylinder", "Create a cylinder");
    createMenu->Append(ID_CreateCone, "&Cone", "Create a cone");
    createMenu->Append(ID_CreateWrench, "&Wrench", "Create a wrench");
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
    viewMenu->AppendCheckItem(ID_SHOW_EDGES, _("Show &Edges"), _("Show/hide object edges")); // Add edge display option
    viewMenu->Append(ID_FixNormals, "&Fix Normals", "Automatically fix incorrect face normals");
    viewMenu->AppendSeparator();
    viewMenu->Append(ID_NavigationCubeConfig, "&Navigation Cube Config...", "Configure navigation cube settings");
    menuBar->Append(viewMenu, "&View");

    wxMenu* editMenu = new wxMenu;
    editMenu->Append(ID_Undo, "&Undo\tCtrl+Z", "Undo the last action");
    editMenu->Append(ID_Redo, "&Redo\tCtrl+Y", "Redo the last undone action");
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
    toolbar->AddTool(ID_ImportSTEP, "Import STEP", wxArtProvider::GetBitmap(wxART_FOLDER_OPEN), "Import STEP/STP CAD file");
    toolbar->AddSeparator();
    toolbar->AddTool(ID_CreateBox, "Box", wxArtProvider::GetBitmap(wxART_HELP_BOOK), "Create a box");
    toolbar->AddTool(ID_CreateSphere, "Sphere", wxArtProvider::GetBitmap(wxART_HELP_PAGE), "Create a sphere");
    toolbar->AddTool(ID_CreateCylinder, "Cylinder", wxArtProvider::GetBitmap(wxART_TIP), "Create a cylinder");
    toolbar->AddTool(ID_CreateCone, "Cone", wxArtProvider::GetBitmap(wxART_INFORMATION), "Create a cone");
    toolbar->AddTool(ID_CreateWrench, "Wrench", wxArtProvider::GetBitmap(wxART_PLUS), "Create a wrench");
    toolbar->AddSeparator();
    toolbar->AddTool(ID_ViewAll, "Fit All", wxArtProvider::GetBitmap(wxART_FULL_SCREEN), "Fit all objects in view");
    toolbar->AddTool(ID_ViewTop, "Top", wxArtProvider::GetBitmap(wxART_GO_UP), "Set top view");
    toolbar->AddTool(ID_ViewFront, "Front", wxArtProvider::GetBitmap(wxART_GO_FORWARD), "Set front view");
    toolbar->AddTool(ID_ViewRight, "Right", wxArtProvider::GetBitmap(wxART_GO_TO_PARENT), "Set right view");
    toolbar->AddTool(ID_ViewIsometric, "Isometric", wxArtProvider::GetBitmap(wxART_HELP_SETTINGS), "Set isometric view");
    toolbar->AddSeparator();
    toolbar->AddCheckTool(ID_ShowNormals, "Show Normals", wxArtProvider::GetBitmap(wxART_LIST_VIEW), wxNullBitmap, "Show/hide face normals");
    toolbar->AddSeparator();
    toolbar->AddTool(ID_Undo, "Undo", wxArtProvider::GetBitmap(wxART_UNDO), "Undo the last action");
    toolbar->AddTool(ID_Redo, "Redo", wxArtProvider::GetBitmap(wxART_REDO), "Redo the last undone action");
    toolbar->AddSeparator();
    toolbar->AddTool(ID_NavigationCubeConfig, "Nav Cube Config", wxArtProvider::GetBitmap(wxART_HELP_SIDE_PANEL), "Configure navigation cube");
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

void MainFrame::onNew(wxCommandEvent& event)
{
    LOG_INF("Creating new project");
    m_canvas->getSceneManager()->cleanup();
    m_canvas->getSceneManager()->initScene();
    m_commandManager->clearHistory();
    SetStatusText("New project created", 0);
}

void MainFrame::onOpen(wxCommandEvent& event)
{
    wxFileDialog openFileDialog(this, "Open Project", "", "", "Project files (*.proj)|*.proj", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL) {
        LOG_INF("Open file dialog cancelled");
        return;
    }

    LOG_INF("Opening project: " + openFileDialog.GetPath().ToStdString());
    SetStatusText("Opened: " + openFileDialog.GetFilename(), 0);
}

void MainFrame::onSave(wxCommandEvent& event)
{
    wxFileDialog saveFileDialog(this, "Save Project", "", "", "Project files (*.proj)|*.proj", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (saveFileDialog.ShowModal() == wxID_CANCEL) {
        LOG_INF("Save file dialog cancelled");
        return;
    }

    LOG_INF("Saving project: " + saveFileDialog.GetPath().ToStdString());
    SetStatusText("Saved: " + saveFileDialog.GetFilename(), 0);
}

void MainFrame::onImportSTEP(wxCommandEvent& event)
{
    if (!m_occViewer) {
        LOG_ERR("OCCViewer is null in onImportSTEP");
        SetStatusText("Error: Cannot import STEP file", 0);
        return;
    }
    
    // Create file dialog for STEP files
    wxString wildcard = "STEP files (*.step;*.stp)|*.step;*.stp|All files (*.*)|*.*";
    wxFileDialog openFileDialog(this, "Import STEP File", "", "", wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (openFileDialog.ShowModal() == wxID_CANCEL) {
        LOG_INF("STEP import dialog cancelled");
        return;
    }
    
    std::string filePath = openFileDialog.GetPath().ToStdString();
    LOG_INF("Importing STEP file: " + filePath);
    SetStatusText("Importing STEP file...", 0);
    
    // Read the STEP file
    STEPReader::ReadResult result = STEPReader::readSTEPFile(filePath);
    
    if (!result.success) {
        wxString errorMsg = "Failed to import STEP file:\n" + result.errorMessage;
        wxMessageBox(errorMsg, "Import Error", wxOK | wxICON_ERROR, this);
        LOG_ERR("STEP import failed: " + result.errorMessage);
        SetStatusText("STEP import failed", 0);
        return;
    }
    
    // Add geometries to the viewer
    int importedCount = 0;
    for (const auto& geometry : result.geometries) {
        if (geometry) {
            m_occViewer->addGeometry(geometry);
            importedCount++;
        }
    }
    
    if (importedCount > 0) {
        // Fit all objects in view
        if (m_canvas && m_canvas->getInputManager()->getNavigationController()) {
            m_canvas->getInputManager()->getNavigationController()->viewAll();
        }
        
        // Update coordinate system scale based on imported content
        if (m_canvas && m_canvas->getSceneManager()) {
            m_canvas->getSceneManager()->updateCoordinateSystemScale();
        }
        
        wxString successMsg = wxString::Format("Successfully imported %d geometry objects from STEP file", importedCount);
        LOG_INF(successMsg.ToStdString());
        SetStatusText(successMsg, 0);
        
        // Show success message
        wxMessageBox(successMsg, "Import Successful", wxOK | wxICON_INFORMATION, this);
    } else {
        wxString errorMsg = "No geometry objects could be imported from the STEP file";
        wxMessageBox(errorMsg, "Import Warning", wxOK | wxICON_WARNING, this);
        LOG_WRN(errorMsg.ToStdString());
        SetStatusText("No geometries imported", 0);
    }
}

void MainFrame::onExit(wxCommandEvent& event)
{
    LOG_INF("Application exit requested");
    Close(true);
}

void MainFrame::onCreateBox(wxCommandEvent& event)
{
    if (!m_mouseHandler) {
        LOG_ERR("MouseHandler is null in onCreateBox");
        SetStatusText("Error: Cannot create box", 0);
        return;
    }
    LOG_INF("Initiating box creation");
    m_mouseHandler->setOperationMode(MouseHandler::OperationMode::CREATE);
    m_mouseHandler->setCreationGeometryType("Box");
    SetStatusText("Creating: Box", 0);
}

void MainFrame::onCreateSphere(wxCommandEvent& event)
{
    if (!m_mouseHandler) {
        LOG_ERR("MouseHandler is null in onCreateSphere");
        SetStatusText("Error: Cannot create sphere", 0);
        return;
    }
    LOG_INF("Initiating sphere creation");
    m_mouseHandler->setOperationMode(MouseHandler::OperationMode::CREATE);
    m_mouseHandler->setCreationGeometryType("Sphere");
    SetStatusText("Creating: Sphere", 0);
}

void MainFrame::onCreateCylinder(wxCommandEvent& event)
{
    if (!m_mouseHandler) {
        LOG_ERR("MouseHandler is null in onCreateCylinder");
        SetStatusText("Error: Cannot create cylinder", 0);
        return;
    }
    LOG_INF("Initiating cylinder creation");
    m_mouseHandler->setOperationMode(MouseHandler::OperationMode::CREATE);
    m_mouseHandler->setCreationGeometryType("Cylinder");
    SetStatusText("Creating: Cylinder", 0);
}

void MainFrame::onCreateCone(wxCommandEvent& event)
{
    if (!m_mouseHandler) {
        LOG_ERR("MouseHandler is null in onCreateCone");
        SetStatusText("Error: Cannot create cone", 0);
        return;
    }
    LOG_INF("Initiating cone creation");
    m_mouseHandler->setOperationMode(MouseHandler::OperationMode::CREATE);
    m_mouseHandler->setCreationGeometryType("Cone");
    SetStatusText("Creating: Cone", 0);
}

void MainFrame::onCreateWrench(wxCommandEvent& event)
{
    // Create an OpenCASCADE wrench at the origin
    m_geometryFactory->createGeometry("Wrench", SbVec3f(0.0f, 0.0f, 0.0f), GeometryType::OPENCASCADE);
}

void MainFrame::onViewAll(wxCommandEvent& event)
{
    if (!m_canvas || !m_canvas->getInputManager()->getNavigationController()) {
        LOG_ERR("Canvas or NavigationController is null in onViewAll");
        SetStatusText("Error: Cannot fit all", 0);
        return;
    }
    LOG_INF("Fitting all objects in view");
    m_canvas->getInputManager()->getNavigationController()->viewAll();
    SetStatusText("View: Fit All", 0);
}

void MainFrame::onViewTop(wxCommandEvent& event)
{
    if (!m_canvas || !m_canvas->getInputManager()->getNavigationController()) {
        LOG_ERR("Canvas or NavigationController is null in onViewTop");
        SetStatusText("Error: Cannot set top view", 0);
        return;
    }
    LOG_INF("Setting top view");
    m_canvas->getInputManager()->getNavigationController()->viewTop();
    SetStatusText("View: Top", 0);
}

void MainFrame::onViewFront(wxCommandEvent& event)
{
    if (!m_canvas || !m_canvas->getInputManager()->getNavigationController()) {
        LOG_ERR("Canvas or NavigationController is null in onViewFront");
        SetStatusText("Error: Cannot set front view", 0);
        return;
    }
    LOG_INF("Setting front view");
    m_canvas->getInputManager()->getNavigationController()->viewFront();
    SetStatusText("View: Front", 0);
}

void MainFrame::onViewRight(wxCommandEvent& event)
{
    if (!m_canvas || !m_canvas->getInputManager()->getNavigationController()) {
        LOG_ERR("Canvas or NavigationController is null in onViewRight");
        SetStatusText("Error: Cannot set right view", 0);
        return;
    }
    LOG_INF("Setting right view");
    m_canvas->getInputManager()->getNavigationController()->viewRight();
    SetStatusText("View: Right", 0);
}

void MainFrame::onViewIsometric(wxCommandEvent& event)
{
    if (!m_canvas || !m_canvas->getInputManager()->getNavigationController()) {
        LOG_ERR("Canvas or NavigationController is null in onViewIsometric");
        SetStatusText("Error: Cannot set isometric view", 0);
        return;
    }
    LOG_INF("Setting isometric view");
    m_canvas->getInputManager()->getNavigationController()->viewIsometric();
    SetStatusText("View: Isometric", 0);
}

void MainFrame::onShowNormals(wxCommandEvent& event)
{
    if (!m_occViewer) {
        LOG_ERR("OCCViewer is null in onShowNormals");
        SetStatusText("Error: Cannot toggle normals", 0);
        return;
    }
    
    bool showNormals = !m_occViewer->isShowNormals();
    m_occViewer->setShowNormals(showNormals);
    
    // Update menu item check state
    wxMenuBar* menuBar = GetMenuBar();
    if (menuBar) {
        menuBar->Check(ID_ShowNormals, showNormals);
    }
    
    LOG_INF(showNormals ? "Showing face normals" : "Hiding face normals");
    SetStatusText(showNormals ? "Normals: ON" : "Normals: OFF", 0);
}

void MainFrame::onFixNormals(wxCommandEvent& event)
{
    if (!m_occViewer) {
        LOG_ERR("OCCViewer is null in onFixNormals");
        SetStatusText("Error: Cannot fix normals", 0);
        return;
    }
    
    LOG_INF("Fixing face normals");
    m_occViewer->fixNormals();
    SetStatusText("Fixed face normals", 0);
}

void MainFrame::onNavigationCubeConfig(wxCommandEvent& event)
{
    if (!m_canvas) {
        LOG_ERR("Canvas is null in onNavigationCubeConfig");
        SetStatusText("Error: Cannot configure navigation cube", 0);
        return;
    }
    LOG_INF("Opening navigation cube configuration dialog");
    m_canvas->ShowNavigationCubeConfigDialog();
    SetStatusText("Navigation Cube Configuration", 0);
}

void MainFrame::onUndo(wxCommandEvent& event)
{
    if (!m_commandManager) {
        LOG_ERR("CommandManager is null in onUndo");
        SetStatusText("Error: Cannot undo", 0);
        return;
    }
    LOG_INF("Undoing last command");
    m_commandManager->undo();
    m_canvas->Refresh();
    SetStatusText("Undo", 0);
}

void MainFrame::onRedo(wxCommandEvent& event)
{
    if (!m_commandManager) {
        LOG_ERR("CommandManager is null in onRedo");
        SetStatusText("Error: Cannot redo", 0);
        return;
    }
    LOG_INF("Redoing last undone command");
    m_commandManager->redo();
    m_canvas->Refresh();
    SetStatusText("Redo", 0);
}

void MainFrame::onAbout(wxCommandEvent& event)
{
    wxAboutDialogInfo aboutInfo;
    aboutInfo.SetName("FreeCAD Navigation");
    aboutInfo.SetVersion("1.0");
    aboutInfo.SetDescription("A 3D CAD application with navigation and geometry creation");
    aboutInfo.SetCopyright("(C) 2025 Your Name");
    wxAboutBox(aboutInfo);
    LOG_INF("About dialog shown");
}

void MainFrame::onClose(wxCloseEvent& event)
{
    LOG_INF("Closing application");
    Destroy();
}

// Add edge display event handler
void MainFrame::onShowEdges(wxCommandEvent& event)
{
    bool showEdges = event.IsChecked();
    if (m_occViewer) {
        m_occViewer->setShowEdges(showEdges);
        LOG_INF(showEdges ? "Edges shown" : "Edges hidden");
    }
}