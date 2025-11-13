#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "SceneManager.h"
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include "Canvas.h"
#include "config/RenderingConfig.h"
#include "config/LightingConfig.h"
#include "CoordinateSystemRenderer.h"
#include "PickingAidManager.h"
#include "ViewRefreshManager.h"
#include "OCCViewer.h"
#include "utils/PerformanceBus.h"
#include "logger/Logger.h"
#include <map>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoEnvironment.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SbLinear.h>
#include <GL/gl.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include "rendering/RenderingToolkitAPI.h"
#include <chrono>
#include "CameraAnimation.h"
#include "config/ConfigManager.h"

SceneManager::SceneManager(Canvas* canvas)
	: m_canvas(canvas)
	, m_sceneRoot(nullptr)
	, m_camera(nullptr)
	, m_light(nullptr)
	, m_lightRoot(nullptr)  // Add initialization for m_lightRoot
	, m_objectRoot(nullptr)
	, m_isPerspectiveCamera(true)
	, m_cullingEnabled(true)
	, m_lastCullingUpdateValid(false)
	, m_enableViewAnimation(true)
	, m_viewAnimationDuration(0.2f)
{
	LOG_INF_S("SceneManager initializing");

	// Initialize rendering toolkit with culling
	// m_renderingToolkit = std::make_unique<RenderingToolkitAPI>(); // Removed as per edit hint
	// m_renderingToolkit->setFrustumCullingEnabled(true); // Removed as per edit hint
	// m_renderingToolkit->setOcclusionCullingEnabled(true); // Removed as per edit hint
}

SceneManager::~SceneManager() {
	// Critical: Remove listeners BEFORE cleanup to prevent dangling pointer access
	if (m_canvas && m_canvas->getRefreshManager()) {
		m_canvas->getRefreshManager()->removeAllListeners();
		LOG_INF_S("Removed all refresh listeners in destructor");
	}
	cleanup();
	LOG_INF_S("SceneManager destroyed");
}

bool SceneManager::initScene() {
	try {
		// Create light root separator first
		m_lightRoot = new SoSeparator;
		m_lightRoot->ref();

		m_sceneRoot = new SoSeparator;
		m_sceneRoot->addChild(m_lightRoot);  // Now m_lightRoot is not NULL
		m_sceneRoot->ref();

		m_camera = new SoPerspectiveCamera;
		m_camera->ref();
		m_camera->position.setValue(5.0f, -5.0f, 5.0f);
		m_camera->nearDistance.setValue(0.001f);
		m_camera->farDistance.setValue(10000.0f);
		m_camera->focalDistance.setValue(8.66f);

		SbVec3f s_viewDir(-5.0f, 5.0f, -5.0f);
		s_viewDir.normalize();
		SbVec3f s_defaultDir(0, 0, -1);
		SbRotation s_rotation(s_defaultDir, s_viewDir);
		m_camera->orientation.setValue(s_rotation);
		m_sceneRoot->addChild(m_camera);

		ConfigManager& configManager = ConfigManager::getInstance();
		m_enableViewAnimation = configManager.getBool("NavigationCube", "EnableAnimation", true);
		m_viewAnimationDuration = static_cast<float>(configManager.getDouble("NavigationCube", "MenuAnimationDuration", 0.3));
		if (m_viewAnimationDuration <= 0.0f) {
			m_viewAnimationDuration = 0.3f;
		}

		auto& navigationAnimator = NavigationAnimator::getInstance();
		navigationAnimator.setCamera(m_camera);
		navigationAnimator.setAnimationType(CameraAnimation::SMOOTH);
		navigationAnimator.setViewRefreshCallback([this]() {
			if (!m_canvas) {
				return;
			}
			if (m_canvas->getRefreshManager()) {
				m_canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::CAMERA_MOVED, true);
			} else {
				m_canvas->Refresh(true);
			}
		});

		// Set a light model to enable proper lighting calculation
		// Place on scene root (not inside lightRoot) so the lighting model applies globally to subsequent nodes
		SoLightModel* lightModel = new SoLightModel;
		lightModel->model.setValue(SoLightModel::PHONG);
		if (m_sceneRoot) {
			// Insert at the beginning to ensure it precedes lights and objects
			m_sceneRoot->insertChild(lightModel, 0);

			// Ensure at least one fallback light exists even if configuration is empty
			SoDirectionalLight* fallbackLight = new SoDirectionalLight;
			fallbackLight->direction.setValue(0.5f, 0.5f, -0.7f);
			fallbackLight->intensity.setValue(1.0f);
			int insertIndex = m_sceneRoot->getNumChildren();
			if (m_objectRoot) {
				int objectIndex = m_sceneRoot->findChild(m_objectRoot);
				if (objectIndex >= 0) insertIndex = objectIndex;
			}
			m_sceneRoot->insertChild(fallbackLight, insertIndex);
		}

		// Initialize lighting from configuration instead of hardcoded values
		initializeLightingFromConfig();

		m_objectRoot = new SoSeparator;
		m_objectRoot->ref();
		m_sceneRoot->addChild(m_objectRoot);

		m_coordSystemRenderer = std::make_unique<CoordinateSystemRenderer>(m_objectRoot);
		m_pickingAidManager = std::make_unique<PickingAidManager>(this, m_canvas, m_canvas->getInputManager());

		// Initialize RenderingConfig callback
		initializeRenderingConfigCallback();

		// Initialize LightingConfig callback
		initializeLightingConfigCallback();

		// Initialize culling system
		RenderingToolkitAPI::setFrustumCullingEnabled(true);
		RenderingToolkitAPI::setOcclusionCullingEnabled(true);
		LOG_INF_S("Culling system initialized and enabled");

		// Add refresh listener for camera clipping plane updates
		// Note: Listener is removed in destructor to prevent dangling pointer access
		if (m_canvas && m_canvas->getRefreshManager()) {
			m_canvas->getRefreshManager()->addRefreshListener(
				[this](ViewRefreshManager::RefreshReason reason) {
					// Extra safety: check if camera is still valid
					if (!m_camera || !m_sceneRoot) {
						return; // Object may be in the process of destruction
					}
					if (reason == ViewRefreshManager::RefreshReason::CAMERA_MOVED) {
						updateCameraClippingPlanes();
					}
				});
			LOG_INF_S("Added camera clipping plane update listener");
		}

		resetView();
		return true;
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Exception during scene initialization: " + std::string(e.what()));
		cleanup();
		return false;
	}
}

void SceneManager::cleanup() {
	// Remove all refresh listeners to prevent dangling pointer issues
	if (m_canvas && m_canvas->getRefreshManager()) {
		m_canvas->getRefreshManager()->removeAllListeners();
		LOG_INF_S("Removed all refresh listeners during cleanup");
	}

	if (m_sceneRoot) {
		m_sceneRoot->unref();
		m_sceneRoot = nullptr;
	}
	if (m_camera) {
		m_camera->unref();
		m_camera = nullptr;
	}
	if (m_light) {
		m_light->unref();
		m_light = nullptr;
	}
	if (m_lightRoot) {
		m_lightRoot->unref();
		m_lightRoot = nullptr;
	}
	if (m_objectRoot) {
		m_objectRoot->unref();
		m_objectRoot = nullptr;
	}
}

