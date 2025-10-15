#pragma once

#include <Inventor/SbLinear.h>
#include <Inventor/SbViewportRegion.h>
#include <vector>

// OpenCASCADE forward declarations
class TopoDS_Edge;

// Forward declarations
class SceneManager;
class SoSeparator;
class SoClipPlane;
class SoTransform;
class SoMaterial;
class SoScale;
class SoCone;
class SoCylinder;
class OCCGeometry;

/**
 * SliceController encapsulates clipping plane (slice) logic and visuals.
 * It owns the Coin3D nodes required to implement a slice plane and provides
 * a comprehensive API for enabling/disabling and configuring the plane.
 *
 * Features:
 * - Adaptive plane visualization based on scene bounds
 * - Section contour display
 * - Smooth animation for plane movement
 * - Multiple plane support (future extension)
 */
class SliceController {
public:
	SliceController(SceneManager* sceneManager, SoSeparator* root);

	void setEnabled(bool enabled);
	bool isEnabled() const { return m_enabled; }

	void setPlane(const SbVec3f& normal, float offset);
	void moveAlongNormal(float delta);

	// Enhanced interaction methods
	void setShowSectionContours(bool show);
	bool isShowSectionContours() const { return m_showSectionContours; }

	void setPlaneColor(const SbVec3f& color);
	const SbVec3f& getPlaneColor() const { return m_planeColor; }

	void setPlaneOpacity(float opacity);
	float getPlaneOpacity() const { return m_planeOpacity; }

	// Set geometries for section contour computation
	void setGeometries(const std::vector<OCCGeometry*>& geometries);

	// Mouse interaction methods
	bool handleMousePress(const SbVec2s* mousePos, const SbViewportRegion* vp);
	bool handleMouseMove(const SbVec2s* mousePos, const SbViewportRegion* vp);
	bool handleMouseRelease(const SbVec2s* mousePos, const SbViewportRegion* vp);
	bool isInteracting() const { return m_isInteracting; }
	
	// Enable/disable drag interaction
	void setDragEnabled(bool enabled);
	bool isDragEnabled() const { return m_dragEnabled; }

	SbVec3f normal() const { return m_normal; }
	float offset() const { return m_offset; }

	// If the viewer root changes, allow re-attachment
	void attachRoot(SoSeparator* root);

private:
	void ensureNodes();
	void updateNodes();
	void updateVisualizationSize();
	void removeNodes();
	void createAdaptivePlaneVisual();
	void createBorderFrame();
	void updateBorderFrame();
	void updateSectionContours();
	bool extractEdgePoints(const TopoDS_Edge& edge, std::vector<SbVec3f>& points);
	bool isMouseOverPlane(const SbVec2s& mousePos, const SbViewportRegion& vp);
	bool isMouseOverBorder(const SbVec2s& mousePos, const SbViewportRegion& vp);

private:
	SceneManager* m_sceneManager{ nullptr };
	SoSeparator* m_root{ nullptr };

	bool m_enabled{ false };
	SbVec3f m_normal{ 0,0,1 };
	float m_offset{ 0.0f };

	// Enhanced visualization settings
	bool m_showSectionContours{ false };
	SbVec3f m_planeColor{ 0.7f, 0.95f, 0.7f };  // Light green
	float m_planeOpacity{ 0.85f };  // 85% transparency

	// Geometries for section computation
	std::vector<OCCGeometry*> m_geometries;

	// Mouse interaction state
	bool m_isInteracting{ false };
	bool m_dragEnabled{ false };
	SbVec2s m_lastMousePos;
	float m_interactionOffset{ 0.0f };
	SbVec3f m_interactionNormal;

	// Coin3D implementation nodes
	SoClipPlane* m_clipPlane{ nullptr };
	SoSeparator* m_sliceVisual{ nullptr };
	SoTransform* m_sliceTransform{ nullptr };
	SoSeparator* m_sectionContours{ nullptr };
	
	// Border frame for interaction
	SoSeparator* m_borderFrame{ nullptr };
};
