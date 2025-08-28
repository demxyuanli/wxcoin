#ifndef MAIN_APPLICATION_HPP
#define MAIN_APPLICATION_HPP

#include <wx/wx.h>

class MainApplication : public wxApp {
public:
    virtual bool OnInit() = 0;
};
#endif // MAIN_APPLICATION_HPP