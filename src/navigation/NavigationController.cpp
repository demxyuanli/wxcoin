#include "NavigationController.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/SbRotation.h>
#include <cmath>
#include <algorithm>
#include <thread>
#include <numeric>

wxBEGIN_EVENT_TABLE(NavigationController, wxEvtHandler)
    EVT_TIMER(wxID_ANY, NavigationController::onRefreshTimer)
    EVT_TIMER(wxID_ANY + 1, NavigationController::onLODTimer)
wxEND_EVENT_TABLE()

NavigationController::NavigationController(Canvas* canvas, SceneManager* sceneManager)
    : m_canvas(canvas)
    , m_sceneManager(sceneManager)
    , m_isDragging(false)
    , m_dragMode(DragMode::NONE)
    , m_zoomSpeedFactor(1.0f)
    , m_refreshStrategy(RefreshStrategy::ADAPTIVE)
    , m_refreshTimer(this)
    , m_lodTimer(this, wxID_ANY + 1)
    , m_lastRefreshTime(std::chrono::steady_clock::now())
    , m_refreshInterval(std::chrono::milliseconds(16)) // 60 FPS default
    , m_minRefreshInterval(std::chrono::milliseconds(8))  // 120 FPS max
    , m_maxRefreshInterval(std::chrono::milliseconds(33)) // 30 FPS min
    , m_asyncRenderingEnabled(true)
    , m_isAsyncRendering(false)
    , m_lodEnabled(true)
    , m_isLODRoughMode(false)
    , m_lodTransitionTime(500) // 500ms default
    , m_lastInteractionTime(std::chrono::steady_clock::now())
    , m_performanceMonitoringEnabled(true)
    , m_mouseMoveThreshold(2.0f) // 2 pixels minimum movement
{
    LOG_INF_S("NavigationController initializing with enhanced features");
    
    // Initialize frame time history
    m_frameTimeHistory.reserve(MAX_FRAME_HISTORY);
    
    // Start refresh timer
    m_refreshTimer.Start(m_refreshInterval.count(), wxTIMER_CONTINUOUS);
}

NavigationController::~NavigationController() {
    m_refreshTimer.Stop();
    m_lodTimer.Stop();
    LOG_INF_S("NavigationController destroying");
}

void NavigationController::handleMouseButton(wxMouseEvent& event) {
    auto now = std::chrono::steady_clock::now();
    
    if (event.LeftDown()) {
        m_isDragging = true;
        m_dragMode = DragMode::ROTATE;
        m_lastMousePos = event.GetPosition();
        m_lastInteractionTime = now;
        
        // Enable LOD for interaction
        if (m_lodEnabled) {
            switchToLODMode(true);
        }
        
        LOG_DBG_S("NavigationController: Started rotation drag");
    }
    else if (event.RightDown()) {
        m_isDragging = true;
        m_dragMode = DragMode::PAN;
        m_lastMousePos = event.GetPosition();
        m_lastInteractionTime = now;
        
        // Enable LOD for interaction
        if (m_lodEnabled) {
            switchToLODMode(true);
        }
        
        LOG_DBG_S("NavigationController: Started pan drag");
    }
    else if (event.LeftUp() || event.RightUp()) {
        m_isDragging = false;
        m_dragMode = DragMode::NONE;
        
        // Schedule LOD transition back to fine mode
        if (m_lodEnabled) {
            m_lodTimer.Start(m_lodTransitionTime, wxTIMER_ONE_SHOT);
        }
        
        // Force immediate refresh for final position
        requestSmartRefresh();
        
        LOG_DBG_S("NavigationController: Ended drag operation");
    }
    event.Skip();
}

void NavigationController::handleMouseMotion(wxMouseEvent& event) {
    if (!m_isDragging) {
        event.Skip();
        return;
    }

    auto now = std::chrono::steady_clock::now();
    wxPoint currentPos = event.GetPosition();
    
    // Check if mouse movement is significant enough to warrant a refresh
    float distance = std::sqrt(
        std::pow(currentPos.x - m_lastMouseMovePos.x, 2) + 
        std::pow(currentPos.y - m_lastMouseMovePos.y, 2)
    );
    
    if (distance < m_mouseMoveThreshold) {
        event.Skip();
        return;
    }
    
    // Update interaction time
    m_lastInteractionTime = now;
    m_lastMouseMoveTime = now;
    m_lastMouseMovePos = currentPos;
    
    // Perform camera transformation
    if (m_dragMode == DragMode::ROTATE) {
        rotateCamera(currentPos, m_lastMousePos);
    }
    else if (m_dragMode == DragMode::PAN) {
        panCamera(currentPos, m_lastMousePos);
    }
    
    m_lastMousePos = currentPos;
    
    // Request smart refresh based on strategy
    requestSmartRefresh();
    
    event.Skip();
}

