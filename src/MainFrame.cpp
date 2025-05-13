#include "MainFrame.h"
#include "Canvas.h"
#include "PropertyPanel.h"
#include "ObjectTreePanel.h"
#include "MouseHandler.h"
#include "Command.h"
#include "GeometryObject.h"
#include "Logger.h"
#include <wx/splitter.h>
#include <wx/artprov.h>
#include <wx/aboutdlg.h>
#include <wx/filedlg.h>
#include <wx/aui/aui.h>

enum {
    ID_NEW = wxID_HIGHEST + 1,
    ID_OPEN,
    ID_SAVE,
    ID_SAVE_AS,

    ID_CREATE_BOX,
    ID_CREATE_SPHERE,
    ID_CREATE_CYLINDER,
    ID_CREATE_CONE,

    ID_VIEW_MODE,
    ID_SELECT_MODE,

    ID_VIEW_ALL,
    ID_VIEW_TOP,
    ID_VIEW_FRONT,
    ID_VIEW_RIGHT,
    ID_VIEW_ISOMETRIC
};

BEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_MENU(ID_NEW, MainFrame::onNew)
EVT_MENU(ID_OPEN, MainFrame::onOpen)
EVT_MENU(ID_SAVE, MainFrame::onSave)
EVT_MENU(ID_SAVE_AS, MainFrame::onSaveAs)
EVT_MENU(wxID_EXIT, MainFrame::onExit)

EVT_MENU(wxID_UNDO, MainFrame::onUndo)
EVT_MENU(wxID_REDO, MainFrame::onRedo)

EVT_MENU(ID_CREATE_BOX, MainFrame::onCreateBox)
EVT_MENU(ID_CREATE_SPHERE, MainFrame::onCreateSphere)
EVT_MENU(ID_CREATE_CYLINDER, MainFrame::onCreateCylinder)
EVT_MENU(ID_CREATE_CONE, MainFrame::onCreateCone)

EVT_MENU(ID_VIEW_MODE, MainFrame::onViewMode)
EVT_MENU(ID_SELECT_MODE, MainFrame::onSelectMode)

EVT_MENU(ID_VIEW_ALL, MainFrame::onViewAll)
EVT_MENU(ID_VIEW_TOP, MainFrame::onViewTop)
EVT_MENU(ID_VIEW_FRONT, MainFrame::onViewFront)
EVT_MENU(ID_VIEW_RIGHT, MainFrame::onViewRight)
EVT_MENU(ID_VIEW_ISOMETRIC, MainFrame::onViewIsometric)

EVT_MENU(wxID_ABOUT, MainFrame::onAbout)
END_EVENT_TABLE()

MainFrame::MainFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(1400, 800))
{
    LOG_INF("MainFrame initializing");
    // Center the window on screen
    Centre();
    // Set minimum window size
    SetMinSize(wxSize(1400, 800));
    
    m_auiManager.SetManagedWindow(this);
    m_commandManager = new CommandManager(); // Initialize before createPanels
    createMenuBar();
    createToolBar();
    createStatusBar();
    createPanels();
    SetIcon(wxArtProvider::GetIcon(wxART_FRAME_ICON));
    updateUI();
    m_auiManager.Update();
    LOG_INF("MainFrame initialized successfully");
}

MainFrame::~MainFrame()
{
    LOG_INF("MainFrame destroying");
    m_auiManager.UnInit();
    delete m_commandManager;
}

void MainFrame::createMenuBar()
{
    wxMenuBar* menuBar = new wxMenuBar();
    wxMenu* fileMenu = new wxMenu();
    fileMenu->Append(ID_NEW, "&New\tCtrl+N");
    fileMenu->Append(ID_OPEN, "&Open...\tCtrl+O");
    fileMenu->Append(ID_SAVE, "&Save\tCtrl+S");
    fileMenu->Append(ID_SAVE_AS, "Save &As...");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4");
    menuBar->Append(fileMenu, "&File");

    wxMenu* editMenu = new wxMenu();
    editMenu->Append(wxID_UNDO, "&Undo\tCtrl+Z");
    editMenu->Append(wxID_REDO, "&Redo\tCtrl+Y");
    menuBar->Append(editMenu, "&Edit");

    wxMenu* createMenu = new wxMenu();
    createMenu->Append(ID_CREATE_BOX, "&Box");
    createMenu->Append(ID_CREATE_SPHERE, "&Sphere");
    createMenu->Append(ID_CREATE_CYLINDER, "&Cylinder");
    createMenu->Append(ID_CREATE_CONE, "C&one");
    menuBar->Append(createMenu, "&Create");

    wxMenu* modeMenu = new wxMenu();
    modeMenu->AppendRadioItem(ID_VIEW_MODE, "&View Mode");
    modeMenu->AppendRadioItem(ID_SELECT_MODE, "&Select Mode");
    menuBar->Append(modeMenu, "&Mode");

    wxMenu* viewMenu = new wxMenu();
    viewMenu->Append(ID_VIEW_ALL, "&Fit All\tF");
    viewMenu->AppendSeparator();
    viewMenu->Append(ID_VIEW_TOP, "&Top View\tT");
    viewMenu->Append(ID_VIEW_FRONT, "&Front View\tF");
    viewMenu->Append(ID_VIEW_RIGHT, "&Right View\tR");
    viewMenu->Append(ID_VIEW_ISOMETRIC, "&Isometric View\tI");
    menuBar->Append(viewMenu, "&View");

    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(wxID_ABOUT, "&About");
    menuBar->Append(helpMenu, "&Help");

    SetMenuBar(menuBar);
}

