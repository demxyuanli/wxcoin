#include "docking/BatchRefreshManager.h"
#include <wx/timer.h>
#include <algorithm>
#include <map>

namespace ads {

BatchRefreshManager& BatchRefreshManager::getInstance() {
    static BatchRefreshManager instance;
    return instance;
}

void BatchRefreshManager::scheduleRefresh(wxWindow* window, const wxRect* rect) {
    if (!window || window->IsBeingDeleted()) {
        return;
    }
    
    if (rect) {
        m_pendingRefreshes.push_back(RefreshRequest(window, *rect, false));
    } else {
        m_pendingFullRefreshes.insert(window);
    }
    
    if (m_pendingRefreshes.size() > m_maxBatchSize) {
        flush();
    } else if (!isBatching()) {
        initializeTimer();
        if (m_flushTimer && !m_flushTimer->IsRunning()) {
            m_flushTimer->StartOnce(16);
        }
    }
}

void BatchRefreshManager::scheduleRefreshAll(wxWindow* window) {
    if (!window || window->IsBeingDeleted()) {
        return;
    }
    
    m_pendingFullRefreshes.insert(window);
    
    if (!isBatching()) {
        initializeTimer();
        if (m_flushTimer && !m_flushTimer->IsRunning()) {
            m_flushTimer->StartOnce(16);
        }
    }
}

void BatchRefreshManager::flush() {
    mergeRefreshRequests();
    
    for (wxWindow* window : m_pendingFullRefreshes) {
        if (window && !window->IsBeingDeleted()) {
            window->Refresh();
        }
    }
    m_pendingFullRefreshes.clear();
    
    for (const auto& request : m_pendingRefreshes) {
        if (request.window && !request.window->IsBeingDeleted()) {
            request.window->RefreshRect(request.rect, false);
        }
    }
    m_pendingRefreshes.clear();
    
    if (m_flushTimer && m_flushTimer->IsRunning()) {
        m_flushTimer->Stop();
    }
}

void BatchRefreshManager::flushImmediate() {
    flush();
}

void BatchRefreshManager::beginBatch() {
    m_batchCount++;
}

void BatchRefreshManager::endBatch() {
    if (m_batchCount > 0) {
        m_batchCount--;
        if (m_batchCount == 0) {
            flush();
        }
    }
}

void BatchRefreshManager::clear() {
    m_pendingRefreshes.clear();
    m_pendingFullRefreshes.clear();
    if (m_flushTimer && m_flushTimer->IsRunning()) {
        m_flushTimer->Stop();
    }
}

void BatchRefreshManager::initializeTimer() {
    if (m_flushTimer) {
        return;
    }
    
    m_flushTimer = new wxTimer();
    m_flushTimer->Bind(wxEVT_TIMER, &BatchRefreshManager::onFlushTimer, this);
}

void BatchRefreshManager::onFlushTimer(wxTimerEvent& event) {
    if (!isBatching()) {
        flush();
    }
}

void BatchRefreshManager::mergeRefreshRequests() {
    std::map<wxWindow*, wxRect> mergedRects;
    
    for (const auto& request : m_pendingRefreshes) {
        if (request.fullRefresh) {
            continue;
        }
        
        std::map<wxWindow*, wxRect>::iterator it = mergedRects.find(request.window);
        if (it == mergedRects.end()) {
            mergedRects[request.window] = request.rect;
        } else {
            it->second.Union(request.rect);
        }
    }
    
    m_pendingRefreshes.clear();
    for (std::map<wxWindow*, wxRect>::const_iterator it = mergedRects.begin(); 
         it != mergedRects.end(); ++it) {
        m_pendingRefreshes.push_back(RefreshRequest(it->first, it->second, false));
    }
}

} // namespace ads

