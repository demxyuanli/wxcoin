#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCamera.h>
#include <wx/frame.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoPointLight.h>
#include <Inventor/nodes/SoSpotLight.h>
#include <memory>
#include "rendering/RenderingToolkitAPI.h"
#include "interfaces/ISceneManager.h"
#include "CameraAnimation.h"

class Canvas;
class CoordinateSystemRenderer;
class PickingAidManager;
class NavigationCube;
class TopoDS_Shape; // Forward declaration for OpenCASCADE

class SceneManager : public ISceneManager {
public:
	SceneManager(Canvas* canvas);
	~SceneManager();
	Canvas* getCanvas() const { return m_canvas; }
	bool initScene() override;
	void initializeScene();  // Add missing method declaration
	void cleanup();
	void resetView(bool animate = false) override;
	void toggleCameraMode();
	void setView(const std::string& viewName);
	void render(const wxSize& size, bool fastMode) override;
	void updateAspectRatio(const wxSize& size) override;
	bool screenToWorld(const wxPoint& screenPos, SbVec3f& worldPos);

	SoSeparator* getObjectRoot() const override { return m_objectRoot; }
	SoSeparator* getSceneRoot() const { return m_sceneRoot; }
	SoCamera* getCamera() const override { return m_camera; }
	PickingAidManager* getPickingAidManager() const { return m_pickingAidManager.get(); }

	// Scene bounds and coordinate system management
	void updateSceneBounds();
	float getSceneBoundingBoxSize() const;
	void updateCoordinateSystemScale();
	void getSceneBoundingBoxMinMax(SbVec3f& min, SbVec3f& max) const;

	// Coordinate system visibility control
	void setCoordinateSystemVisible(bool visible);
	bool isCoordinateSystemVisible() const;

	// Coordinate system color adaptation
	void updateCoordinateSystemColorsForBackground(float backgroundBrightness);

	// Checkerboard plane control
	void setCheckerboardVisible(bool visible);
	bool isCheckerboardVisible() const;

	// Debug method to check lighting state
	void debugLightingState() const;

	// Initialize RenderingConfig callback
	void initializeRenderingConfigCallback();

	// Initialize LightingConfig callback
	void initializeLightingConfigCallback();

	// Update scene lighting from configuration
	void updateSceneLighting();
	void initializeLightingFromConfig();

	// Culling system integration
	void updateCulling();
	bool shouldRenderShape(const TopoDS_Shape& shape) const;
	void addOccluder(const TopoDS_Shape& shape);
	void removeOccluder(const TopoDS_Shape& shape);
	void setFrustumCullingEnabled(bool enabled);
	void setOcclusionCullingEnabled(bool enabled);
	std::string getCullingStats() const;

	// Error recovery
	void rebuildScene();

private:
	Canvas* m_canvas;
	SoSeparator* m_sceneRoot;
	SoCamera* m_camera;
	SoDirectionalLight* m_light;
	SoSeparator* m_lightRoot;  // Add missing member variable
	SoSeparator* m_objectRoot;
	std::unique_ptr<CoordinateSystemRenderer> m_coordSystemRenderer;
	std::unique_ptr<PickingAidManager> m_pickingAidManager;
	bool m_isPerspectiveCamera;
	SbBox3f m_sceneBoundingBox;

	// Checkerboard plane state
	SoSeparator* m_checkerboardSeparator = nullptr;
	bool m_checkerboardVisible = false;

	void createCheckerboardPlane(float planeZ = 0.0f);

	// Camera clipping planes
	void updateCameraClippingPlanes();

	// Culling state
	bool m_cullingEnabled;
	bool m_lastCullingUpdateValid;

	// View animation settings
	bool m_enableViewAnimation;
	float m_viewAnimationDuration;

	CameraAnimation::CameraState captureCameraState() const;
};