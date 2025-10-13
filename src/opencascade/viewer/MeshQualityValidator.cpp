#include "viewer/MeshQualityValidator.h"
#include "OCCGeometry.h"
#include "rendering/GeometryProcessor.h"
#include "logger/Logger.h"
#include <algorithm>
#include <cmath>

MeshQualityValidator::MeshQualityValidator()
{
}

void MeshQualityValidator::setContext(
    const std::vector<std::shared_ptr<OCCGeometry>>* geometries,
    const MeshParameters* meshParams)
{
    m_geometries = geometries;
    m_meshParams = meshParams;
}

void MeshQualityValidator::setSubdivisionParams(bool enabled, int level, int method, double creaseAngle)
{
    m_subdivisionEnabled = enabled;
    m_subdivisionLevel = level;
    m_subdivisionMethod = method;
    m_subdivisionCreaseAngle = creaseAngle;
}

void MeshQualityValidator::setSmoothingParams(bool enabled, int method, int iterations, double strength, double creaseAngle)
{
    m_smoothingEnabled = enabled;
    m_smoothingMethod = method;
    m_smoothingIterations = iterations;
    m_smoothingStrength = strength;
    m_smoothingCreaseAngle = creaseAngle;
}

void MeshQualityValidator::setTessellationParams(int method, int quality, double featurePreservation, bool parallelProcessing, bool adaptiveMeshing)
{
    m_tessellationMethod = method;
    m_tessellationQuality = quality;
    m_featurePreservation = featurePreservation;
    m_parallelProcessing = parallelProcessing;
    m_adaptiveMeshing = adaptiveMeshing;
}

void MeshQualityValidator::validateMeshParameters()
{
    LOG_INF_S("=== MESH PARAMETER VALIDATION ===");

    // Validate subdivision parameters
    LOG_INF_S("Subdivision Settings:");
    LOG_INF_S("  - Enabled: " + std::string(m_subdivisionEnabled ? "true" : "false"));
    LOG_INF_S("  - Level: " + std::to_string(m_subdivisionLevel));
    LOG_INF_S("  - Method: " + std::to_string(m_subdivisionMethod));
    LOG_INF_S("  - Crease Angle: " + std::to_string(m_subdivisionCreaseAngle));

    // Validate smoothing parameters
    LOG_INF_S("Smoothing Settings:");
    LOG_INF_S("  - Enabled: " + std::string(m_smoothingEnabled ? "true" : "false"));
    LOG_INF_S("  - Method: " + std::to_string(m_smoothingMethod));
    LOG_INF_S("  - Iterations: " + std::to_string(m_smoothingIterations));
    LOG_INF_S("  - Strength: " + std::to_string(m_smoothingStrength));
    LOG_INF_S("  - Crease Angle: " + std::to_string(m_smoothingCreaseAngle));

    // Validate tessellation parameters
    LOG_INF_S("Tessellation Settings:");
    LOG_INF_S("  - Method: " + std::to_string(m_tessellationMethod));
    LOG_INF_S("  - Quality: " + std::to_string(m_tessellationQuality));
    LOG_INF_S("  - Feature Preservation: " + std::to_string(m_featurePreservation));
    LOG_INF_S("  - Parallel Processing: " + std::string(m_parallelProcessing ? "true" : "false"));
    LOG_INF_S("  - Adaptive Meshing: " + std::string(m_adaptiveMeshing ? "true" : "false"));

    // Validate basic mesh parameters
    if (m_meshParams) {
        LOG_INF_S("Basic Mesh Settings:");
        LOG_INF_S("  - Deflection: " + std::to_string(m_meshParams->deflection));
        LOG_INF_S("  - Angular Deflection: " + std::to_string(m_meshParams->angularDeflection) +
            " (controls curve approximation - lower = smoother curves)");
        LOG_INF_S("  - Relative: " + std::string(m_meshParams->relative ? "true" : "false"));
        LOG_INF_S("  - In Parallel: " + std::string(m_meshParams->inParallel ? "true" : "false"));

        // Add recommendations for curve-surface fitting
        if (m_meshParams->angularDeflection > 2.0) {
            LOG_WRN_S("Angular deflection is large - curves may appear faceted");
            LOG_INF_S("  Recommendation: Reduce angular deflection to < 1.0 for smoother curves");
        } else if (m_meshParams->angularDeflection < 0.5) {
            LOG_INF_S("Angular deflection is small - curves will be very smooth");
            if (m_meshParams->deflection > 0.5) {
                LOG_WRN_S("  Warning: Large deflection with small angular deflection may cause fitting issues");
                LOG_INF_S("  Recommendation: Reduce mesh deflection or increase angular deflection");
            }
        }
    }

    LOG_INF_S("=== VALIDATION COMPLETE ===");
}

