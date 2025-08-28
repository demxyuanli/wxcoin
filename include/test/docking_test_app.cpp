#include "docking_test_app.h"
#include "docking/DockArea.h"
#include "docking/FloatingDockContainer.h"
#include "docking/AutoHideContainer.h"
#include <wx/treectrl.h>
#include <wx/grid.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/splitter.h>
#include <wx/artprov.h>

namespace ads {

// Application implementation
wxIMPLEMENT_APP(DockingTestApp);

bool DockingTestApp::OnInit() {
    if (!wxApp::OnInit()) {
        return false;
    }
    
    // Initialize image handlers
    wxInitAllImageHandlers();
    
    // Create main frame
    DockingTestFrame* frame = new DockingTestFrame();
    frame->Show(true);
    
    return true;
}

// Event table for main frame
wxBEGIN_EVENT_TABLE(DockingTestFrame, wxFrame)
    // File menu
    EVT_MENU(wxID_NEW, DockingTestFrame::OnFileNew)
    EVT_MENU(wxID_OPEN, DockingTestFrame::OnFileOpen)
    EVT_MENU(wxID_SAVE, DockingTestFrame::OnFileSave)
    EVT_MENU(wxID_EXIT, DockingTestFrame::OnFileExit)
    
    // View menu
    EVT_MENU(ID_VIEW_SAVE_LAYOUT, DockingTestFrame::OnViewSaveLayout)
    EVT_MENU(ID_VIEW_LOAD_LAYOUT, DockingTestFrame::OnViewLoadLayout)
    EVT_MENU(ID_VIEW_RESET_LAYOUT, DockingTestFrame::OnViewResetLayout)
    EVT_MENU(ID_VIEW_PERSPECTIVES, DockingTestFrame::OnViewManagePerspectives)
    
    // Docking menu
    EVT_MENU(ID_DOCKING_ADD_EDITOR, DockingTestFrame::OnDockingAddEditor)
    EVT_MENU(ID_DOCKING_ADD_TOOL, DockingTestFrame::OnDockingAddTool)
    EVT_MENU(ID_DOCKING_SHOW_ALL, DockingTestFrame::OnDockingShowAll)
    EVT_MENU(ID_DOCKING_HIDE_ALL, DockingTestFrame::OnDockingHideAll)
    EVT_MENU(ID_DOCKING_TOGGLE_AUTOHIDE, DockingTestFrame::OnDockingToggleAutoHide)
    
