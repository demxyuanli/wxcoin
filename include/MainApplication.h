#ifndef MAIN_APPLICATION_HPP
#define MAIN_APPLICATION_HPP

#include <wx/wx.h>

class MainApplication : public wxApp {
public:
    bool OnInit() override;
};
#endif // MAIN_APPLICATION_HPP