void SceneManager::resetView(bool animate) {
	if (!m_camera || !m_sceneRoot) {
		LOG_ERR_S("Failed to reset view: Invalid camera or scene");
		return;
	}

	bool shouldAnimate = animate && m_enableViewAnimation;
	CameraAnimation::CameraState originalState;
	if (shouldAnimate) {
		originalState = captureCameraState();
	}

	// Apply default orientation and fit scene
	m_camera->position.setValue(5.0f, -5.0f, 5.0f);
	SbVec3f r_position = m_camera->position.getValue();
	SbVec3f r_viewDir(-r_position[0], -r_position[1], -r_position[2]);
	r_viewDir.normalize();
	SbVec3f r_defaultDir(0, 0, -1);
	SbRotation r_rotation(r_defaultDir, r_viewDir);
	m_camera->orientation.setValue(r_rotation);
	m_camera->focalDistance.setValue(8.66f);
	if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
		SoOrthographicCamera* orthoCam = static_cast<SoOrthographicCamera*>(m_camera);
		orthoCam->height.setValue(8.66f);
	}

	SbViewportRegion viewport(m_canvas->GetClientSize().x, m_canvas->GetClientSize().y);
	m_camera->viewAll(m_sceneRoot, viewport, 1.1f);

	CameraAnimation::CameraState targetState;
	if (shouldAnimate) {
		targetState = captureCameraState();
	}

	// Update scene bounds and clipping planes dynamically
	updateSceneBounds(); // This will call updateCameraClippingPlanes() automatically

	if (shouldAnimate) {
		// Restore original camera state before starting animation
		m_camera->position.setValue(originalState.position);
		m_camera->orientation.setValue(originalState.rotation);

		if (m_camera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
			auto* perspCam = static_cast<SoPerspectiveCamera*>(m_camera);
			perspCam->focalDistance.setValue(originalState.focalDistance);
		} else if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
			auto* orthoCam = static_cast<SoOrthographicCamera*>(m_camera);
			orthoCam->focalDistance.setValue(originalState.focalDistance);
			orthoCam->height.setValue(originalState.height);
		}

		auto& animator = NavigationAnimator::getInstance();
		animator.stopCurrentAnimation();
		animator.setCamera(m_camera);
		animator.setAnimationType(CameraAnimation::SMOOTH);
		animator.animateToPosition(
			targetState.position,
			targetState.rotation,
			m_viewAnimationDuration,
			targetState.focalDistance,
			targetState.height);
	} else {
		if (m_canvas->getRefreshManager()) {
			m_canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::CAMERA_MOVED, true);
		}
		else {
			m_canvas->Refresh(true);
		}
	}
}

void SceneManager::toggleCameraMode() {
	if (!m_sceneRoot || !m_camera) {
		LOG_ERR_S("Failed to toggle camera mode: Invalid context or scene");
		return;
	}

	SbVec3f oldPosition = m_camera->position.getValue();
	SbRotation oldOrientation = m_camera->orientation.getValue();
	float oldFocalDistance = m_camera->focalDistance.getValue();

	m_sceneRoot->removeChild(m_camera);
	m_camera->unref();

	m_isPerspectiveCamera = !m_isPerspectiveCamera;
	if (m_isPerspectiveCamera) {
		m_camera = new SoPerspectiveCamera;
	}
	else {
		m_camera = new SoOrthographicCamera;
	}
	m_camera->ref();

	m_camera->position.setValue(oldPosition);
	m_camera->orientation.setValue(oldOrientation);
	m_camera->focalDistance.setValue(oldFocalDistance);

	wxSize size = m_canvas->GetClientSize();
	if (size.x > 0 && size.y > 0) {
		m_camera->aspectRatio.setValue(static_cast<float>(size.x) / static_cast<float>(size.y));
	}

	m_sceneRoot->insertChild(m_camera, 0);

	SbViewportRegion viewportToggle(size.x, size.y);
	m_camera->viewAll(m_sceneRoot, viewportToggle);

	// Update clipping planes based on scene bounds
	updateSceneBounds();

	if (m_canvas->getRefreshManager()) {
		m_canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::CAMERA_MOVED, true);
	}
	else {
		m_canvas->Refresh(true);
	}
	LOG_INF_S(m_isPerspectiveCamera ? "Switched to Perspective Camera" : "Switched to Orthographic Camera");
}

void SceneManager::setView(const std::string& viewName) {
	if (!m_camera || !m_sceneRoot) {
		LOG_ERR_S("Failed to set view: Invalid camera or scene");
		return;
	}

	static const std::map<std::string, SbVec3f> viewDirections = {
		{ "Top", SbVec3f(0, 0, -1) },
		{ "Bottom", SbVec3f(0, 0, 1) },
		{ "Front", SbVec3f(0, -1, 0) },
		{ "Back", SbVec3f(0, 1, 0) },
		{ "Left", SbVec3f(-1, 0, 0) },
		{ "Right", SbVec3f(1, 0, 0) },
		{ "Isometric", SbVec3f(1, 1, 1) }
	};

	auto directionIt = viewDirections.find(viewName);
	if (directionIt == viewDirections.end()) {
		LOG_WRN_S("Invalid view name: " + viewName);
		return;
	}

	SbVec3f direction = directionIt->second;
	if (direction.normalize() == 0.0f) {
		LOG_WRN_S("SceneManager::setView: Direction for view '" + viewName + "' is zero vector");
		return;
	}

	// Preserve original camera state so we can animate from it
	CameraAnimation::CameraState originalState = captureCameraState();

	// Apply temporary state to leverage Inventor's viewAll for fitting
	SbRotation rotation(SbVec3f(0, 0, -1), direction);
	m_camera->orientation.setValue(rotation);
	float defaultDistance = 10.0f;
	m_camera->position.setValue(direction * defaultDistance);
	m_camera->focalDistance.setValue(defaultDistance);
	if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
		static_cast<SoOrthographicCamera*>(m_camera)->height.setValue(defaultDistance);
	}

	wxSize clientSize = m_canvas ? m_canvas->GetClientSize() : wxSize(1, 1);
	SoGetBoundingBoxAction bboxAction(SbViewportRegion(clientSize.x, clientSize.y));
	bboxAction.apply(m_sceneRoot);
	SbBox3f bbox = bboxAction.getBoundingBox();
	if (!bbox.isEmpty()) {
		SbVec3f center = bbox.getCenter();
		float radius = (bbox.getMax() - bbox.getMin()).length() / 2.0f;
		if (radius < 2.0f) {
			radius = 2.0f;
		}
		m_camera->position.setValue(center + direction * (radius * 2.0f));
		m_camera->focalDistance.setValue(radius * 2.0f);
		if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
			static_cast<SoOrthographicCamera*>(m_camera)->height.setValue(radius * 2.0f);
		}
	}

	SbViewportRegion viewport(clientSize.x, clientSize.y);
	m_camera->viewAll(m_sceneRoot, viewport, 1.1f);

	// Capture resulting state as animation target
	CameraAnimation::CameraState targetState = captureCameraState();

	// Restore original state prior to animation/direct application
	m_camera->position.setValue(originalState.position);
	m_camera->orientation.setValue(originalState.rotation);
	if (m_camera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
		static_cast<SoPerspectiveCamera*>(m_camera)->focalDistance.setValue(originalState.focalDistance);
	} else if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
		SoOrthographicCamera* orthoCam = static_cast<SoOrthographicCamera*>(m_camera);
		orthoCam->focalDistance.setValue(originalState.focalDistance);
		orthoCam->height.setValue(originalState.height);
	}

	if (m_enableViewAnimation) {
		auto& animator = NavigationAnimator::getInstance();
		animator.stopCurrentAnimation();
		animator.setCamera(m_camera);
		animator.setAnimationType(CameraAnimation::SMOOTH);
		animator.animateToPosition(
			targetState.position,
			targetState.rotation,
			m_viewAnimationDuration,
			targetState.focalDistance,
			targetState.height);
	} else {
		m_camera->position.setValue(targetState.position);
		m_camera->orientation.setValue(targetState.rotation);
		if (m_camera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
			static_cast<SoPerspectiveCamera*>(m_camera)->focalDistance.setValue(targetState.focalDistance);
		} else if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
			SoOrthographicCamera* orthoCam = static_cast<SoOrthographicCamera*>(m_camera);
			orthoCam->focalDistance.setValue(targetState.focalDistance);
			orthoCam->height.setValue(targetState.height);
		}
	}

	updateSceneBounds();

	LOG_INF_S("Switched to view: " + viewName);

	if (m_canvas->getRefreshManager()) {
		m_canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::CAMERA_MOVED, true);
	}
	else {
		m_canvas->Refresh(true);
	}
}

