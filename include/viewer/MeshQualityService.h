#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

class MeshQualityValidator;

/**
 * @brief Service for mesh quality monitoring and reporting
 *
 * This service provides real-time monitoring of mesh quality parameters,
 * logging of parameter changes, and quality reporting functionality.
 * Separated from MeshQualityValidator to focus on monitoring aspects.
 */
class MeshQualityService {
public:
    MeshQualityService();
    ~MeshQualityService();

    // Parameter monitoring
    void enableParameterMonitoring(bool enabled);
    bool isParameterMonitoringEnabled() const;

    // Parameter change logging
    void logParameterChange(const std::string& parameterName, double oldValue, double newValue);
    void logCurrentMeshSettings();

    // Quality validation and reporting
    bool validateMeshParameters() const;
    std::string getMeshQualityReport() const;
    std::vector<std::string> getValidationIssues() const;

    // Quality metrics
    double getAverageMeshQuality() const;
    double getWorstMeshQuality() const;
    std::unordered_map<std::string, double> getQualityMetrics() const;

    // Statistical reporting
    void exportMeshStatistics(const std::string& filename) const;
    std::string generateQualitySummary() const;

    // Threshold management
    void setQualityThresholds(double minQuality, double maxDeviation);
    void getQualityThresholds(double& minQuality, double& maxDeviation) const;

    // Monitoring configuration
    void setMonitoringInterval(int milliseconds);
    int getMonitoringInterval() const;

    // Alert system
    void enableQualityAlerts(bool enabled);
    bool areQualityAlertsEnabled() const;
    std::vector<std::string> getActiveAlerts() const;

private:
    bool m_parameterMonitoringEnabled;
    double m_minQualityThreshold;
    double m_maxDeviationThreshold;
    int m_monitoringIntervalMs;
    bool m_qualityAlertsEnabled;

    std::vector<std::string> m_validationIssues;
    std::vector<std::string> m_activeAlerts;
    std::unordered_map<std::string, double> m_qualityMetrics;

    // Internal methods
    void updateQualityMetrics();
    void checkQualityThresholds();
    void generateAlert(const std::string& message);
    void clearExpiredAlerts();

    // Parameter tracking
    std::unordered_map<std::string, double> m_lastParameterValues;
    void updateParameterTracking(const std::string& parameterName, double value);
};
