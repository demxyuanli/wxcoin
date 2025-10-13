#pragma once

#include <string>
#include <vector>
#include <memory>

class OCCGeometry;
struct MeshParameters;

/**
 * @brief Mesh quality validation and monitoring service
 * 
 * Provides comprehensive mesh parameter validation, quality reporting,
 * and real-time parameter change monitoring for debugging and optimization.
 */
class MeshQualityValidator {
public:
    MeshQualityValidator();
    ~MeshQualityValidator() = default;

    // Context setup
    void setContext(
        const std::vector<std::shared_ptr<OCCGeometry>>* geometries,
        const MeshParameters* meshParams);

    // Advanced mesh parameter access
    void setSubdivisionParams(bool enabled, int level, int method, double creaseAngle);
    void setSmoothingParams(bool enabled, int method, int iterations, double strength, double creaseAngle);
    void setTessellationParams(int method, int quality, double featurePreservation, bool parallelProcessing, bool adaptiveMeshing);

    // Validation operations
    void validateMeshParameters();
    void logCurrentMeshSettings();
    void compareMeshQuality(const std::string& geometryName);
    std::string getMeshQualityReport() const;
    void exportMeshStatistics(const std::string& filename);
    bool verifyParameterApplication(const std::string& parameterName, double expectedValue);

    // Real-time parameter monitoring
    void enableParameterMonitoring(bool enabled);
    bool isParameterMonitoringEnabled() const;
    void logParameterChange(const std::string& parameterName, double oldValue, double newValue);

private:
    // Context references (non-owning)
    const std::vector<std::shared_ptr<OCCGeometry>>* m_geometries{ nullptr };
    const MeshParameters* m_meshParams{ nullptr };

    // Subdivision parameters
    bool m_subdivisionEnabled{ false };
    int m_subdivisionLevel{ 2 };
    int m_subdivisionMethod{ 0 };
    double m_subdivisionCreaseAngle{ 30.0 };

    // Smoothing parameters
    bool m_smoothingEnabled{ false };
    int m_smoothingMethod{ 0 };
    int m_smoothingIterations{ 2 };
    double m_smoothingStrength{ 0.5 };
    double m_smoothingCreaseAngle{ 30.0 };

    // Tessellation parameters
    int m_tessellationMethod{ 0 };
    int m_tessellationQuality{ 2 };
    double m_featurePreservation{ 0.5 };
    bool m_parallelProcessing{ true };
    bool m_adaptiveMeshing{ false };

    // Monitoring state
    bool m_parameterMonitoringEnabled{ false };

    // Helper methods
    std::shared_ptr<OCCGeometry> findGeometry(const std::string& name) const;
};