CameraAnimation::CameraState SceneManager::captureCameraState() const {
	CameraAnimation::CameraState state;
	if (!m_camera) {
		return state;
	}

	state.position = m_camera->position.getValue();
	state.rotation = m_camera->orientation.getValue();

	if (m_camera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
		const auto* perspCam = static_cast<const SoPerspectiveCamera*>(m_camera);
		state.focalDistance = perspCam->focalDistance.getValue();
	} else if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
		const auto* orthoCam = static_cast<const SoOrthographicCamera*>(m_camera);
		state.focalDistance = orthoCam->focalDistance.getValue();
		state.height = orthoCam->height.getValue();
	}

	return state;
}

void SceneManager::render(const wxSize& size, bool fastMode) {
	auto sceneRenderStartTime = std::chrono::high_resolution_clock::now();

	if (m_camera) {
		m_camera->aspectRatio.setValue(static_cast<float>(size.x) / static_cast<float>(size.y));
	}

	// Update culling system before rendering
	if (m_cullingEnabled && RenderingToolkitAPI::isInitialized()) {
		auto cullingStartTime = std::chrono::high_resolution_clock::now();
		updateCulling();
		auto cullingEndTime = std::chrono::high_resolution_clock::now();
		auto cullingDuration = std::chrono::duration_cast<std::chrono::microseconds>(cullingEndTime - cullingStartTime);
	}

	auto viewportStartTime = std::chrono::high_resolution_clock::now();

	// Create viewport region with validation
	SbViewportRegion viewport(size.x, size.y);
	if (viewport.getViewportSizePixels()[0] <= 0 || viewport.getViewportSizePixels()[1] <= 0) {
		LOG_ERR_S("SceneManager::render: Invalid viewport size: " +
			std::to_string(size.x) + "x" + std::to_string(size.y));
		return;
	}

	// Create render action with error checking
	SoGLRenderAction renderAction(viewport);
	try {
		renderAction.setSmoothing(!fastMode);
		renderAction.setNumPasses(fastMode ? 1 : 2);
		renderAction.setTransparencyType(
			fastMode ? SoGLRenderAction::BLEND : SoGLRenderAction::SORTED_OBJECT_BLEND
		);

		// Additional render action configuration for stability
		renderAction.setCacheContext(1); // Use a specific cache context

	} catch (const std::exception& e) {
		LOG_ERR_S("SceneManager::render: Failed to configure SoGLRenderAction: " + std::string(e.what()));
		return;
	} catch (...) {
		LOG_ERR_S("SceneManager::render: Failed to configure SoGLRenderAction (unknown exception)");
		return;
	}

	auto viewportEndTime = std::chrono::high_resolution_clock::now();
	auto viewportDuration = std::chrono::duration_cast<std::chrono::microseconds>(viewportEndTime - viewportStartTime);

	// Explicitly enable blending for line smoothing
	auto glSetupStartTime = std::chrono::high_resolution_clock::now();
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);
	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE); // allow two-sided lighting for CAD meshes
	glEnable(GL_TEXTURE_2D);
	// Note: Color buffer was already cleared by RenderingEngine::clearBuffers() with background
	// Only clear depth buffer here to maintain proper depth testing for scene rendering
	glClear(GL_DEPTH_BUFFER_BIT);

	// Basic OpenGL capability check
	const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	if (version) {
		static std::string lastVersion;
		if (lastVersion != version) {
			lastVersion = version;
			LOG_INF_S("SceneManager::render: OpenGL version: " + std::string(version));
		}
	}

	// Reset OpenGL errors before rendering
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		LOG_WRN_S("SceneManager::render: Pre-render OpenGL error: " + std::to_string(err));
	}

	// Reset OpenGL state to prevent errors
	glDisable(GL_TEXTURE_2D);
	auto glSetupEndTime = std::chrono::high_resolution_clock::now();
	auto glSetupDuration = std::chrono::duration_cast<std::chrono::microseconds>(glSetupEndTime - glSetupStartTime);

	// Render the scene with error protection
	auto coinRenderStartTime = std::chrono::high_resolution_clock::now();
	
	// Check if scene root is valid before rendering
	if (!m_sceneRoot) {
		LOG_ERR_S("SceneManager::render: Scene root is null, skipping render");
		return;
	}

	// Additional validation: Check if scene root has valid children
	if (m_sceneRoot->getNumChildren() == 0) {
		LOG_WRN_S("SceneManager::render: Scene root has no children, this may indicate an empty scene");
	}

	// Validate and repair geometry objects before rendering
	if (m_canvas && m_canvas->getOCCViewer()) {
		auto geometries = m_canvas->getOCCViewer()->getAllGeometry();
		int validGeometries = 0;
		int repairedGeometries = 0;

		for (const auto& geometry : geometries) {
			if (geometry) {
				auto coinNode = geometry->getCoinNode();
				if (coinNode) {
					validGeometries++;
				} else {
					LOG_WRN_S("SceneManager::render: Geometry '" + geometry->getName() + "' has null Coin3D node, rebuilding...");
					try {
						// Force rebuild the Coin3D representation
						MeshParameters defaultParams;
						geometry->forceCoinRepresentationRebuild(defaultParams);
						repairedGeometries++;
						LOG_INF_S("SceneManager::render: Successfully rebuilt Coin3D node for geometry '" + geometry->getName() + "'");
					} catch (const std::exception& e) {
						LOG_ERR_S("SceneManager::render: Failed to rebuild geometry '" + geometry->getName() + "': " + std::string(e.what()));
					} catch (...) {
						LOG_ERR_S("SceneManager::render: Failed to rebuild geometry '" + geometry->getName() + "' (unknown exception)");
					}
				}
			}
		}

		if (repairedGeometries > 0) {
			LOG_INF_S("SceneManager::render: Repaired " + std::to_string(repairedGeometries) +
				" geometries with invalid Coin3D nodes");
		}
	}

	try {
		renderAction.apply(m_sceneRoot);
	} catch (const std::exception& e) {
		LOG_ERR_S("SceneManager::render: Exception during Coin3D rendering: " + std::string(e.what()));
		// Try to recover by clearing and rebuilding scene
		rebuildScene();
		return;
	} catch (...) {
		LOG_ERR_S("SceneManager::render: Unknown exception during Coin3D rendering");
		// Try to recover by clearing and rebuilding scene
		rebuildScene();
		return;
	}
	
	auto coinRenderEndTime = std::chrono::high_resolution_clock::now();
	auto coinRenderDuration = std::chrono::duration_cast<std::chrono::milliseconds>(coinRenderEndTime - coinRenderStartTime);

	// Check for OpenGL errors after rendering
	while ((err = glGetError()) != GL_NO_ERROR) {
		LOG_ERR_S("Post-render: OpenGL error: " + std::to_string(err));
	}

	auto sceneRenderEndTime = std::chrono::high_resolution_clock::now();
	auto sceneRenderDuration = std::chrono::duration_cast<std::chrono::milliseconds>(sceneRenderEndTime - sceneRenderStartTime);

	// Publish to PerformanceDataBus instead of logging
	perf::ScenePerfSample s;
	s.width = size.x;
	s.height = size.y;
	s.mode = fastMode ? "FAST" : "QUALITY";
	s.viewportUs = static_cast<int>(viewportDuration.count());
	s.glSetupUs = static_cast<int>(glSetupDuration.count());
	s.coinSceneMs = static_cast<int>(coinRenderDuration.count());
	s.totalSceneMs = static_cast<int>(sceneRenderDuration.count());
	s.fps = 1000.0 / std::max(1, s.totalSceneMs);
	perf::PerformanceBus::instance().setScene(s);
}

