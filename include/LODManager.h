#pragma once

#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <wx/timer.h>

class SceneManager;
class OCCGeometry;

/**
 * @brief Enhanced Level of Detail (LOD) Manager
 * 
 * Provides intelligent LOD management with adaptive quality adjustment,
 * performance monitoring, and smooth transitions between detail levels.
 */
class LODManager : public wxEvtHandler {
public:
    enum class LODLevel {
        ULTRA_FINE,    // Highest quality, slowest rendering
        FINE,          // High quality, normal rendering
        MEDIUM,        // Medium quality, faster rendering
        ROUGH,         // Low quality, fast rendering
        ULTRA_ROUGH    // Lowest quality, fastest rendering
    };
    
    struct LODSettings {
        double deflection;           // Mesh deflection for this level
        double angularDeflection;    // Angular deflection
        bool relative;               // Relative deflection
        bool inParallel;             // Parallel processing
        int transitionTime;          // Transition time in ms
        float performanceThreshold;  // FPS threshold for this level
        
        LODSettings() : deflection(0.01), angularDeflection(0.2), relative(true), 
                       inParallel(true), transitionTime(500), performanceThreshold(45.0f) {}
        
        LODSettings(double def, double angDef, bool rel, bool parallel, int transTime, float perfThresh)
            : deflection(def), angularDeflection(angDef), relative(rel), inParallel(parallel),
              transitionTime(transTime), performanceThreshold(perfThresh) {}
    };
    
    struct PerformanceProfile {
        double targetFPS;            // Target FPS for this profile
        LODLevel defaultLevel;       // Default LOD level
        std::vector<LODLevel> fallbackLevels; // Fallback levels if performance drops
        bool adaptiveEnabled;        // Enable adaptive LOD
        
        PerformanceProfile() : targetFPS(60.0), defaultLevel(LODLevel::FINE), adaptiveEnabled(true) {}
    };

public:
    explicit LODManager(SceneManager* sceneManager);
    ~LODManager();
    
    // Core LOD control
    void setLODEnabled(bool enabled);
    bool isLODEnabled() const;
    
    void setLODLevel(LODLevel level);
    LODLevel getCurrentLODLevel() const;
    
    // Settings management
    void setLODSettings(LODLevel level, const LODSettings& settings);
    LODSettings getLODSettings(LODLevel level) const;
    
    // Performance-based LOD
    void setPerformanceProfile(const PerformanceProfile& profile);
    PerformanceProfile getPerformanceProfile() const;
    
    void setAdaptiveLODEnabled(bool enabled);
    bool isAdaptiveLODEnabled() const;
    
    // Interaction handling
    void startInteraction();
    void endInteraction();
    void updateInteraction();
    
    // Transition control
    void setTransitionTime(int milliseconds);
    int getTransitionTime() const;
    
    void setSmoothTransitionsEnabled(bool enabled);
    bool isSmoothTransitionsEnabled() const;
    
    // Geometry-specific LOD
    void setGeometryLODLevel(const std::string& geometryName, LODLevel level);
    LODLevel getGeometryLODLevel(const std::string& geometryName) const;
    
    void setGeometryLODEnabled(const std::string& geometryName, bool enabled);
    bool isGeometryLODEnabled(const std::string& geometryName) const;
    
    // Performance monitoring
    void setPerformanceMonitoringEnabled(bool enabled);
    bool isPerformanceMonitoringEnabled() const;
    
    void recordFrameTime(std::chrono::nanoseconds frameTime);
    
    struct PerformanceMetrics {
        double currentFPS;
        double averageFPS;
        double minFPS;
        double maxFPS;
        size_t frameCount;
        size_t droppedFrames;
        LODLevel currentLevel;
        bool isTransitioning;
        
        PerformanceMetrics() : currentFPS(0.0), averageFPS(0.0), minFPS(0.0), maxFPS(0.0),
                              frameCount(0), droppedFrames(0), currentLevel(LODLevel::FINE), isTransitioning(false) {}
    };
    
    PerformanceMetrics getPerformanceMetrics() const;
    
    // Callbacks
    using LODChangeCallback = std::function<void(LODLevel oldLevel, LODLevel newLevel)>;
    void setLODChangeCallback(LODChangeCallback callback);
    
    using PerformanceCallback = std::function<void(const PerformanceMetrics& metrics)>;
    void setPerformanceCallback(PerformanceCallback callback);

private:
    // Timer event handlers
    void onTransitionTimer(wxTimerEvent& event);
    void onPerformanceTimer(wxTimerEvent& event);
    
    // Internal LOD management
    void switchToLODLevel(LODLevel level, bool immediate = false);
    void applyLODSettings(const LODSettings& settings);
    void updateGeometryLOD();
    
    // Performance monitoring
    void updatePerformanceMetrics();
    void adjustLODForPerformance();
    
    // Transition management
    void startTransition(LODLevel targetLevel);
    void updateTransition();
    void completeTransition();
    
    // Utility methods
    LODSettings getDefaultLODSettings(LODLevel level) const;
    double calculateOptimalDeflection(LODLevel level) const;
    bool shouldTransitionToLevel(LODLevel level) const;
    
    // Initialization
    void initializeDefaultSettings();

private:
    SceneManager* m_sceneManager;
    
    // Core state
    std::atomic<bool> m_lodEnabled;
    std::atomic<int> m_currentLevel;  // Store as int for atomic operations
    std::atomic<int> m_targetLevel;   // Store as int for atomic operations
    std::atomic<bool> m_isTransitioning;
    std::atomic<bool> m_isInteracting;
    
    // Settings
    std::unordered_map<LODLevel, LODSettings> m_lodSettings;
    PerformanceProfile m_performanceProfile;
    
    // Timers
    wxTimer m_transitionTimer;
    wxTimer m_performanceTimer;
    
    // Timing control
    std::chrono::steady_clock::time_point m_lastInteractionTime;
    std::chrono::steady_clock::time_point m_transitionStartTime;
    int m_transitionTime;
    bool m_smoothTransitionsEnabled;
    
    // Performance monitoring
    std::atomic<bool> m_performanceMonitoringEnabled;
    mutable std::mutex m_metricsMutex;
    PerformanceMetrics m_performanceMetrics;
    std::vector<std::chrono::nanoseconds> m_frameTimeHistory;
    static constexpr size_t MAX_FRAME_HISTORY = 60;
    
    // Geometry-specific LOD
    std::unordered_map<std::string, LODLevel> m_geometryLODLevels;
    std::unordered_map<std::string, bool> m_geometryLODEnabled;
    mutable std::mutex m_geometryMutex;
    
    // Callbacks
    LODChangeCallback m_lodChangeCallback;
    PerformanceCallback m_performanceCallback;
    
    // Transition state
    float m_transitionProgress;
    LODSettings m_transitionStartSettings;
    LODSettings m_transitionEndSettings;
    
    wxDECLARE_EVENT_TABLE();
}; 