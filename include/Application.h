#ifndef APPLICATION_H
#define APPLICATION_H

#include <wx/wx.h>
#include "MainFrame.h"

class Application : public wxApp
{
public:
    virtual bool OnInit();
    virtual int OnExit();

private:
    MainFrame* m_mainFrame;
};

#endif // APPLICATION_H