void SceneManager::updateAspectRatio(const wxSize& size) {
	if (m_camera && size.x > 0 && size.y > 0) {
		m_camera->aspectRatio.setValue(static_cast<float>(size.x) / static_cast<float>(size.y));
	}
}

bool SceneManager::screenToWorld(const wxPoint& screenPos, SbVec3f& worldPos) {
	if (!m_camera) {
		LOG_ERR_S("Cannot convert screen to world: Invalid camera");
		return false;
	}

	wxSize size = m_canvas->GetClientSize();
	if (size.x <= 0 || size.y <= 0) {
		LOG_ERR_S("Invalid viewport size");
		return false;
	}

	// Convert screen coordinates to normalized device coordinates
	float x = static_cast<float>(screenPos.x) / size.GetWidth();
	float y = 1.0f - static_cast<float>(screenPos.y) / size.GetHeight();

	// For SoRayPickAction, we need to use the original screen coordinates
	// but with Y-axis flipped to match OpenGL/OpenInventor coordinate system
	int pickY = size.GetHeight() - screenPos.y;

	SbViewportRegion viewport(size.GetWidth(), size.GetHeight());
	SbVec2f normalizedPos(x, y);
	SbLine lineFromCamera;
	m_camera->getViewVolume().projectPointToLine(normalizedPos, lineFromCamera);

	// First, try to pick objects in the scene using SoRayPickAction
	SoRayPickAction pickAction(viewport);
	pickAction.setPoint(SbVec2s(screenPos.x, pickY));  // Use flipped Y coordinate
	pickAction.setRadius(2); // Small radius for picking
	pickAction.apply(m_sceneRoot);

	SoPickedPoint* pickedPoint = pickAction.getPickedPoint();
	if (pickedPoint) {
		worldPos = pickedPoint->getPoint();
		return true;
	}
	else {
		LOG_ERR_S("No geometry picked at screen position: (" + std::to_string(screenPos.x) + ", " + std::to_string(screenPos.y) + ")");
	}

	// If no geometry was picked, try intersecting with the current reference plane
	float referenceZ = m_pickingAidManager ? m_pickingAidManager->getReferenceZ() : 0.0f;
	SbPlane referencePlane(SbVec3f(0, 0, 1), referenceZ);
	if (referencePlane.intersect(lineFromCamera, worldPos)) {
		LOG_INF_S("Ray intersected reference plane at Z=" + std::to_string(referenceZ));
		return true;
	}

	// If reference plane intersection fails, try other common planes
	// XY plane at different Z levels
	float zLevels[] = { 0.0f, 1.0f, -1.0f, 2.0f, -2.0f, 5.0f, -5.0f };
	for (float z : zLevels) {
		if (z == referenceZ) continue; // Skip already tested reference plane
		SbPlane plane(SbVec3f(0, 0, 1), z);
		if (plane.intersect(lineFromCamera, worldPos)) {
			LOG_INF_S("Ray intersected plane at Z=" + std::to_string(z));
			return true;
		}
	}

	// XZ plane (Y=0)
	SbPlane xzPlane(SbVec3f(0, 1, 0), 0.0f);
	if (xzPlane.intersect(lineFromCamera, worldPos)) {
		LOG_INF_S("Ray intersected XZ plane (Y=0)");
		return true;
	}

	// YZ plane (X=0)
	SbPlane yzPlane(SbVec3f(1, 0, 0), 0.0f);
	if (yzPlane.intersect(lineFromCamera, worldPos)) {
		LOG_INF_S("Ray intersected YZ plane (X=0)");
		return true;
	}

	// As a last resort, project to a point at the focal distance
	LOG_WRN_S("No plane intersection found, using focal distance projection");
	worldPos = lineFromCamera.getPosition() + lineFromCamera.getDirection() * m_camera->focalDistance.getValue();
	return true;
}

void SceneManager::updateSceneBounds() {
	if (!m_objectRoot || m_objectRoot->getNumChildren() == 0) {
		m_sceneBoundingBox.makeEmpty();
		return;
	}

	SbViewportRegion viewport(m_canvas->GetClientSize().x, m_canvas->GetClientSize().y);
	SoGetBoundingBoxAction bboxAction(viewport);
	bboxAction.apply(m_objectRoot);
	m_sceneBoundingBox = bboxAction.getBoundingBox();

	if (!m_sceneBoundingBox.isEmpty()) {
		LOG_INF_S("Scene bounds updated.");
		if (m_coordSystemRenderer) {
			m_coordSystemRenderer->updateCoordinateSystemSize(getSceneBoundingBoxSize());
		}
		// Add this line to update reference grid
		if (m_pickingAidManager) {
			m_pickingAidManager->updateReferenceGrid();
		}

		// Update camera clipping planes based on scene bounds
		updateCameraClippingPlanes();
	}
}

void SceneManager::updateCameraClippingPlanes() {
	if (!m_camera || m_sceneBoundingBox.isEmpty()) {
		LOG_DBG_S("Skipping clipping plane update - camera or scene bounds invalid");
		return;
	}

	// Get camera position
	SbVec3f cameraPos = m_camera->position.getValue();

	// Get scene bounding box
	SbVec3f min, max;
	m_sceneBoundingBox.getBounds(min, max);
	SbVec3f sceneCenter = m_sceneBoundingBox.getCenter();

	// Calculate scene dimensions
	float sceneRadius = (max - min).length() * 0.5f;
	float cameraDist = (sceneCenter - cameraPos).length();

	// Calculate distances from camera to scene bounds
	float distToMin = (min - cameraPos).length();
	float distToMax = (max - cameraPos).length();

	// Calculate optimal near and far clipping planes
	// Near plane: camera to closest point - safety margin
	float optimalNear = std::max(0.001f, cameraDist - sceneRadius * 1.5f);
	
	// Far plane: camera to farthest point + safety margin
	float optimalFar = cameraDist + sceneRadius * 1.5f;

	// Ensure minimum values
	optimalNear = std::max(0.001f, optimalNear);
	optimalFar = std::max(10.0f, optimalFar);

	// For very large scenes, don't limit the far plane
	// Only apply reasonable near plane limit
	optimalNear = std::min(optimalNear, sceneRadius * 0.1f);  // Near can't be more than 10% of scene size
	
	// No upper limit on far plane - let it adapt to scene size
	// optimalFar = std::min(optimalFar, 10000.0f); // REMOVED - this was causing clipping

	// Update camera clipping planes
	m_camera->nearDistance.setValue(optimalNear);
	m_camera->farDistance.setValue(optimalFar);

	// LOG_INF_S("Updated clipping planes - Near: " + std::to_string(optimalNear) +
	//           ", Far: " + std::to_string(optimalFar) +
	//           ", Scene radius: " + std::to_string(sceneRadius) +
	//           ", Camera dist to scene: " + std::to_string(cameraDist));
}

float SceneManager::getSceneBoundingBoxSize() const {
	if (m_sceneBoundingBox.isEmpty()) {
		return 10.0f; // Return a default size if scene is empty
	}
	SbVec3f min, max;
	m_sceneBoundingBox.getBounds(min, max);
	return (max - min).length();
}

void SceneManager::updateCoordinateSystemScale() {
	if (m_coordSystemRenderer) {
		// Implementation to be added
	}
}

