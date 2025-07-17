#pragma once

#include <chrono>
#include <vector>
#include <atomic>
#include <mutex>
#include <functional>
#include <string>
#include <unordered_map>

/**
 * @brief Comprehensive Performance Monitor
 * 
 * Monitors various performance metrics and provides optimization recommendations
 */
class PerformanceMonitor {
public:
    struct FrameMetrics {
        std::chrono::nanoseconds frameTime{0};
        std::chrono::nanoseconds renderTime{0};
        std::chrono::nanoseconds eventTime{0};
        std::chrono::nanoseconds lodTime{0};
        size_t triangleCount{0};
        size_t vertexCount{0};
        size_t drawCalls{0};
        double fps{0};
        bool isDroppedFrame{false};
        
        FrameMetrics() : frameTime(0), renderTime(0), eventTime(0), lodTime(0),
                        triangleCount(0), vertexCount(0), drawCalls(0), fps(0), isDroppedFrame(false) {}
    };
    
    struct PerformanceReport {
        double averageFPS{0.0};
        double minFPS{0.0};
        double maxFPS{0.0};
        double frameTimePercentile95{0.0};
        size_t totalFrames{0};
        size_t droppedFrames{0};
        size_t totalTriangles{0};
        size_t totalVertices{0};
        size_t totalDrawCalls{0};
        std::vector<std::string> recommendations;
        
        PerformanceReport() : averageFPS(0.0), minFPS(0.0), maxFPS(0.0), frameTimePercentile95(0.0),
                             totalFrames(0), droppedFrames(0), totalTriangles(0), totalVertices(0), totalDrawCalls(0) {}
    };
    
    enum class PerformanceLevel {
        EXCELLENT,  // > 55 FPS
        GOOD,       // 30-55 FPS
        ACCEPTABLE, // 20-30 FPS
        POOR,       // 10-20 FPS
        UNACCEPTABLE // < 10 FPS
    };
    
    struct OptimizationRecommendation {
        std::string category;
        std::string description;
        std::string action;
        double expectedImprovement; // Expected FPS improvement
        bool isAutomatic; // Can be applied automatically
        
        OptimizationRecommendation() : expectedImprovement(0.0), isAutomatic(false) {}
        
        OptimizationRecommendation(const std::string& cat, const std::string& desc, 
                                  const std::string& act, double improvement, bool automatic)
            : category(cat), description(desc), action(act), 
              expectedImprovement(improvement), isAutomatic(automatic) {}
    };

public:
    PerformanceMonitor();
    ~PerformanceMonitor();
    
    // Frame recording
    void recordFrame(const FrameMetrics& metrics);
    void startFrame();
    void endFrame();
    
    // Performance analysis
    PerformanceReport generateReport() const;
    PerformanceLevel getCurrentPerformanceLevel() const;
    std::vector<OptimizationRecommendation> getRecommendations() const;
    
    // Configuration
    void setMonitoringEnabled(bool enabled);
    bool isMonitoringEnabled() const;
    
    void setHistorySize(size_t size);
    size_t getHistorySize() const;
    
    void setPerformanceThresholds(double excellent, double good, double acceptable, double poor);
    
    // Callbacks
    using PerformanceCallback = std::function<void(const PerformanceReport&)>;
    void setPerformanceCallback(PerformanceCallback callback);
    
    using RecommendationCallback = std::function<void(const OptimizationRecommendation&)>;
    void setRecommendationCallback(RecommendationCallback callback);
    
    // Automatic optimization
    void setAutoOptimizationEnabled(bool enabled);
    bool isAutoOptimizationEnabled() const;
    
    void applyAutomaticOptimizations();

private:
    // Performance analysis
    void analyzePerformance();
    void generateRecommendations();
    void updatePerformanceLevel();
    
    // Optimization logic
    void optimizeLODSettings();
    void optimizeRefreshStrategy();
    void optimizeRenderingSettings();
    
    // Utility methods
    double calculatePercentile(const std::vector<double>& values, double percentile) const;
    std::string getPerformanceLevelString(PerformanceLevel level) const;

private:
    // Configuration
    std::atomic<bool> m_monitoringEnabled;
    std::atomic<bool> m_autoOptimizationEnabled;
    size_t m_historySize;
    
    // Performance thresholds
    double m_excellentThreshold;
    double m_goodThreshold;
    double m_acceptableThreshold;
    double m_poorThreshold;
    
    // Data storage
    mutable std::mutex m_dataMutex;
    std::vector<FrameMetrics> m_frameHistory;
    PerformanceReport m_currentReport;
    PerformanceLevel m_currentPerformanceLevel;
    std::vector<OptimizationRecommendation> m_recommendations;
    
    // Timing
    std::chrono::steady_clock::time_point m_frameStartTime;
    
    // Callbacks
    PerformanceCallback m_performanceCallback;
    RecommendationCallback m_recommendationCallback;
    
    // Statistics
    std::atomic<size_t> m_totalFrames;
    std::atomic<size_t> m_droppedFrames;
    std::atomic<double> m_currentFPS;
    
    // Optimization state
    bool m_optimizationsApplied;
    std::unordered_map<std::string, bool> m_appliedOptimizations;
}; 