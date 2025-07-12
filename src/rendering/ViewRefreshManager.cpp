#include "ViewRefreshManager.h"
#include "Canvas.h"
#include "logger/Logger.h"

wxBEGIN_EVENT_TABLE(ViewRefreshManager, wxEvtHandler)
    EVT_TIMER(wxID_ANY, ViewRefreshManager::onDebounceTimer)
wxEND_EVENT_TABLE()

ViewRefreshManager::ViewRefreshManager(Canvas* canvas)
    : m_canvas(canvas)
    , m_debounceTimer(this)
    , m_pendingReason(RefreshReason::MANUAL_REQUEST)
    , m_hasPendingRefresh(false)
    , m_debounceTime(16)  // ~60fps
    , m_enabled(true)
{
    LOG_INF_S("ViewRefreshManager: Initialized");
}

ViewRefreshManager::~ViewRefreshManager() {
    m_debounceTimer.Stop();
    removeAllListeners();
    LOG_INF_S("ViewRefreshManager: Destroyed");
}

void ViewRefreshManager::requestRefresh(RefreshReason reason, bool immediate) {
    if (!m_enabled || !m_canvas) {
        return;
    }
    
    if (immediate) {
        // Cancel any pending debounced refresh
        m_debounceTimer.Stop();
        m_hasPendingRefresh = false;
        performRefresh(reason);
    } else {
        // Use debouncing to avoid excessive refreshes
        m_pendingReason = reason;
        m_hasPendingRefresh = true;
        
        if (!m_debounceTimer.IsRunning()) {
            m_debounceTimer.Start(m_debounceTime, wxTIMER_ONE_SHOT);
        }
    }
}

void ViewRefreshManager::addRefreshListener(RefreshListener listener) {
    m_listeners.push_back(listener);
}

void ViewRefreshManager::removeAllListeners() {
    m_listeners.clear();
}

void ViewRefreshManager::performRefresh(RefreshReason reason) {
    if (!m_canvas) {
        return;
    }
    
    // Notify all listeners before refresh
    for (const auto& listener : m_listeners) {
        try {
            listener(reason);
        } catch (const std::exception& e) {
            LOG_ERR_S("ViewRefreshManager: Listener exception: " + std::string(e.what()));
        }
    }
    
    // Perform the actual refresh
    m_canvas->Refresh();
    m_canvas->render(false);
    
    LOG_DBG_S("ViewRefreshManager: Refresh completed for reason: " + std::to_string(static_cast<int>(reason)));
}

void ViewRefreshManager::onDebounceTimer(wxTimerEvent& event) {
    if (m_hasPendingRefresh) {
        performRefresh(m_pendingReason);
        m_hasPendingRefresh = false;
    }
}