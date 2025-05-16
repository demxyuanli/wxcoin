#include "Application.h"
#include "MainFrame.h"
#include "Logger.h"
#include <Inventor/SoDB.h>


wxIMPLEMENT_APP(Application);

bool Application::OnInit()
{
    LOG_INF("Application initializing");
    SoDB::init();

    m_mainFrame = new MainFrame("wxCoin 3D Application");
    m_mainFrame->SetSize(1024, 768);
    m_mainFrame->Center();
    m_mainFrame->Show();

    LOG_INF("Application initialized successfully");
    return true;
}

int Application::OnExit()
{
    LOG_INF("Application exiting");
    return wxApp::OnExit();
}