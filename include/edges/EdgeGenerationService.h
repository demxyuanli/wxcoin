#pragma once

#include "EdgeTypes.h"
#include "OCCGeometry.h"
#include "rendering/GeometryProcessor.h"

// Responsible solely for generating edge nodes on a geometry
class EdgeGenerationService {
public:
	EdgeGenerationService() = default;

	bool ensureOriginalEdges(std::shared_ptr<OCCGeometry>& geom, double samplingDensity = 80.0, double minLength = 0.01, bool showLinesOnly = false, const Quantity_Color& color = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB), double width = 1.0);
	bool ensureFeatureEdges(std::shared_ptr<OCCGeometry>& geom,
		double featureAngleDeg,
		double minLength,
		bool onlyConvex,
		bool onlyConcave);

	// Generate mesh-derived edges (mesh edges, vertex normals, face normals) as requested
	// Returns true if any node was generated
	bool ensureMeshDerivedEdges(std::shared_ptr<OCCGeometry>& geom,
		const MeshParameters& meshParams,
		bool needMeshEdges,
		bool needNormalLines,
		bool needFaceNormalLines);
};