void NavigationController::handleMouseWheel(wxMouseEvent& event) {
    auto now = std::chrono::steady_clock::now();
    m_lastInteractionTime = now;
    
    float delta = event.GetWheelRotation() / 120.0f;
    zoomCamera(delta);
    
    // Enable LOD for wheel interaction
    if (m_lodEnabled) {
        switchToLODMode(true);
        m_lodTimer.Start(m_lodTransitionTime, wxTIMER_ONE_SHOT);
    }
    
    // Force immediate refresh for zoom
    requestSmartRefresh();
    
    event.Skip();
}

void NavigationController::requestSmartRefresh() {
    auto now = std::chrono::steady_clock::now();
    auto timeSinceLastRefresh = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_lastRefreshTime
    );
    
    switch (m_refreshStrategy) {
        case RefreshStrategy::IMMEDIATE:
            // Always refresh immediately
            if (m_canvas) {
                m_canvas->Refresh();
            }
            break;
            
        case RefreshStrategy::THROTTLED:
            // Throttled refresh based on interval
            if (timeSinceLastRefresh >= m_refreshInterval) {
                if (m_canvas) {
                    m_canvas->Refresh();
                }
                m_lastRefreshTime = now;
            }
            break;
            
        case RefreshStrategy::ADAPTIVE:
            // Adaptive refresh based on performance
            if (timeSinceLastRefresh >= m_refreshInterval) {
                if (m_asyncRenderingEnabled && !m_isAsyncRendering) {
                    startAsyncRender();
                } else {
                    if (m_canvas) {
                        m_canvas->Refresh();
                    }
                }
                m_lastRefreshTime = now;
            }
            break;
            
        case RefreshStrategy::ASYNC:
            // Always use async rendering
            if (m_asyncRenderingEnabled && !m_isAsyncRendering) {
                startAsyncRender();
            }
            break;
    }
}

void NavigationController::onRefreshTimer(wxTimerEvent& event) {
    // Update refresh interval based on performance
    if (m_refreshStrategy == RefreshStrategy::ADAPTIVE) {
        updatePerformanceMetrics();
        
        // Adjust interval based on performance
        if (m_performanceMetrics.fps < 30.0) {
            m_refreshInterval = std::min(m_refreshInterval * 2, m_maxRefreshInterval);
        } else if (m_performanceMetrics.fps > 55.0) {
            m_refreshInterval = std::max(m_refreshInterval / 2, m_minRefreshInterval);
        }
        
        // Update timer interval
        m_refreshTimer.Start(m_refreshInterval.count(), wxTIMER_CONTINUOUS);
    }
}

void NavigationController::onLODTimer(wxTimerEvent& event) {
    // Transition back to fine LOD mode
    if (m_lodEnabled) {
        switchToLODMode(false);
    }
}

void NavigationController::startAsyncRender() {
    if (m_isAsyncRendering) {
        return;
    }
    
    m_isAsyncRendering = true;
    
    // Start async rendering in background thread
    std::thread([this]() {
        // Simulate async rendering
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        
        // Call completion callback on main thread
        if (m_asyncRenderCallback) {
            m_asyncRenderCallback();
        }
        
        m_isAsyncRendering = false;
    }).detach();
}

void NavigationController::onAsyncRenderComplete() {
    if (m_canvas) {
        m_canvas->Refresh();
    }
}

void NavigationController::switchToLODMode(bool roughMode) {
    if (m_isLODRoughMode == roughMode) {
        return;
    }
    
    m_isLODRoughMode = roughMode;
    onLODModeChange(roughMode);
    
    LOG_DBG_S("NavigationController: Switched to " + std::string(roughMode ? "rough" : "fine") + " LOD mode");
}

