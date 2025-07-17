#include "PerformanceMonitor.h"
#include "logger/Logger.h"
#include <algorithm>
#include <numeric>
#include <cmath>

PerformanceMonitor::PerformanceMonitor()
    : m_monitoringEnabled(true)
    , m_autoOptimizationEnabled(false)
    , m_historySize(120) // 2 seconds at 60 FPS
    , m_excellentThreshold(55.0)
    , m_goodThreshold(30.0)
    , m_acceptableThreshold(20.0)
    , m_poorThreshold(10.0)
    , m_currentPerformanceLevel(PerformanceLevel::GOOD)
    , m_totalFrames(0)
    , m_droppedFrames(0)
    , m_currentFPS(60.0)
    , m_optimizationsApplied(false)
{
    LOG_INF_S("PerformanceMonitor: Initializing comprehensive performance monitoring");
    m_frameHistory.reserve(m_historySize);
}

PerformanceMonitor::~PerformanceMonitor() {
    LOG_INF_S("PerformanceMonitor: Destroying");
}

void PerformanceMonitor::recordFrame(const FrameMetrics& metrics) {
    if (!m_monitoringEnabled) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    m_frameHistory.push_back(metrics);
    if (m_frameHistory.size() > m_historySize) {
        m_frameHistory.erase(m_frameHistory.begin());
    }
    
    m_totalFrames++;
    if (metrics.isDroppedFrame) {
        m_droppedFrames++;
    }
    
    m_currentFPS = metrics.fps;
    
    // Analyze performance periodically
    if (m_totalFrames % 30 == 0) { // Every 30 frames
        analyzePerformance();
    }
}

void PerformanceMonitor::startFrame() {
    m_frameStartTime = std::chrono::steady_clock::now();
}

void PerformanceMonitor::endFrame() {
    auto frameEnd = std::chrono::steady_clock::now();
    auto frameTime = frameEnd - m_frameStartTime;
    
    FrameMetrics metrics;
    metrics.frameTime = frameTime;
    metrics.fps = 1000000000.0 / frameTime.count();
    metrics.isDroppedFrame = frameTime > std::chrono::milliseconds(33); // 30 FPS threshold
    
    recordFrame(metrics);
}

PerformanceMonitor::PerformanceReport PerformanceMonitor::generateReport() const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    
    PerformanceReport report;
    
    if (m_frameHistory.empty()) {
        return report;
    }
    
    // Calculate FPS statistics
    std::vector<double> fpsValues;
    fpsValues.reserve(m_frameHistory.size());
    
    for (const auto& frame : m_frameHistory) {
        fpsValues.push_back(frame.fps);
    }
    
    report.averageFPS = std::accumulate(fpsValues.begin(), fpsValues.end(), 0.0) / fpsValues.size();
    report.minFPS = *std::min_element(fpsValues.begin(), fpsValues.end());
    report.maxFPS = *std::max_element(fpsValues.begin(), fpsValues.end());
    report.frameTimePercentile95 = calculatePercentile(fpsValues, 95.0);
    
    // Calculate rendering statistics
    report.totalFrames = m_totalFrames;
    report.droppedFrames = m_droppedFrames;
    
    // Calculate geometry statistics
    report.totalTriangles = 0;
    report.totalVertices = 0;
    report.totalDrawCalls = 0;
    for (const auto& frame : m_frameHistory) {
        report.totalTriangles += frame.triangleCount;
        report.totalVertices += frame.vertexCount;
        report.totalDrawCalls += frame.drawCalls;
    }
    
    // Add current recommendations
    for (const auto& rec : m_recommendations) {
        report.recommendations.push_back(rec.description);
    }
    
    return report;
}

PerformanceMonitor::PerformanceLevel PerformanceMonitor::getCurrentPerformanceLevel() const {
    return m_currentPerformanceLevel;
}

std::vector<PerformanceMonitor::OptimizationRecommendation> PerformanceMonitor::getRecommendations() const {
    std::lock_guard<std::mutex> lock(m_dataMutex);
    return m_recommendations;
}

void PerformanceMonitor::setMonitoringEnabled(bool enabled) {
    m_monitoringEnabled = enabled;
    LOG_INF_S("PerformanceMonitor: Monitoring " + std::string(enabled ? "enabled" : "disabled"));
}

bool PerformanceMonitor::isMonitoringEnabled() const {
    return m_monitoringEnabled;
}

void PerformanceMonitor::setHistorySize(size_t size) {
    m_historySize = size;
    std::lock_guard<std::mutex> lock(m_dataMutex);
    m_frameHistory.reserve(size);
}

size_t PerformanceMonitor::getHistorySize() const {
    return m_historySize;
}

void PerformanceMonitor::setPerformanceThresholds(double excellent, double good, double acceptable, double poor) {
    m_excellentThreshold = excellent;
    m_goodThreshold = good;
    m_acceptableThreshold = acceptable;
    m_poorThreshold = poor;
    
    LOG_INF_S("PerformanceMonitor: Updated thresholds - Excellent: " + std::to_string(excellent) + 
             ", Good: " + std::to_string(good) + ", Acceptable: " + std::to_string(acceptable) + 
             ", Poor: " + std::to_string(poor));
}

void PerformanceMonitor::setPerformanceCallback(PerformanceCallback callback) {
    m_performanceCallback = callback;
}

