#include "LODManager.h"
#include "SceneManager.h"
#include "OCCViewer.h"
#include "logger/Logger.h"
#include <algorithm>
#include <numeric>
#include <cmath>

wxBEGIN_EVENT_TABLE(LODManager, wxEvtHandler)
    EVT_TIMER(wxID_ANY, LODManager::onTransitionTimer)
    EVT_TIMER(wxID_ANY + 1, LODManager::onPerformanceTimer)
wxEND_EVENT_TABLE()

LODManager::LODManager(SceneManager* sceneManager)
    : m_sceneManager(sceneManager)
    , m_lodEnabled(true)
    , m_currentLevel(static_cast<int>(LODLevel::FINE))
    , m_targetLevel(static_cast<int>(LODLevel::FINE))
    , m_isTransitioning(false)
    , m_isInteracting(false)
    , m_transitionTimer(this)
    , m_performanceTimer(this, wxID_ANY + 1)
    , m_lastInteractionTime(std::chrono::steady_clock::now())
    , m_transitionStartTime(std::chrono::steady_clock::now())
    , m_transitionTime(500) // 500ms default
    , m_smoothTransitionsEnabled(true)
    , m_performanceMonitoringEnabled(true)
    , m_transitionProgress(0.0f)
{
    LOG_INF_S("LODManager: Initializing enhanced LOD system");
    
    // Initialize default LOD settings
    initializeDefaultSettings();
    
    // Initialize performance profile
    m_performanceProfile.targetFPS = 60.0;
    m_performanceProfile.defaultLevel = LODLevel::FINE;
    m_performanceProfile.fallbackLevels = {LODLevel::MEDIUM, LODLevel::ROUGH, LODLevel::ULTRA_ROUGH};
    m_performanceProfile.adaptiveEnabled = true;
    
    // Initialize frame time history
    m_frameTimeHistory.reserve(MAX_FRAME_HISTORY);
    
    // Start performance monitoring timer
    m_performanceTimer.Start(100, wxTIMER_CONTINUOUS); // 100ms monitoring
    
    LOG_INF_S("LODManager: Enhanced LOD system initialized");
}

LODManager::~LODManager() {
    m_transitionTimer.Stop();
    m_performanceTimer.Stop();
    LOG_INF_S("LODManager: Destroying");
}

void LODManager::initializeDefaultSettings() {
    // Ultra Fine - Highest quality
    m_lodSettings[LODLevel::ULTRA_FINE] = LODSettings(0.001, 0.1, true, true, 1000, 55.0f);
    
    // Fine - High quality
    m_lodSettings[LODLevel::FINE] = LODSettings(0.01, 0.2, true, true, 500, 45.0f);
    
    // Medium - Medium quality
    m_lodSettings[LODLevel::MEDIUM] = LODSettings(0.05, 0.5, true, true, 300, 30.0f);
    
    // Rough - Low quality
    m_lodSettings[LODLevel::ROUGH] = LODSettings(0.1, 1.0, true, true, 200, 20.0f);
    
    // Ultra Rough - Lowest quality
    m_lodSettings[LODLevel::ULTRA_ROUGH] = LODSettings(0.2, 2.0, true, true, 100, 10.0f);
}

void LODManager::setLODEnabled(bool enabled) {
    if (m_lodEnabled != enabled) {
        m_lodEnabled = enabled;
        
        if (!enabled) {
            // Stop all timers and transitions
            m_transitionTimer.Stop();
            m_isTransitioning = false;
            
            // Switch to fine level when disabling
            switchToLODLevel(LODLevel::FINE, true);
        }
        
        LOG_INF_S("LODManager: LOD " + std::string(enabled ? "enabled" : "disabled"));
    }
}

bool LODManager::isLODEnabled() const {
    return m_lodEnabled;
}

void LODManager::setLODLevel(LODLevel level) {
    if (!m_lodEnabled) {
        return;
    }
    
    int currentLevelInt = m_currentLevel.load();
    int targetLevelInt = static_cast<int>(level);
    
    if (currentLevelInt != targetLevelInt && !m_isTransitioning) {
        if (m_smoothTransitionsEnabled && !m_isInteracting) {
            startTransition(level);
        } else {
            switchToLODLevel(level, true);
        }
    }
}

LODManager::LODLevel LODManager::getCurrentLODLevel() const {
    return static_cast<LODLevel>(m_currentLevel.load());
}

void LODManager::setLODSettings(LODLevel level, const LODSettings& settings) {
    m_lodSettings[level] = settings;
    LOG_INF_S("LODManager: Updated settings for level " + std::to_string(static_cast<int>(level)));
}