void NavigationController::onLODModeChange(bool roughMode) {
    // This would typically interact with LODManager
    // For now, just log the change
    LOG_DBG_S("NavigationController: LOD mode changed to " + std::string(roughMode ? "rough" : "fine"));
}

void NavigationController::recordFrameTime(std::chrono::nanoseconds frameTime) {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    m_frameTimeHistory.push_back(frameTime);
    if (m_frameTimeHistory.size() > MAX_FRAME_HISTORY) {
        m_frameTimeHistory.erase(m_frameTimeHistory.begin());
    }
    
    m_performanceMetrics.totalFrames++;
    
    // Check for dropped frames
    if (frameTime > std::chrono::milliseconds(33)) { // 30 FPS threshold
        m_performanceMetrics.droppedFrames++;
    }
}

void NavigationController::updatePerformanceMetrics() {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    if (m_frameTimeHistory.empty()) {
        return;
    }
    
    // Calculate average frame time
    auto totalTime = std::accumulate(m_frameTimeHistory.begin(), m_frameTimeHistory.end(), 
                                    std::chrono::nanoseconds(0));
    auto averageFrameTime = totalTime / m_frameTimeHistory.size();
    
    // Calculate FPS
    if (averageFrameTime.count() > 0) {
        m_performanceMetrics.fps = 1000000000.0 / averageFrameTime.count();
    }
    
    // Update max frame time
    auto maxFrameTime = *std::max_element(m_frameTimeHistory.begin(), m_frameTimeHistory.end());
    m_performanceMetrics.maxFrameTime = maxFrameTime;
    
    // Update average frame time
    m_performanceMetrics.averageFrameTime = averageFrameTime;
}

void NavigationController::rotateCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    if (!m_sceneManager) {
        return;
    }
    
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        return;
    }
    
    // Calculate rotation based on mouse movement
    float deltaX = static_cast<float>(currentPos.x - lastPos.x) / 100.0f;
    float deltaY = static_cast<float>(currentPos.y - lastPos.y) / 100.0f;
    
    // Apply rotation to camera
    SbRotation rotX(SbVec3f(1, 0, 0), deltaY);
    SbRotation rotY(SbVec3f(0, 1, 0), deltaX);
    
    SbRotation currentRotation = camera->orientation.getValue();
    SbRotation newRotation = currentRotation * rotY * rotX;
    
    camera->orientation.setValue(newRotation);
}

void NavigationController::panCamera(const wxPoint& currentPos, const wxPoint& lastPos) {
    if (!m_sceneManager) {
        return;
    }
    
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        return;
    }
    
    // Calculate pan based on mouse movement
    float deltaX = static_cast<float>(currentPos.x - lastPos.x) / 100.0f;
    float deltaY = static_cast<float>(currentPos.y - lastPos.y) / 100.0f;
    
    // Get current camera position and orientation
    SbVec3f position = camera->position.getValue();
    SbRotation orientation = camera->orientation.getValue();
    
    // Calculate pan direction in world space
    SbVec3f rightDir(1, 0, 0);
    SbVec3f upDir(0, 1, 0);
    orientation.multVec(rightDir, rightDir);
    orientation.multVec(upDir, upDir);
    
    // Apply pan
    SbVec3f newPosition = position - rightDir * deltaX + upDir * deltaY;
    camera->position.setValue(newPosition);
}

void NavigationController::zoomCamera(float delta) {
    if (!m_sceneManager) {
        return;
    }
    
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        return;
    }
    
    // Apply zoom with speed factor
    float zoomFactor = 1.0f + delta * m_zoomSpeedFactor * 0.1f;
    
    // Get current camera position and focal point
    SbVec3f position = camera->position.getValue();
    float focalDistance = camera->focalDistance.getValue();
    
    // Calculate focal point from position and orientation
    SbVec3f viewDirection(0, 0, -1);
    SbRotation orientation = camera->orientation.getValue();
    orientation.multVec(viewDirection, viewDirection);
    SbVec3f focalPoint = position + viewDirection * focalDistance;
    
    // Calculate new position
    SbVec3f direction = position - focalPoint;
    SbVec3f newPosition = focalPoint + direction * zoomFactor;
    
    camera->position.setValue(newPosition);
}

void NavigationController::viewAll() {
    if (m_sceneManager) {
        m_sceneManager->viewAll();
    }
}