    // Help menu
    EVT_MENU(wxID_ABOUT, DockingTestFrame::OnHelpAbout)
wxEND_EVENT_TABLE()

// Menu IDs
enum {
    ID_VIEW_SAVE_LAYOUT = wxID_HIGHEST + 1,
    ID_VIEW_LOAD_LAYOUT,
    ID_VIEW_RESET_LAYOUT,
    ID_VIEW_PERSPECTIVES,
    ID_DOCKING_ADD_EDITOR,
    ID_DOCKING_ADD_TOOL,
    ID_DOCKING_SHOW_ALL,
    ID_DOCKING_HIDE_ALL,
    ID_DOCKING_TOGGLE_AUTOHIDE
};

DockingTestFrame::DockingTestFrame()
    : wxFrame(nullptr, wxID_ANY, "Advanced Docking System Test", 
              wxDefaultPosition, wxSize(1200, 800))
    , m_editorCounter(1)
{
    // Set frame icon
    SetIcon(wxICON(sample));
    
    // Create UI components
    CreateMenuBar();
    CreateToolBar();
    CreateStatusBar();
    CreateDockingSystem();
    
    // Center on screen
    Centre();
}

DockingTestFrame::~DockingTestFrame() {
    // DockManager will be deleted by its parent
}

void DockingTestFrame::CreateMenuBar() {
    wxMenuBar* menuBar = new wxMenuBar();
    
    // File menu
    wxMenu* fileMenu = new wxMenu();
    fileMenu->Append(wxID_NEW, "&New\tCtrl+N", "Create new file");
    fileMenu->Append(wxID_OPEN, "&Open\tCtrl+O", "Open file");
    fileMenu->Append(wxID_SAVE, "&Save\tCtrl+S", "Save file");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4", "Exit application");
    
    // View menu
    wxMenu* viewMenu = new wxMenu();
    viewMenu->Append(ID_VIEW_SAVE_LAYOUT, "Save &Layout\tCtrl+L", "Save current layout");
    viewMenu->Append(ID_VIEW_LOAD_LAYOUT, "Load L&ayout\tCtrl+Shift+L", "Load saved layout");
    viewMenu->Append(ID_VIEW_RESET_LAYOUT, "&Reset Layout", "Reset to default layout");
    viewMenu->AppendSeparator();
    viewMenu->Append(ID_VIEW_PERSPECTIVES, "&Manage Perspectives...", "Manage saved perspectives");
    
    // Docking menu
    wxMenu* dockingMenu = new wxMenu();
    dockingMenu->Append(ID_DOCKING_ADD_EDITOR, "Add &Editor\tCtrl+E", "Add new editor window");
    dockingMenu->Append(ID_DOCKING_ADD_TOOL, "Add &Tool Window\tCtrl+T", "Add new tool window");
    dockingMenu->AppendSeparator();
    dockingMenu->Append(ID_DOCKING_SHOW_ALL, "&Show All", "Show all dock widgets");
    dockingMenu->Append(ID_DOCKING_HIDE_ALL, "&Hide All", "Hide all dock widgets");
    dockingMenu->AppendSeparator();
    dockingMenu->Append(ID_DOCKING_TOGGLE_AUTOHIDE, "Toggle &Auto-hide", "Toggle auto-hide for current widget");
    
    // Help menu
    wxMenu* helpMenu = new wxMenu();
    helpMenu->Append(wxID_ABOUT, "&About\tF1", "About this application");
    
    // Add menus to menubar
    menuBar->Append(fileMenu, "&File");
    menuBar->Append(viewMenu, "&View");
    menuBar->Append(dockingMenu, "&Docking");
    menuBar->Append(helpMenu, "&Help");
    
    SetMenuBar(menuBar);
}

void DockingTestFrame::CreateToolBar() {
    wxToolBar* toolBar = CreateToolBar(wxTB_FLAT | wxTB_HORIZONTAL);
    
    // Add toolbar buttons
    toolBar->AddTool(wxID_NEW, "New", wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR), "New file");
    toolBar->AddTool(wxID_OPEN, "Open", wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR), "Open file");
    toolBar->AddTool(wxID_SAVE, "Save", wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR), "Save file");
    toolBar->AddSeparator();
    
    toolBar->AddTool(ID_DOCKING_ADD_EDITOR, "Add Editor", 
                     wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_TOOLBAR), "Add editor");
    toolBar->AddTool(ID_DOCKING_ADD_TOOL, "Add Tool", 
                     wxArtProvider::GetBitmap(wxART_LIST_VIEW, wxART_TOOLBAR), "Add tool window");
    
    toolBar->Realize();
}

void DockingTestFrame::CreateStatusBar() {
    CreateStatusBar(3);
    SetStatusText("Ready", 0);
    SetStatusText("Line: 1, Col: 1", 1);
    SetStatusText("INS", 2);
}

void DockingTestFrame::CreateDockingSystem() {
    // Create dock manager
    m_dockManager = new DockManager(this);
    
    // Configure dock manager
    m_dockManager->setConfigFlag(DockManager::OpaqueSplitterResize, true);
    m_dockManager->setConfigFlag(DockManager::DragPreviewIsDynamic, true);
    m_dockManager->setConfigFlag(DockManager::DragPreviewShowsContentPixmap, true);
    m_dockManager->setConfigFlag(DockManager::DragPreviewHasWindowFrame, true);
    
    // Create initial dock widgets
    TestBasicDocking();
}

