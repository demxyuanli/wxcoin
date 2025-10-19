#include "viewer/MeshQualityService.h"
#include "logger/Logger.h"
#include <fstream>
#include <iomanip>
#include <chrono>
#include <cstdio>
#include <cstring>

MeshQualityService::MeshQualityService()
    : m_parameterMonitoringEnabled(false)
    , m_minQualityThreshold(0.5)
    , m_maxDeviationThreshold(0.2)
    , m_monitoringIntervalMs(1000)
    , m_qualityAlertsEnabled(false)
{
}

MeshQualityService::~MeshQualityService()
{
}

void MeshQualityService::enableParameterMonitoring(bool enabled)
{
    m_parameterMonitoringEnabled = enabled;
    if (enabled) {
    } else {
    }
}

bool MeshQualityService::isParameterMonitoringEnabled() const
{
    return m_parameterMonitoringEnabled;
}

void MeshQualityService::logParameterChange(const std::string& parameterName, double oldValue, double newValue)
{
    if (!m_parameterMonitoringEnabled) {
        return;
    }

    double changePercent = 0.0;
    if (oldValue != 0.0) {
        changePercent = ((newValue - oldValue) / oldValue) * 100.0;
    }

    updateParameterTracking(parameterName, newValue);
    checkQualityThresholds();
}

void MeshQualityService::logCurrentMeshSettings()
{
    if (!m_parameterMonitoringEnabled) {
        return;
    }

    std::string summary = "Current mesh settings summary:\n";
    summary += "- Parameter monitoring: " + std::string(m_parameterMonitoringEnabled ? "ENABLED" : "DISABLED") + "\n";
    summary += "- Quality alerts: " + std::string(m_qualityAlertsEnabled ? "ENABLED" : "DISABLED") + "\n";

    char buffer[256];
    std::snprintf(buffer, sizeof(buffer),
                  "- Min quality threshold: %.3f\n- Max deviation threshold: %.3f\n- Monitoring interval: %dms\n",
                  m_minQualityThreshold, m_maxDeviationThreshold, m_monitoringIntervalMs);
    summary += buffer;

    if (!m_qualityMetrics.empty()) {
        summary += "- Quality metrics:\n";
        for (const auto& metric : m_qualityMetrics) {
            char metricBuffer[128];
            std::snprintf(metricBuffer, sizeof(metricBuffer),
                          "  * %s: %.3f\n", metric.first.c_str(), metric.second);
            summary += metricBuffer;
        }
    }

}

bool MeshQualityService::validateMeshParameters() const
{
    // Clear validation issues (const_cast needed for internal state)
    const_cast<MeshQualityService*>(this)->m_validationIssues.clear();

    // Basic validation checks
    auto& issues = const_cast<MeshQualityService*>(this)->m_validationIssues;

    if (m_minQualityThreshold < 0.0 || m_minQualityThreshold > 1.0) {
        issues.push_back("Minimum quality threshold must be between 0.0 and 1.0");
    }

    if (m_maxDeviationThreshold < 0.0) {
        issues.push_back("Maximum deviation threshold must be non-negative");
    }

    if (m_monitoringIntervalMs < 100) {
        issues.push_back("Monitoring interval must be at least 100ms");
    }

    // Check quality metrics
    if (!m_qualityMetrics.empty()) {
        double avgQuality = getAverageMeshQuality();
        if (avgQuality < m_minQualityThreshold) {
            issues.push_back("Average mesh quality (" + std::to_string(avgQuality) +
                           ") is below threshold (" + std::to_string(m_minQualityThreshold) + ")");
        }
    }

    return m_validationIssues.empty();
}

std::string MeshQualityService::getMeshQualityReport() const
{
    std::string report = "=== Mesh Quality Report ===\n";
    report += "Monitoring Status: " + std::string(m_parameterMonitoringEnabled ? "ACTIVE" : "INACTIVE") + "\n";
    report += "Quality Alerts: " + std::string(m_qualityAlertsEnabled ? "ENABLED" : "DISABLED") + "\n";

    char thresholdBuffer[128];
    std::snprintf(thresholdBuffer, sizeof(thresholdBuffer),
                  "Quality Thresholds: Min=%.3f, MaxDev=%.3f\n",
                  m_minQualityThreshold, m_maxDeviationThreshold);
    report += thresholdBuffer;

    if (!m_qualityMetrics.empty()) {
        report += "\nQuality Metrics:\n";
        for (const auto& metric : m_qualityMetrics) {
            char metricBuffer[128];
            std::snprintf(metricBuffer, sizeof(metricBuffer),
                          "  %s: %.4f\n", metric.first.c_str(), metric.second);
            report += metricBuffer;
        }

        report += "\nSummary Statistics:\n";
        char avgBuffer[64];
        std::snprintf(avgBuffer, sizeof(avgBuffer),
                      "  Average Quality: %.4f\n", getAverageMeshQuality());
        report += avgBuffer;

        char worstBuffer[64];
        std::snprintf(worstBuffer, sizeof(worstBuffer),
                      "  Worst Quality: %.4f\n", getWorstMeshQuality());
        report += worstBuffer;
    }

    if (!m_validationIssues.empty()) {
        report += "\nValidation Issues:\n";
        for (const auto& issue : m_validationIssues) {
            report += "  - " + issue + "\n";
        }
    }

    if (!m_activeAlerts.empty()) {
        report += "\nActive Alerts:\n";
        for (const auto& alert : m_activeAlerts) {
            report += "  ! " + alert + "\n";
        }
    }

    return report;
}