void SceneManager::initializeScene() {
	// This method was declared but not defined.
	// Providing a basic implementation.
	LOG_INF_S("SceneManager::initializeScene called.");
	// If there's specific initialization logic needed, it should go here.
}

void SceneManager::createCheckerboardPlane(float planeZ) {
	if (m_checkerboardSeparator) return;
	m_checkerboardSeparator = new SoSeparator;
	m_checkerboardSeparator->ref();

	auto* coordinates = new SoCoordinate3;
	const int gridSize = 8;
	const float cellSize = 10.0f;
	const float halfSize = (gridSize * cellSize) / 2.0f;

	for (int z = 0; z <= gridSize; ++z) {
		for (int x = 0; x <= gridSize; ++x) {
			float xPos = x * cellSize - halfSize;
			float yPos = z * cellSize - halfSize;
			coordinates->point.set1Value(z * (gridSize + 1) + x, SbVec3f(xPos, yPos, planeZ));
		}
	}
	m_checkerboardSeparator->addChild(coordinates);

	for (int z = 0; z < gridSize; ++z) {
		for (int x = 0; x < gridSize; ++x) {
			auto* cellGroup = new SoSeparator;
			bool isLightCell = ((x + z) % 2) == 0;
			auto* cellMaterial = new SoMaterial;
			if (isLightCell) {
				cellMaterial->diffuseColor.setValue(0.90f, 0.90f, 0.92f);
				cellMaterial->ambientColor.setValue(0.70f, 0.70f, 0.72f);
			}
			else {
				cellMaterial->diffuseColor.setValue(0.60f, 0.60f, 0.65f);
				cellMaterial->ambientColor.setValue(0.40f, 0.40f, 0.45f);
			}
			cellMaterial->transparency.setValue(0.6f);
			cellGroup->addChild(cellMaterial);

			auto* faceSet = new SoIndexedFaceSet;
			int baseIndex = z * (gridSize + 1) + x;
			faceSet->coordIndex.set1Value(0, baseIndex);
			faceSet->coordIndex.set1Value(1, baseIndex + 1);
			faceSet->coordIndex.set1Value(2, baseIndex + gridSize + 2);
			faceSet->coordIndex.set1Value(3, baseIndex + gridSize + 1);
			faceSet->coordIndex.set1Value(4, -1);
			cellGroup->addChild(faceSet);
			m_checkerboardSeparator->addChild(cellGroup);
		}
	}

	auto* lineSet = new SoIndexedLineSet;
	for (int z = 0; z < gridSize; ++z) {
		for (int x = 0; x < gridSize; ++x) {
			int baseIndex = z * (gridSize + 1) + x;
			lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), baseIndex);
			lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), baseIndex + 1);
			lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), -1);
			lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), baseIndex);
			lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), baseIndex + gridSize + 1);
			lineSet->coordIndex.set1Value(lineSet->coordIndex.getNum(), -1);
		}
	}
	auto* lineMaterial = new SoMaterial;
	lineMaterial->diffuseColor.setValue(0.3f, 0.3f, 0.3f);
	m_checkerboardSeparator->addChild(lineMaterial);
	m_checkerboardSeparator->addChild(lineSet);
}

void SceneManager::setCheckerboardVisible(bool visible) {
	if (visible) {
		if (!m_checkerboardSeparator) createCheckerboardPlane(0.0f);
		if (!m_checkerboardVisible && m_objectRoot && m_checkerboardSeparator) {
			m_objectRoot->addChild(m_checkerboardSeparator);
			m_checkerboardVisible = true;
		}
	}
	else {
		if (m_checkerboardSeparator && m_objectRoot && m_checkerboardVisible) {
			m_objectRoot->removeChild(m_checkerboardSeparator);
			m_checkerboardVisible = false;
		}
	}
	if (m_canvas) {
		m_canvas->Refresh(true);
	}
}

bool SceneManager::isCheckerboardVisible() const {
	return m_checkerboardVisible;
}

void SceneManager::getSceneBoundingBoxMinMax(SbVec3f& min, SbVec3f& max) const {
	if (m_sceneBoundingBox.isEmpty()) {
		min = SbVec3f(-10.0f, -10.0f, 0.0f);
		max = SbVec3f(10.0f, 10.0f, 0.0f);
	}
	else {
		m_sceneBoundingBox.getBounds(min, max);
	}
}

void SceneManager::initializeRenderingConfigCallback()
{
	// Register callback to update geometries when RenderingConfig changes
	RenderingConfig& config = RenderingConfig::getInstance();
	config.registerSettingsChangedCallback([this]() {
		LOG_INF_S("RenderingConfig callback triggered - updating geometries and lighting");

		// Check if display mode changed to/from NoShading - if so, update lighting
		if (m_canvas && m_canvas->getOCCViewer()) {
			auto displaySettings = m_canvas->getOCCViewer()->getDisplaySettings();
			static RenderingConfig::DisplayMode lastDisplayMode = displaySettings.displayMode;

			if (lastDisplayMode != displaySettings.displayMode) {
				if (lastDisplayMode == RenderingConfig::DisplayMode::NoShading ||
					displaySettings.displayMode == RenderingConfig::DisplayMode::NoShading) {
					LOG_INF_S("RenderingConfig callback: Display mode changed to/from NoShading, updating lighting");
					updateSceneLighting();
				}
				lastDisplayMode = displaySettings.displayMode;
			}
		}

		if (m_canvas && m_canvas->getOCCViewer()) {
			OCCViewer* viewer = m_canvas->getOCCViewer();
			auto selectedGeometries = viewer->getSelectedGeometries();

			// Check if any objects are selected
			if (!selectedGeometries.empty()) {
				LOG_INF_S("Found " + std::to_string(selectedGeometries.size()) + " selected geometries to update");

				// Update only selected geometries
				for (auto& geometry : selectedGeometries) {
					LOG_INF_S("Updating selected geometry: " + geometry->getName());
					geometry->updateFromRenderingConfig();
				}

				// Test feedback for selected objects
				LOG_INF_S("=== Test Feedback: Updated " + std::to_string(selectedGeometries.size()) + " selected objects ===");
			}
			else {
				LOG_INF_S("No objects selected, updating all geometries");

				// Update all geometries if none are selected
				auto allGeometries = viewer->getAllGeometry();
				LOG_INF_S("Found " + std::to_string(allGeometries.size()) + " total geometries to update");

				for (auto& geometry : allGeometries) {
					LOG_INF_S("Updating geometry: " + geometry->getName());
					geometry->updateFromRenderingConfig();
				}

				// Test feedback for all objects
				LOG_INF_S("=== Test Feedback: Updated " + std::to_string(allGeometries.size()) + " total objects ===");
			}

			// Force refresh with multiple methods to ensure update
			LOG_INF_S("Requesting refresh via multiple methods");

			// Method 1: RefreshManager
			if (m_canvas->getRefreshManager()) {
				m_canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::RENDERING_CHANGED, true);
			}

			// Method 2: Direct Canvas refresh
			m_canvas->Refresh(true);

			// Method 3: Force immediate update
			m_canvas->Update();

			// Method 4: Touch the scene root to force Coin3D update
			if (m_sceneRoot) {
				m_sceneRoot->touch();
				LOG_INF_S("Touched scene root to force Coin3D update");
			}

			LOG_INF_S("Updated geometries from RenderingConfig changes");
		}
		else {
			LOG_ERR_S("Cannot update geometries: Canvas or OCCViewer not available");
		}
		});

	LOG_INF_S("RenderingConfig callback initialized in SceneManager");
}

void SceneManager::initializeLightingConfigCallback() {
	// Register callback to update lighting when LightingConfig changes
	LightingConfig& config = LightingConfig::getInstance();
	config.addSettingsChangedCallback([this]() {
		LOG_INF_S("LightingConfig callback triggered - updating scene lighting");
		updateSceneLighting();
		});

	LOG_INF_S("LightingConfig callback initialized in SceneManager");
}

