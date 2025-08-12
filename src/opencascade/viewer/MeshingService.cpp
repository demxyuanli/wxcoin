#include "viewer/MeshingService.h"

#include "rendering/RenderingToolkitAPI.h"
#include "OCCGeometry.h"
#include "logger/Logger.h"

void MeshingService::applyAndRemesh(
    const MeshParameters& meshParams,
    const std::vector<std::shared_ptr<OCCGeometry>>& geometries,
    bool smoothingEnabled,
    int smoothingIterations,
    double smoothingStrength,
    double smoothingCreaseAngle,
    bool subdivisionEnabled,
    int subdivisionLevel,
    int subdivisionMethod,
    double subdivisionCreaseAngle,
    int tessellationMethod,
    int tessellationQuality,
    double featurePreservation,
    bool adaptiveMeshing,
    bool parallelProcessing
) {
    // Update RenderingToolkitAPI configuration with current parameters
    auto& config = RenderingToolkitAPI::getConfig();

    // Update smoothing settings
    auto& smoothingSettings = config.getSmoothingSettings();
    smoothingSettings.enabled = smoothingEnabled;
    smoothingSettings.creaseAngle = smoothingCreaseAngle;
    smoothingSettings.iterations = smoothingIterations;

    // Update subdivision settings
    auto& subdivisionSettings = config.getSubdivisionSettings();
    subdivisionSettings.enabled = subdivisionEnabled;
    subdivisionSettings.levels = subdivisionLevel;

    // Update edge settings (reuse crease as feature angle placeholder)
    auto& edgeSettings = config.getEdgeSettings();
    edgeSettings.featureEdgeAngle = subdivisionCreaseAngle;

    // Set custom parameters for advanced settings
    config.setParameter("tessellation_quality", std::to_string(tessellationQuality));
    config.setParameter("adaptive_meshing", adaptiveMeshing ? "true" : "false");
    config.setParameter("parallel_processing", parallelProcessing ? "true" : "false");
    config.setParameter("smoothing_strength", std::to_string(smoothingStrength));
    config.setParameter("tessellation_method", std::to_string(tessellationMethod));
    config.setParameter("feature_preservation", std::to_string(featurePreservation));

    // Regenerate all geometries with updated parameters
    for (auto& geometry : geometries) {
        if (geometry) {
            geometry->updateCoinRepresentationIfNeeded(meshParams);
            LOG_INF_S("Updated mesh (if needed) for geometry: " + geometry->getName());
        }
    }
}


