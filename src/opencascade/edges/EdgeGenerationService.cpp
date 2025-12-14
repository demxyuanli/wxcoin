#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "edges/EdgeGenerationService.h"
#include "edges/ModularEdgeComponent.h"
#include "edges/extractors/OriginalEdgeExtractor.h"
#include "rendering/RenderingToolkitAPI.h"
#include "logger/Logger.h"

bool EdgeGenerationService::ensureOriginalEdges(std::shared_ptr<OCCGeometry>& geom, double samplingDensity, double minLength, bool showLinesOnly, const Quantity_Color& color, double width,
	bool highlightIntersectionNodes, const Quantity_Color& intersectionNodeColor, double intersectionNodeSize, IntersectionNodeShape intersectionNodeShape) {
	if (!geom) return false;

	// Use the component specified by the geometry
	// Migration completed - always use modular edge component
	if (!geom->modularEdgeComponent) {
		geom->modularEdgeComponent = std::make_unique<ModularEdgeComponent>();
	}

	auto& comp = geom->modularEdgeComponent;

	// Check if original edge node already exists
	if (comp->getEdgeNode(EdgeType::Original) != nullptr) {
		// Node exists - just apply new appearance (color, width) without regenerating geometry
		comp->applyAppearanceToEdgeNode(EdgeType::Original, color, width);

		// Handle intersection nodes
		if (highlightIntersectionNodes) {
			// If intersection nodes don't exist, we need to generate them
			if (comp->getEdgeNode(EdgeType::IntersectionNodes) == nullptr) {
				// Generate intersection nodes without regenerating the entire edge geometry
				std::vector<gp_Pnt> intersectionPoints;
				auto extractor = std::dynamic_pointer_cast<OriginalEdgeExtractor>(comp->getOriginalExtractor());
				if (extractor) {
					extractor->findEdgeIntersections(geom->getShape(), intersectionPoints, 0.0); // Use adaptive tolerance

					if (!intersectionPoints.empty()) {
						comp->createIntersectionNodesNode(intersectionPoints, intersectionNodeColor, intersectionNodeSize, intersectionNodeShape);
					}
				}
			} else {
				// Intersection nodes exist - just update appearance
				comp->applyAppearanceToEdgeNode(EdgeType::IntersectionNodes, intersectionNodeColor, intersectionNodeSize);
			}
		} else {
			// If intersection highlighting is disabled, clean up intersection nodes
			comp->clearEdgeNode(EdgeType::IntersectionNodes);
		}

		return false; // No new geometry generated
	}

	// Node doesn't exist - generate new geometry with specified parameters
	comp->extractOriginalEdges(geom->getShape(), samplingDensity, minLength, showLinesOnly, color, width,
		highlightIntersectionNodes, intersectionNodeColor, intersectionNodeSize, intersectionNodeShape);
	return true;
}

bool EdgeGenerationService::ensureFeatureEdges(std::shared_ptr<OCCGeometry>& geom,
	double featureAngleDeg,
	double minLength,
	bool onlyConvex,
	bool onlyConcave,
	const Quantity_Color& color,
	double width) {
	if (!geom) return false;

	// Migration completed - always use modular edge component
	if (!geom->modularEdgeComponent) {
		geom->modularEdgeComponent = std::make_unique<ModularEdgeComponent>();
	}
	if (geom->modularEdgeComponent->getEdgeNode(EdgeType::Feature) != nullptr) return false;
	geom->modularEdgeComponent->extractFeatureEdges(geom->getShape(), featureAngleDeg, minLength, onlyConvex, onlyConcave, color, width);
	return true;
}

