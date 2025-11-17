#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCamera.h>
#include <wx/frame.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoPointLight.h>
#include <Inventor/nodes/SoSpotLight.h>
#include <memory>
#include <vector>
#include <functional>
#include "rendering/RenderingToolkitAPI.h"
#include "interfaces/ISceneManager.h"
#include "CameraAnimation.h"

class Canvas;
class CoordinateSystemRenderer;
class PickingAidManager;
class NavigationCube;
class TopoDS_Shape; // Forward declaration for OpenCASCADE

// Forward declaration for PassCallbackState
class SceneManager;

// Structure to track pass state for callback
struct PassCallbackState {
	SceneManager* sceneManager;
	int passCount;

	PassCallbackState(SceneManager* sm) : sceneManager(sm), passCount(0) {}
};

// Deferred update system structures
enum class UpdateType {
	LIGHTING_UPDATE,
	GEOMETRY_UPDATE,
	VISIBILITY_UPDATE,
	COORDINATE_SYSTEM_UPDATE,
	CHECKERBOARD_UPDATE,
	FULL_REBUILD
};

struct DeferredUpdate {
	UpdateType type;
	std::function<void()> action;
	int priority; // Higher priority = execute first
	std::string description;
};

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

	// Camera state management utilities
	void applyCameraState(const SbVec3f& position, const SbRotation& orientation, float focalDistance, float height = 0.0f);
	void restoreCameraState(const CameraAnimation::CameraState& state);
	void setupCameraForViewAll();
	void performViewAll();
	void positionCameraForDirection(const SbVec3f& direction, const SbBox3f& sceneBounds);

private:
	// Render method helper - geometry validation
	void validateAndRepairGeometries();

	// Lighting setup helper - unified lighting configuration
	void setupLightingFromConfig(bool isUpdate, bool isNoShading = false);

	// Geometry validation optimization
	size_t m_lastGeometryCount = 0;        // Cache of last geometry count
	int m_geometryValidationFrameSkip = 0; // Frame counter for validation throttling
	static constexpr int GEOMETRY_VALIDATION_INTERVAL = 30; // Validate every N frames
	bool m_forceGeometryValidation = false; // Force validation flag

	// Mark geometry as dirty, requiring validation
	void markGeometryDirty();

	// Public method to invalidate geometry cache (called when geometry changes)
	void invalidateGeometryCache();

	// Scene bounds optimization
	int m_boundsUpdateFrameSkip = 0;       // Frame counter for bounds update throttling
	static constexpr int BOUNDS_UPDATE_INTERVAL = 60; // Update bounds every N frames
	bool m_forceBoundsUpdate = false;       // Force bounds update flag

	// Mark scene bounds as dirty, requiring update
	void markBoundsDirty();

	// Public method to force bounds update (called when geometry changes)
	void forceBoundsUpdate();

	// Error handling utilities
	enum class ErrorSeverity { LOW, MEDIUM, HIGH, CRITICAL };
	enum class ErrorCategory { RENDERING, GEOMETRY, LIGHTING, GENERAL };

	// Unified error handling
	void handleError(ErrorCategory category, ErrorSeverity severity, const std::string& message,
		const std::exception* e = nullptr, std::function<void()> recoveryAction = nullptr);

	// Multi-pass rendering optimization
	int determineOptimalPassCount();
	bool hasTransparentObjects() const;

	// Deferred update system
	void deferUpdate(UpdateType type, std::function<void()> action, int priority = 0, const std::string& description = "");
	void processDeferredUpdates();
	bool hasDeferredUpdates() const;
	void clearDeferredUpdates();

private:
	// Deferred update queue
	std::vector<DeferredUpdate> m_deferredUpdates;
};