void SceneManager::updateSceneLighting() {
	if (!m_sceneRoot) {
		LOG_ERR_S("Cannot update lighting: Scene root not available");
		return;
	}

	LOG_INF_S("Updating lighting from configuration");

	// Check if we're in NoShading mode - use different lighting setup
	bool isNoShading = false;
	if (m_canvas && m_canvas->getOCCViewer()) {
		auto displaySettings = m_canvas->getOCCViewer()->getDisplaySettings();
		isNoShading = (displaySettings.displayMode == RenderingConfig::DisplayMode::NoShading);
	}

	// Clear existing environment and light nodes that were previously inserted
	// We only remove nodes that appear before m_objectRoot to avoid touching other overlays
	int objectIndex = m_objectRoot ? m_sceneRoot->findChild(m_objectRoot) : -1;
	for (int i = m_sceneRoot->getNumChildren() - 1; i >= 0; --i) {
		if (objectIndex >= 0 && i >= objectIndex) continue; // do not remove anything after objects
		SoNode* child = m_sceneRoot->getChild(i);
		if (child->isOfType(SoEnvironment::getClassTypeId()) ||
			child->isOfType(SoDirectionalLight::getClassTypeId()) ||
			child->isOfType(SoPointLight::getClassTypeId()) ||
			child->isOfType(SoSpotLight::getClassTypeId())) {
			m_sceneRoot->removeChild(i);
		}
	}

	// Force rebuild geometries to apply new lighting/material settings
	if (m_canvas && m_canvas->getOCCViewer()) {
		OCCViewer* viewer = m_canvas->getOCCViewer();
		auto allGeometries = viewer->getAllGeometry();
		LOG_INF_S("Updating " + std::to_string(allGeometries.size()) + " geometries for lighting changes");

		for (auto& geometry : allGeometries) {
			if (geometry) {
				LOG_INF_S("Updated material for lighting: " + geometry->getName());
				geometry->forceCoinRepresentationRebuild(MeshParameters());
		}
	}
	}

	// Get lighting configuration
	LightingConfig& config = LightingConfig::getInstance();

	// Set light model based on display mode
	SoLightModel* lightModel = new SoLightModel;
	if (isNoShading) {
		// Use BASE_COLOR for no shading - no lighting calculations, direct color
		lightModel->model.setValue(SoLightModel::BASE_COLOR);
		LOG_INF_S("SceneManager::updateSceneLighting: Using BASE_COLOR light model for NoShading");
	} else {
		// Use PHONG for normal lighting
		lightModel->model.setValue(SoLightModel::PHONG);
	}

	// Insert light model at the beginning of scene root (after camera)
	if (m_sceneRoot) {
		// Insert new light model after camera (index 1)
		m_sceneRoot->insertChild(lightModel, 1);
	}

	// Add environment settings
	auto envSettings = config.getEnvironmentSettings();
	SoEnvironment* environment = new SoEnvironment;

	// Convert Quantity_Color to SbColor
	Standard_Real r = 0.9, g = 0.9, b = 0.9; // Default neutral values
	if (isNoShading) {
		// For NoShading mode, use neutral ambient color with moderate intensity
		environment->ambientColor.setValue(0.9f, 0.9f, 0.9f); // Light gray ambient
		environment->ambientIntensity.setValue(0.5f); // Moderate ambient intensity
		LOG_INF_S("SceneManager::updateSceneLighting: Using neutral ambient lighting for NoShading");
	} else {
		// Use configured ambient settings for normal modes
		envSettings.ambientColor.Values(r, g, b, Quantity_TOC_RGB);
		environment->ambientColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
		environment->ambientIntensity.setValue(static_cast<float>(envSettings.ambientIntensity));
	}

	// Place environment on scene root before object root so it affects geometry
	{
		int insertIndex = m_sceneRoot->getNumChildren();
		if (m_objectRoot) {
			int objIdx = m_sceneRoot->findChild(m_objectRoot);
			if (objIdx >= 0) insertIndex = objIdx; // insert before objects
		}
		m_sceneRoot->insertChild(environment, insertIndex);
	}

	// Use appropriate intensity value for logging
	float logIntensity = isNoShading ? 0.5f : static_cast<float>(envSettings.ambientIntensity);
	LOG_INF_S("Added environment lighting - ambient color: " +
		std::to_string(r) + "," + std::to_string(g) + "," + std::to_string(b) +
		", intensity: " + std::to_string(logIntensity));

	// Add lights from configuration
	auto lights = config.getAllLights();
	LOG_INF_S("Adding " + std::to_string(lights.size()) + " lights from configuration");

	for (const auto& lightSettings : lights) {
		if (!lightSettings.enabled) {
			LOG_INF_S("Skipping disabled light: " + lightSettings.name);
			continue;
		}

		try {
			if (lightSettings.type == "directional") {
				SoDirectionalLight* light = new SoDirectionalLight;

				// Set light properties
				light->direction.setValue(static_cast<float>(lightSettings.directionX),
					static_cast<float>(lightSettings.directionY),
					static_cast<float>(lightSettings.directionZ));

				Standard_Real r, g, b;
				lightSettings.color.Values(r, g, b, Quantity_TOC_RGB);
				light->color.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
				light->intensity.setValue(static_cast<float>(lightSettings.intensity));
				light->on.setValue(true);

				// Place lights on scene root before object root so they affect subsequent geometry
				{
					int insertIndex = m_sceneRoot->getNumChildren();
					if (m_objectRoot) {
						int objIdx = m_sceneRoot->findChild(m_objectRoot);
						if (objIdx >= 0) insertIndex = objIdx; // insert before objects
					}
					m_sceneRoot->insertChild(light, insertIndex);
				}

				// Store reference to main light for compatibility
				if (lightSettings.name == "Main Light") {
					if (m_light) { m_light->unref(); }
					m_light = light;
					m_light->ref();
				}

				LOG_INF_S("Added directional light: " + lightSettings.name);
			}
			else if (lightSettings.type == "point") {
				SoPointLight* light = new SoPointLight;

				// Set light properties
				light->location.setValue(static_cast<float>(lightSettings.positionX),
					static_cast<float>(lightSettings.positionY),
					static_cast<float>(lightSettings.positionZ));

				Standard_Real r, g, b;
				lightSettings.color.Values(r, g, b, Quantity_TOC_RGB);
				light->color.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
				light->intensity.setValue(static_cast<float>(lightSettings.intensity));
				light->on.setValue(true);

				{
					int insertIndex = m_sceneRoot->getNumChildren();
					if (m_objectRoot) {
						int objIdx = m_sceneRoot->findChild(m_objectRoot);
						if (objIdx >= 0) insertIndex = objIdx; // insert before objects
					}
					m_sceneRoot->insertChild(light, insertIndex);
				}

				LOG_INF_S("Added point light: " + lightSettings.name);
			}
			else if (lightSettings.type == "spot") {
				SoSpotLight* light = new SoSpotLight;

				// Set light properties
				light->location.setValue(static_cast<float>(lightSettings.positionX),
					static_cast<float>(lightSettings.positionY),
					static_cast<float>(lightSettings.positionZ));
				light->direction.setValue(static_cast<float>(lightSettings.directionX),
					static_cast<float>(lightSettings.directionY),
					static_cast<float>(lightSettings.directionZ));

				Standard_Real r, g, b;
				lightSettings.color.Values(r, g, b, Quantity_TOC_RGB);
				light->color.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
				light->intensity.setValue(static_cast<float>(lightSettings.intensity));
				light->on.setValue(true);

				{
					int insertIndex = m_sceneRoot->getNumChildren();
					if (m_objectRoot) {
						int objIdx = m_sceneRoot->findChild(m_objectRoot);
						if (objIdx >= 0) insertIndex = objIdx; // insert before objects
					}
					m_sceneRoot->insertChild(light, insertIndex);
				}

				LOG_INF_S("Added spot light: " + lightSettings.name);
			}
		}
		catch (const std::exception& e) {
			LOG_ERR_S("Exception while creating light " + lightSettings.name + ": " + std::string(e.what()));
		}
		catch (...) {
			LOG_ERR_S("Unknown exception while creating light " + lightSettings.name);
		}
	}

	// Ensure we have a main light reference for compatibility
	if (!m_light) {
		// Create a default main light if none exists
		m_light = new SoDirectionalLight;
		m_light->ref();
		m_light->direction.setValue(0.5f, 0.5f, -0.7f);
		m_light->intensity.setValue(1.0f);
		m_light->color.setValue(1.0f, 1.0f, 1.0f);
		m_light->on.setValue(true);
		{
			int insertIndex = m_sceneRoot->getNumChildren();
			if (m_objectRoot) {
				int objIdx = m_sceneRoot->findChild(m_objectRoot);
				if (objIdx >= 0) insertIndex = objIdx;
			}
			m_sceneRoot->insertChild(m_light, insertIndex);
		}
		LOG_INF_S("Created default main light for compatibility");
	}

	// Force scene update
	if (m_sceneRoot) {
		m_sceneRoot->touch();
		LOG_INF_S("Touched scene root to force lighting update");
	}

	// Update all geometries to respond to lighting changes
	if (m_canvas && m_canvas->getOCCViewer()) {
		OCCViewer* viewer = m_canvas->getOCCViewer();
		auto allGeometries = viewer->getAllGeometry();
		LOG_INF_S("Updating " + std::to_string(allGeometries.size()) + " geometries for lighting changes");

		for (auto& geometry : allGeometries) {
			if (geometry) {
				// Update material properties for better lighting response
				geometry->updateMaterialForLighting();
				LOG_INF_S("Updated material for lighting: " + geometry->getName());
			}
		}
	}

	// Request refresh
	if (m_canvas) {
		if (m_canvas->getRefreshManager()) {
			m_canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::LIGHTING_CHANGED, true);
		}
		else {
			m_canvas->Refresh(true);
		}
		LOG_INF_S("Requested scene refresh for lighting changes");
	}

	LOG_INF_S("Scene lighting updated successfully");
}