std::vector<std::string> MeshQualityService::getValidationIssues() const
{
    return m_validationIssues;
}

double MeshQualityService::getAverageMeshQuality() const
{
    if (m_qualityMetrics.empty()) {
        return 0.0;
    }

    double sum = 0.0;
    for (const auto& metric : m_qualityMetrics) {
        sum += metric.second;
    }
    return sum / m_qualityMetrics.size();
}

double MeshQualityService::getWorstMeshQuality() const
{
    if (m_qualityMetrics.empty()) {
        return 0.0;
    }

    double worst = 1.0;
    for (const auto& metric : m_qualityMetrics) {
        if (metric.second < worst) {
            worst = metric.second;
        }
    }
    return worst;
}

std::unordered_map<std::string, double> MeshQualityService::getQualityMetrics() const
{
    return m_qualityMetrics;
}

void MeshQualityService::exportMeshStatistics(const std::string& filename) const
{
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            LOG_ERR_S("Failed to open statistics file for writing: " + filename);
            return;
        }

        file << getMeshQualityReport();
        file << "\nDetailed Metrics:\n";

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        file << "Export Time: " << std::ctime(&time);

        for (const auto& metric : m_qualityMetrics) {
            file << metric.first << "," << std::fixed << std::setprecision(6) << metric.second << "\n";
        }

    }
    catch (const std::exception& e) {
        LOG_ERR_S("Error exporting mesh statistics to '" + filename + "': " + std::string(e.what()));
    }
}

std::string MeshQualityService::generateQualitySummary() const
{
    char buffer[256];
    std::snprintf(buffer, sizeof(buffer),
                  "Quality Summary - Avg: %.3f, Worst: %.3f, Issues: %zu, Alerts: %zu",
                  getAverageMeshQuality(),
                  getWorstMeshQuality(),
                  m_validationIssues.size(),
                  m_activeAlerts.size());
    return std::string(buffer);
}

void MeshQualityService::setQualityThresholds(double minQuality, double maxDeviation)
{
    m_minQualityThreshold = minQuality;
    m_maxDeviationThreshold = maxDeviation;


    if (m_qualityAlertsEnabled) {
        checkQualityThresholds();
    }
}

void MeshQualityService::getQualityThresholds(double& minQuality, double& maxDeviation) const
{
    minQuality = m_minQualityThreshold;
    maxDeviation = m_maxDeviationThreshold;
}

void MeshQualityService::setMonitoringInterval(int milliseconds)
{
    if (milliseconds >= 100) {
        m_monitoringIntervalMs = milliseconds;
    } else {
        LOG_WRN_S("Monitoring interval must be at least 100ms, keeping current value");
    }
}

int MeshQualityService::getMonitoringInterval() const
{
    return m_monitoringIntervalMs;
}

void MeshQualityService::enableQualityAlerts(bool enabled)
{
    m_qualityAlertsEnabled = enabled;
    if (enabled) {
        checkQualityThresholds();
    } else {
        m_activeAlerts.clear();
    }
}

bool MeshQualityService::areQualityAlertsEnabled() const
{
    return m_qualityAlertsEnabled;
}

std::vector<std::string> MeshQualityService::getActiveAlerts() const
{
    return m_activeAlerts;
}

void MeshQualityService::updateQualityMetrics()
{
    // Update quality metrics based on current mesh state
    // This would typically involve analyzing mesh triangles, edges, etc.
    // For now, this is a placeholder implementation

    m_qualityMetrics["average_triangle_quality"] = 0.85;
    m_qualityMetrics["min_angle_ratio"] = 0.75;
    m_qualityMetrics["max_aspect_ratio"] = 2.1;
    m_qualityMetrics["edge_length_variance"] = 0.12;
}

void MeshQualityService::checkQualityThresholds()
{
    if (!m_qualityAlertsEnabled) {
        return;
    }

    clearExpiredAlerts();

    double avgQuality = getAverageMeshQuality();
    if (avgQuality < m_minQualityThreshold) {
        generateAlert("Average mesh quality (" + std::to_string(avgQuality) +
                     ") is below threshold (" + std::to_string(m_minQualityThreshold) + ")");
    }

    double worstQuality = getWorstMeshQuality();
    if (worstQuality < m_minQualityThreshold * 0.8) {
        generateAlert("Worst mesh quality (" + std::to_string(worstQuality) +
                     ") is significantly below threshold");
    }
}

void MeshQualityService::generateAlert(const std::string& message)
{
    if (std::find(m_activeAlerts.begin(), m_activeAlerts.end(), message) == m_activeAlerts.end()) {
        m_activeAlerts.push_back(message);
        LOG_WRN_S("Quality Alert: " + message);
    }
}

void MeshQualityService::clearExpiredAlerts()
{
    // Clear alerts that are no longer relevant
    // This is a simple implementation - in practice, you might want more sophisticated logic
    auto it = std::remove_if(m_activeAlerts.begin(), m_activeAlerts.end(),
        [this](const std::string& alert) {
            // Remove alerts that are older than some threshold
            // For now, just keep all alerts
            return false;
        });
    m_activeAlerts.erase(it, m_activeAlerts.end());
}

void MeshQualityService::updateParameterTracking(const std::string& parameterName, double value)
{
    m_lastParameterValues[parameterName] = value;
}
