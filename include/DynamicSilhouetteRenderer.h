#pragma once

#include <Inventor/nodes/SoCallback.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <opencascade/TopoDS_Shape.hxx>
#include <opencascade/TopoDS_Face.hxx>
#include <opencascade/TopoDS_Edge.hxx>
#include <opencascade/gp_Pnt.hxx>
#include <opencascade/gp_Vec.hxx>
#include <vector>
#include <chrono>

class DynamicSilhouetteRenderer {
public:
	DynamicSilhouetteRenderer(SoSeparator* sceneRoot = nullptr);
	~DynamicSilhouetteRenderer();

	// Set the shape to render silhouettes for
	void setShape(const TopoDS_Shape& shape);

	// Get the Coin3D node that will render dynamic silhouettes
	SoSeparator* getSilhouetteNode();

	// Update silhouettes based on current camera position
	void updateSilhouettes(const gp_Pnt& cameraPos, const SbMatrix* modelMatrix = nullptr);

	// Enable/disable silhouette rendering
	void setEnabled(bool enabled);
	bool isEnabled() const;

	// Appearance controls
	void setLineWidth(float width) { if (m_drawStyle) m_drawStyle->lineWidth = width; }
	void setLineColor(float r, float g, float b) {
		if (m_material) {
			m_material->diffuseColor.setValue(r, g, b);
			m_material->ambientColor.setValue(r, g, b);
			m_material->emissiveColor.setValue(r, g, b);
			m_material->specularColor.setValue(r, g, b);
		}
	}

	// Enable simplified fast mode (boundary edges only, camera-independent)
	void setFastMode(bool enabled) { m_fastMode = enabled; }
	bool isFastMode() const { return m_fastMode; }

private:
	// Dynamic silhouette calculation
	void calculateSilhouettes(const gp_Pnt& cameraPos, const SbMatrix* modelMatrix = nullptr);
	void buildBoundaryOnlyCache();

	// Helper function to get face normal at a point
	static gp_Vec getNormalAt(const TopoDS_Face& face, const gp_Pnt& p);

	// Coin3D rendering callback
	static void renderCallback(void* userData, SoAction* action);

private:
	TopoDS_Shape m_shape;
	SoSeparator* m_silhouetteNode;
	SoSeparator* m_sceneRoot;  // 主场景根节点
	SoMaterial* m_material;
	SoDrawStyle* m_drawStyle;
	SoCoordinate3* m_coordinates;
	SoIndexedLineSet* m_lineSet;
	SoCallback* m_renderCallback;

	std::vector<gp_Pnt> m_silhouettePoints;
	std::vector<int32_t> m_silhouetteIndices;

	// Cached boundary-only polyline for fast mode
	std::vector<gp_Pnt> m_cachedBoundaryPoints;
	std::vector<int32_t> m_cachedBoundaryIndices;

	bool m_enabled;
	bool m_needsUpdate;
	bool m_fastMode{ true }; // default to fast mode for performance

	// Throttling
	gp_Pnt m_lastCameraPos{ 0,0,0 };
	double m_minCameraMove{ 1.0 }; // world units
	int m_minUpdateIntervalMs{ 200 }; // ms
	std::chrono::steady_clock::time_point m_lastUpdateTs{ std::chrono::steady_clock::now() - std::chrono::milliseconds(1000) };
};