void SceneManager::initializeLightingFromConfig() {
	if (!m_sceneRoot) {
		LOG_ERR_S("Cannot initialize lighting: Scene root not available");
		return;
	}

	LOG_INF_S("Initializing lighting from configuration");

	// Clear existing environment and light nodes before objects
	int objectIndex = m_objectRoot ? m_sceneRoot->findChild(m_objectRoot) : -1;
	for (int i = m_sceneRoot->getNumChildren() - 1; i >= 0; --i) {
		if (objectIndex >= 0 && i >= objectIndex) continue;
		SoNode* child = m_sceneRoot->getChild(i);
		if (child->isOfType(SoEnvironment::getClassTypeId()) ||
			child->isOfType(SoDirectionalLight::getClassTypeId()) ||
			child->isOfType(SoPointLight::getClassTypeId()) ||
			child->isOfType(SoSpotLight::getClassTypeId())) {
			m_sceneRoot->removeChild(i);
		}
	}

	// Get lighting configuration
	LightingConfig& config = LightingConfig::getInstance();

	// Add environment settings
	auto envSettings = config.getEnvironmentSettings();
	SoEnvironment* environment = new SoEnvironment;

	// Convert Quantity_Color to SbColor
	Standard_Real r, g, b;
	envSettings.ambientColor.Values(r, g, b, Quantity_TOC_RGB);
	environment->ambientColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
	environment->ambientIntensity.setValue(static_cast<float>(envSettings.ambientIntensity));

	// Place environment on scene root before object root so it affects geometry
	{
		int insertIndex = m_sceneRoot->getNumChildren();
		if (m_objectRoot) {
			int objIdx = m_sceneRoot->findChild(m_objectRoot);
			if (objIdx >= 0) insertIndex = objIdx; // insert before objects
		}
		m_sceneRoot->insertChild(environment, insertIndex);
	}

	LOG_INF_S("Added environment lighting - ambient color: " +
		std::to_string(r) + "," + std::to_string(g) + "," + std::to_string(b) +
		", intensity: " + std::to_string(envSettings.ambientIntensity));

	// Add lights from configuration
	auto lights = config.getAllLights();
	LOG_INF_S("Adding " + std::to_string(lights.size()) + " lights from configuration");

	for (const auto& lightSettings : lights) {
		if (!lightSettings.enabled) {
			LOG_INF_S("Skipping disabled light: " + lightSettings.name);
			continue;
		}

		try {
			if (lightSettings.type == "directional") {
				SoDirectionalLight* light = new SoDirectionalLight;

				// Set light properties
				light->direction.setValue(static_cast<float>(lightSettings.directionX),
					static_cast<float>(lightSettings.directionY),
					static_cast<float>(lightSettings.directionZ));

				Standard_Real r, g, b;
				lightSettings.color.Values(r, g, b, Quantity_TOC_RGB);
				light->color.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
				light->intensity.setValue(static_cast<float>(lightSettings.intensity));
				light->on.setValue(true);

				{
					int insertIndex = m_sceneRoot->getNumChildren();
					if (m_objectRoot) {
						int objIdx = m_sceneRoot->findChild(m_objectRoot);
						if (objIdx >= 0) insertIndex = objIdx; // insert before objects
					}
					m_sceneRoot->insertChild(light, insertIndex);
				}

				// Store reference to main light for compatibility
				if (lightSettings.name == "Main Light") {
					m_light = light;
					m_light->ref();
				}

				LOG_INF_S("Added directional light: " + lightSettings.name);
			}
			else if (lightSettings.type == "point") {
				SoPointLight* light = new SoPointLight;

				// Set light properties
				light->location.setValue(static_cast<float>(lightSettings.positionX),
					static_cast<float>(lightSettings.positionY),
					static_cast<float>(lightSettings.positionZ));

				Standard_Real r, g, b;
				lightSettings.color.Values(r, g, b, Quantity_TOC_RGB);
				light->color.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
				light->intensity.setValue(static_cast<float>(lightSettings.intensity));
				light->on.setValue(true);

				{
					int insertIndex = m_sceneRoot->getNumChildren();
					if (m_objectRoot) {
						int objIdx = m_sceneRoot->findChild(m_objectRoot);
						if (objIdx >= 0) insertIndex = objIdx; // insert before objects
					}
					m_sceneRoot->insertChild(light, insertIndex);
				}

				LOG_INF_S("Added point light: " + lightSettings.name);
			}
			else if (lightSettings.type == "spot") {
				SoSpotLight* light = new SoSpotLight;

				// Set light properties
				light->location.setValue(static_cast<float>(lightSettings.positionX),
					static_cast<float>(lightSettings.positionY),
					static_cast<float>(lightSettings.positionZ));
				light->direction.setValue(static_cast<float>(lightSettings.directionX),
					static_cast<float>(lightSettings.directionY),
					static_cast<float>(lightSettings.directionZ));

				Standard_Real r, g, b;
				lightSettings.color.Values(r, g, b, Quantity_TOC_RGB);
				light->color.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
				light->intensity.setValue(static_cast<float>(lightSettings.intensity));
				light->on.setValue(true);

				{
					int insertIndex = m_sceneRoot->getNumChildren();
					if (m_objectRoot) {
						int objIdx = m_sceneRoot->findChild(m_objectRoot);
						if (objIdx >= 0) insertIndex = objIdx; // insert before objects
					}
					m_sceneRoot->insertChild(light, insertIndex);
				}

				LOG_INF_S("Added spot light: " + lightSettings.name);
			}
		}
		catch (const std::exception& e) {
			LOG_ERR_S("Exception while creating light " + lightSettings.name + ": " + std::string(e.what()));
		}
		catch (...) {
			LOG_ERR_S("Unknown exception while creating light " + lightSettings.name);
		}
	}

	// Ensure we have a main light reference for compatibility
	if (!m_light) {
		// Create a default main light if none exists
		m_light = new SoDirectionalLight;
		m_light->ref();
		m_light->direction.setValue(0.5f, 0.5f, -0.7f);
		m_light->intensity.setValue(1.0f);
		m_light->color.setValue(1.0f, 1.0f, 1.0f);
		m_light->on.setValue(true);
		{
			int insertIndex = m_sceneRoot->getNumChildren();
			if (m_objectRoot) {
				int objIdx = m_sceneRoot->findChild(m_objectRoot);
				if (objIdx >= 0) insertIndex = objIdx;
			}
			m_sceneRoot->insertChild(m_light, insertIndex);
		}
		LOG_INF_S("Created default main light for compatibility");
	}

	LOG_INF_S("Lighting initialization from configuration completed");
}