LODManager::LODSettings LODManager::getLODSettings(LODLevel level) const {
    auto it = m_lodSettings.find(level);
    if (it != m_lodSettings.end()) {
        return it->second;
    }
    return getDefaultLODSettings(level);
}

void LODManager::setPerformanceProfile(const PerformanceProfile& profile) {
    m_performanceProfile = profile;
    LOG_INF_S("LODManager: Updated performance profile");
}

LODManager::PerformanceProfile LODManager::getPerformanceProfile() const {
    return m_performanceProfile;
}

void LODManager::setAdaptiveLODEnabled(bool enabled) {
    m_performanceProfile.adaptiveEnabled = enabled;
    LOG_INF_S("LODManager: Adaptive LOD " + std::string(enabled ? "enabled" : "disabled"));
}

bool LODManager::isAdaptiveLODEnabled() const {
    return m_performanceProfile.adaptiveEnabled;
}

void LODManager::startInteraction() {
    m_isInteracting = true;
    m_lastInteractionTime = std::chrono::steady_clock::now();
    
    // Switch to rough mode immediately for interaction
    LODLevel currentLevel = static_cast<LODLevel>(m_currentLevel.load());
    if (m_lodEnabled && currentLevel > LODLevel::ROUGH) {
        switchToLODLevel(LODLevel::ROUGH, true);
    }
    
    LOG_DBG_S("LODManager: Interaction started");
}

void LODManager::endInteraction() {
    m_isInteracting = false;
    
    // Schedule transition back to default level
    if (m_lodEnabled && m_smoothTransitionsEnabled) {
        startTransition(m_performanceProfile.defaultLevel);
    }
    
    LOG_DBG_S("LODManager: Interaction ended");
}

void LODManager::updateInteraction() {
    m_lastInteractionTime = std::chrono::steady_clock::now();
}

void LODManager::setTransitionTime(int milliseconds) {
    m_transitionTime = milliseconds;
    LOG_INF_S("LODManager: Transition time set to " + std::to_string(milliseconds) + "ms");
}

int LODManager::getTransitionTime() const {
    return m_transitionTime;
}

void LODManager::setSmoothTransitionsEnabled(bool enabled) {
    m_smoothTransitionsEnabled = enabled;
    LOG_INF_S("LODManager: Smooth transitions " + std::string(enabled ? "enabled" : "disabled"));
}

bool LODManager::isSmoothTransitionsEnabled() const {
    return m_smoothTransitionsEnabled;
}

void LODManager::setGeometryLODLevel(const std::string& geometryName, LODLevel level) {
    std::lock_guard<std::mutex> lock(m_geometryMutex);
    m_geometryLODLevels[geometryName] = level;
    updateGeometryLOD();
}

LODManager::LODLevel LODManager::getGeometryLODLevel(const std::string& geometryName) const {
    std::lock_guard<std::mutex> lock(m_geometryMutex);
    auto it = m_geometryLODLevels.find(geometryName);
    return it != m_geometryLODLevels.end() ? it->second : static_cast<LODLevel>(m_currentLevel.load());
}

void LODManager::setGeometryLODEnabled(const std::string& geometryName, bool enabled) {
    std::lock_guard<std::mutex> lock(m_geometryMutex);
    m_geometryLODEnabled[geometryName] = enabled;
    updateGeometryLOD();
}

bool LODManager::isGeometryLODEnabled(const std::string& geometryName) const {
    std::lock_guard<std::mutex> lock(m_geometryMutex);
    auto it = m_geometryLODEnabled.find(geometryName);
    return it != m_geometryLODEnabled.end() ? it->second : true;
}

void LODManager::setPerformanceMonitoringEnabled(bool enabled) {
    m_performanceMonitoringEnabled = enabled;
    if (enabled) {
        m_performanceTimer.Start(100, wxTIMER_CONTINUOUS);
    } else {
        m_performanceTimer.Stop();
    }
    LOG_INF_S("LODManager: Performance monitoring " + std::string(enabled ? "enabled" : "disabled"));
}

bool LODManager::isPerformanceMonitoringEnabled() const {
    return m_performanceMonitoringEnabled;
}

LODManager::PerformanceMetrics LODManager::getPerformanceMetrics() const {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    return m_performanceMetrics;
}

void LODManager::setLODChangeCallback(LODChangeCallback callback) {
    m_lodChangeCallback = callback;
}

void LODManager::setPerformanceCallback(PerformanceCallback callback) {
    m_performanceCallback = callback;
}

void LODManager::onTransitionTimer(wxTimerEvent& event) {
    updateTransition();
}

