#pragma once

#include <wx/window.h>
#include <wx/log.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/listbox.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

class FlatUIBar;
class FlatUIPage;
class FlatUIPanel;
class FlatUIButtonBar;
class FlatUIHomeSpace;
class FlatUIFunctionSpace;
class FlatUIProfileSpace;
class FlatUISystemButtons;
class FlatUIGallery;
class FlatUICustomControl;

class UIHierarchyDebugger
{
public:

    UIHierarchyDebugger();

    ~UIHierarchyDebugger();

    void DebugUIHierarchy(wxWindow* window = nullptr, int depth = 0);

    void SetLogTextCtrl(wxTextCtrl* textCtrl);

    void PrintUIHierarchy(wxWindow* window);

private:
    wxTextCtrl* m_logTextCtrl;
    wxLog* m_oldLog;
    bool m_usingCustomLog;
}; 