DockWidget* DockingTestFrame::CreateEditorWidget(const wxString& title, int id) {
    DockWidget* dockWidget = new DockWidget(title, m_dockManager);
    
    EditorWidget* editor = new EditorWidget(dockWidget, id);
    dockWidget->setWidget(editor);
    dockWidget->setFeature(DockWidget::DockWidgetClosable, true);
    dockWidget->setFeature(DockWidget::DockWidgetMovable, true);
    dockWidget->setFeature(DockWidget::DockWidgetFloatable, true);
    dockWidget->setIcon(wxArtProvider::GetIcon(wxART_NORMAL_FILE, wxART_MENU));
    
    return dockWidget;
}

DockWidget* DockingTestFrame::CreateTreeWidget(const wxString& title) {
    DockWidget* dockWidget = new DockWidget(title, m_dockManager);
    
    wxTreeCtrl* tree = new wxTreeCtrl(dockWidget, wxID_ANY, 
                                      wxDefaultPosition, wxDefaultSize,
                                      wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT);
    
    // Populate tree with sample data
    wxTreeItemId root = tree->AddRoot("Root");
    wxTreeItemId proj = tree->AppendItem(root, "Project");
    tree->AppendItem(proj, "Source Files");
    tree->AppendItem(proj, "Header Files");
    tree->AppendItem(proj, "Resources");
    tree->Expand(proj);
    
    dockWidget->setWidget(tree);
    dockWidget->setFeature(DockWidget::DockWidgetClosable, true);
    dockWidget->setIcon(wxArtProvider::GetIcon(wxART_FOLDER, wxART_MENU));
    
    return dockWidget;
}

DockWidget* DockingTestFrame::CreateListWidget(const wxString& title) {
    DockWidget* dockWidget = new DockWidget(title, m_dockManager);
    
    wxListCtrl* list = new wxListCtrl(dockWidget, wxID_ANY,
                                      wxDefaultPosition, wxDefaultSize,
                                      wxLC_REPORT | wxLC_SINGLE_SEL);
    
    // Add columns
    list->AppendColumn("Name", wxLIST_FORMAT_LEFT, 150);
    list->AppendColumn("Size", wxLIST_FORMAT_RIGHT, 80);
    list->AppendColumn("Modified", wxLIST_FORMAT_LEFT, 120);
    
    // Add sample items
    long item = list->InsertItem(0, "file1.cpp");
    list->SetItem(item, 1, "12 KB");
    list->SetItem(item, 2, "2024-01-15 10:30");
    
    item = list->InsertItem(1, "file2.h");
    list->SetItem(item, 1, "3 KB");
    list->SetItem(item, 2, "2024-01-15 11:45");
    
    dockWidget->setWidget(list);
    dockWidget->setFeature(DockWidget::DockWidgetClosable, true);
    dockWidget->setIcon(wxArtProvider::GetIcon(wxART_LIST_VIEW, wxART_MENU));
    
    return dockWidget;
}

DockWidget* DockingTestFrame::CreatePropertyGridWidget(const wxString& title) {
    DockWidget* dockWidget = new DockWidget(title, m_dockManager);
    
    PropertyWidget* propWidget = new PropertyWidget(dockWidget);
    propWidget->PopulateProperties();
    
    dockWidget->setWidget(propWidget);
    dockWidget->setFeature(DockWidget::DockWidgetClosable, true);
    dockWidget->setIcon(wxArtProvider::GetIcon(wxART_REPORT_VIEW, wxART_MENU));
    
    return dockWidget;
}