void MainFrame::createToolBar()
{
    wxToolBar* toolBar = CreateToolBar(wxTB_FLAT | wxTB_HORIZONTAL);
    toolBar->AddTool(ID_NEW, "New", wxArtProvider::GetBitmap(wxART_NEW), "Create new document");
    toolBar->AddTool(ID_OPEN, "Open", wxArtProvider::GetBitmap(wxART_FILE_OPEN), "Open a document");
    toolBar->AddTool(ID_SAVE, "Save", wxArtProvider::GetBitmap(wxART_FILE_SAVE), "Save document");
    toolBar->AddSeparator();
    toolBar->AddTool(wxID_UNDO, "Undo", wxArtProvider::GetBitmap(wxART_UNDO), "Undo last action");
    toolBar->AddTool(wxID_REDO, "Redo", wxArtProvider::GetBitmap(wxART_REDO), "Redo last action");
    toolBar->AddSeparator();
    toolBar->AddRadioTool(ID_VIEW_MODE, "View Mode", wxArtProvider::GetBitmap(wxART_FIND), wxNullBitmap, "View mode");
    toolBar->AddRadioTool(ID_SELECT_MODE, "Select Mode", wxArtProvider::GetBitmap(wxART_FIND_AND_REPLACE), wxNullBitmap, "Select mode");
    toolBar->AddSeparator();
    toolBar->AddTool(ID_CREATE_BOX, "Create Box", wxArtProvider::GetBitmap(wxART_PLUS), "Create a box");
    toolBar->AddTool(ID_CREATE_SPHERE, "Create Sphere", wxArtProvider::GetBitmap(wxART_PLUS), "Create a sphere");
    toolBar->AddTool(ID_CREATE_CYLINDER, "Create Cylinder", wxArtProvider::GetBitmap(wxART_PLUS), "Create a cylinder");
    toolBar->AddTool(ID_CREATE_CONE, "Create Cone", wxArtProvider::GetBitmap(wxART_PLUS), "Create a cone");
    toolBar->AddSeparator();
    toolBar->AddTool(ID_VIEW_ALL, "Fit All", wxArtProvider::GetBitmap(wxART_FULL_SCREEN), "Fit all objects");
    toolBar->AddTool(ID_VIEW_TOP, "Top View", wxArtProvider::GetBitmap(wxART_GO_UP), "Top view");
    toolBar->AddTool(ID_VIEW_FRONT, "Front View", wxArtProvider::GetBitmap(wxART_GO_FORWARD), "Front view");
    toolBar->AddTool(ID_VIEW_RIGHT, "Right View", wxArtProvider::GetBitmap(wxART_CDROM), "Right view");
    toolBar->Realize();
    LOG_INF("Toolbar created with geometry creation tools");
}

void MainFrame::createStatusBar()
{
    CreateStatusBar(3);
    SetStatusText("Ready", 0);
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

    // Connect ObjectTreePanel with PropertyPanel
    objectTreePanel->setPropertyPanel(propertyPanel);

    m_mouseHandler = new MouseHandler(m_canvas, objectTreePanel, propertyPanel, m_commandManager);
    if (!m_mouseHandler) {
        LOG_ERR("Failed to create MouseHandler");
        return;
    }

    m_canvas->setMouseHandler(m_mouseHandler);

    // Create and set NavigationStyle
    NavigationStyle* navStyle = new NavigationStyle(m_canvas);
    if (!navStyle) {
        LOG_ERR("Failed to create NavigationStyle");
        return;
    }
    m_canvas->setNavigationStyle(navStyle);
    m_mouseHandler->setNavigationStyle(navStyle);

    m_auiManager.AddPane(m_canvas, wxAuiPaneInfo().Name("Canvas").CenterPane().Caption("Canvas"));
    m_auiManager.AddPane(objectTreePanel, wxAuiPaneInfo().Name("Objects").Left().Caption("Objects View").MinSize(wxSize(250, 400)).Layer(1));
    m_auiManager.AddPane(propertyPanel, wxAuiPaneInfo().Name("Properties").Left().Caption("Properties View").MinSize(wxSize(250, 200)).Layer(1));
}

