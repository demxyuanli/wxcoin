/**
 * @brief Simple standalone test for the docking system
 * 
 * This is a minimal example showing how to use the docking system
 */

#include <wx/wx.h>
#include "docking/DockManager.h"
#include "docking/DockWidget.h"

using namespace ads;

class SimpleTestFrame : public wxFrame {
public:
    SimpleTestFrame() : wxFrame(nullptr, wxID_ANY, "Simple Docking Test", 
                               wxDefaultPosition, wxSize(800, 600)) {
        
        // Create dock manager
        m_dockManager = new DockManager(this);
        
        // Create a simple text editor widget
        DockWidget* editor = new DockWidget("Editor", m_dockManager);
        wxTextCtrl* textCtrl = new wxTextCtrl(editor, wxID_ANY, 
                                              "Type here...\n\nThis is a dockable editor.",
                                              wxDefaultPosition, wxDefaultSize,
                                              wxTE_MULTILINE);
        editor->setWidget(textCtrl);
        m_dockManager->addDockWidget(CenterDockWidgetArea, editor);
        
        // Create a tree view widget
        DockWidget* tree = new DockWidget("Project", m_dockManager);
        wxTreeCtrl* treeCtrl = new wxTreeCtrl(tree, wxID_ANY);
        wxTreeItemId root = treeCtrl->AddRoot("Project");
        treeCtrl->AppendItem(root, "src");
        treeCtrl->AppendItem(root, "include");
        treeCtrl->AppendItem(root, "test");
        treeCtrl->Expand(root);
        tree->setWidget(treeCtrl);
        m_dockManager->addDockWidget(LeftDockWidgetArea, tree);
        
        // Create an output window
        DockWidget* output = new DockWidget("Output", m_dockManager);
        wxTextCtrl* outputCtrl = new wxTextCtrl(output, wxID_ANY,
                                               "Program output will appear here...",
                                               wxDefaultPosition, wxDefaultSize,
                                               wxTE_MULTILINE | wxTE_READONLY);
        output->setWidget(outputCtrl);
        m_dockManager->addDockWidget(BottomDockWidgetArea, output);
        
        // Create status bar
        CreateStatusBar();
        SetStatusText("Ready");
        
        Centre();
    }
    
private:
    DockManager* m_dockManager;
};

class SimpleTestApp : public wxApp {
public:
    virtual bool OnInit() override {
        SimpleTestFrame* frame = new SimpleTestFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(SimpleTestApp);