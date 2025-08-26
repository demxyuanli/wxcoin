#include "docking/DockingExample.h"
#include "docking/DockWidget.h"
#include "docking/DockingIntegration.h"
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include <wx/textctrl.h>
#include <wx/splitter.h>

namespace ads {

// Event IDs
enum {
    ID_SaveLayout = wxID_HIGHEST + 1,
    ID_RestoreLayout,
    ID_ResetLayout,
    ID_AddExampleWidgets
};

// Event table
wxBEGIN_EVENT_TABLE(DockingExampleFrame, wxFrame)
    EVT_MENU(wxID_EXIT, DockingExampleFrame::OnExit)
    EVT_MENU(wxID_ABOUT, DockingExampleFrame::OnAbout)
    EVT_MENU(ID_SaveLayout, DockingExampleFrame::OnSaveLayout)
    EVT_MENU(ID_RestoreLayout, DockingExampleFrame::OnRestoreLayout)
    EVT_MENU(ID_ResetLayout, DockingExampleFrame::OnResetLayout)
    EVT_MENU(ID_AddExampleWidgets, DockingExampleFrame::OnAddExampleWidgets)
wxEND_EVENT_TABLE()

DockingExampleFrame::DockingExampleFrame()
    : wxFrame(nullptr, wxID_ANY, "Qt-Advanced-Docking-System Example",
             wxDefaultPosition, wxSize(1200, 800))
    , m_dockManager(nullptr)
{
    // Set minimum size
    SetMinSize(wxSize(800, 600));
    
    // Create menu bar
    CreateMenuBar();
    
    // Create status bar
    CreateStatusBar();
    SetStatusText("Welcome to Qt-Advanced-Docking-System Example!");
    
    // Create docking layout
    CreateDockingLayout();
    
    // Center on screen
    Centre();
}

DockingExampleFrame::~DockingExampleFrame() {
    // DockManager will be deleted by parent window
}

void DockingExampleFrame::CreateMenuBar() {
    wxMenuBar* menuBar = new wxMenuBar;
    
    // File menu
    wxMenu* fileMenu = new wxMenu;
    fileMenu->Append(wxID_NEW, "&New\tCtrl+N", "Create a new file");
    fileMenu->Append(wxID_OPEN, "&Open\tCtrl+O", "Open a file");
    fileMenu->Append(wxID_SAVE, "&Save\tCtrl+S", "Save the file");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4", "Exit the application");
    menuBar->Append(fileMenu, "&File");
    
    // View menu
    wxMenu* viewMenu = new wxMenu;
    viewMenu->Append(ID_SaveLayout, "Save &Layout", "Save current dock layout");
    viewMenu->Append(ID_RestoreLayout, "&Restore Layout", "Restore saved dock layout");
    viewMenu->Append(ID_ResetLayout, "Reset Layout to &Default", "Reset to default layout");
    viewMenu->AppendSeparator();
    viewMenu->Append(ID_AddExampleWidgets, "Add &Example Widgets", "Add more example dock widgets");
    menuBar->Append(viewMenu, "&View");
    
    // Help menu
    wxMenu* helpMenu = new wxMenu;
    helpMenu->Append(wxID_ABOUT, "&About\tF1", "Show about dialog");
    menuBar->Append(helpMenu, "&Help");
    
    SetMenuBar(menuBar);
}

void DockingExampleFrame::CreateDockingLayout() {
    // Create main sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create dock manager
    m_dockManager = new DockManager(this);
    
    // Configure dock manager features
    m_dockManager->setConfigFlag(OpaqueSplitterResize, true);
    m_dockManager->setConfigFlag(XmlAutoFormattingEnabled, true);
    m_dockManager->setConfigFlag(AlwaysShowTabs, false);
    m_dockManager->setConfigFlag(AllTabsHaveCloseButton, true);
    m_dockManager->setConfigFlag(DockAreaHasCloseButton, true);
    m_dockManager->setConfigFlag(FocusHighlighting, true);
    
    // Add dock manager container to sizer
    mainSizer->Add(m_dockManager->containerWidget(), 1, wxEXPAND);
    
    // Create default widgets
    CreateDefaultWidgets();
    
    // Add view menu items for dock widgets
    wxMenuBar* menuBar = GetMenuBar();
    if (menuBar) {
        wxMenu* viewMenu = menuBar->GetMenu(1); // View menu is second
        if (viewMenu) {
            viewMenu->AppendSeparator();
            DockingIntegration::SetupViewMenu(viewMenu, m_dockManager);
        }
    }
    
    SetSizer(mainSizer);
}

void DockingExampleFrame::CreateDefaultWidgets() {
    // Create main editor
    wxTextCtrl* mainEditor = CreateTextEditor("main.cpp");
    mainEditor->SetValue(
        "#include <iostream>\n\n"
        "int main() {\n"
        "    std::cout << \"Hello, Docking System!\" << std::endl;\n"
        "    return 0;\n"
        "}\n"
    );
    
    DockWidget* editorWidget = new DockWidget("Editor - main.cpp");
    editorWidget->setWidget(mainEditor);
    editorWidget->setObjectName("MainEditor");
    
    // Create project tree
    wxTreeCtrl* projectTree = CreateProjectTree();
    DockWidget* projectWidget = new DockWidget("Project Explorer");
    projectWidget->setWidget(projectTree);
    projectWidget->setObjectName("ProjectExplorer");
    
    // Create file list
    wxListCtrl* fileList = CreateFileList();
    DockWidget* filesWidget = new DockWidget("File Browser");
    filesWidget->setWidget(fileList);
    filesWidget->setObjectName("FileBrowser");
    
    // Create output window
    wxTextCtrl* output = new wxTextCtrl(m_dockManager->containerWidget(), wxID_ANY,
        "Build output will appear here...\n",
        wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    output->SetBackgroundColour(wxColour(40, 40, 40));
    output->SetForegroundColour(wxColour(200, 200, 200));
    
    DockWidget* outputWidget = new DockWidget("Output");
    outputWidget->setWidget(output);
    outputWidget->setObjectName("Output");
    
    // Create tool panel
    wxPanel* toolPanel = CreateToolPanel();
    DockWidget* toolWidget = new DockWidget("Toolbox");
    toolWidget->setWidget(toolPanel);
    toolWidget->setObjectName("Toolbox");
    
    // Add widgets to dock manager
    m_dockManager->addDockWidget(CenterDockWidgetArea, editorWidget);
    
    DockArea* leftArea = m_dockManager->addDockWidget(LeftDockWidgetArea, projectWidget);
    m_dockManager->addDockWidgetTabToArea(filesWidget, leftArea);
    
    m_dockManager->addDockWidget(BottomDockWidgetArea, outputWidget);
    m_dockManager->addDockWidget(RightDockWidgetArea, toolWidget);
}

wxTextCtrl* DockingExampleFrame::CreateTextEditor(const wxString& title) {
    wxTextCtrl* editor = new wxTextCtrl(m_dockManager->containerWidget(), wxID_ANY,
        "", wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxTE_RICH2);
    
    // Set monospace font
    wxFont font = editor->GetFont();
    font.SetFamily(wxFONTFAMILY_TELETYPE);
    editor->SetFont(font);
    
    return editor;
}

wxListCtrl* DockingExampleFrame::CreateFileList() {
    wxListCtrl* list = new wxListCtrl(m_dockManager->containerWidget(), wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxLC_REPORT | wxLC_SINGLE_SEL);
    
    // Add columns
    list->AppendColumn("Name", wxLIST_FORMAT_LEFT, 200);
    list->AppendColumn("Size", wxLIST_FORMAT_RIGHT, 80);
    list->AppendColumn("Modified", wxLIST_FORMAT_LEFT, 150);
    
    // Add some sample files
    long item = list->InsertItem(0, "main.cpp");
    list->SetItem(item, 1, "1.2 KB");
    list->SetItem(item, 2, "2024-01-15 10:30");
    
    item = list->InsertItem(1, "utils.h");
    list->SetItem(item, 1, "3.5 KB");
    list->SetItem(item, 2, "2024-01-15 09:45");
    
    item = list->InsertItem(2, "config.json");
    list->SetItem(item, 1, "0.8 KB");
    list->SetItem(item, 2, "2024-01-14 16:20");
    
    return list;
}

wxTreeCtrl* DockingExampleFrame::CreateProjectTree() {
    wxTreeCtrl* tree = new wxTreeCtrl(m_dockManager->containerWidget(), wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT);
    
    // Create sample project structure
    wxTreeItemId root = tree->AddRoot("Project");
    wxTreeItemId src = tree->AppendItem(root, "src");
    tree->AppendItem(src, "main.cpp");
    tree->AppendItem(src, "utils.cpp");
    tree->AppendItem(src, "utils.h");
    
    wxTreeItemId include = tree->AppendItem(root, "include");
    tree->AppendItem(include, "config.h");
    tree->AppendItem(include, "types.h");
    
    wxTreeItemId resources = tree->AppendItem(root, "resources");
    tree->AppendItem(resources, "icon.png");
    tree->AppendItem(resources, "style.css");
    
    tree->AppendItem(root, "CMakeLists.txt");
    tree->AppendItem(root, "README.md");
    
    // Expand all
    tree->ExpandAll();
    
    return tree;
}

wxPanel* DockingExampleFrame::CreateToolPanel() {
    wxPanel* panel = new wxPanel(m_dockManager->containerWidget());
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Add some tool buttons
    wxButton* buildBtn = new wxButton(panel, wxID_ANY, "Build");
    wxButton* runBtn = new wxButton(panel, wxID_ANY, "Run");
    wxButton* debugBtn = new wxButton(panel, wxID_ANY, "Debug");
    wxButton* profileBtn = new wxButton(panel, wxID_ANY, "Profile");
    
    sizer->Add(new wxStaticText(panel, wxID_ANY, "Build Tools:"), 0, wxALL, 5);
    sizer->Add(buildBtn, 0, wxEXPAND | wxALL, 5);
    sizer->Add(runBtn, 0, wxEXPAND | wxALL, 5);
    sizer->Add(debugBtn, 0, wxEXPAND | wxALL, 5);
    sizer->Add(profileBtn, 0, wxEXPAND | wxALL, 5);
    sizer->AddStretchSpacer();
    
    panel->SetSizer(sizer);
    return panel;
}

void DockingExampleFrame::OnExit(wxCommandEvent& event) {
    Close(true);
}

void DockingExampleFrame::OnAbout(wxCommandEvent& event) {
    wxMessageBox("Qt-Advanced-Docking-System Example\n\n"
                "This example demonstrates the features of the docking system:\n"
                "- Drag and drop dock widgets\n"
                "- Tab multiple widgets together\n"
                "- Create floating windows\n"
                "- Save and restore layouts\n"
                "- Customizable dock behavior",
                "About Docking System Example",
                wxOK | wxICON_INFORMATION);
}

void DockingExampleFrame::OnSaveLayout(wxCommandEvent& event) {
    m_dockManager->saveState(m_savedLayout);
    SetStatusText("Layout saved!");
}

void DockingExampleFrame::OnRestoreLayout(wxCommandEvent& event) {
    if (m_savedLayout.IsEmpty()) {
        wxMessageBox("No saved layout to restore!", "Restore Layout", 
                    wxOK | wxICON_WARNING);
        return;
    }
    
    m_dockManager->restoreState(m_savedLayout);
    SetStatusText("Layout restored!");
}

void DockingExampleFrame::OnResetLayout(wxCommandEvent& event) {
    // Clear all dock widgets and recreate default layout
    std::vector<DockWidget*> widgets = m_dockManager->dockWidgets();
    for (auto* widget : widgets) {
        widget->deleteDockWidget();
    }
    
    CreateDefaultWidgets();
    SetStatusText("Layout reset to default!");
}

void DockingExampleFrame::OnAddExampleWidgets(wxCommandEvent& event) {
    DockingIntegration::CreateExampleDockWidgets(m_dockManager);
    SetStatusText("Example widgets added!");
}

// Application implementation
bool DockingExampleApp::OnInit() {
    // Create and show the main frame
    DockingExampleFrame* frame = new DockingExampleFrame();
    frame->Show(true);
    
    return true;
}

} // namespace ads

// Implement the application
wxIMPLEMENT_APP(ads::DockingExampleApp);