DockWidget* DockingTestFrame::CreateOutputWidget(const wxString& title) {
    DockWidget* dockWidget = new DockWidget(title, m_dockManager);
    
    wxTextCtrl* output = new wxTextCtrl(dockWidget, wxID_ANY, wxEmptyString,
                                        wxDefaultPosition, wxDefaultSize,
                                        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    
    // Add some sample output
    output->SetDefaultStyle(wxTextAttr(*wxBLACK));
    output->AppendText("Build started...\n");
    output->SetDefaultStyle(wxTextAttr(*wxBLUE));
    output->AppendText("Compiling: main.cpp\n");
    output->AppendText("Compiling: utils.cpp\n");
    output->SetDefaultStyle(wxTextAttr(*wxGREEN));
    output->AppendText("Linking...\n");
    output->AppendText("Build succeeded.\n");
    
    dockWidget->setWidget(output);
    dockWidget->setFeature(DockWidget::DockWidgetClosable, true);
    dockWidget->setIcon(wxArtProvider::GetIcon(wxART_INFORMATION, wxART_MENU));
    
    return dockWidget;
}

DockWidget* DockingTestFrame::CreateToolboxWidget(const wxString& title) {
    DockWidget* dockWidget = new DockWidget(title, m_dockManager);
    
    wxPanel* panel = new wxPanel(dockWidget);
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Add tool buttons
    wxButton* btn1 = new wxButton(panel, wxID_ANY, "Select");
    wxButton* btn2 = new wxButton(panel, wxID_ANY, "Move");
    wxButton* btn3 = new wxButton(panel, wxID_ANY, "Rotate");
    wxButton* btn4 = new wxButton(panel, wxID_ANY, "Scale");
    
    sizer->Add(btn1, 0, wxEXPAND | wxALL, 2);
    sizer->Add(btn2, 0, wxEXPAND | wxALL, 2);
    sizer->Add(btn3, 0, wxEXPAND | wxALL, 2);
    sizer->Add(btn4, 0, wxEXPAND | wxALL, 2);
    sizer->AddStretchSpacer();
    
    panel->SetSizer(sizer);
    
    dockWidget->setWidget(panel);
    dockWidget->setFeature(DockWidget::DockWidgetClosable, true);
    dockWidget->setIcon(wxArtProvider::GetIcon(wxART_EXECUTABLE_FILE, wxART_MENU));
    
    return dockWidget;
}

void DockingTestFrame::TestBasicDocking() {
    // Create main editor in center
    DockWidget* editor1 = CreateEditorWidget("Editor 1", 1);
    m_dockManager->addDockWidget(CenterDockWidgetArea, editor1);
    
    // Create project explorer on left
    DockWidget* projectTree = CreateTreeWidget("Project Explorer");
    m_dockManager->addDockWidget(LeftDockWidgetArea, projectTree);
    
    // Create file browser below project explorer
    DockWidget* fileList = CreateListWidget("File Browser");
    m_dockManager->addDockWidget(BottomDockWidgetArea, fileList, projectTree->dockAreaWidget());
    
    // Create properties on right
    DockWidget* properties = CreatePropertyGridWidget("Properties");
    m_dockManager->addDockWidget(RightDockWidgetArea, properties);
    
    // Create output window at bottom
    DockWidget* output = CreateOutputWidget("Output");
    m_dockManager->addDockWidget(BottomDockWidgetArea, output);
    
    // Create toolbox and add as tab to properties
    DockWidget* toolbox = CreateToolboxWidget("Toolbox");
    m_dockManager->addDockWidget(CenterDockWidgetArea, toolbox, properties->dockAreaWidget());
}

// Menu event handlers
void DockingTestFrame::OnFileNew(wxCommandEvent& event) {
    wxString title = wxString::Format("Editor %d", m_editorCounter++);
    DockWidget* editor = CreateEditorWidget(title, m_editorCounter);
    m_dockManager->addDockWidget(CenterDockWidgetArea, editor);
}

void DockingTestFrame::OnFileOpen(wxCommandEvent& event) {
    wxFileDialog dlg(this, "Open File", "", "",
                     "All files (*.*)|*.*|C++ files (*.cpp;*.h)|*.cpp;*.h",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (dlg.ShowModal() == wxID_OK) {
        wxString path = dlg.GetPath();
        wxString filename = dlg.GetFilename();
        
        DockWidget* editor = CreateEditorWidget(filename, m_editorCounter++);
        EditorWidget* editorCtrl = static_cast<EditorWidget*>(editor->widget());
        editorCtrl->LoadFile(path);
        
        m_dockManager->addDockWidget(CenterDockWidgetArea, editor);
    }
}

void DockingTestFrame::OnFileSave(wxCommandEvent& event) {
    // Find active editor and save
    SetStatusText("Save not implemented in demo", 0);
}

void DockingTestFrame::OnFileExit(wxCommandEvent& event) {
    Close(true);
}

void DockingTestFrame::OnViewSaveLayout(wxCommandEvent& event) {
    wxString layout;
    m_dockManager->saveState(layout);
    
    wxFileDialog dlg(this, "Save Layout", "", "layout.xml",
                     "XML files (*.xml)|*.xml",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dlg.ShowModal() == wxID_OK) {
        wxFile file(dlg.GetPath(), wxFile::write);
        if (file.IsOpened()) {
            file.Write(layout);
            file.Close();
            SetStatusText("Layout saved", 0);
        }
    }
}

void DockingTestFrame::OnViewLoadLayout(wxCommandEvent& event) {
    wxFileDialog dlg(this, "Load Layout", "", "",
                     "XML files (*.xml)|*.xml",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    
    if (dlg.ShowModal() == wxID_OK) {
        wxFile file(dlg.GetPath(), wxFile::read);
        if (file.IsOpened()) {
            wxString layout;
            file.ReadAll(&layout);
            file.Close();
            
            if (m_dockManager->restoreState(layout)) {
                SetStatusText("Layout loaded", 0);
            } else {
                wxMessageBox("Failed to load layout", "Error", wxOK | wxICON_ERROR);
            }
        }
    }
}

void DockingTestFrame::OnViewResetLayout(wxCommandEvent& event) {
    // Clear current layout
    m_dockManager->hideManagerAndFloatingContainers();
    
    // Recreate default layout
    TestBasicDocking();
    SetStatusText("Layout reset to default", 0);
}

void DockingTestFrame::OnViewManagePerspectives(wxCommandEvent& event) {
    PerspectiveDialog dlg(this, m_dockManager->perspectiveManager());
    dlg.ShowModal();
}

void DockingTestFrame::OnDockingAddEditor(wxCommandEvent& event) {
    OnFileNew(event);
}

void DockingTestFrame::OnDockingAddTool(wxCommandEvent& event) {
    static int toolCounter = 1;
    wxString title = wxString::Format("Tool Window %d", toolCounter++);
    
    DockWidget* tool = CreateListWidget(title);
    m_dockManager->addDockWidget(LeftDockWidgetArea, tool);
}

void DockingTestFrame::OnDockingShowAll(wxCommandEvent& event) {
    auto widgets = m_dockManager->dockWidgets();
    for (auto* widget : widgets) {
        widget->setVisible(true);
    }
    SetStatusText("All widgets shown", 0);
}

void DockingTestFrame::OnDockingHideAll(wxCommandEvent& event) {
    auto widgets = m_dockManager->dockWidgets();
    for (auto* widget : widgets) {
        widget->setVisible(false);
    }
    SetStatusText("All widgets hidden", 0);
}

void DockingTestFrame::OnDockingToggleAutoHide(wxCommandEvent& event) {
    // Get current focused dock widget
    auto widgets = m_dockManager->dockWidgets();
    for (auto* widget : widgets) {
        if (widget->isCurrentTab()) {
            bool isAutoHide = widget->isAutoHide();
            widget->setAutoHide(!isAutoHide);
            SetStatusText(isAutoHide ? "Auto-hide disabled" : "Auto-hide enabled", 0);
            break;
        }
    }
}

void DockingTestFrame::OnHelpAbout(wxCommandEvent& event) {
    wxMessageBox("Advanced Docking System Test Application\n\n"
                 "This application demonstrates the features of the wxWidgets\n"
                 "port of the Qt Advanced Docking System.\n\n"
                 "Features:\n"
                 "- Dockable windows\n"
                 "- Tabbed docking\n"
                 "- Floating windows\n"
                 "- Auto-hide functionality\n"
                 "- Perspectives\n"
                 "- Splitter-based layout\n"
                 "- State persistence",
                 "About Docking Test",
                 wxOK | wxICON_INFORMATION);
}

// EditorWidget implementation
EditorWidget::EditorWidget(wxWindow* parent, int id)
    : wxStyledTextCtrl(parent, id)
{
    SetupStyling();
}

void EditorWidget::SetupStyling() {
    // Set up basic styling for C++ syntax
    StyleClearAll();
    SetLexer(wxSTC_LEX_CPP);
    
    // Set some style colors
    StyleSetForeground(wxSTC_C_COMMENT, wxColour(0, 128, 0));
    StyleSetForeground(wxSTC_C_COMMENTLINE, wxColour(0, 128, 0));
    StyleSetForeground(wxSTC_C_COMMENTDOC, wxColour(0, 128, 0));
    StyleSetForeground(wxSTC_C_STRING, wxColour(128, 0, 0));
    StyleSetForeground(wxSTC_C_CHARACTER, wxColour(128, 0, 0));
    StyleSetForeground(wxSTC_C_WORD, wxColour(0, 0, 255));
    StyleSetForeground(wxSTC_C_WORD2, wxColour(128, 0, 255));
    StyleSetForeground(wxSTC_C_NUMBER, wxColour(0, 128, 128));
    StyleSetForeground(wxSTC_C_OPERATOR, wxColour(0, 0, 0));
    
    // Set keywords
    SetKeyWords(0, "if else switch case default break continue return "
                   "while for do goto class struct union enum typedef "
                   "public private protected virtual friend inline "
                   "const static extern auto register volatile");
    
    SetKeyWords(1, "void bool char short int long float double "
                   "signed unsigned namespace using template typename");
    
    // Set up margins
    SetMarginType(0, wxSTC_MARGIN_NUMBER);
    SetMarginWidth(0, 50);
    
    // Set default text
    SetText("// Welcome to the Advanced Docking System Test\n"
            "#include <iostream>\n\n"
            "int main() {\n"
            "    std::cout << \"Hello, Docking!\" << std::endl;\n"
            "    return 0;\n"
            "}\n");
}

void EditorWidget::LoadFile(const wxString& filename) {
    wxFile file(filename, wxFile::read);
    if (file.IsOpened()) {
        wxString content;
        file.ReadAll(&content);
        SetText(content);
        file.Close();
    }
}

void EditorWidget::SaveFile(const wxString& filename) {
    wxFile file(filename, wxFile::write);
    if (file.IsOpened()) {
        file.Write(GetText());
        file.Close();
    }
}

// PropertyWidget implementation
PropertyWidget::PropertyWidget(wxWindow* parent)
    : wxPanel(parent)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    m_list = new wxListCtrl(this, wxID_ANY,
                           wxDefaultPosition, wxDefaultSize,
                           wxLC_REPORT | wxLC_SINGLE_SEL);
    
    m_list->AppendColumn("Property", wxLIST_FORMAT_LEFT, 120);
    m_list->AppendColumn("Value", wxLIST_FORMAT_LEFT, 150);
    
    sizer->Add(m_list, 1, wxEXPAND);
    SetSizer(sizer);
}

void PropertyWidget::PopulateProperties() {
    // Add sample properties
    long item = m_list->InsertItem(0, "Name");
    m_list->SetItem(item, 1, "DockWidget1");
    
    item = m_list->InsertItem(1, "Type");
    m_list->SetItem(item, 1, "Editor");
    
    item = m_list->InsertItem(2, "Visible");
    m_list->SetItem(item, 1, "True");
    
    item = m_list->InsertItem(3, "Docked");
    m_list->SetItem(item, 1, "True");
    
    item = m_list->InsertItem(4, "Size");
    m_list->SetItem(item, 1, "800x600");
    
    item = m_list->InsertItem(5, "Features");
    m_list->SetItem(item, 1, "Closable, Movable, Floatable");
}

} // namespace ads
