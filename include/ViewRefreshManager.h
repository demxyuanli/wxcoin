#pragma once

#include <functional>
#include <vector>
#include <memory>
#include <wx/timer.h>

class Canvas;

/**
 * @brief View refresh manager with listener mechanism
 * 
 * Provides centralized view refresh management with debouncing and listener pattern
 */
class ViewRefreshManager : public wxEvtHandler {
public:
    enum class RefreshReason {
        GEOMETRY_CHANGED,
        NORMALS_TOGGLED,
        EDGES_TOGGLED,
        MATERIAL_CHANGED,
        CAMERA_MOVED,
        SELECTION_CHANGED,
        MANUAL_REQUEST
    };
    
    using RefreshListener = std::function<void(RefreshReason reason)>;
    
public:
    explicit ViewRefreshManager(Canvas* canvas);
    ~ViewRefreshManager();
    
    // Request refresh with optional debouncing
    void requestRefresh(RefreshReason reason = RefreshReason::MANUAL_REQUEST, bool immediate = false);
    
    // Listener management
    void addRefreshListener(RefreshListener listener);
    void removeAllListeners();
    
    // Configuration
    void setDebounceTime(int milliseconds) { m_debounceTime = milliseconds; }
    int getDebounceTime() const { return m_debounceTime; }
    
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool isEnabled() const { return m_enabled; }
    
private:
    void performRefresh(RefreshReason reason);
    void onDebounceTimer(wxTimerEvent& event);
    
private:
    Canvas* m_canvas;
    std::vector<RefreshListener> m_listeners;
    
    wxTimer m_debounceTimer;
    RefreshReason m_pendingReason;
    bool m_hasPendingRefresh;
    
    int m_debounceTime;  // milliseconds
    bool m_enabled;
    
    wxDECLARE_EVENT_TABLE();
};