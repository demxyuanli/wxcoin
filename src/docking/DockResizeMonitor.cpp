#include "docking/DockResizeMonitor.h"
#include "logger/Logger.h"
#include <numeric>
#include <sstream>
#include <iomanip>

namespace ads {

DockResizeMonitor& DockResizeMonitor::getInstance() {
    static DockResizeMonitor instance;
    return instance;
}

void DockResizeMonitor::startResize(const wxSize& startSize) {
    if (!m_enabled || m_resizeInProgress) {
        return;
    }
    
    m_resizeInProgress = true;
    m_resizeStartTime = std::chrono::steady_clock::now();
    
    // Reset current metrics
    m_currentMetrics = ResizeMetrics();
    m_currentMetrics.startSize = startSize;
}

void DockResizeMonitor::endResize(const wxSize& endSize) {
    if (!m_enabled || !m_resizeInProgress) {
        return;
    }
    
    auto endTime = std::chrono::steady_clock::now();
    m_currentMetrics.totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - m_resizeStartTime);
    m_currentMetrics.endSize = endSize;
    
    m_resizeInProgress = false;
    
    // Record metrics to history
    recordMetrics();
    
    // Log summary if duration was significant
    if (m_currentMetrics.totalDuration.count() > 50) {
        LOG_DBG_S("Resize completed: " + std::to_string(m_currentMetrics.totalDuration.count()) + 
                 "ms, Layout: " + std::to_string(m_currentMetrics.layoutCalculation.count()) + 
                 "ms, Paint: " + std::to_string(m_currentMetrics.paintTime.count()) + "ms");
    }
}

void DockResizeMonitor::beginLayoutCalculation() {
    if (!m_enabled || !m_resizeInProgress) {
        return;
    }
    
    m_operationStartTime = std::chrono::steady_clock::now();
}

void DockResizeMonitor::endLayoutCalculation() {
    if (!m_enabled || !m_resizeInProgress) {
        return;
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - m_operationStartTime);
    
    m_currentMetrics.layoutCalculation += duration;
    m_currentMetrics.layoutUpdateCount++;
}

void DockResizeMonitor::beginSplitterAdjustment() {
    if (!m_enabled || !m_resizeInProgress) {
        return;
    }
    
    m_operationStartTime = std::chrono::steady_clock::now();
}

void DockResizeMonitor::endSplitterAdjustment() {
    if (!m_enabled || !m_resizeInProgress) {
        return;
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - m_operationStartTime);
    
    m_currentMetrics.splitterAdjustment += duration;
}

void DockResizeMonitor::beginPaint() {
    if (!m_enabled || !m_resizeInProgress) {
        return;
    }
    
    m_operationStartTime = std::chrono::steady_clock::now();
}

void DockResizeMonitor::endPaint() {
    if (!m_enabled || !m_resizeInProgress) {
        return;
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - m_operationStartTime);
    
    m_currentMetrics.paintTime += duration;
    m_currentMetrics.paintEventCount++;
}

DockResizeMonitor::ResizeMetrics DockResizeMonitor::getAverageMetrics(size_t lastN) const {
    ResizeMetrics avg;
    
    if (m_history.empty()) {
        return avg;
    }
    
    size_t count = std::min(lastN, m_history.size());
    size_t startIdx = m_history.size() - count;
    
    for (size_t i = startIdx; i < m_history.size(); ++i) {
        const auto& metrics = m_history[i];
        avg.totalDuration += metrics.totalDuration;
        avg.layoutCalculation += metrics.layoutCalculation;
        avg.splitterAdjustment += metrics.splitterAdjustment;
        avg.paintTime += metrics.paintTime;
        avg.layoutUpdateCount += metrics.layoutUpdateCount;
        avg.paintEventCount += metrics.paintEventCount;
    }
    
    // Calculate averages
    avg.totalDuration /= count;
    avg.layoutCalculation /= count;
    avg.splitterAdjustment /= count;
    avg.paintTime /= count;
    avg.layoutUpdateCount /= count;
    avg.paintEventCount /= count;
    
    return avg;
}

std::string DockResizeMonitor::generateReport() const {
    std::ostringstream report;
    
    report << "=== Dock Resize Performance Report ===\n";
    report << "History size: " << m_history.size() << " resize operations\n\n";
    
    if (!m_history.empty()) {
        auto avgMetrics = getAverageMetrics(10);
        
        report << "Average metrics (last 10 resizes):\n";
        report << std::fixed << std::setprecision(1);
        report << "  Total duration: " << avgMetrics.totalDuration.count() << "ms\n";
        report << "  Layout calculation: " << avgMetrics.layoutCalculation.count() << "ms\n";
        report << "  Splitter adjustment: " << avgMetrics.splitterAdjustment.count() << "ms\n";
        report << "  Paint time: " << avgMetrics.paintTime.count() << "ms\n";
        report << "  Layout updates per resize: " << avgMetrics.layoutUpdateCount << "\n";
        report << "  Paint events per resize: " << avgMetrics.paintEventCount << "\n\n";
        
        // Find slowest resize
        auto slowest = std::max_element(m_history.begin(), m_history.end(),
            [](const ResizeMetrics& a, const ResizeMetrics& b) {
                return a.totalDuration < b.totalDuration;
            });
        
        if (slowest != m_history.end()) {
            report << "Slowest resize:\n";
            report << "  Duration: " << slowest->totalDuration.count() << "ms\n";
            report << "  Size change: " << slowest->startSize.x << "x" << slowest->startSize.y
                   << " -> " << slowest->endSize.x << "x" << slowest->endSize.y << "\n";
        }
    }
    
    return report.str();
}

void DockResizeMonitor::recordMetrics() {
    m_history.push_back(m_currentMetrics);
    
    // Keep only last 100 resize operations
    if (m_history.size() > 100) {
        m_history.erase(m_history.begin());
    }
}

} // namespace ads