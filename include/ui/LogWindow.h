#ifndef LOG_WINDOW_H
#define LOG_WINDOW_H

#include <wx/wx.h>
#include <wx/textctrl.h>
#include <wx/log.h>

class LogWindow : public wxFrame {
public:
    LogWindow(wxWindow* parent, const wxString& title = "Log Messages");
    ~LogWindow();
    
    static LogWindow* GetInstance();
    static void ShowLogWindow(wxWindow* parent = nullptr);
    
private:
    wxTextCtrl* m_logText;
    wxLog* m_oldLog;
    static LogWindow* s_instance;
    
    void onClose(wxCloseEvent& event);
    void onClear(wxCommandEvent& event);
    void onCopy(wxCommandEvent& event);
    
    DECLARE_EVENT_TABLE()
};

// Custom log target that writes to our text control
class TextCtrlLog : public wxLog {
public:
    TextCtrlLog(wxTextCtrl* textCtrl) : m_textCtrl(textCtrl) {}
    
protected:
    virtual void DoLogText(const wxString& msg) override {
        if (m_textCtrl) {
            m_textCtrl->AppendText(msg + "\n");
        }
    }
    
private:
    wxTextCtrl* m_textCtrl;
};

#endif // LOG_WINDOW_H