// Culling system integration methods
void SceneManager::updateCulling() {
	if (!m_camera) return;
	RenderingToolkitAPI::updateCulling(m_camera);
	m_lastCullingUpdateValid = true;
}
bool SceneManager::shouldRenderShape(const TopoDS_Shape& shape) const {
	if (!m_cullingEnabled || !m_lastCullingUpdateValid) return true;
	return RenderingToolkitAPI::shouldRenderShape(shape);
}

void SceneManager::addOccluder(const TopoDS_Shape& shape) {
	RenderingToolkitAPI::addOccluder(shape, nullptr);
}

void SceneManager::removeOccluder(const TopoDS_Shape& shape) {
	RenderingToolkitAPI::removeOccluder(shape);
}

void SceneManager::setFrustumCullingEnabled(bool enabled) {
	RenderingToolkitAPI::setFrustumCullingEnabled(enabled);
}

void SceneManager::setOcclusionCullingEnabled(bool enabled) {
	RenderingToolkitAPI::setOcclusionCullingEnabled(enabled);
}

std::string SceneManager::getCullingStats() const {
	return RenderingToolkitAPI::getCullingStats();
}

void SceneManager::debugLightingState() const
{
	LOG_INF_S("=== SceneManager Lighting Debug ===");

	if (!m_sceneRoot) {
		LOG_INF_S("Scene root is null");
		return;
	}

	LOG_INF_S("Scene root has " + std::to_string(m_sceneRoot->getNumChildren()) + " children");

	if (!m_lightRoot) {
		LOG_INF_S("Light root is null");
		return;
	}

	LOG_INF_S("Light root has " + std::to_string(m_lightRoot->getNumChildren()) + " children");

	// Check each child of light root
	for (int i = 0; i < m_lightRoot->getNumChildren(); ++i) {
		SoNode* child = m_lightRoot->getChild(i);
		if (child->isOfType(SoLightModel::getClassTypeId())) {
			LOG_INF_S("Child " + std::to_string(i) + ": SoLightModel");
		}
		else if (child->isOfType(SoEnvironment::getClassTypeId())) {
			SoEnvironment* env = static_cast<SoEnvironment*>(child);
			SbColor color = env->ambientColor.getValue();
			float intensity = env->ambientIntensity.getValue();
			LOG_INF_S("Child " + std::to_string(i) + ": SoEnvironment (ambient color: " +
				std::to_string(color[0]) + "," + std::to_string(color[1]) + "," + std::to_string(color[2]) +
				", intensity: " + std::to_string(intensity) + ")");
		}
		else if (child->isOfType(SoDirectionalLight::getClassTypeId())) {
			SoDirectionalLight* light = static_cast<SoDirectionalLight*>(child);
			SbVec3f dir = light->direction.getValue();
			SbColor color = light->color.getValue();
			float intensity = light->intensity.getValue();
			bool on = light->on.getValue();
			LOG_INF_S("Child " + std::to_string(i) + ": SoDirectionalLight (direction: " +
				std::to_string(dir[0]) + "," + std::to_string(dir[1]) + "," + std::to_string(dir[2]) +
				", color: " + std::to_string(color[0]) + "," + std::to_string(color[1]) + "," + std::to_string(color[2]) +
				", intensity: " + std::to_string(intensity) + ", on: " + std::to_string(on) + ")");
		}
		else {
			LOG_INF_S("Child " + std::to_string(i) + ": Unknown type");
		}
	}

	LOG_INF_S("=== End Lighting Debug ===");
}

void SceneManager::setCoordinateSystemVisible(bool visible)
{
	if (m_coordSystemRenderer) {
		m_coordSystemRenderer->setVisible(visible);

		// Force multiple refresh methods to ensure update
		LOG_INF_S("Forcing scene refresh for coordinate system visibility change");

		// Method 1: Touch the scene root to force Coin3D update
		if (m_sceneRoot) {
			m_sceneRoot->touch();
			LOG_INF_S("Touched scene root for coordinate system visibility");
		}

		// Method 2: Force immediate render update
		if (m_canvas) {
			m_canvas->Refresh(true);
			m_canvas->Update();
			LOG_INF_S("Forced canvas refresh and update for coordinate system visibility");
		}

		// Method 3: RefreshManager
		if (m_canvas && m_canvas->getRefreshManager()) {
			m_canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::GEOMETRY_CHANGED, true);
			LOG_INF_S("Requested refresh via RefreshManager for coordinate system visibility");
		}

		LOG_INF_S("Set coordinate system visibility: " + std::string(visible ? "visible" : "hidden"));
	}
	else {
		LOG_WRN_S("Coordinate system renderer not available");
	}
}

bool SceneManager::isCoordinateSystemVisible() const
{
	if (m_coordSystemRenderer) {
		return m_coordSystemRenderer->isVisible();
	}
	return false;
}

void SceneManager::updateCoordinateSystemColorsForBackground(float backgroundBrightness)
{
	if (m_coordSystemRenderer) {
		m_coordSystemRenderer->updateColorsForBackground(backgroundBrightness);
		LOG_INF_S("Updated coordinate system colors for background brightness: " + std::to_string(backgroundBrightness));
	} else {
		LOG_WRN_S("Coordinate system renderer not available for color update");
	}
}

void SceneManager::rebuildScene()
{
	LOG_WRN_S("SceneManager::rebuildScene: Attempting to rebuild scene after rendering error");
	
	try {
		// Clear existing scene
		if (m_sceneRoot) {
			m_sceneRoot->removeAllChildren();
		}
		
		// Reinitialize scene components
		initializeScene();
		
		// Rebuild lighting
		initializeLightingFromConfig();
		
		// Rebuild coordinate system if it was visible
		if (m_coordSystemRenderer && m_coordSystemRenderer->isVisible()) {
			m_coordSystemRenderer->setVisible(false);
			m_coordSystemRenderer->setVisible(true);
		}
		
		// Rebuild checkerboard if it was visible
		if (m_checkerboardVisible) {
			setCheckerboardVisible(false);
			setCheckerboardVisible(true);
		}
		
		LOG_INF_S("SceneManager::rebuildScene: Scene rebuild completed successfully");
		
	} catch (const std::exception& e) {
		LOG_ERR_S("SceneManager::rebuildScene: Exception during rebuild: " + std::string(e.what()));
	} catch (...) {
		LOG_ERR_S("SceneManager::rebuildScene: Unknown exception during rebuild");
	}
}