#pragma once

#include "EdgeTypes.h"
#include "OCCGeometry.h"
#include "rendering/GeometryProcessor.h"
#include "edges/ModularEdgeComponent.h"
#include <functional>

namespace async {
    class AsyncEngineIntegration;
}

// Responsible solely for generating edge nodes on a geometry
class EdgeGenerationService {
public:
	EdgeGenerationService() = default;

	bool ensureOriginalEdges(std::shared_ptr<OCCGeometry>& geom, double samplingDensity = 80.0, double minLength = 0.01, bool showLinesOnly = false, const Quantity_Color& color = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB), double width = 1.0,
		bool highlightIntersectionNodes = false, const Quantity_Color& intersectionNodeColor = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB), double intersectionNodeSize = 3.0, IntersectionNodeShape intersectionNodeShape = IntersectionNodeShape::Point);
	bool ensureFeatureEdges(std::shared_ptr<OCCGeometry>& geom,
		double featureAngleDeg,
		double minLength,
		bool onlyConvex,
		bool onlyConcave,
		const Quantity_Color& color = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB),
		double width = 2.0);

	// Generate mesh-derived edges (mesh edges, vertex normals, face normals) as requested
	// Returns true if any node was generated
	bool ensureMeshDerivedEdges(std::shared_ptr<OCCGeometry>& geom,
		const MeshParameters& meshParams,
		bool needMeshEdges,
		bool needNormalLines,
		bool needFaceNormalLines);

	// Force regenerate mesh-derived edges (clear existing nodes and regenerate)
	// This is used when mesh parameters change
	bool forceRegenerateMeshDerivedEdges(std::shared_ptr<OCCGeometry>& geom,
		const MeshParameters& meshParams,
		bool needMeshEdges,
		bool needNormalLines,
		bool needFaceNormalLines);

	// Async intersection computation
	void computeIntersectionsAsync(
		std::shared_ptr<OCCGeometry>& geom,
		double tolerance,
		class IAsyncEngine* engine,
		std::function<void(const std::vector<gp_Pnt>&, bool, const std::string&)> onComplete,
		std::function<void(int, const std::string&)> onProgress = nullptr);
};