void MainFrame::onNew(wxCommandEvent& event)
{
    LOG_INF("Creating new document");
    SetStatusText("New document created", 0);
}

void MainFrame::onOpen(wxCommandEvent& event)
{
    LOG_INF("Opening file dialog");
    wxFileDialog openDialog(this, "Open File", "", "", "All Files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openDialog.ShowModal() == wxID_OK)
    {
        wxString path = openDialog.GetPath();
        LOG_INF("Opening file: " + path.ToStdString());
        SetStatusText("Opened: " + path, 0);
    }
}

void MainFrame::onSave(wxCommandEvent& event)
{
    LOG_INF("Saving document");
    SetStatusText("Document saved", 0);
}

void MainFrame::onSaveAs(wxCommandEvent& event)
{
    LOG_INF("Opening save as dialog");
    wxFileDialog saveDialog(this, "Save File", "", "", "All Files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (saveDialog.ShowModal() == wxID_OK)
    {
        wxString path = saveDialog.GetPath();
        LOG_INF("Saving file as: " + path.ToStdString());
        SetStatusText("Saved as: " + path, 0);
    }
}

void MainFrame::onExit(wxCommandEvent& event)
{
    LOG_INF("Exiting application");
    Close();
}

void MainFrame::onUndo(wxCommandEvent& event)
{
    if (m_commandManager->canUndo())
    {
        LOG_INF("Performing undo: " + m_commandManager->getUndoCommandDescription());
        m_commandManager->undo();
        SetStatusText("Undo: " + m_commandManager->getRedoCommandDescription(), 0);
        updateUI();
    }
}

void MainFrame::onRedo(wxCommandEvent& event)
{
    if (m_commandManager->canRedo())
    {
        LOG_INF("Performing redo: " + m_commandManager->getRedoCommandDescription());
        m_commandManager->redo();
        SetStatusText("Redo: " + m_commandManager->getUndoCommandDescription(), 0);
        updateUI();
    }
}

void MainFrame::onCreateBox(wxCommandEvent& event)
{
    if (!m_mouseHandler) {
        LOG_ERR("MouseHandler is null in onCreateBox");
        SetStatusText("Error: Cannot create box", 0);
        return;
    }
    LOG_INF("Initiating box creation");
    m_mouseHandler->setOperationMode(MouseHandler::CREATE);
    m_mouseHandler->setCreationGeometryType("Box");
    SetStatusText("Create Box: Click to place", 0);
}

void MainFrame::onCreateSphere(wxCommandEvent& event)
{
    if (!m_mouseHandler) {
        LOG_ERR("MouseHandler is null in onCreateSphere");
        SetStatusText("Error: Cannot create sphere", 0);
        return;
    }
    LOG_INF("Initiating sphere creation");
    m_mouseHandler->setOperationMode(MouseHandler::CREATE);
    m_mouseHandler->setCreationGeometryType("Sphere");
    SetStatusText("Create Sphere: Click to place", 0);
}

void MainFrame::onCreateCylinder(wxCommandEvent& event)
{
    if (!m_mouseHandler) {
        LOG_ERR("MouseHandler is null in onCreateCylinder");
        SetStatusText("Error: Cannot create cylinder", 0);
        return;
    }
    LOG_INF("Initiating cylinder creation");
    m_mouseHandler->setOperationMode(MouseHandler::CREATE);
    m_mouseHandler->setCreationGeometryType("Cylinder");
    SetStatusText("Create Cylinder: Click to place", 0);
}

void MainFrame::onCreateCone(wxCommandEvent& event)
{
    if (!m_mouseHandler) {
        LOG_ERR("MouseHandler is null in onCreateCone");
        SetStatusText("Error: Cannot create cone", 0);
        return;
    }
    LOG_INF("Initiating cone creation");
    m_mouseHandler->setOperationMode(MouseHandler::CREATE);
    m_mouseHandler->setCreationGeometryType("Cone");
    SetStatusText("Create Cone: Click to place", 0);
}

void MainFrame::onViewMode(wxCommandEvent& event)
{
    if (!m_mouseHandler) {
        LOG_ERR("MouseHandler is null in onViewMode");
        SetStatusText("Error: Cannot switch to view mode", 0);
        return;
    }
    LOG_INF("Switching to view mode");
    m_mouseHandler->setOperationMode(MouseHandler::NAVIGATE);
    SetStatusText("View Mode: Use mouse to navigate", 0);
}

void MainFrame::onSelectMode(wxCommandEvent& event)
{
    if (!m_mouseHandler) {
        LOG_ERR("MouseHandler is null in onSelectMode");
        SetStatusText("Error: Cannot switch to select mode", 0);
        return;
    }
    LOG_INF("Switching to select mode");
    m_mouseHandler->setOperationMode(MouseHandler::SELECT);
    SetStatusText("Select Mode: Click to select objects", 0);
}

void MainFrame::onViewAll(wxCommandEvent& event)
{
    if (!m_canvas || !m_mouseHandler) {
        LOG_ERR("Canvas or MouseHandler is null in onViewAll");
        SetStatusText("Error: Cannot view all", 0);
        return;
    }
    LOG_INF("Viewing all objects");
    NavigationStyle* navStyle = m_mouseHandler->getNavigationStyle();
    if (navStyle)
    {
        navStyle->viewAll();
        m_canvas->render();
    }
    SetStatusText("View: Fit All", 0);
}

void MainFrame::onViewTop(wxCommandEvent& event)
{
    if (!m_canvas || !m_mouseHandler) {
        LOG_ERR("Canvas or MouseHandler is null in onViewTop");
        SetStatusText("Error: Cannot switch to top view", 0);
        return;
    }
    LOG_INF("Switching to top view");
    NavigationStyle* navStyle = m_mouseHandler->getNavigationStyle();
    if (navStyle)
    {
        navStyle->viewTop();
        m_canvas->render();
    }
    SetStatusText("View: Top", 0);
}

void MainFrame::onViewFront(wxCommandEvent& event)
{
    if (!m_canvas || !m_mouseHandler) {
        LOG_ERR("Canvas or MouseHandler is null in onViewFront");
        SetStatusText("Error: Cannot switch to front view", 0);
        return;
    }
    LOG_INF("Switching to front view");
    NavigationStyle* navStyle = m_mouseHandler->getNavigationStyle();
    if (navStyle)
    {
        navStyle->viewFront();
        m_canvas->render();
    }
    SetStatusText("View: Front", 0);
}

void MainFrame::onViewRight(wxCommandEvent& event)
{
    if (!m_canvas || !m_mouseHandler) {
        LOG_ERR("Canvas or MouseHandler is null in onViewRight");
        SetStatusText("Error: Cannot switch to right view", 0);
        return;
    }
    LOG_INF("Switching to right view");
    NavigationStyle* navStyle = m_mouseHandler->getNavigationStyle();
    if (navStyle)
    {
        navStyle->viewRight();
        m_canvas->render();
    }
    SetStatusText("View: Right", 0);
}

void MainFrame::onViewIsometric(wxCommandEvent& event)
{
    if (!m_canvas || !m_mouseHandler) {
        LOG_ERR("Canvas or MouseHandler is null in onViewIsometric");
        SetStatusText("Error: Cannot switch to isometric view", 0);
        return;
    }
    LOG_INF("Switching to isometric view");
    NavigationStyle* navStyle = m_mouseHandler->getNavigationStyle();
    if (navStyle)
    {
        navStyle->viewIsometric();
        m_canvas->render();
    }
    SetStatusText("View: Isometric", 0);
}

void MainFrame::onAbout(wxCommandEvent& event)
{
    LOG_INF("Showing about dialog");
    wxAboutDialogInfo info;
    info.SetName("wxCoin 3D Modeler");
    info.SetVersion("1.0");
    info.SetDescription("A simple 3D modeling application using wxWidgets and Coin3D");
    info.SetCopyright("(C) 2023");
    wxAboutBox(info);
}

void MainFrame::updateUI()
{
    wxMenuBar* menuBar = GetMenuBar();
    if (menuBar)
    {
        menuBar->Enable(wxID_UNDO, m_commandManager->canUndo());
        menuBar->Enable(wxID_REDO, m_commandManager->canRedo());
    }

    wxToolBar* toolBar = GetToolBar();
    if (toolBar)
    {
        toolBar->EnableTool(wxID_UNDO, m_commandManager->canUndo());
        toolBar->EnableTool(wxID_REDO, m_commandManager->canRedo());
    }

    if (m_commandManager->canUndo())
    {
        SetStatusText("Undo: " + m_commandManager->getUndoCommandDescription(), 1);
    }
    else
    {
        SetStatusText("", 1);
    }

    if (m_commandManager->canRedo())
    {
        SetStatusText("Redo: " + m_commandManager->getRedoCommandDescription(), 2);
    }
    else
    {
        SetStatusText("", 2);
    }
}