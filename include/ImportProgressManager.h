#pragma once

#include <wx/wx.h>
#include <wx/timer.h>
#include <atomic>
#include <mutex>
#include <memory>
#include "widgets/FlatProgressBar.h"

class ImportProgressManager : public wxEvtHandler
{
public:
    ImportProgressManager(wxWindow* parent);
    ~ImportProgressManager();
    
    // Thread-safe progress update
    void SetProgress(int value, const wxString& message = wxEmptyString);
    void SetRange(int min, int max);
    void Show(bool show = true);
    void Reset();
    
    // Get the progress bar widget
    FlatProgressBar* GetProgressBar() { return m_progressBar; }
    
    // Thread-safe status messages
    void SetStatusMessage(const wxString& message);
    
private:
    wxWindow* m_parent;
    FlatProgressBar* m_progressBar;
    wxStaticText* m_statusText;
    wxPanel* m_progressPanel;
    
    // Thread safety
    std::mutex m_mutex;
    std::atomic<int> m_currentValue;
    std::atomic<int> m_minValue;
    std::atomic<int> m_maxValue;
    
    // Pending updates
    struct PendingUpdate {
        int value;
        wxString message;
        bool hasMessage;
    };
    PendingUpdate m_pendingUpdate;
    bool m_hasPendingUpdate;
    
    // Timer for UI updates
    wxTimer* m_updateTimer;
    void OnUpdateTimer(wxTimerEvent& event);
    void ApplyPendingUpdates();
    
    wxDECLARE_EVENT_TABLE();
};