#pragma once

#include <memory>
#include <unordered_map>
#include <wx/gdicmn.h>

class OCCGeometry;
class SceneManager;
class SoSeparator;
class SoPath;

/**
 * @brief Detailed picking result containing geometry and face information
 */
struct PickingResult {
	std::shared_ptr<OCCGeometry> geometry;
	int triangleIndex = -1;  // Index of clicked triangle in mesh (for faces)
	int geometryFaceId = -1; // Corresponding face ID in original geometry
	int lineIndex = -1;      // Index of clicked line in mesh (for edges)
	int geometryEdgeId = -1; // Corresponding edge ID in original geometry
	int vertexIndex = -1;    // Index of clicked vertex in mesh (for vertices)
	int geometryVertexId = -1; // Corresponding vertex ID in original geometry
	std::string subElementName; // Sub-element name like "Face5", "Edge12", "Vertex3" (FreeCAD style)
	std::string elementType; // Type of element: "Face", "Edge", "Vertex", or empty
	float x = 0.0f, y = 0.0f, z = 0.0f; // 3D coordinates where picking occurred
	SoPath* pickedPath = nullptr; // Path to the picked node (for edge/vertex extraction)

	PickingResult() = default;
	PickingResult(std::shared_ptr<OCCGeometry> geom, int triIdx = -1, int faceId = -1)
		: geometry(geom), triangleIndex(triIdx), geometryFaceId(faceId) {}
};

// Service that performs screen-space picking and resolves to top-level geometries
class PickingService {
public:
	PickingService(SceneManager* sceneManager,
		SoSeparator* occRoot,
		const std::unordered_map<SoSeparator*, std::shared_ptr<OCCGeometry>>* nodeToGeom);

	void setRoot(SoSeparator* occRoot);

	std::shared_ptr<OCCGeometry> pickGeometryAtScreen(const wxPoint& screenPos) const;

	// Extended picking with face index information
	PickingResult pickDetailedAtScreen(const wxPoint& screenPos) const;

private:
	static SoSeparator* findTopLevelSeparatorInPath(class SoPath* path, SoSeparator* occRoot);

private:
	SceneManager* m_sceneManager{ nullptr };
	SoSeparator* m_occRoot{ nullptr };
	const std::unordered_map<SoSeparator*, std::shared_ptr<OCCGeometry>>* m_nodeToGeom{ nullptr };
};
