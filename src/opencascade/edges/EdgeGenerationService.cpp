#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "edges/EdgeGenerationService.h"
#include "EdgeComponent.h"
#include "rendering/RenderingToolkitAPI.h"

bool EdgeGenerationService::ensureOriginalEdges(std::shared_ptr<OCCGeometry>& geom) {
	if (!geom) return false;
	if (!geom->edgeComponent) geom->edgeComponent = std::make_unique<EdgeComponent>();
	if (geom->edgeComponent->getEdgeNode(EdgeType::Original) != nullptr) return false;
	geom->edgeComponent->extractOriginalEdges(geom->getShape());
	return true;
}

bool EdgeGenerationService::ensureFeatureEdges(std::shared_ptr<OCCGeometry>& geom,
	double featureAngleDeg,
	double minLength,
	bool onlyConvex,
	bool onlyConcave) {
	if (!geom) return false;
	if (!geom->edgeComponent) geom->edgeComponent = std::make_unique<EdgeComponent>();
	if (geom->edgeComponent->getEdgeNode(EdgeType::Feature) != nullptr) return false;
	geom->edgeComponent->extractFeatureEdges(geom->getShape(), featureAngleDeg, minLength, onlyConvex, onlyConcave);
	return true;
}

bool EdgeGenerationService::ensureMeshDerivedEdges(std::shared_ptr<OCCGeometry>& geom,
	const MeshParameters& meshParams,
	bool needMeshEdges,
	bool needNormalLines,
	bool needFaceNormalLines) {
	if (!geom) return false;
	if (!geom->edgeComponent) geom->edgeComponent = std::make_unique<EdgeComponent>();
	bool generated = false;
	bool needMesh = false;
	if (needMeshEdges && geom->edgeComponent->getEdgeNode(EdgeType::Mesh) == nullptr) needMesh = true;
	if (needNormalLines && geom->edgeComponent->getEdgeNode(EdgeType::NormalLine) == nullptr) needMesh = true;
	if (needFaceNormalLines && geom->edgeComponent->getEdgeNode(EdgeType::FaceNormalLine) == nullptr) needMesh = true;
	if (!needMesh) return false;

	auto& manager = RenderingToolkitAPI::getManager();
	auto processor = manager.getGeometryProcessor("OpenCASCADE");
	if (!processor) return false;
	TriangleMesh mesh = processor->convertToMesh(geom->getShape(), meshParams);
	if (needMeshEdges && geom->edgeComponent->getEdgeNode(EdgeType::Mesh) == nullptr) {
		geom->edgeComponent->extractMeshEdges(mesh);
		generated = true;
	}
	if (needNormalLines && geom->edgeComponent->getEdgeNode(EdgeType::NormalLine) == nullptr) {
		geom->edgeComponent->generateNormalLineNode(mesh, 0.5);
		generated = true;
	}
	if (needFaceNormalLines && geom->edgeComponent->getEdgeNode(EdgeType::FaceNormalLine) == nullptr) {
		geom->edgeComponent->generateFaceNormalLineNode(mesh, 0.5);
		generated = true;
	}
	return generated;
}