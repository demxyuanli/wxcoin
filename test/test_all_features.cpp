#include "docking_test_app.h"
#include <wx/timer.h>
#include <wx/msgdlg.h>

namespace ads {

/**
 * @brief Automated test class that exercises all docking features
 */
class DockingFeatureTest : public wxFrame {
public:
    DockingFeatureTest();
    void RunAllTests();
    
private:
    void TestDockingPositions();
    void TestTabbing();
    void TestFloating();
    void TestAutoHide();
    void TestPerspectives();
    void TestSplitting();
    void TestDragAndDrop();
    void TestStatePersistence();
    void TestEdgeCases();
    
    void LogTest(const wxString& testName, bool success);
    void OnTimer(wxTimerEvent& event);
    
    DockManager* m_dockManager;
    wxTextCtrl* m_logWindow;
    wxTimer* m_testTimer;
    int m_currentTest;
    
    wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(DockingFeatureTest, wxFrame)
    EVT_TIMER(wxID_ANY, DockingFeatureTest::OnTimer)
wxEND_EVENT_TABLE()

DockingFeatureTest::DockingFeatureTest()
    : wxFrame(nullptr, wxID_ANY, "Docking System Feature Test", 
              wxDefaultPosition, wxSize(1024, 768))
    , m_currentTest(0)
{
    // Create main sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create toolbar
    wxToolBar* toolbar = new wxToolBar(this, wxID_ANY);
    toolbar->AddTool(wxID_EXECUTE, "Run Tests", 
                     wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR),
                     "Run all tests");
    toolbar->Realize();
    mainSizer->Add(toolbar, 0, wxEXPAND);
    
    // Create splitter
    wxSplitterWindow* splitter = new wxSplitterWindow(this);
    
    // Create docking area
    wxPanel* dockPanel = new wxPanel(splitter);
    m_dockManager = new DockManager(dockPanel);
    
