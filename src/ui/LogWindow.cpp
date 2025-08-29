#include "ui/LogWindow.h"

LogWindow* LogWindow::s_instance = nullptr;

enum {
    ID_CLEAR = wxID_HIGHEST + 1,
    ID_COPY_ALL
};

BEGIN_EVENT_TABLE(LogWindow, wxFrame)
    EVT_CLOSE(LogWindow::onClose)
    EVT_BUTTON(ID_CLEAR, LogWindow::onClear)
    EVT_BUTTON(ID_COPY_ALL, LogWindow::onCopy)
END_EVENT_TABLE()

LogWindow::LogWindow(wxWindow* parent, const wxString& title)
    : wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxSize(800, 400))
{
    // Create main panel
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Create text control with monospace font
    m_logText = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
                               wxDefaultPosition, wxDefaultSize,
                               wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    
    wxFont font(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    m_logText->SetFont(font);
    
    // Create button panel
    wxPanel* buttonPanel = new wxPanel(panel);
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    wxButton* clearBtn = new wxButton(buttonPanel, ID_CLEAR, "Clear");
    wxButton* copyBtn = new wxButton(buttonPanel, ID_COPY_ALL, "Copy All");
    wxButton* closeBtn = new wxButton(buttonPanel, wxID_CLOSE, "Close");
    
    buttonSizer->Add(clearBtn, 0, wxALL, 5);
    buttonSizer->Add(copyBtn, 0, wxALL, 5);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(closeBtn, 0, wxALL, 5);
    
    buttonPanel->SetSizer(buttonSizer);
    
    // Layout
    mainSizer->Add(m_logText, 1, wxEXPAND | wxALL, 5);
    mainSizer->Add(buttonPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    panel->SetSizer(mainSizer);
    
    // Set up custom log target
    m_oldLog = wxLog::GetActiveTarget();
    wxLog::SetActiveTarget(new TextCtrlLog(m_logText));
    
    // Add initial message
    wxLogMessage("=== Log Window Started ===");
    wxLogMessage("Right-click to copy selected text, or use 'Copy All' button");
    
    // Enable context menu for copy
    m_logText->Bind(wxEVT_RIGHT_DOWN, [this](wxMouseEvent& event) {
        wxMenu menu;
        menu.Append(wxID_COPY, "Copy");
        menu.Append(ID_COPY_ALL, "Copy All");
        menu.AppendSeparator();
        menu.Append(wxID_SELECTALL, "Select All");
        
        menu.Bind(wxEVT_MENU, [this](wxCommandEvent& evt) {
            if (evt.GetId() == wxID_COPY) {
                m_logText->Copy();
            } else if (evt.GetId() == ID_COPY_ALL) {
                m_logText->SelectAll();
                m_logText->Copy();
                m_logText->SetSelection(0, 0);
            } else if (evt.GetId() == wxID_SELECTALL) {
                m_logText->SelectAll();
            }
        });
        
        PopupMenu(&menu);
    });
}

LogWindow::~LogWindow() {
    // Restore old log target
    wxLog::SetActiveTarget(m_oldLog);
    s_instance = nullptr;
}

LogWindow* LogWindow::GetInstance() {
    return s_instance;
}

void LogWindow::ShowLogWindow(wxWindow* parent) {
    if (!s_instance) {
        s_instance = new LogWindow(parent);
    }
    s_instance->Show();
    s_instance->Raise();
}

void LogWindow::onClose(wxCloseEvent& event) {
    Hide();
    // Don't destroy, just hide
}

void LogWindow::onClear(wxCommandEvent& event) {
    m_logText->Clear();
    wxLogMessage("=== Log Cleared ===");
}

void LogWindow::onCopy(wxCommandEvent& event) {
    m_logText->SelectAll();
    m_logText->Copy();
    m_logText->SetSelection(0, 0);
    wxLogMessage("Log copied to clipboard");
}