void NavigationController::viewTop() {
    if (!m_sceneManager) {
        return;
    }
    
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        return;
    }
    
    // Set camera to top view
    SbVec3f position(0, 0, 10);
    SbVec3f focalPoint(0, 0, 0);
    SbVec3f upVector(0, 1, 0);
    
    camera->position.setValue(position);
    camera->focalDistance.setValue(10.0f);
    camera->orientation.setValue(SbRotation::identity());
    
    if (m_canvas) {
        m_canvas->Refresh();
    }
}

void NavigationController::viewFront() {
    if (!m_sceneManager) {
        return;
    }
    
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        return;
    }
    
    // Set camera to front view
    SbVec3f position(0, -10, 0);
    SbVec3f focalPoint(0, 0, 0);
    SbVec3f upVector(0, 0, 1);
    
    camera->position.setValue(position);
    camera->focalDistance.setValue(10.0f);
    camera->orientation.setValue(SbRotation::identity());
    
    if (m_canvas) {
        m_canvas->Refresh();
    }
}

void NavigationController::viewRight() {
    if (!m_sceneManager) {
        return;
    }
    
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        return;
    }
    
    // Set camera to right view
    SbVec3f position(10, 0, 0);
    SbVec3f focalPoint(0, 0, 0);
    SbVec3f upVector(0, 0, 1);
    
    camera->position.setValue(position);
    camera->focalDistance.setValue(10.0f);
    camera->orientation.setValue(SbRotation::identity());
    
    if (m_canvas) {
        m_canvas->Refresh();
    }
}

void NavigationController::viewIsometric() {
    if (!m_sceneManager) {
        return;
    }
    
    SoCamera* camera = m_sceneManager->getCamera();
    if (!camera) {
        return;
    }
    
    // Set camera to isometric view
    SbVec3f position(10, 10, 10);
    SbVec3f focalPoint(0, 0, 0);
    SbVec3f upVector(0, 0, 1);
    
    camera->position.setValue(position);
    camera->focalDistance.setValue(17.32f); // sqrt(10^2 + 10^2 + 10^2)
    camera->orientation.setValue(SbRotation::identity());
    
    if (m_canvas) {
        m_canvas->Refresh();
    }
}

void NavigationController::setZoomSpeedFactor(float factor) {
    m_zoomSpeedFactor = factor;
    LOG_INF_S("NavigationController: Zoom speed factor set to " + std::to_string(factor));
}

float NavigationController::getZoomSpeedFactor() const {
    return m_zoomSpeedFactor;
}

void NavigationController::setRefreshStrategy(RefreshStrategy strategy) {
    m_refreshStrategy = strategy;
    LOG_INF_S("NavigationController: Refresh strategy set to " + std::to_string(static_cast<int>(strategy)));
}

NavigationController::RefreshStrategy NavigationController::getRefreshStrategy() const {
    return m_refreshStrategy;
}

void NavigationController::setAsyncRenderingEnabled(bool enabled) {
    m_asyncRenderingEnabled = enabled;
    LOG_INF_S("NavigationController: Async rendering " + std::string(enabled ? "enabled" : "disabled"));
}

bool NavigationController::isAsyncRenderingEnabled() const {
    return m_asyncRenderingEnabled;
}

void NavigationController::setLODEnabled(bool enabled) {
    m_lodEnabled = enabled;
    LOG_INF_S("NavigationController: LOD " + std::string(enabled ? "enabled" : "disabled"));
}

bool NavigationController::isLODEnabled() const {
    return m_lodEnabled;
}

void NavigationController::setLODTransitionTime(int milliseconds) {
    m_lodTransitionTime = milliseconds;
    LOG_INF_S("NavigationController: LOD transition time set to " + std::to_string(milliseconds) + "ms");
}

int NavigationController::getLODTransitionTime() const {
    return m_lodTransitionTime;
}

void NavigationController::setPerformanceMonitoringEnabled(bool enabled) {
    m_performanceMonitoringEnabled = enabled;
    LOG_INF_S("NavigationController: Performance monitoring " + std::string(enabled ? "enabled" : "disabled"));
}

bool NavigationController::isPerformanceMonitoringEnabled() const {
    return m_performanceMonitoringEnabled;
}

NavigationController::PerformanceMetrics NavigationController::getPerformanceMetrics() const {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    return m_performanceMetrics;
}
