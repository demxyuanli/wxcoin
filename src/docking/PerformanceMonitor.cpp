#include "docking/PerformanceMonitor.h"
#include <wx/log.h>
#include <algorithm>
#include <numeric>

namespace ads {

PerformanceMonitor& PerformanceMonitor::getInstance() {
    static PerformanceMonitor instance;
    return instance;
}

void PerformanceMonitor::startOperation(const std::string& operationName) {
    if (!m_profilingEnabled) {
        return;
    }
    
    m_activeOperations[operationName] = std::chrono::high_resolution_clock::now();
}

void PerformanceMonitor::endOperation(const std::string& operationName) {
    if (!m_profilingEnabled) {
        return;
    }
    
    auto it = m_activeOperations.find(operationName);
    if (it == m_activeOperations.end()) {
        return;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - it->second);
    double durationMs = duration_ns.count() / 1000000.0;
    
    m_operationTimings[operationName].push_back(durationMs);
    
    if (m_operationTimings[operationName].size() > 100) {
        m_operationTimings[operationName].erase(m_operationTimings[operationName].begin());
    }
    
    m_activeOperations.erase(it);
}

void PerformanceMonitor::recordRefresh() {
    if (!m_profilingEnabled) {
        return;
    }
    m_metrics.refreshCount++;
}

void PerformanceMonitor::recordRender(double timeMs) {
    if (!m_profilingEnabled) {
        return;
    }
    m_metrics.renderCount++;
    updateAverage(m_metrics.averageRenderTime, timeMs, m_metrics.renderCount);
}

void PerformanceMonitor::recordLayoutUpdate(double timeMs) {
    if (!m_profilingEnabled) {
        return;
    }
    m_metrics.layoutUpdateCount++;
    updateAverage(m_metrics.averageLayoutTime, timeMs, m_metrics.layoutUpdateCount);
}

void PerformanceMonitor::recordMemoryAllocation(size_t size) {
    if (!m_profilingEnabled) {
        return;
    }
    m_metrics.memoryAllocations++;
    m_metrics.currentMemoryUsage += size;
    if (m_metrics.currentMemoryUsage > m_metrics.peakMemoryUsage) {
        m_metrics.peakMemoryUsage = m_metrics.currentMemoryUsage;
    }
}

void PerformanceMonitor::recordMemoryDeallocation(size_t size) {
    if (!m_profilingEnabled) {
        return;
    }
    m_metrics.memoryDeallocations++;
    if (m_metrics.currentMemoryUsage >= size) {
        m_metrics.currentMemoryUsage -= size;
    } else {
        m_metrics.currentMemoryUsage = 0;
    }
}

void PerformanceMonitor::resetMetrics() {
    m_metrics.reset();
    m_operationTimings.clear();
    m_activeOperations.clear();
}

double PerformanceMonitor::getAverageOperationTime(const std::string& operationName) const {
    auto it = m_operationTimings.find(operationName);
    if (it == m_operationTimings.end() || it->second.empty()) {
        return 0.0;
    }
    
    double sum = std::accumulate(it->second.begin(), it->second.end(), 0.0);
    return sum / it->second.size();
}

std::vector<std::string> PerformanceMonitor::getSlowOperations(double thresholdMs) const {
    std::vector<std::string> slowOps;
    
    for (const auto& pair : m_operationTimings) {
        double avg = getAverageOperationTime(pair.first);
        if (avg > thresholdMs) {
            slowOps.push_back(pair.first + " (avg: " + std::to_string(avg) + "ms)");
        }
    }
    
    return slowOps;
}

void PerformanceMonitor::logMetrics() const {
    if (!m_profilingEnabled) {
        return;
    }
    
    wxLogDebug("=== Performance Metrics ===");
    wxLogDebug("Refresh count: %d", m_metrics.refreshCount);
    wxLogDebug("Render count: %d (avg: %.2f ms)", m_metrics.renderCount, m_metrics.averageRenderTime);
    wxLogDebug("Layout updates: %d (avg: %.2f ms)", m_metrics.layoutUpdateCount, m_metrics.averageLayoutTime);
    wxLogDebug("Memory: %zu bytes (peak: %zu)", m_metrics.currentMemoryUsage, m_metrics.peakMemoryUsage);
    wxLogDebug("Allocations: %d, Deallocations: %d", m_metrics.memoryAllocations, m_metrics.memoryDeallocations);
    
    auto slowOps = getSlowOperations(16.0);
    if (!slowOps.empty()) {
        wxLogDebug("Slow operations (>16ms):");
        for (const auto& op : slowOps) {
            wxLogDebug("  - %s", op.c_str());
        }
    }
}

void PerformanceMonitor::updateAverage(double& average, double newValue, int& count) {
    if (count == 1) {
        average = newValue;
    } else {
        average = (average * (count - 1) + newValue) / count;
    }
}

ScopedPerformanceTimer::ScopedPerformanceTimer(const std::string& operationName)
    : m_operationName(operationName) {
    PerformanceMonitor::getInstance().startOperation(operationName);
}

ScopedPerformanceTimer::~ScopedPerformanceTimer() {
    PerformanceMonitor::getInstance().endOperation(m_operationName);
}

} // namespace ads

