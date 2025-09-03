// 轮廓渲染预览示例程序
// 展示如何使用增强版的轮廓渲染组件

#include <wx/app.h>
#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/msgdlg.h>
#include <wx/aboutdlg.h>

#include "ui/EnhancedOutlinePreviewCanvas.h"
#include "ui/EnhancedOutlineSettingsDialog.h"

#include <Inventor/SoDB.h>
#include <Inventor/SoInteraction.h>

// 主应用程序类
class OutlinePreviewApp : public wxApp {
public:
    virtual bool OnInit();
    virtual int OnExit();
};

// 主窗口类
class OutlinePreviewFrame : public wxFrame {
public:
    OutlinePreviewFrame(const wxString& title);
    
private:
    // 菜单事件处理
    void OnFileExit(wxCommandEvent& event);
    void OnViewSettings(wxCommandEvent& event);
    void OnViewReset(wxCommandEvent& event);
    void OnHelpAbout(wxCommandEvent& event);
    
    // 轮廓方法快速切换
    void OnMethodBasic(wxCommandEvent& event);
    void OnMethodInvertedHull(wxCommandEvent& event);
    void OnMethodScreenSpace(wxCommandEvent& event);
    void OnMethodGeometry(wxCommandEvent& event);
    void OnMethodStencil(wxCommandEvent& event);
    void OnMethodMultiPass(wxCommandEvent& event);
    
    // 创建菜单栏
    void CreateMenuBar();
    
private:
    EnhancedOutlinePreviewCanvas* m_canvas;
    EnhancedOutlineParams m_outlineParams;
    
    DECLARE_EVENT_TABLE()
};

// 事件ID定义
enum {
    ID_ViewSettings = wxID_HIGHEST + 1,
    ID_ViewReset,
    ID_MethodBasic,
    ID_MethodInvertedHull,
    ID_MethodScreenSpace,
    ID_MethodGeometry,
    ID_MethodStencil,
    ID_MethodMultiPass
};

// 事件表
BEGIN_EVENT_TABLE(OutlinePreviewFrame, wxFrame)
    EVT_MENU(wxID_EXIT, OutlinePreviewFrame::OnFileExit)
    EVT_MENU(ID_ViewSettings, OutlinePreviewFrame::OnViewSettings)
    EVT_MENU(ID_ViewReset, OutlinePreviewFrame::OnViewReset)
    EVT_MENU(wxID_ABOUT, OutlinePreviewFrame::OnHelpAbout)
    EVT_MENU(ID_MethodBasic, OutlinePreviewFrame::OnMethodBasic)
    EVT_MENU(ID_MethodInvertedHull, OutlinePreviewFrame::OnMethodInvertedHull)
    EVT_MENU(ID_MethodScreenSpace, OutlinePreviewFrame::OnMethodScreenSpace)
    EVT_MENU(ID_MethodGeometry, OutlinePreviewFrame::OnMethodGeometry)
    EVT_MENU(ID_MethodStencil, OutlinePreviewFrame::OnMethodStencil)
    EVT_MENU(ID_MethodMultiPass, OutlinePreviewFrame::OnMethodMultiPass)
END_EVENT_TABLE()

// 应用程序入口
wxIMPLEMENT_APP(OutlinePreviewApp);

bool OutlinePreviewApp::OnInit() {
    // 初始化Coin3D
    SoDB::init();
    SoInteraction::init();
    
    // 创建主窗口
    OutlinePreviewFrame* frame = new OutlinePreviewFrame("Enhanced Outline Preview");
    frame->Show(true);
    
    return true;
}

int OutlinePreviewApp::OnExit() {
    // 清理Coin3D
    SoDB::finish();
    return 0;
}

// 主窗口实现
OutlinePreviewFrame::OutlinePreviewFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(1200, 800)) {
    
    // 创建菜单栏
    CreateMenuBar();
    
    // 创建主面板
    wxPanel* mainPanel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // 创建工具栏提示
    wxPanel* infoPanel = new wxPanel(mainPanel);
    wxBoxSizer* infoSizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxStaticText* infoText = new wxStaticText(infoPanel, wxID_ANY, 
        "Left click and drag to rotate | Use menu to switch outline methods | View > Settings for detailed control");
    infoSizer->Add(infoText, 1, wxALIGN_CENTER_VERTICAL | wxALL, 5);
    
    infoPanel->SetBackgroundColour(wxColour(255, 255, 200));
    infoPanel->SetSizer(infoSizer);
    
    mainSizer->Add(infoPanel, 0, wxEXPAND);
    
    // 创建预览画布
    m_canvas = new EnhancedOutlinePreviewCanvas(mainPanel);
    mainSizer->Add(m_canvas, 1, wxEXPAND);
    
    mainPanel->SetSizer(mainSizer);
    
    // 设置窗口图标（如果有的话）
    // SetIcon(wxIcon("app_icon"));
    
    // 居中窗口
    Centre();
}