bool EdgeGenerationService::ensureMeshDerivedEdges(std::shared_ptr<OCCGeometry>& geom,
	const MeshParameters& meshParams,
	bool needMeshEdges,
	bool needVerticeNormals,
	bool needFaceNormals) {
	if (!geom) return false;

	bool generated = false;
	bool needMesh = false;

	// Migration completed - always use modular edge component
	if (!geom->modularEdgeComponent) {
		geom->modularEdgeComponent = std::make_unique<ModularEdgeComponent>();
	}
	if (needMeshEdges && geom->modularEdgeComponent->getEdgeNode(EdgeType::Mesh) == nullptr) needMesh = true;
	if (needVerticeNormals && geom->modularEdgeComponent->getEdgeNode(EdgeType::VerticeNormal) == nullptr) needMesh = true;
	if (needFaceNormals && geom->modularEdgeComponent->getEdgeNode(EdgeType::FaceNormal) == nullptr) needMesh = true;

	if (!needMesh) return false;

	LOG_INF_S("EdgeGenerationService::ensureMeshDerivedEdges called for '" + geom->getName() + 
	         "', needMeshEdges=" + std::string(needMeshEdges ? "true" : "false") +
	         ", needVerticeNormals=" + std::string(needVerticeNormals ? "true" : "false") +
	         ", needFaceNormals=" + std::string(needFaceNormals ? "true" : "false"));
	
	// CRITICAL FIX: For mesh-only geometries (STL, OBJ), use cached mesh instead of converting from shape
	// This enables mesh edges, vertex normals, and face normals for mesh-only geometries
	TriangleMesh mesh;
	if (geom->hasCachedMesh()) {
		// Use cached mesh for mesh-only geometries
		mesh = geom->getCachedMesh();
		LOG_INF_S("EdgeGenerationService: Using cached mesh for '" + geom->getName() + "' (" + 
		         std::to_string(mesh.vertices.size()) + " vertices, " + 
		         std::to_string(mesh.triangles.size() / 3) + " triangles)");
	} else {
		// Convert from BRep shape for standard geometries
		LOG_INF_S("EdgeGenerationService: Converting from BRep shape for '" + geom->getName() + "'");
		auto& manager = RenderingToolkitAPI::getManager();
		auto processor = manager.getGeometryProcessor("OpenCASCADE");
		if (!processor) {
			LOG_ERR_S("EdgeGenerationService: Failed to get OpenCASCADE processor");
			return false;
		}
		mesh = processor->convertToMesh(geom->getShape(), meshParams);
		LOG_INF_S("EdgeGenerationService: Converted mesh has " + 
		         std::to_string(mesh.vertices.size()) + " vertices, " + 
		         std::to_string(mesh.triangles.size() / 3) + " triangles");
	}
	
	// Check if mesh is valid
	if (mesh.vertices.empty() || mesh.triangles.empty()) {
		LOG_WRN_S("EdgeGenerationService: Empty mesh for '" + geom->getName() + "', cannot generate mesh-derived edges");
		return false;
	}

	auto& comp = geom->modularEdgeComponent;
	if (needMeshEdges && comp->getEdgeNode(EdgeType::Mesh) == nullptr) {
		Quantity_Color meshColor(0.0, 0.0, 0.0, Quantity_TOC_RGB);
		comp->extractMeshEdges(mesh, meshColor, 1.0);
		generated = true;
	}
	if (needVerticeNormals && comp->getEdgeNode(EdgeType::VerticeNormal) == nullptr) {
		comp->generateNormalLineNode(mesh, 0.5);
		generated = true;
	}
	if (needFaceNormals && comp->getEdgeNode(EdgeType::FaceNormal) == nullptr) {
		comp->generateFaceNormalLineNode(mesh, 0.5);
		generated = true;
	}

	return generated;
}

bool EdgeGenerationService::forceRegenerateMeshDerivedEdges(std::shared_ptr<OCCGeometry>& geom,
	const MeshParameters& meshParams,
	bool needMeshEdges,
	bool needVerticeNormals,
	bool needFaceNormals) {
	if (!geom) return false;

	bool generated = false;

	// CRITICAL FIX: For mesh-only geometries (STL, OBJ), use cached mesh instead of converting from shape
	TriangleMesh mesh;
	if (geom->hasCachedMesh()) {
		// Use cached mesh for mesh-only geometries
		mesh = geom->getCachedMesh();
		LOG_INF_S("EdgeGenerationService: Using cached mesh for force regeneration '" + geom->getName() + "'");
	} else {
		// Convert from BRep shape for standard geometries
		auto& manager = RenderingToolkitAPI::getManager();
		auto processor = manager.getGeometryProcessor("OpenCASCADE");
		if (!processor) return false;
		mesh = processor->convertToMesh(geom->getShape(), meshParams);
	}
	
	// Check if mesh is valid
	if (mesh.vertices.empty() || mesh.triangles.empty()) {
		LOG_WRN_S("EdgeGenerationService: Empty mesh for force regeneration '" + geom->getName() + "'");
		return false;
	}

	// Migration completed - always use modular edge component
	if (!geom->modularEdgeComponent) {
		geom->modularEdgeComponent = std::make_unique<ModularEdgeComponent>();
	}

	auto& comp = geom->modularEdgeComponent;

	// Force regenerate mesh edges if requested
	if (needMeshEdges) {
		comp->clearEdgeNode(EdgeType::Mesh);
		Quantity_Color meshColor(0.0, 0.0, 0.0, Quantity_TOC_RGB);
		comp->extractMeshEdges(mesh, meshColor, 1.0);
		generated = true;
	}

	// Force regenerate normal lines if requested
	if (needVerticeNormals) {
		comp->clearEdgeNode(EdgeType::VerticeNormal);
		comp->generateNormalLineNode(mesh, 0.5);
		generated = true;
	}

	// Force regenerate face normal lines if requested
	if (needFaceNormals) {
		comp->clearEdgeNode(EdgeType::FaceNormal);
		comp->generateFaceNormalLineNode(mesh, 0.5);
		generated = true;
	}

	return generated;
}

void EdgeGenerationService::computeIntersectionsAsync(
	std::shared_ptr<OCCGeometry>& geom,
	double tolerance,
	class IAsyncEngine* engine,
	std::function<void(const std::vector<gp_Pnt>&, bool, const std::string&)> onComplete,
	std::function<void(int, const std::string&)> onProgress)
{
	if (!geom || !geom->modularEdgeComponent) {
		LOG_ERR_S("EdgeGenerationService: Invalid geometry or component");
		if (onComplete) {
			onComplete({}, false, "Invalid geometry");
		}
		return;
	}

	geom->modularEdgeComponent->computeIntersectionsAsync(
		geom->getShape(),
		tolerance,
		engine,
		onComplete,
		onProgress
	);
}
