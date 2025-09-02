#include "ImportProgressManager.h"
#include "logger/Logger.h"
#include <wx/sizer.h>

wxBEGIN_EVENT_TABLE(ImportProgressManager, wxEvtHandler)
    EVT_TIMER(wxID_ANY, ImportProgressManager::OnUpdateTimer)
wxEND_EVENT_TABLE()

ImportProgressManager::ImportProgressManager(wxWindow* parent)
    : wxEvtHandler()
    , m_parent(parent)
    , m_progressBar(nullptr)
    , m_statusText(nullptr)
    , m_progressPanel(nullptr)
    , m_currentValue(0)
    , m_minValue(0)
    , m_maxValue(100)
    , m_hasPendingUpdate(false)
    , m_updateTimer(nullptr)
{
    // Create progress panel
    m_progressPanel = new wxPanel(parent, wxID_ANY);
    m_progressPanel->SetBackgroundColour(parent->GetBackgroundColour());
    
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Status text
    m_statusText = new wxStaticText(m_progressPanel, wxID_ANY, "Ready");
    m_statusText->SetForegroundColour(wxColour(64, 64, 64));
    sizer->Add(m_statusText, 0, wxALL | wxEXPAND, 5);
    
    // Progress bar
    m_progressBar = new FlatProgressBar(m_progressPanel, wxID_ANY, 0, 0, 100,
                                       wxDefaultPosition, wxSize(-1, 20),
                                       FlatProgressBar::ProgressBarStyle::DEFAULT_STYLE);
    m_progressBar->SetShowPercentage(true);
    m_progressBar->SetShowValue(false);
    // Note: SetAnimationEnabled might not exist in current FlatProgressBar implementation
    
    sizer->Add(m_progressBar, 0, wxALL | wxEXPAND, 5);
    
    m_progressPanel->SetSizer(sizer);
    m_progressPanel->Hide(); // Initially hidden
    
    // Create update timer (runs on main thread)
    m_updateTimer = new wxTimer(this, wxID_ANY);
    m_updateTimer->Start(50); // Update UI every 50ms
}

ImportProgressManager::~ImportProgressManager()
{
    if (m_updateTimer) {
        m_updateTimer->Stop();
        delete m_updateTimer;
    }
}

void ImportProgressManager::SetProgress(int value, const wxString& message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Store pending update
    m_pendingUpdate.value = value;
    m_pendingUpdate.message = message;
    m_pendingUpdate.hasMessage = !message.IsEmpty();
    m_hasPendingUpdate = true;
    
    // Update atomic value for immediate access
    m_currentValue = value;
    
    LOG_INF_S(wxString::Format("Progress update: %d%% - %s", value, message));
}

void ImportProgressManager::SetRange(int min, int max)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_minValue = min;
    m_maxValue = max;
    
    // Apply immediately on main thread
    if (wxThread::IsMain()) {
        m_progressBar->SetRange(min, max);
    }
}

void ImportProgressManager::Show(bool show)
{
    // Must be called from main thread
    if (!wxThread::IsMain()) {
        wxTheApp->CallAfter([this, show]() { Show(show); });
        return;
    }
    
    if (show) {
        m_progressPanel->Show();
        m_progressPanel->GetParent()->Layout();
    } else {
        m_progressPanel->Hide();
        m_progressPanel->GetParent()->Layout();
    }
}

void ImportProgressManager::Reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_currentValue.store(m_minValue.load());
    m_hasPendingUpdate = false;
    
    if (wxThread::IsMain()) {
        m_progressBar->SetValue(m_minValue.load());
        m_statusText->SetLabel("Ready");
        m_progressPanel->Hide();
        m_progressPanel->GetParent()->Layout();
    } else {
        wxTheApp->CallAfter([this]() { Reset(); });
    }
}

void ImportProgressManager::SetStatusMessage(const wxString& message)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_pendingUpdate.message = message;
    m_pendingUpdate.hasMessage = true;
    m_hasPendingUpdate = true;
}

void ImportProgressManager::OnUpdateTimer(wxTimerEvent& event)
{
    // This runs on the main thread
    ApplyPendingUpdates();
}

void ImportProgressManager::ApplyPendingUpdates()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_hasPendingUpdate) {
        return;
    }
    
    // Apply pending updates
    if (m_progressBar) {
        int currentBarValue = m_progressBar->GetValue();
        if (currentBarValue != m_pendingUpdate.value) {
            m_progressBar->SetValue(m_pendingUpdate.value);
            
            // Update state based on progress
            if (m_pendingUpdate.value >= m_maxValue.load()) {
                m_progressBar->SetState(FlatProgressBar::ProgressBarState::COMPLETED);
            } else if (m_pendingUpdate.value > 0) {
                m_progressBar->SetState(FlatProgressBar::ProgressBarState::DEFAULT_STATE);
            }
        }
    }
    
    if (m_statusText && m_pendingUpdate.hasMessage) {
        m_statusText->SetLabel(m_pendingUpdate.message);
    }
    
    m_hasPendingUpdate = false;
}