void LODManager::onPerformanceTimer(wxTimerEvent& event) {
    if (m_performanceMonitoringEnabled) {
        updatePerformanceMetrics();
        
        if (m_performanceProfile.adaptiveEnabled) {
            adjustLODForPerformance();
        }
        
        // Notify callback
        if (m_performanceCallback) {
            m_performanceCallback(m_performanceMetrics);
        }
    }
}

void LODManager::switchToLODLevel(LODLevel level, bool immediate) {
    int currentLevelInt = m_currentLevel.load();
    int targetLevelInt = static_cast<int>(level);
    
    if (currentLevelInt == targetLevelInt) {
        return;
    }
    
    LODLevel oldLevel = static_cast<LODLevel>(currentLevelInt);
    m_currentLevel.store(targetLevelInt);
    
    // Apply LOD settings
    auto settings = getLODSettings(level);
    applyLODSettings(settings);
    
    // Update geometry LOD
    updateGeometryLOD();
    
    // Notify callback
    if (m_lodChangeCallback) {
        m_lodChangeCallback(oldLevel, level);
    }
    
    LOG_INF_S("LODManager: Switched to level " + std::to_string(static_cast<int>(level)));
}

void LODManager::applyLODSettings(const LODSettings& settings) {
    if (!m_sceneManager) {
        return;
    }
    
    // Apply settings to scene manager or OCC viewer
    // This would typically involve updating mesh parameters
    LOG_DBG_S("LODManager: Applied settings - deflection: " + std::to_string(settings.deflection));
}

void LODManager::updateGeometryLOD() {
    if (!m_sceneManager) {
        return;
    }
    
    // Update individual geometry LOD levels
    std::lock_guard<std::mutex> lock(m_geometryMutex);
    
    for (const auto& [name, level] : m_geometryLODLevels) {
        if (m_geometryLODEnabled[name]) {
            auto settings = getLODSettings(level);
            // Apply settings to specific geometry
            LOG_DBG_S("LODManager: Updated geometry " + name + " to level " + std::to_string(static_cast<int>(level)));
        }
    }
}

void LODManager::recordFrameTime(std::chrono::nanoseconds frameTime) {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    m_frameTimeHistory.push_back(frameTime);
    if (m_frameTimeHistory.size() > MAX_FRAME_HISTORY) {
        m_frameTimeHistory.erase(m_frameTimeHistory.begin());
    }
    
    m_performanceMetrics.frameCount++;
    
    // Check for dropped frames
    if (frameTime > std::chrono::milliseconds(33)) { // 30 FPS threshold
        m_performanceMetrics.droppedFrames++;
    }
}

void LODManager::updatePerformanceMetrics() {
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
        m_performanceMetrics.averageFPS = 1000000000.0 / averageFrameTime.count();
    }
    
    // Update current FPS (most recent frame)
    auto currentFrameTime = m_frameTimeHistory.back();
    if (currentFrameTime.count() > 0) {
        m_performanceMetrics.currentFPS = 1000000000.0 / currentFrameTime.count();
    }
    
    // Update min/max FPS
    auto minFrameTime = *std::min_element(m_frameTimeHistory.begin(), m_frameTimeHistory.end());
    auto maxFrameTime = *std::max_element(m_frameTimeHistory.begin(), m_frameTimeHistory.end());
    
    if (minFrameTime.count() > 0) {
        m_performanceMetrics.maxFPS = 1000000000.0 / minFrameTime.count();
    }
    if (maxFrameTime.count() > 0) {
        m_performanceMetrics.minFPS = 1000000000.0 / maxFrameTime.count();
    }
    
    m_performanceMetrics.currentLevel = static_cast<LODLevel>(m_currentLevel.load());
    m_performanceMetrics.isTransitioning = m_isTransitioning;
}

void LODManager::adjustLODForPerformance() {
    if (!m_performanceProfile.adaptiveEnabled) {
        return;
    }
    
    double currentFPS = m_performanceMetrics.currentFPS;
    LODLevel targetLevel = static_cast<LODLevel>(m_currentLevel.load());
    
    // Find appropriate level based on performance
    for (LODLevel level : m_performanceProfile.fallbackLevels) {
        auto settings = getLODSettings(level);
        if (currentFPS < settings.performanceThreshold) {
            targetLevel = level;
            break;
        }
    }
    
    // If performance is good, try to improve quality
    LODLevel currentLevel = static_cast<LODLevel>(m_currentLevel.load());
    if (currentFPS > m_performanceProfile.targetFPS && currentLevel > LODLevel::FINE) {
        targetLevel = static_cast<LODLevel>(static_cast<int>(currentLevel) - 1);
    }
    
    LODLevel currentLevel2 = static_cast<LODLevel>(m_currentLevel.load());
    if (targetLevel != currentLevel2 && shouldTransitionToLevel(targetLevel)) {
        setLODLevel(targetLevel);
    }
}

