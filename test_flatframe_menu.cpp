// 测试 FlatFrameDocking 菜单功能
#include <wx/wx.h>
#include <wx/artprov.h>

// 创建一个简化的测试程序来验证菜单
class TestFrame : public wxFrame {
public:
    TestFrame() : wxFrame(nullptr, wxID_ANY, "FlatFrameDocking Menu Test", 
                         wxDefaultPosition, wxSize(800, 600)) {
        
        // 创建菜单栏
        wxMenuBar* menuBar = new wxMenuBar();
        
        // File 菜单
        wxMenu* fileMenu = new wxMenu();
        fileMenu->Append(wxID_EXIT, "E&xit\tCtrl+Q");
        menuBar->Append(fileMenu, "&File");
        
        // View 菜单
        wxMenu* viewMenu = new wxMenu();
        
        // 面板可见性选项
        viewMenu->AppendCheckItem(wxID_ANY, "Object Tree\tCtrl+Alt+O", "Show/hide object tree panel");
        viewMenu->AppendCheckItem(wxID_ANY, "Properties\tCtrl+Alt+P", "Show/hide properties panel");
        viewMenu->AppendCheckItem(wxID_ANY, "Message\tCtrl+Alt+M", "Show/hide message output panel");
        viewMenu->AppendCheckItem(wxID_ANY, "Performance\tCtrl+Alt+F", "Show/hide performance monitor panel");
        
        viewMenu->AppendSeparator();
        
        // Docking 菜单项
        viewMenu->Append(wxID_ANY, "Save &Layout...\tCtrl+L", "Save current docking layout");
        viewMenu->Append(wxID_ANY, "Load L&ayout...\tCtrl+Shift+L", "Load saved docking layout");
        viewMenu->Append(wxID_ANY, "&Reset Layout", "Reset to default docking layout");
        viewMenu->AppendSeparator();
        viewMenu->Append(wxID_ANY, "&Manage Perspectives...", "Manage saved layout perspectives");
        viewMenu->Append(wxID_ANY, "Toggle &Auto-hide\tCtrl+H", "Toggle auto-hide for current panel");
        viewMenu->AppendSeparator();
        
        // 配置布局菜单项
        int configureId = wxNewId();
        viewMenu->Append(configureId, "&Configure Layout...", "Configure dock panel sizes and layout");
        
        menuBar->Append(viewMenu, "&View");
        
        SetMenuBar(menuBar);
        
        // 创建主面板
        wxPanel* panel = new wxPanel(this);
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        
        wxStaticText* label = new wxStaticText(panel, wxID_ANY, 
            "This is a test to verify the menu structure.\n"
            "Check the View menu for 'Configure Layout...' option.");
        sizer->Add(label, 0, wxALL | wxCENTER, 20);
        
        panel->SetSizer(sizer);
        
        // 绑定事件
        Bind(wxEVT_MENU, [](wxCommandEvent&) { wxTheApp->Exit(); }, wxID_EXIT);
        
        Bind(wxEVT_MENU, [this](wxCommandEvent&) {
            wxMessageBox("Configure Layout dialog would open here.\n\n"
                        "In the actual FlatFrameDocking, this opens the DockLayoutConfigDialog\n"
                        "which allows you to:\n"
                        "- Set dock panel sizes (pixels or percentages)\n"
                        "- Show/hide dock areas\n"
                        "- Preview the layout\n"
                        "- Use quick presets (20/80, 3-column, IDE layout)",
                        "Configure Layout", wxOK | wxICON_INFORMATION);
        }, configureId);
        
        // 设置状态栏
        CreateStatusBar();
        SetStatusText("Look for 'View -> Configure Layout...' menu item");
    }
};

class TestApp : public wxApp {
public:
    virtual bool OnInit() override {
        TestFrame* frame = new TestFrame();
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(TestApp);