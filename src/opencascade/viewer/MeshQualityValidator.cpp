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

    // Validate subdivision parameters

    // Validate smoothing parameters

    // Validate tessellation parameters

    // Validate basic mesh parameters
    if (m_meshParams) {

        // Add recommendations for curve-surface fitting
        if (m_meshParams->angularDeflection > 2.0) {
            LOG_WRN_S("Angular deflection is large - curves may appear faceted");
        } else if (m_meshParams->angularDeflection < 0.5) {
            if (m_meshParams->deflection > 0.5) {
                LOG_WRN_S("  Warning: Large deflection with small angular deflection may cause fitting issues");
            }
        }
    }

}

void MeshQualityValidator::logCurrentMeshSettings()
{
    
    if (m_geometries) {

        for (const auto& geometry : *m_geometries) {
            if (geometry) {
                // TODO: Add geometry-specific mesh statistics
            }
        }
    } else {
        LOG_WRN_S("No geometry context available");
    }

}

void MeshQualityValidator::compareMeshQuality(const std::string& geometryName)
{
    auto geometry = findGeometry(geometryName);
    if (!geometry) {
        LOG_ERR_S("Geometry not found: " + geometryName);
        return;
    }


    // TODO: Implement mesh quality comparison
    // This would compare current mesh with previous state or reference mesh

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

    // TODO: Implement mesh statistics export
    // This would export detailed mesh information to a file

}

bool MeshQualityValidator::verifyParameterApplication(const std::string& parameterName, double expectedValue)
{

    // Check if parameter matches expected value
    if (parameterName == "deflection") {
        if (m_meshParams) {
            bool matches = std::abs(m_meshParams->deflection - expectedValue) < 1e-6;
            return matches;
        }
    }
    else if (parameterName == "subdivision_level") {
        bool matches = (m_subdivisionLevel == static_cast<int>(expectedValue));
        return matches;
    }
    else if (parameterName == "smoothing_iterations") {
        bool matches = (m_smoothingIterations == static_cast<int>(expectedValue));
        return matches;
    }
    else if (parameterName == "angular_deflection") {
        if (m_meshParams) {
            bool matches = std::abs(m_meshParams->angularDeflection - expectedValue) < 1e-6;
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
}

bool MeshQualityValidator::isParameterMonitoringEnabled() const
{
    return m_parameterMonitoringEnabled;
}

void MeshQualityValidator::logParameterChange(const std::string& parameterName, double oldValue, double newValue)
{
    if (m_parameterMonitoringEnabled) {
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