void LODManager::startTransition(LODLevel targetLevel) {
    if (m_isTransitioning) {
        return;
    }
    
    m_targetLevel.store(static_cast<int>(targetLevel));
    m_isTransitioning = true;
    m_transitionProgress = 0.0f;
    m_transitionStartTime = std::chrono::steady_clock::now();
    
    // Store start and end settings
    m_transitionStartSettings = getLODSettings(static_cast<LODLevel>(m_currentLevel.load()));
    m_transitionEndSettings = getLODSettings(targetLevel);
    
    // Start transition timer
    m_transitionTimer.Start(16, wxTIMER_CONTINUOUS); // 60 FPS transition updates
    
    LOG_DBG_S("LODManager: Started transition to level " + std::to_string(static_cast<int>(targetLevel)));
}

void LODManager::updateTransition() {
    if (!m_isTransitioning) {
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_transitionStartTime
    );
    
    m_transitionProgress = std::min(1.0f, static_cast<float>(elapsed.count()) / m_transitionTime);
    
    // Interpolate between start and end settings
    LODSettings interpolatedSettings;
    interpolatedSettings.deflection = m_transitionStartSettings.deflection + 
        (m_transitionEndSettings.deflection - m_transitionStartSettings.deflection) * m_transitionProgress;
    interpolatedSettings.angularDeflection = m_transitionStartSettings.angularDeflection + 
        (m_transitionEndSettings.angularDeflection - m_transitionStartSettings.angularDeflection) * m_transitionProgress;
    interpolatedSettings.relative = m_transitionEndSettings.relative;
    interpolatedSettings.inParallel = m_transitionEndSettings.inParallel;
    
    // Apply interpolated settings
    applyLODSettings(interpolatedSettings);
    
    if (m_transitionProgress >= 1.0f) {
        completeTransition();
    }
}

void LODManager::completeTransition() {
    m_isTransitioning = false;
    m_transitionTimer.Stop();
    
    // Update current level directly without triggering another transition
    LODLevel oldLevel = static_cast<LODLevel>(m_currentLevel.load());
    LODLevel targetLevel = static_cast<LODLevel>(m_targetLevel.load());
    m_currentLevel.store(static_cast<int>(targetLevel));
    
    // Apply the final settings
    auto settings = getLODSettings(targetLevel);
    applyLODSettings(settings);
    
    // Notify callback if set
    if (m_lodChangeCallback) {
        m_lodChangeCallback(oldLevel, targetLevel);
    }
    
    LOG_DBG_S("LODManager: Completed transition to level " + std::to_string(static_cast<int>(targetLevel)));
}

LODManager::LODSettings LODManager::getDefaultLODSettings(LODLevel level) const {
    switch (level) {
        case LODLevel::ULTRA_FINE: return LODSettings(0.001, 0.1, true, true, 1000, 55.0f);
        case LODLevel::FINE: return LODSettings(0.01, 0.2, true, true, 500, 45.0f);
        case LODLevel::MEDIUM: return LODSettings(0.05, 0.5, true, true, 300, 30.0f);
        case LODLevel::ROUGH: return LODSettings(0.1, 1.0, true, true, 200, 20.0f);
        case LODLevel::ULTRA_ROUGH: return LODSettings(0.2, 2.0, true, true, 100, 10.0f);
        default: return LODSettings(0.01, 0.2, true, true, 500, 45.0f);
    }
}

double LODManager::calculateOptimalDeflection(LODLevel level) const {
    // Calculate optimal deflection based on scene complexity and performance
    double baseDeflection = getLODSettings(level).deflection;
    
    // Adjust based on scene size and complexity
    if (m_sceneManager) {
        // This would typically involve analyzing scene complexity
        // For now, return base deflection
        return baseDeflection;
    }
    
    return baseDeflection;
}

bool LODManager::shouldTransitionToLevel(LODLevel level) const {
    // Prevent rapid transitions
    if (m_isTransitioning) {
        return false;
    }
    
    // Don't transition during interaction unless necessary
    if (m_isInteracting && level > LODLevel::ROUGH) {
        return false;
    }
    
    // Check if the transition would be beneficial
    LODLevel currentLevel = static_cast<LODLevel>(m_currentLevel.load());
    auto currentSettings = getLODSettings(currentLevel);
    auto targetSettings = getLODSettings(level);
    
    // Only transition if performance difference is significant
    return std::abs(currentSettings.performanceThreshold - targetSettings.performanceThreshold) > 5.0;
} 