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

	// Check if advanced parameters require forced rebuild
	bool needsForcedRebuild = false;
	if (!geometries.empty()) {
		auto& config = RenderingToolkitAPI::getConfig();
		auto& smoothingSettings = config.getSmoothingSettings();
		auto& subdivisionSettings = config.getSubdivisionSettings();

		// Check if this is a significant change that requires forced rebuild
		needsForcedRebuild = (
			smoothingSettings.enabled != geometries[0]->isSmoothingEnabled() ||
			subdivisionSettings.enabled != geometries[0]->isSubdivisionEnabled() ||
			smoothingSettings.iterations != geometries[0]->getSmoothingIterations() ||
			subdivisionSettings.levels != geometries[0]->getSubdivisionLevel()
		);
	}

	// Regenerate all geometries with updated parameters
	for (auto& geometry : geometries) {
		if (geometry) {
			if (needsForcedRebuild) {
				geometry->forceCoinRepresentationRebuild(meshParams);
				LOG_INF_S("Forced rebuild for geometry: " + geometry->getName());
			} else {
				geometry->updateCoinRepresentationIfNeeded(meshParams);
				LOG_INF_S("Updated mesh (if needed) for geometry: " + geometry->getName());
			}
		}
	}
}