void PerformanceMonitor::setRecommendationCallback(RecommendationCallback callback) {
    m_recommendationCallback = callback;
}

void PerformanceMonitor::setAutoOptimizationEnabled(bool enabled) {
    m_autoOptimizationEnabled = enabled;
    LOG_INF_S("PerformanceMonitor: Auto-optimization " + std::string(enabled ? "enabled" : "disabled"));
}

bool PerformanceMonitor::isAutoOptimizationEnabled() const {
    return m_autoOptimizationEnabled;
}

void PerformanceMonitor::applyAutomaticOptimizations() {
    if (!m_autoOptimizationEnabled) {
        return;
    }
    
    LOG_INF_S("PerformanceMonitor: Applying automatic optimizations");
    
    optimizeLODSettings();
    optimizeRefreshStrategy();
    optimizeRenderingSettings();
    
    m_optimizationsApplied = true;
}

void PerformanceMonitor::analyzePerformance() {
    auto report = generateReport();
    
    // Update performance level
    updatePerformanceLevel();
    
    // Generate recommendations
    generateRecommendations();
    
    // Notify callback
    if (m_performanceCallback) {
        m_performanceCallback(report);
    }
    
    // Apply automatic optimizations if enabled
    if (m_autoOptimizationEnabled && m_currentPerformanceLevel <= PerformanceLevel::ACCEPTABLE) {
        applyAutomaticOptimizations();
    }
    
    LOG_DBG_S("PerformanceMonitor: Performance analysis completed - Level: " + 
             getPerformanceLevelString(m_currentPerformanceLevel) + 
            ", FPS: " + std::to_string(report.averageFPS));
}

void PerformanceMonitor::generateRecommendations() {
    m_recommendations.clear();
    
    auto report = generateReport();
    
    // LOD recommendations
    if (report.averageFPS < m_goodThreshold) {
        OptimizationRecommendation rec;
        rec.category = "LOD";
        rec.description = "Enable adaptive LOD for better performance";
        rec.action = "Set LOD to ROUGH mode during interaction";
        rec.expectedImprovement = 15.0;
        rec.isAutomatic = true;
        m_recommendations.push_back(rec);
    }
    
    // Refresh strategy recommendations
    if (report.droppedFrames > report.totalFrames * 0.1) { // More than 10% dropped frames
        OptimizationRecommendation rec;
        rec.category = "Refresh Strategy";
        rec.description = "Switch to throttled refresh mode";
        rec.action = "Set refresh strategy to THROTTLED";
        rec.expectedImprovement = 10.0;
        rec.isAutomatic = true;
        m_recommendations.push_back(rec);
    }
    
    // Rendering recommendations
    if (report.totalTriangles > 1000000) { // More than 1M triangles
        OptimizationRecommendation rec;
        rec.category = "Rendering";
        rec.description = "Reduce geometry complexity";
        rec.action = "Increase LOD deflection values";
        rec.expectedImprovement = 20.0;
        rec.isAutomatic = false;
        m_recommendations.push_back(rec);
    }
    
    // Notify callback for each recommendation
    for (const auto& rec : m_recommendations) {
        if (m_recommendationCallback) {
            m_recommendationCallback(rec);
        }
    }
}

void PerformanceMonitor::updatePerformanceLevel() {
    auto report = generateReport();
    
    if (report.averageFPS >= m_excellentThreshold) {
        m_currentPerformanceLevel = PerformanceLevel::EXCELLENT;
    } else if (report.averageFPS >= m_goodThreshold) {
        m_currentPerformanceLevel = PerformanceLevel::GOOD;
    } else if (report.averageFPS >= m_acceptableThreshold) {
        m_currentPerformanceLevel = PerformanceLevel::ACCEPTABLE;
    } else if (report.averageFPS >= m_poorThreshold) {
        m_currentPerformanceLevel = PerformanceLevel::POOR;
    } else {
        m_currentPerformanceLevel = PerformanceLevel::UNACCEPTABLE;
    }
}

void PerformanceMonitor::optimizeLODSettings() {
    LOG_INF_S("PerformanceMonitor: Optimizing LOD settings");
    // Implementation would interact with LODManager
}

void PerformanceMonitor::optimizeRefreshStrategy() {
    LOG_INF_S("PerformanceMonitor: Optimizing refresh strategy");
    // Implementation would interact with NavigationController
}

void PerformanceMonitor::optimizeRenderingSettings() {
    LOG_INF_S("PerformanceMonitor: Optimizing rendering settings");
    // Implementation would interact with rendering system
}

double PerformanceMonitor::calculatePercentile(const std::vector<double>& values, double percentile) const {
    if (values.empty()) {
        return 0.0;
    }
    
    std::vector<double> sortedValues = values;
    std::sort(sortedValues.begin(), sortedValues.end());
    
    size_t index = static_cast<size_t>(percentile / 100.0 * (sortedValues.size() - 1));
    return sortedValues[index];
}

std::string PerformanceMonitor::getPerformanceLevelString(PerformanceLevel level) const {
    switch (level) {
        case PerformanceLevel::EXCELLENT: return "EXCELLENT";
        case PerformanceLevel::GOOD: return "GOOD";
        case PerformanceLevel::ACCEPTABLE: return "ACCEPTABLE";
        case PerformanceLevel::POOR: return "POOR";
        case PerformanceLevel::UNACCEPTABLE: return "UNACCEPTABLE";
        default: return "UNKNOWN";
    }
} 