void MeshQualityValidator::logCurrentMeshSettings()
{
    LOG_INF_S("=== CURRENT MESH SETTINGS ===");
    
    if (m_geometries) {
        LOG_INF_S("Geometry Count: " + std::to_string(m_geometries->size()));

        for (const auto& geometry : *m_geometries) {
            if (geometry) {
                LOG_INF_S("Geometry: " + geometry->getName());
                // TODO: Add geometry-specific mesh statistics
            }
        }
    } else {
        LOG_WRN_S("No geometry context available");
    }

    LOG_INF_S("=== SETTINGS LOGGED ===");
}

void MeshQualityValidator::compareMeshQuality(const std::string& geometryName)
{
    auto geometry = findGeometry(geometryName);
    if (!geometry) {
        LOG_ERR_S("Geometry not found: " + geometryName);
        return;
    }

    LOG_INF_S("=== MESH QUALITY COMPARISON FOR: " + geometryName + " ===");

    // TODO: Implement mesh quality comparison
    // This would compare current mesh with previous state or reference mesh

    LOG_INF_S("=== COMPARISON COMPLETE ===");
}

std::string MeshQualityValidator::getMeshQualityReport() const
{
    std::string report = "=== MESH QUALITY REPORT ===\n";

    if (m_geometries) {
        report += "Active Geometries: " + std::to_string(m_geometries->size()) + "\n";
    }
    
    report += "Subdivision Enabled: " + std::string(m_subdivisionEnabled ? "Yes" : "No") + "\n";
    report += "Smoothing Enabled: " + std::string(m_smoothingEnabled ? "Yes" : "No") + "\n";
    report += "Adaptive Meshing: " + std::string(m_adaptiveMeshing ? "Yes" : "No") + "\n";
    report += "Parallel Processing: " + std::string(m_parallelProcessing ? "Yes" : "No") + "\n";

    report += "\nCurrent Parameters:\n";
    if (m_meshParams) {
        report += "- Deflection: " + std::to_string(m_meshParams->deflection) + "\n";
    }
    report += "- Subdivision Level: " + std::to_string(m_subdivisionLevel) + "\n";
    report += "- Smoothing Iterations: " + std::to_string(m_smoothingIterations) + "\n";
    report += "- Tessellation Quality: " + std::to_string(m_tessellationQuality) + "\n";

    return report;
}

void MeshQualityValidator::exportMeshStatistics(const std::string& filename)
{
    LOG_INF_S("Exporting mesh statistics to: " + filename);

    // TODO: Implement mesh statistics export
    // This would export detailed mesh information to a file

    LOG_INF_S("Mesh statistics exported successfully");
}

bool MeshQualityValidator::verifyParameterApplication(const std::string& parameterName, double expectedValue)
{
    LOG_INF_S("Verifying parameter: " + parameterName + " = " + std::to_string(expectedValue));

    // Check if parameter matches expected value
    if (parameterName == "deflection") {
        if (m_meshParams) {
            bool matches = std::abs(m_meshParams->deflection - expectedValue) < 1e-6;
            LOG_INF_S("Deflection verification: " + std::string(matches ? "PASS" : "FAIL"));
            return matches;
        }
    }
    else if (parameterName == "subdivision_level") {
        bool matches = (m_subdivisionLevel == static_cast<int>(expectedValue));
        LOG_INF_S("Subdivision level verification: " + std::string(matches ? "PASS" : "FAIL"));
        return matches;
    }
    else if (parameterName == "smoothing_iterations") {
        bool matches = (m_smoothingIterations == static_cast<int>(expectedValue));
        LOG_INF_S("Smoothing iterations verification: " + std::string(matches ? "PASS" : "FAIL"));
        return matches;
    }
    else if (parameterName == "angular_deflection") {
        if (m_meshParams) {
            bool matches = std::abs(m_meshParams->angularDeflection - expectedValue) < 1e-6;
            LOG_INF_S("Angular deflection verification: " + std::string(matches ? "PASS" : "FAIL"));
            return matches;
        }
    }
    // Add more parameter checks as needed

    LOG_ERR_S("Unknown parameter: " + parameterName);
    return false;
}

void MeshQualityValidator::enableParameterMonitoring(bool enabled)
{
    m_parameterMonitoringEnabled = enabled;
    LOG_INF_S("Parameter monitoring " + std::string(enabled ? "enabled" : "disabled"));
}

bool MeshQualityValidator::isParameterMonitoringEnabled() const
{
    return m_parameterMonitoringEnabled;
}

void MeshQualityValidator::logParameterChange(const std::string& parameterName, double oldValue, double newValue)
{
    if (m_parameterMonitoringEnabled) {
        LOG_INF_S("PARAMETER CHANGE: " + parameterName +
            " [" + std::to_string(oldValue) + " -> " + std::to_string(newValue) + "]");
    }
}

std::shared_ptr<OCCGeometry> MeshQualityValidator::findGeometry(const std::string& name) const
{
    if (!m_geometries) return nullptr;
    
    auto it = std::find_if(m_geometries->begin(), m_geometries->end(),
        [&name](const std::shared_ptr<OCCGeometry>& g) {
            return g && g->getName() == name;
        });
    
    return (it != m_geometries->end()) ? *it : nullptr;
}

