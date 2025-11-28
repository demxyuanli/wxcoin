#pragma once

#include <wx/wx.h>
#include <chrono>
#include <map>
#include <string>
#include <vector>
#include <memory>

namespace ads {

struct PerformanceMetrics {
    int refreshCount = 0;
    int renderCount = 0;
    int layoutUpdateCount = 0;
    int memoryAllocations = 0;
    int memoryDeallocations = 0;
    double averageRefreshTime = 0.0;
    double averageRenderTime = 0.0;
    double averageLayoutTime = 0.0;
    size_t currentMemoryUsage = 0;
    size_t peakMemoryUsage = 0;
    
    void reset() {
        refreshCount = 0;
        renderCount = 0;
        layoutUpdateCount = 0;
        memoryAllocations = 0;
        memoryDeallocations = 0;
        averageRefreshTime = 0.0;
        averageRenderTime = 0.0;
        averageLayoutTime = 0.0;
    }
};

struct OperationTiming {
    std::string operationName;
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point endTime;
    double duration = 0.0;
    
    OperationTiming(const std::string& name) : operationName(name) {
        startTime = std::chrono::high_resolution_clock::now();
    }
    
    void finish() {
        endTime = std::chrono::high_resolution_clock::now();
        auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
        duration = duration_ns.count() / 1000000.0;
    }
};

class PerformanceMonitor {
public:
    static PerformanceMonitor& getInstance();
    
    void startOperation(const std::string& operationName);
    void endOperation(const std::string& operationName);
    
    void recordRefresh();
    void recordRender(double timeMs);
    void recordLayoutUpdate(double timeMs);
    void recordMemoryAllocation(size_t size);
    void recordMemoryDeallocation(size_t size);
    
    PerformanceMetrics getMetrics() const { return m_metrics; }
    void resetMetrics();
    
    double getAverageOperationTime(const std::string& operationName) const;
    std::vector<std::string> getSlowOperations(double thresholdMs = 16.0) const;
    
    void enableProfiling(bool enable) { m_profilingEnabled = enable; }
    bool isProfilingEnabled() const { return m_profilingEnabled; }
    
    void logMetrics() const;
    
private:
    PerformanceMonitor() = default;
    ~PerformanceMonitor() = default;
    PerformanceMonitor(const PerformanceMonitor&) = delete;
    PerformanceMonitor& operator=(const PerformanceMonitor&) = delete;
    
    PerformanceMetrics m_metrics;
    std::map<std::string, std::vector<double>> m_operationTimings;
    std::map<std::string, std::chrono::high_resolution_clock::time_point> m_activeOperations;
    bool m_profilingEnabled = true;
    
    void updateAverage(double& average, double newValue, int& count);
};

class ScopedPerformanceTimer {
public:
    ScopedPerformanceTimer(const std::string& operationName);
    ~ScopedPerformanceTimer();
    
private:
    std::string m_operationName;
};

} // namespace ads