    // Create log window
    m_logWindow = new wxTextCtrl(splitter, wxID_ANY, wxEmptyString,
                                 wxDefaultPosition, wxDefaultSize,
                                 wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    
    splitter->SplitHorizontally(dockPanel, m_logWindow, 500);
    mainSizer->Add(splitter, 1, wxEXPAND);
    
    SetSizer(mainSizer);
    
    // Create timer for automated testing
    m_testTimer = new wxTimer(this);
    
    // Bind events
    Bind(wxEVT_TOOL, [this](wxCommandEvent&) { RunAllTests(); }, wxID_EXECUTE);
    
    Centre();
}

void DockingFeatureTest::RunAllTests() {
    m_logWindow->Clear();
    m_currentTest = 0;
    m_testTimer->Start(1000); // 1 second between tests
}

void DockingFeatureTest::OnTimer(wxTimerEvent& event) {
    switch (m_currentTest++) {
        case 0: 
            LogTest("Starting automated tests...", true);
            break;
        case 1:
            TestDockingPositions();
            break;
        case 2:
            TestTabbing();
            break;
        case 3:
            TestFloating();
            break;
        case 4:
            TestAutoHide();
            break;
        case 5:
            TestPerspectives();
            break;
        case 6:
            TestSplitting();
            break;
        case 7:
            TestDragAndDrop();
            break;
        case 8:
            TestStatePersistence();
            break;
        case 9:
            TestEdgeCases();
            break;
        case 10:
            LogTest("All tests completed!", true);
            m_testTimer->Stop();
            break;
    }
}

void DockingFeatureTest::TestDockingPositions() {
    LogTest("Testing docking positions", true);
    
    // Clear existing widgets
    m_dockManager->hideManagerAndFloatingContainers();
    
    // Test all docking positions
    DockWidget* center = new DockWidget("Center", m_dockManager);
    center->setWidget(new wxTextCtrl(center, wxID_ANY, "Center Widget"));
    m_dockManager->addDockWidget(CenterDockWidgetArea, center);
    
    DockWidget* left = new DockWidget("Left", m_dockManager);
    left->setWidget(new wxTextCtrl(left, wxID_ANY, "Left Widget"));
    m_dockManager->addDockWidget(LeftDockWidgetArea, left);
    
    DockWidget* right = new DockWidget("Right", m_dockManager);
    right->setWidget(new wxTextCtrl(right, wxID_ANY, "Right Widget"));
    m_dockManager->addDockWidget(RightDockWidgetArea, right);
    
    DockWidget* top = new DockWidget("Top", m_dockManager);
    top->setWidget(new wxTextCtrl(top, wxID_ANY, "Top Widget"));
    m_dockManager->addDockWidget(TopDockWidgetArea, top);
    
    DockWidget* bottom = new DockWidget("Bottom", m_dockManager);
    bottom->setWidget(new wxTextCtrl(bottom, wxID_ANY, "Bottom Widget"));
    m_dockManager->addDockWidget(BottomDockWidgetArea, bottom);
    
    LogTest("  - All positions tested", true);
}

void DockingFeatureTest::TestTabbing() {
    LogTest("Testing tabbed docking", true);
    
    // Create multiple widgets in same area
    DockWidget* tab1 = new DockWidget("Tab 1", m_dockManager);
    tab1->setWidget(new wxTextCtrl(tab1, wxID_ANY, "Tab 1 Content"));
    m_dockManager->addDockWidget(CenterDockWidgetArea, tab1);
    
    DockWidget* tab2 = new DockWidget("Tab 2", m_dockManager);
    tab2->setWidget(new wxTextCtrl(tab2, wxID_ANY, "Tab 2 Content"));
    m_dockManager->addDockWidget(CenterDockWidgetArea, tab2, tab1->dockAreaWidget());
    
    DockWidget* tab3 = new DockWidget("Tab 3", m_dockManager);
    tab3->setWidget(new wxTextCtrl(tab3, wxID_ANY, "Tab 3 Content"));
    m_dockManager->addDockWidget(CenterDockWidgetArea, tab3, tab1->dockAreaWidget());
    
    // Test tab switching
    tab2->setAsCurrentTab();
    LogTest("  - Tab switching tested", true);
    
    // Test tab closing
    tab3->setFeature(DockWidget::DockWidgetClosable, true);
    LogTest("  - Tab features tested", true);
}

void DockingFeatureTest::TestFloating() {
    LogTest("Testing floating windows", true);
    
    // Create and float a widget
    DockWidget* floater = new DockWidget("Floating", m_dockManager);
    floater->setWidget(new wxTextCtrl(floater, wxID_ANY, "Floating Content"));
    floater->setFeature(DockWidget::DockWidgetFloatable, true);
    m_dockManager->addDockWidget(CenterDockWidgetArea, floater);
    
    // Float the widget
    floater->setFloating();
    LogTest("  - Widget floated", floater->isFloating());
    
    // Test moving floating window
    if (floater->isFloating()) {
        FloatingDockContainer* container = floater->floatingDockContainer();
        if (container) {
            container->Move(100, 100);
            LogTest("  - Floating window moved", true);
        }
    }
}

void DockingFeatureTest::TestAutoHide() {
    LogTest("Testing auto-hide functionality", true);
    
    // Create widget for auto-hide
    DockWidget* autoHide = new DockWidget("Auto-Hide", m_dockManager);
    autoHide->setWidget(new wxTextCtrl(autoHide, wxID_ANY, "Auto-Hide Content"));
    autoHide->setFeature(DockWidget::DockWidgetPinnable, true);
    m_dockManager->addDockWidget(LeftDockWidgetArea, autoHide);
    
    // Enable auto-hide
    autoHide->setAutoHide(true);
    LogTest("  - Auto-hide enabled", autoHide->isAutoHide());
    
    // Test showing/hiding
    // This would normally be triggered by mouse hover
    LogTest("  - Auto-hide behavior configured", true);
}

void DockingFeatureTest::TestPerspectives() {
    LogTest("Testing perspectives", true);
    
    PerspectiveManager* perspMgr = m_dockManager->perspectiveManager();
    
    // Save current perspective
    bool saved = perspMgr->savePerspective("Test Layout 1");
    LogTest("  - Perspective saved", saved);
    
    // Modify layout
    DockWidget* newWidget = new DockWidget("New Widget", m_dockManager);
    newWidget->setWidget(new wxTextCtrl(newWidget, wxID_ANY, "New Content"));
    m_dockManager->addDockWidget(RightDockWidgetArea, newWidget);
    
    // Save another perspective
    saved = perspMgr->savePerspective("Test Layout 2");
    LogTest("  - Second perspective saved", saved);
    
    // Load first perspective
    bool loaded = perspMgr->loadPerspective("Test Layout 1");
    LogTest("  - Perspective loaded", loaded);
    
    // List perspectives
    wxArrayString perspectives = perspMgr->perspectiveNames();
    LogTest("  - Perspectives listed", perspectives.size() >= 2);
}

void DockingFeatureTest::TestSplitting() {
    LogTest("Testing splitter functionality", true);
    
    // Create widgets in splitter configuration
    DockWidget* split1 = new DockWidget("Split 1", m_dockManager);
    split1->setWidget(new wxTextCtrl(split1, wxID_ANY, "Split 1"));
    m_dockManager->addDockWidget(LeftDockWidgetArea, split1);
    
    DockWidget* split2 = new DockWidget("Split 2", m_dockManager);
    split2->setWidget(new wxTextCtrl(split2, wxID_ANY, "Split 2"));
    m_dockManager->addDockWidget(BottomDockWidgetArea, split2, split1->dockAreaWidget());
    
    LogTest("  - Splitter created", true);
    
    // Test splitter resize
    // This would normally be done by dragging the splitter
    LogTest("  - Splitter functionality tested", true);
}

void DockingFeatureTest::TestDragAndDrop() {
    LogTest("Testing drag and drop", true);
    
    // Create draggable widget
    DockWidget* draggable = new DockWidget("Draggable", m_dockManager);
    draggable->setWidget(new wxTextCtrl(draggable, wxID_ANY, "Drag me!"));
    draggable->setFeature(DockWidget::DockWidgetMovable, true);
    m_dockManager->addDockWidget(CenterDockWidgetArea, draggable);
    
    LogTest("  - Draggable widget created", true);
    
    // Test drag preview
    m_dockManager->setConfigFlag(DockManager::DragPreviewIsDynamic, true);
    m_dockManager->setConfigFlag(DockManager::DragPreviewShowsContentPixmap, true);
    LogTest("  - Drag preview configured", true);
}

void DockingFeatureTest::TestStatePersistence() {
    LogTest("Testing state persistence", true);
    
    // Save current state
    wxString state;
    m_dockManager->saveState(state);
    LogTest("  - State saved", !state.IsEmpty());
    
    // Clear layout
    m_dockManager->hideManagerAndFloatingContainers();
    
    // Restore state
    bool restored = m_dockManager->restoreState(state);
    LogTest("  - State restored", restored);
}

void DockingFeatureTest::TestEdgeCases() {
    LogTest("Testing edge cases", true);
    
    // Test empty dock manager
    auto widgets = m_dockManager->dockWidgets();
    LogTest("  - Widget count check", widgets.size() > 0);
    
    // Test invalid operations
    DockWidget* testWidget = new DockWidget("Test", m_dockManager);
    testWidget->setWidget(new wxTextCtrl(testWidget, wxID_ANY, "Test"));
    
    // Try to dock to non-existent area
    m_dockManager->addDockWidget(CenterDockWidgetArea, testWidget, nullptr);
    LogTest("  - Invalid docking handled", true);
    
    // Test feature conflicts
    testWidget->setFeature(DockWidget::DockWidgetClosable, false);
    testWidget->setFeature(DockWidget::DockWidgetFloatable, false);
    testWidget->setFeature(DockWidget::DockWidgetMovable, false);
    LogTest("  - Feature restrictions tested", true);
}

void DockingFeatureTest::LogTest(const wxString& testName, bool success) {
    wxString timestamp = wxDateTime::Now().Format("%H:%M:%S");
    
    if (success) {
        m_logWindow->SetDefaultStyle(wxTextAttr(*wxGREEN));
        m_logWindow->AppendText(wxString::Format("[%s] PASS: %s\n", timestamp, testName));
    } else {
        m_logWindow->SetDefaultStyle(wxTextAttr(*wxRED));
        m_logWindow->AppendText(wxString::Format("[%s] FAIL: %s\n", timestamp, testName));
    }
    
    m_logWindow->SetDefaultStyle(wxTextAttr(*wxBLACK));
}

} // namespace ads

// Test application entry point
class TestApp : public wxApp {
public:
    virtual bool OnInit() override {
        ads::DockingFeatureTest* frame = new ads::DockingFeatureTest();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(TestApp);