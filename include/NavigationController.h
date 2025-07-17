#pragma once

#include <wx/event.h>
#include <wx/timer.h>
#include <Inventor/SbLinear.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <functional>
#include <mutex>
#include <vector>

class Canvas;
class SceneManager;

class NavigationController : public wxEvtHandler {
public:
    NavigationController(Canvas* canvas, SceneManager* sceneManager);
    ~NavigationController();

    void handleMouseButton(wxMouseEvent& event);
    void handleMouseMotion(wxMouseEvent& event);
    void handleMouseWheel(wxMouseEvent& event);

    void viewAll();
    void viewTop();
    void viewFront();
    void viewRight();
    void viewIsometric();

    // Zoom speed adjustment
    void setZoomSpeedFactor(float factor);
    float getZoomSpeedFactor() const;

    // Smart refresh strategy
    enum class RefreshStrategy {
        IMMEDIATE,      // Refresh on every mouse move
        THROTTLED,      // Throttled refresh (default)
        ADAPTIVE,       // Adaptive based on performance
        ASYNC           // Async rendering
    };
    
    void setRefreshStrategy(RefreshStrategy strategy);
    RefreshStrategy getRefreshStrategy() const;
    
    // Async rendering control
    void setAsyncRenderingEnabled(bool enabled);
    bool isAsyncRenderingEnabled() const;
    
    // Enhanced LOD system
    void setLODEnabled(bool enabled);
    bool isLODEnabled() const;
    void setLODTransitionTime(int milliseconds);
    int getLODTransitionTime() const;
    
    // Performance monitoring
    void setPerformanceMonitoringEnabled(bool enabled);
    bool isPerformanceMonitoringEnabled() const;
    
    // Get performance metrics
    struct PerformanceMetrics {
        std::chrono::nanoseconds averageFrameTime{0};
        std::chrono::nanoseconds maxFrameTime{0};
        size_t totalFrames{0};
        size_t droppedFrames{0};
        double fps{0.0};
        
        PerformanceMetrics() : averageFrameTime(0), maxFrameTime(0), totalFrames(0), 
                              droppedFrames(0), fps(0.0) {}
    };
    PerformanceMetrics getPerformanceMetrics() const;

private:
    void rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void panCamera(const wxPoint& currentPos, const wxPoint& lastPos);
    void zoomCamera(float delta);

    // Smart refresh strategy implementation
    void requestSmartRefresh();
    void onRefreshTimer(wxTimerEvent& event);
    void onLODTimer(wxTimerEvent& event);
    
    // Async rendering implementation
    void startAsyncRender();
    void onAsyncRenderComplete();
    
    // Performance monitoring
    void recordFrameTime(std::chrono::nanoseconds frameTime);
    void updatePerformanceMetrics();
    
    // Enhanced LOD system
    void switchToLODMode(bool roughMode);
    void onLODModeChange(bool roughMode);

    Canvas* m_canvas;
    SceneManager* m_sceneManager;
    bool m_isDragging;
    wxPoint m_lastMousePos;
    enum class DragMode { NONE, ROTATE, PAN, ZOOM };
    DragMode m_dragMode;
    float m_zoomSpeedFactor;  // Multiplier for mouse wheel zoom speed
    
    // Smart refresh strategy
    RefreshStrategy m_refreshStrategy;
    
    // Refresh timing control
    wxTimer m_refreshTimer;
    wxTimer m_lodTimer;
    std::chrono::steady_clock::time_point m_lastRefreshTime;
    std::chrono::milliseconds m_refreshInterval;
    std::chrono::milliseconds m_minRefreshInterval;
    std::chrono::milliseconds m_maxRefreshInterval;
    
    // Async rendering
    std::atomic<bool> m_asyncRenderingEnabled;
    std::atomic<bool> m_isAsyncRendering;
    std::function<void()> m_asyncRenderCallback;
    
    // Enhanced LOD system
    std::atomic<bool> m_lodEnabled;
    std::atomic<bool> m_isLODRoughMode;
    int m_lodTransitionTime;
    std::chrono::steady_clock::time_point m_lastInteractionTime;
    
    // Performance monitoring
    std::atomic<bool> m_performanceMonitoringEnabled;
    mutable std::mutex m_metricsMutex;
    PerformanceMetrics m_performanceMetrics;
    std::vector<std::chrono::nanoseconds> m_frameTimeHistory;
    static constexpr size_t MAX_FRAME_HISTORY = 60; // last 60 frames
    
    // Interaction state tracking
    std::chrono::steady_clock::time_point m_lastMouseMoveTime;
    wxPoint m_lastMouseMovePos;
    float m_mouseMoveThreshold; // Minimum mouse movement to trigger refresh
    
    wxDECLARE_EVENT_TABLE();
};