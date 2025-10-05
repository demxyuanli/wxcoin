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
	LOG_INF_S("=== MESHING SERVICE: UPDATING RENDERING TOOLKIT CONFIG ===");
	LOG_INF_S("Mesh parameters: deflection=" + std::to_string(meshParams.deflection) +
		", angularDeflection=" + std::to_string(meshParams.angularDeflection) +
		", relative=" + std::string(meshParams.relative ? "true" : "false") +
		", inParallel=" + std::string(meshParams.inParallel ? "true" : "false"));
	LOG_INF_S("Processing settings: smoothing=" + std::string(smoothingEnabled ? "true" : "false") +
		", subdivision=" + std::string(subdivisionEnabled ? "true" : "false") +
		", geometries=" + std::to_string(geometries.size()));

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
	LOG_INF_S("=== MESHING SERVICE: STARTING GEOMETRY REGENERATION ===");
	for (auto& geometry : geometries) {
		if (geometry) {
			LOG_INF_S("Processing geometry: " + geometry->getName());
			// Force mesh regeneration by setting the flag and calling updateCoinRepresentationIfNeeded
			geometry->setMeshRegenerationNeeded(true);
			geometry->updateCoinRepresentationIfNeeded(meshParams);
			LOG_INF_S("Updated mesh (if needed) for geometry: " + geometry->getName());
		}
	}
	LOG_INF_S("=== MESHING SERVICE: GEOMETRY REGENERATION COMPLETED ===");
}