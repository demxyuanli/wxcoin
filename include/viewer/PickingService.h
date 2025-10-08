#pragma once

#include <memory>
#include <unordered_map>
#include <wx/gdicmn.h>

class OCCGeometry;
class SceneManager;
class SoSeparator;

/**
 * @brief Detailed picking result containing geometry and face information
 */
struct PickingResult {
	std::shared_ptr<OCCGeometry> geometry;
	int triangleIndex = -1;  // Index of clicked triangle in mesh
	int geometryFaceId = -1; // Corresponding face ID in original geometry

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
