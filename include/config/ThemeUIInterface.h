#ifndef THEME_UI_INTERFACE_H
#define THEME_UI_INTERFACE_H

#include <wx/wx.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <vector>
#include <string>

class ThemeUIInterface : public wxPanel
{
public:
    ThemeUIInterface(wxWindow* parent, wxWindowID id = wxID_ANY);
    virtual ~ThemeUIInterface();

    // Interface methods
    void RefreshThemeList();
    void ApplyCurrentTheme();
    
    // Event handlers
    void OnThemeChange(wxCommandEvent& event);
    void OnRefreshThemes(wxCommandEvent& event);
    
    // Callback registration for theme change notifications
    void SetThemeChangeCallback(std::function<void()> callback);

private:
    void CreateControls();
    void UpdateThemeDisplay();
    
    wxChoice* m_themeChoice;
    wxButton* m_refreshButton;
    wxStaticText* m_currentThemeLabel;
    std::function<void()> m_themeChangeCallback;
    
    wxDECLARE_EVENT_TABLE();
};

enum
{
    ID_THEME_CHOICE = 1000,
    ID_REFRESH_THEMES
};

#endif // THEME_UI_INTERFACE_H 