void OutlinePreviewFrame::CreateMenuBar() {
    wxMenuBar* menuBar = new wxMenuBar;
    
    // 文件菜单
    wxMenu* fileMenu = new wxMenu;
    fileMenu->Append(wxID_EXIT, "E&xit\tAlt+F4", "Exit the application");
    menuBar->Append(fileMenu, "&File");
    
    // 视图菜单
    wxMenu* viewMenu = new wxMenu;
    viewMenu->Append(ID_ViewSettings, "&Settings...\tCtrl+S", "Open outline settings dialog");
    viewMenu->Append(ID_ViewReset, "&Reset View\tCtrl+R", "Reset camera view");
    menuBar->Append(viewMenu, "&View");
    
    // 方法菜单
    wxMenu* methodMenu = new wxMenu;
    methodMenu->AppendRadioItem(ID_MethodBasic, "&Basic (No Outline)", "Basic rendering without outline");
    methodMenu->AppendRadioItem(ID_MethodInvertedHull, "&Inverted Hull\tCtrl+1", "Inverted hull outline method");
    methodMenu->AppendRadioItem(ID_MethodScreenSpace, "&Screen Space\tCtrl+2", "Screen space edge detection");
    methodMenu->AppendRadioItem(ID_MethodGeometry, "&Geometry Silhouette\tCtrl+3", "Geometry-based silhouette extraction");
    methodMenu->AppendRadioItem(ID_MethodStencil, "S&tencil Buffer\tCtrl+4", "Stencil buffer outline method");
    methodMenu->AppendRadioItem(ID_MethodMultiPass, "&Multi-Pass\tCtrl+5", "Multi-pass combined method");
    
    // 默认选中Inverted Hull
    methodMenu->Check(ID_MethodInvertedHull, true);
    
    menuBar->Append(methodMenu, "&Method");
    
    // 帮助菜单
    wxMenu* helpMenu = new wxMenu;
    helpMenu->Append(wxID_ABOUT, "&About...\tF1", "Show about dialog");
    menuBar->Append(helpMenu, "&Help");
    
    SetMenuBar(menuBar);
}

void OutlinePreviewFrame::OnFileExit(wxCommandEvent& event) {
    Close(true);
}

void OutlinePreviewFrame::OnViewSettings(wxCommandEvent& event) {
    // 打开设置对话框
    EnhancedOutlineSettingsDialog dlg(this, m_outlineParams);
    
    if (dlg.ShowModal() == wxID_OK) {
        m_outlineParams = dlg.getParams();
        m_canvas->updateOutlineParams(m_outlineParams);
    }
}

void OutlinePreviewFrame::OnViewReset(wxCommandEvent& event) {
    m_canvas->resetCamera();
}

void OutlinePreviewFrame::OnHelpAbout(wxCommandEvent& event) {
    wxAboutDialogInfo info;
    info.SetName("Enhanced Outline Preview");
    info.SetVersion("1.0");
    info.SetDescription("A demonstration of various real-time outline rendering techniques.\n\n"
                       "Features:\n"
                       "- Multiple outline rendering methods\n"
                       "- Real-time parameter adjustment\n"
                       "- Performance statistics\n"
                       "- Interactive 3D preview");
    info.SetCopyright("(C) 2024");
    
    wxAboutBox(info);
}

void OutlinePreviewFrame::OnMethodBasic(wxCommandEvent& event) {
    m_canvas->setOutlineMethod(OutlineMethod::BASIC);
}

void OutlinePreviewFrame::OnMethodInvertedHull(wxCommandEvent& event) {
    m_canvas->setOutlineMethod(OutlineMethod::INVERTED_HULL);
}

void OutlinePreviewFrame::OnMethodScreenSpace(wxCommandEvent& event) {
    m_canvas->setOutlineMethod(OutlineMethod::SCREEN_SPACE);
}

void OutlinePreviewFrame::OnMethodGeometry(wxCommandEvent& event) {
    m_canvas->setOutlineMethod(OutlineMethod::GEOMETRY_SILHOUETTE);
}

void OutlinePreviewFrame::OnMethodStencil(wxCommandEvent& event) {
    m_canvas->setOutlineMethod(OutlineMethod::STENCIL_BUFFER);
}

void OutlinePreviewFrame::OnMethodMultiPass(wxCommandEvent& event) {
    m_canvas->setOutlineMethod(OutlineMethod::MULTI_PASS);
}