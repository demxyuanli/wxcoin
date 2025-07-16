#include "ViewRefreshManager.h"
#include "Canvas.h"
#include "CommandDispatcher.h"
#include "logger/Logger.h"

wxBEGIN_EVENT_TABLE(ViewRefreshManager, wxEvtHandler)
    EVT_TIMER(wxID_ANY, ViewRefreshManager::onDebounceTimer)
wxEND_EVENT_TABLE()

ViewRefreshManager::ViewRefreshManager(Canvas* canvas)
    : m_canvas(canvas)
    , m_commandDispatcher(nullptr)
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
    try {
        // Avoid logging during shutdown to prevent crashes
        std::cout << "ViewRefreshManager: Destroyed" << std::endl;
    } catch (...) {
        // Ignore any exceptions during shutdown
    }
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

void ViewRefreshManager::requestRefreshByCommand(const std::string& commandType, 
                                               const std::unordered_map<std::string, std::string>& parameters)
{
    if (!m_commandDispatcher) {
        LOG_WRN_S("ViewRefreshManager: Command dispatcher not set, using direct refresh");
        requestRefresh(RefreshReason::MANUAL_REQUEST, true);
        return;
    }
    
    // Dispatch the refresh command
    auto result = m_commandDispatcher->dispatchCommand(commandType, parameters);
    if (!result.success) {
        LOG_WRN_S("ViewRefreshManager: Failed to dispatch refresh command: " + result.message);
        // Fallback to direct refresh
        requestRefresh(RefreshReason::MANUAL_REQUEST, true);
    }
}

std::string ViewRefreshManager::refreshReasonToString(RefreshReason reason)
{
    switch (reason) {
        case RefreshReason::GEOMETRY_CHANGED: return "GEOMETRY_CHANGED";
        case RefreshReason::NORMALS_TOGGLED: return "NORMALS_TOGGLED";
        case RefreshReason::EDGES_TOGGLED: return "EDGES_TOGGLED";
        case RefreshReason::MATERIAL_CHANGED: return "MATERIAL_CHANGED";
        case RefreshReason::CAMERA_MOVED: return "CAMERA_MOVED";
        case RefreshReason::SELECTION_CHANGED: return "SELECTION_CHANGED";
        case RefreshReason::SCENE_CHANGED: return "SCENE_CHANGED";
        case RefreshReason::OBJECT_CHANGED: return "OBJECT_CHANGED";
        case RefreshReason::UI_CHANGED: return "UI_CHANGED";
        case RefreshReason::TEXTURE_CHANGED: return "TEXTURE_CHANGED";
        case RefreshReason::TRANSPARENCY_CHANGED: return "TRANSPARENCY_CHANGED";
        case RefreshReason::RENDERING_SETTINGS_CHANGED: return "RENDERING_SETTINGS_CHANGED";
        case RefreshReason::MANUAL_REQUEST: return "MANUAL_REQUEST";
        default: return "UNKNOWN";
    }
}

ViewRefreshManager::RefreshReason ViewRefreshManager::stringToRefreshReason(const std::string& reasonStr)
{
    if (reasonStr == "GEOMETRY_CHANGED") return RefreshReason::GEOMETRY_CHANGED;
    if (reasonStr == "NORMALS_TOGGLED") return RefreshReason::NORMALS_TOGGLED;
    if (reasonStr == "EDGES_TOGGLED") return RefreshReason::EDGES_TOGGLED;
    if (reasonStr == "MATERIAL_CHANGED") return RefreshReason::MATERIAL_CHANGED;
    if (reasonStr == "CAMERA_MOVED") return RefreshReason::CAMERA_MOVED;
    if (reasonStr == "SELECTION_CHANGED") return RefreshReason::SELECTION_CHANGED;
    if (reasonStr == "SCENE_CHANGED") return RefreshReason::SCENE_CHANGED;
    if (reasonStr == "OBJECT_CHANGED") return RefreshReason::OBJECT_CHANGED;
    if (reasonStr == "UI_CHANGED") return RefreshReason::UI_CHANGED;
    if (reasonStr == "TEXTURE_CHANGED") return RefreshReason::TEXTURE_CHANGED;
    if (reasonStr == "TRANSPARENCY_CHANGED") return RefreshReason::TRANSPARENCY_CHANGED;
    if (reasonStr == "RENDERING_SETTINGS_CHANGED") return RefreshReason::RENDERING_SETTINGS_CHANGED;
    if (reasonStr == "MANUAL_REQUEST") return RefreshReason::MANUAL_REQUEST;
    return RefreshReason::MANUAL_REQUEST;
}