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
#include "RenderingEngine.h"

// Optimized pass callback that avoids redundant background rendering
// Background is only drawn once at the beginning, preserving it across passes
static void renderPassCallback(void* userdata) {
	PassCallbackState* state = static_cast<PassCallbackState*>(userdata);
	if (state && state->sceneManager) {
		state->passCount++;

		// For multi-pass rendering, we only clear depth buffer between passes
		// Background was already rendered once in clearBuffers(), no need to redraw
		if (state->passCount > 1) {
			// Only clear depth buffer to separate layers, preserve background
			glClear(GL_DEPTH_BUFFER_BIT);
		}
	}
}

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
	, m_lastRenderTime(std::chrono::steady_clock::now())
	, m_isFirstRender(true)
	, m_forceCacheClearCounter(0)
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
		lightModel->ref(); // Reference for our management
		lightModel->model.setValue(SoLightModel::PHONG);
		if (m_sceneRoot) {
			// Insert at the beginning to ensure it precedes lights and objects
			m_sceneRoot->insertChild(lightModel, 0);
			lightModel->unref(); // Scene graph now holds the reference

			// Ensure at least one fallback light exists even if configuration is empty
			SoDirectionalLight* fallbackLight = new SoDirectionalLight;
			fallbackLight->ref(); // Reference for our management
			fallbackLight->direction.setValue(0.5f, 0.5f, -0.7f);
			fallbackLight->intensity.setValue(1.0f);
			int insertIndex = m_sceneRoot->getNumChildren();
			if (m_objectRoot) {
				int objectIndex = m_sceneRoot->findChild(m_objectRoot);
				if (objectIndex >= 0) insertIndex = objectIndex;
			}
			m_sceneRoot->insertChild(fallbackLight, insertIndex);
			fallbackLight->unref(); // Scene graph now holds the reference
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
	SbVec3f defaultPos(5.0f, -5.0f, 5.0f);
	SbVec3f viewDir(-defaultPos[0], -defaultPos[1], -defaultPos[2]);
	viewDir.normalize();
	SbVec3f defaultDir(0, 0, -1);
	SbRotation rotation(defaultDir, viewDir);

	applyCameraState(defaultPos, rotation, 8.66f, 8.66f);
	setupCameraForViewAll();
	performViewAll();

	CameraAnimation::CameraState targetState;
	if (shouldAnimate) {
		targetState = captureCameraState();
	}

	// Update scene bounds and clipping planes dynamically
	forceBoundsUpdate(); // Force bounds update when resetting view

	if (shouldAnimate) {
		// Restore original camera state before starting animation
		restoreCameraState(originalState);

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

	// Capture current camera state
	SbVec3f oldPosition = m_camera->position.getValue();
	SbRotation oldOrientation = m_camera->orientation.getValue();
	float oldFocalDistance = m_camera->focalDistance.getValue();
	float oldHeight = 0.0f;
	if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
		oldHeight = static_cast<SoOrthographicCamera*>(m_camera)->height.getValue();
	}

	// Remove old camera
	m_sceneRoot->removeChild(m_camera);
	m_camera->unref();

	// Create new camera of opposite type
	m_isPerspectiveCamera = !m_isPerspectiveCamera;
	if (m_isPerspectiveCamera) {
		m_camera = new SoPerspectiveCamera;
	}
	else {
		m_camera = new SoOrthographicCamera;
	}
	m_camera->ref();

	// Restore camera state to new camera
	applyCameraState(oldPosition, oldOrientation, oldFocalDistance, oldHeight);
	setupCameraForViewAll();

	// Add new camera to scene
	m_sceneRoot->insertChild(m_camera, 0);

	// Fit view to scene
	performViewAll();

	// Update clipping planes based on scene bounds
	forceBoundsUpdate();

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

	// Position camera for the desired direction
	positionCameraForDirection(direction, m_sceneBoundingBox);

	// Set camera orientation
	SbRotation rotation(SbVec3f(0, 0, -1), direction);
	m_camera->orientation.setValue(rotation);

	// Setup aspect ratio and perform view all
	setupCameraForViewAll();
	performViewAll();

	// Capture resulting state as animation target
	CameraAnimation::CameraState targetState = captureCameraState();

	// Restore original state prior to animation/direct application
	restoreCameraState(originalState);

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
		// Apply target state directly
		applyCameraState(targetState.position, targetState.rotation, targetState.focalDistance, targetState.height);
	}

	forceBoundsUpdate();

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

	// Set camera aspect ratio and update culling
	if (m_camera) {
		m_camera->aspectRatio.setValue(static_cast<float>(size.x) / static_cast<float>(size.y));
	}
	if (m_cullingEnabled && RenderingToolkitAPI::isInitialized()) {
		updateCulling();
	}

	// Create viewport region with validation
	SbViewportRegion viewport(size.x, size.y);
	if (viewport.getViewportSizePixels()[0] <= 0 || viewport.getViewportSizePixels()[1] <= 0) {
#ifdef _DEBUG
		LOG_ERR_S("SceneManager::render: Invalid viewport size: " +
			std::to_string(size.x) + "x" + std::to_string(size.y));
#endif
		return;
	}

	// CRITICAL: MUST check GL context BEFORE any GL calls
	// Check time since last render - long gaps may indicate context loss risk
	auto now = std::chrono::steady_clock::now();
	auto timeSinceLastRender = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastRenderTime);
	
	// If more than 60 seconds since last render, force cache clear as precaution
	// Windows may recycle GL resources after long idle periods
	const int LONG_IDLE_THRESHOLD_SECONDS = 60;
	bool longIdleDetected = !m_isFirstRender && (timeSinceLastRender.count() > LONG_IDLE_THRESHOLD_SECONDS);
	
	if (longIdleDetected) {
		LOG_WRN_S("SceneManager::render: Long idle period detected (" + 
			std::to_string(timeSinceLastRender.count()) + 
			" seconds). Forcing Coin3D cache invalidation to prevent stale GL resources.");
		invalidateCoin3DCache();
	}
	
	// Record this render time for next check
	recordRenderTime();

	// CRITICAL: Comprehensive GL context health check BEFORE any GL calls
	if (!validateGLContextHealth()) {
		LOG_ERR_S("SceneManager::render: GL context health check failed. Skipping render to prevent crash.");
		return;
	}

	// Setup OpenGL state - ONLY after context validation
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);
	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	glEnable(GL_TEXTURE_2D);
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Basic OpenGL capability check - verify GL context is valid
	const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	GLint maxTexSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
	if (glGetError() != GL_NO_ERROR) {
		LOG_ERR_S("GL context corrupted - glGetIntegerv failed");
	}
	
	if (!version) {
		LOG_ERR_S("SceneManager::render: Failed to get OpenGL version - no valid GL context. Skipping render.");
		// CRITICAL FIX: Return immediately. Proceeding with a null context causes Coin3D to assert/crash
		// Mark that GL resources need to be rebuilt
		static bool contextLossLogged = false;
		if (!contextLossLogged) {
			LOG_ERR_S("SceneManager::render: GL context appears to be lost. This may happen after long periods without rendering.");
			LOG_ERR_S("SceneManager::render: Coin3D display lists and GL resources may need to be rebuilt.");
			contextLossLogged = true;
		}
		return;
	}
	
	static std::string lastVersion;
	static uint32_t lastCacheContext = 0;
	
	// Detect GL context change (context recreation after loss)
	if (lastVersion.empty() || lastVersion != version) {
		if (!lastVersion.empty() && lastVersion != version) {
			LOG_WRN_S("SceneManager::render: GL context version changed from '" + lastVersion + "' to '" + std::string(version) + "'");
			LOG_WRN_S("SceneManager::render: This indicates GL context was recreated. Invalidating Coin3D cache.");
			
			// Force Coin3D to rebuild all display lists and GL resources
			// by changing the cache context ID
			lastCacheContext++;
		}
		lastVersion = version;
		LOG_INF_S("SceneManager::render: OpenGL version: " + std::string(version));
	}
	
	// Apply forced cache clear if requested (from external invalidation or long idle)
	if (m_forceCacheClearCounter > 0) {
		lastCacheContext += m_forceCacheClearCounter;
		LOG_INF_S("SceneManager::render: Applying " + std::to_string(m_forceCacheClearCounter) + 
			" forced cache invalidations. New cache version: " + std::to_string(lastCacheContext));
		m_forceCacheClearCounter = 0;
	}

	// Reset OpenGL errors before rendering (only log in debug builds)
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
#ifdef _DEBUG
		static int errorCount = 0;
		if (errorCount < 5) { // Limit error messages to prevent spam
			LOG_WRN_S("SceneManager::render: Pre-render OpenGL error: " + std::to_string(err));
			errorCount++;
		}
#endif
	}

	// Reset OpenGL state to prevent errors
	glDisable(GL_TEXTURE_2D);

	// Save current blend state to restore later
	bool blendEnabled = glIsEnabled(GL_BLEND);
	int blendSrc, blendDst;
	glGetIntegerv(GL_BLEND_SRC, &blendSrc);
	glGetIntegerv(GL_BLEND_DST, &blendDst);

	// Create pass callback state for rendering
	PassCallbackState passState(this);

	// Validate and repair geometries (extracted to separate method)
	validateAndRepairGeometries();

	// Process any deferred updates before rendering
	processDeferredUpdates();

	// Configure optimized multi-pass rendering with adaptive pass count
	SoGLRenderAction renderAction(viewport);
	try {
		renderAction.setSmoothing(true);

		// Dynamically determine optimal pass count based on scene content
		int optimalPasses = determineOptimalPassCount();
		renderAction.setNumPasses(optimalPasses);

		// Optimize transparency rendering based on pass count and scene complexity
		if (optimalPasses > 2 && hasTransparentObjects()) {
			// Use more sophisticated transparency sorting for complex scenes with transparency
			renderAction.setTransparencyType(SoGLRenderAction::SORTED_OBJECT_SORTED_TRIANGLE_BLEND);
		} else {
			// Use standard transparency method for simpler scenes
			renderAction.setTransparencyType(SoGLRenderAction::SORTED_OBJECT_BLEND);
		}

		renderAction.setPassCallback(renderPassCallback, &passState);
		
		// CRITICAL FIX: Use Canvas ID combined with context version for cache context
		// This ensures uniqueness between viewers AND forces cache rebuild after context loss
		uint32_t baseId = (m_canvas) ? static_cast<uint32_t>(m_canvas->GetId()) : 1;
		uint32_t cacheId = (baseId << 16) | (lastCacheContext & 0xFFFF);
		renderAction.setCacheContext(cacheId);
		
		// Log cache context change for debugging
		static uint32_t lastLoggedCacheId = 0;
		if (lastLoggedCacheId != cacheId) {
			LOG_INF_S("SceneManager::render: Using cache context ID: " + std::to_string(cacheId) + 
				" (baseId=" + std::to_string(baseId) + ", version=" + std::to_string(lastCacheContext) + ")");
			lastLoggedCacheId = cacheId;
		}

		// Check if scene root is valid before rendering
		if (!m_sceneRoot) {
#ifdef _DEBUG
			LOG_ERR_S("SceneManager::render: Scene root is null, skipping render");
#endif
			return;
		}

		// Additional validation: Check if scene root has valid children
		int numChildren = m_sceneRoot->getNumChildren();
		if (numChildren == 0) {
#ifdef _DEBUG
			LOG_WRN_S("SceneManager::render: Scene root has no children, this may indicate an empty scene");
#endif
		}
		
		// Log scene complexity for large scenes (helps diagnose resource exhaustion)
		static int lastLoggedChildCount = 0;
		if (numChildren > 100 && numChildren != lastLoggedChildCount) {
			LOG_INF_S("SceneManager::render: Rendering complex scene with " + std::to_string(numChildren) + 
				" root children. Cache ID: " + std::to_string(cacheId));
			lastLoggedChildCount = numChildren;
		}
		
		// CRITICAL: Final GL error check before Coin3D rendering
		// This catches any GL errors that might cause Coin3D to assert
		GLenum preRenderError = glGetError();
		if (preRenderError != GL_NO_ERROR) {
			LOG_ERR_S("SceneManager::render: GL error detected before renderAction.apply(): " + 
				std::to_string(preRenderError) + ". Attempting to clear and continue.");
			// Clear all errors
			while (glGetError() != GL_NO_ERROR) {}
		}
		
		// DEBUG: Log before Coin3D rendering for crash diagnosis
		LOG_INF_S("SceneManager::render: About to call renderAction.apply() on scene with " + 
			std::to_string(numChildren) + " root children, cacheId=" + std::to_string(cacheId));

		renderAction.apply(m_sceneRoot);
		
		LOG_INF_S("SceneManager::render: renderAction.apply() completed successfully");
		
		// Check for GL errors after rendering
		GLenum postRenderError = glGetError();
		if (postRenderError != GL_NO_ERROR) { 
			static int renderErrorCount = 0;
			if (renderErrorCount < 5) {
				LOG_WRN_S("SceneManager::render: GL error after renderAction.apply(): " + 
					std::to_string(postRenderError));
				renderErrorCount++;
			}
		} 
	} catch (const std::exception& e) {
		// Restore blend state before returning
		glBlendFunc(blendSrc, blendDst);
		if (!blendEnabled) {
			glDisable(GL_BLEND);
		}

		handleError(ErrorCategory::RENDERING, ErrorSeverity::HIGH,
			"Exception during Coin3D rendering", &e,
			[this]() { deferUpdate(UpdateType::FULL_REBUILD, [this]() { rebuildScene(); }, 10, "Full scene rebuild after rendering error"); });
		return;
	} catch (...) {
		// Restore blend state before returning
		glBlendFunc(blendSrc, blendDst);
		if (!blendEnabled) {
			glDisable(GL_BLEND);
		}

		handleError(ErrorCategory::RENDERING, ErrorSeverity::HIGH,
			"Unknown exception during Coin3D rendering", nullptr,
			[this]() { deferUpdate(UpdateType::FULL_REBUILD, [this]() { rebuildScene(); }, 10, "Full scene rebuild after unknown rendering error"); });
		return;
	}

	// Restore previous blend state
	glBlendFunc(blendSrc, blendDst);
	if (!blendEnabled) {
		glDisable(GL_BLEND);
	}

	// Check for OpenGL errors after rendering (only log in debug builds)
	while ((err = glGetError()) != GL_NO_ERROR) {
#ifdef _DEBUG
		static int postErrorCount = 0;
		if (postErrorCount < 5) { // Limit error messages to prevent spam
			LOG_ERR_S("Post-render: OpenGL error: " + std::to_string(err));
			postErrorCount++;
		}
#endif
	}

	// Publish to PerformanceDataBus instead of logging
	perf::ScenePerfSample s;
	s.width = size.x;
	s.height = size.y;
	s.mode = fastMode ? "FAST" : "QUALITY";
	s.viewportUs = 0;
	s.glSetupUs = 0;
	s.coinSceneMs = 0;
	auto sceneRenderEndTime = std::chrono::high_resolution_clock::now();
	auto sceneRenderDuration = std::chrono::duration_cast<std::chrono::milliseconds>(sceneRenderEndTime - sceneRenderStartTime);
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

// Optimized scene bounds calculation with caching and frame skipping
void SceneManager::updateSceneBounds() {
	// Always update if forced, or if no valid bounds exist, or periodically
	bool needsUpdate = m_forceBoundsUpdate ||
		m_sceneBoundingBox.isEmpty() ||
		(++m_boundsUpdateFrameSkip >= BOUNDS_UPDATE_INTERVAL);

	if (!needsUpdate) {
		return; // Skip bounds update this frame
	}

	// Reset update state
	m_forceBoundsUpdate = false;
	m_boundsUpdateFrameSkip = 0;

	// Check if we have valid objects to bound
	if (!m_objectRoot || m_objectRoot->getNumChildren() == 0) {
		m_sceneBoundingBox.makeEmpty();
		return;
	}

	// Perform expensive bounds calculation
	SbViewportRegion viewport(m_canvas->GetClientSize().x, m_canvas->GetClientSize().y);

	// Check if GL context is available before performing bbox calculation
	// Some Coin3D nodes may require GL context for bounding box computation
	const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	if (!glVersion) {
		LOG_WRN_S("SceneManager::updateSceneBounds: GL context not available, skipping bounds update");
		return;
	}

	SoGetBoundingBoxAction bboxAction(viewport);
	bboxAction.apply(m_objectRoot);
	SbBox3f newBounds = bboxAction.getBoundingBox();

	// Only update if bounds actually changed (to avoid unnecessary updates)
	if (newBounds != m_sceneBoundingBox) {
		m_sceneBoundingBox = newBounds;

		if (!m_sceneBoundingBox.isEmpty()) {
#ifdef _DEBUG
			static int boundsUpdateCount = 0;
			if (boundsUpdateCount < 5) { // Limit bounds update messages
				LOG_INF_S("Scene bounds updated.");
				boundsUpdateCount++;
			}
#endif
			// Update dependent systems only when bounds actually change
			if (m_coordSystemRenderer) {
				m_coordSystemRenderer->updateCoordinateSystemSize(getSceneBoundingBoxSize());
			}
			if (m_pickingAidManager) {
				m_pickingAidManager->updateReferenceGrid();
			}

			// Update camera clipping planes based on scene bounds
			updateCameraClippingPlanes();
		}
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
	// Use larger multiplier to ensure geometry is not clipped when zooming out
	float optimalFar = cameraDist + sceneRadius * 3.0f;  // Increased from 1.5f to 3.0f for better safety margin

	// Ensure minimum values
	optimalNear = std::max(0.001f, optimalNear);
	optimalFar = std::max(10000.0f, optimalFar);  // Increased minimum from 10.0f to 10000.0f

	// For very large scenes, don't limit the far plane
	// Only apply reasonable near plane limit
	optimalNear = std::min(optimalNear, sceneRadius * 0.1f);  // Near can't be more than 10% of scene size
	
	// No upper limit on far plane - let it adapt to scene size
	// This ensures geometry is never clipped, even when zooming out very far
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
	// Use deferred update to avoid immediate canvas refresh
	deferUpdate(UpdateType::CHECKERBOARD_UPDATE, [this, visible]() {
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
	}, 1, "Set checkerboard visibility to " + std::string(visible ? "visible" : "hidden"));
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

		// Check if display mode changed - always update lighting to ensure correct light model
		if (m_canvas && m_canvas->getOCCViewer()) {
			auto displaySettings = m_canvas->getOCCViewer()->getDisplaySettings();
			static RenderingConfig::DisplayMode lastDisplayMode = displaySettings.displayMode;

			if (lastDisplayMode != displaySettings.displayMode) {
				LOG_INF_S("RenderingConfig callback: Display mode changed from " + 
					std::to_string(static_cast<int>(lastDisplayMode)) + " to " + 
					std::to_string(static_cast<int>(displaySettings.displayMode)) + ", updating lighting");
				updateSceneLighting();
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
	// Use deferred update for lighting changes to avoid blocking the main thread
	deferUpdate(UpdateType::LIGHTING_UPDATE, [this]() {
		// Check if we're in NoShading mode - use different lighting setup
		bool isNoShading = false;
		if (m_canvas && m_canvas->getOCCViewer()) {
			auto displaySettings = m_canvas->getOCCViewer()->getDisplaySettings();
			isNoShading = (displaySettings.displayMode == RenderingConfig::DisplayMode::NoShading);
		}

		// Use unified lighting setup method
		setupLightingFromConfig(true, isNoShading);

		// Update all geometries to respond to lighting changes (only for updates)
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
	}, 2, "Update scene lighting configuration"); // Higher priority for lighting updates
}

void SceneManager::initializeLightingFromConfig() {
	// Use unified lighting setup method for initialization (no update-specific features)
	setupLightingFromConfig(false, false);

	LOG_INF_S("Lighting initialization from configuration completed");
}

// Camera state management utilities
void SceneManager::applyCameraState(const SbVec3f& position, const SbRotation& orientation, float focalDistance, float height) {
	if (!m_camera) return;

	m_camera->position.setValue(position);
	m_camera->orientation.setValue(orientation);
	m_camera->focalDistance.setValue(focalDistance);

	if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId()) && height > 0.0f) {
		static_cast<SoOrthographicCamera*>(m_camera)->height.setValue(height);
	}
}

void SceneManager::restoreCameraState(const CameraAnimation::CameraState& state) {
	if (!m_camera) return;

	m_camera->position.setValue(state.position);
	m_camera->orientation.setValue(state.rotation);

	if (m_camera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
		static_cast<SoPerspectiveCamera*>(m_camera)->focalDistance.setValue(state.focalDistance);
	} else if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
		SoOrthographicCamera* orthoCam = static_cast<SoOrthographicCamera*>(m_camera);
		orthoCam->focalDistance.setValue(state.focalDistance);
		orthoCam->height.setValue(state.height);
	}
}

void SceneManager::setupCameraForViewAll() {
	if (!m_camera || !m_canvas) return;

	wxSize size = m_canvas->GetClientSize();
	if (size.x > 0 && size.y > 0) {
		m_camera->aspectRatio.setValue(static_cast<float>(size.x) / static_cast<float>(size.y));
	}
}

void SceneManager::performViewAll() {
	if (!m_camera || !m_sceneRoot || !m_canvas) return;

	// Check if GL context is available before performing viewAll
	// Coin3D's viewAll method may require GL context for accurate bounds computation
	const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	if (!glVersion) {
		LOG_WRN_S("SceneManager::performViewAll: GL context not available, using fallback positioning");
		// Fallback: position camera at a default distance if no valid bounds
		SbVec3f defaultPos(5.0f, -5.0f, 5.0f);
		m_camera->position.setValue(defaultPos);
		m_camera->focalDistance.setValue(10.0f);
		if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
			static_cast<SoOrthographicCamera*>(m_camera)->height.setValue(10.0f);
		}
		return;
	}

	SbViewportRegion viewport(m_canvas->GetClientSize().x, m_canvas->GetClientSize().y);
	m_camera->viewAll(m_sceneRoot, viewport, 1.1f);
}

void SceneManager::positionCameraForDirection(const SbVec3f& direction, const SbBox3f& sceneBounds) {
	if (!m_camera) return;

	float defaultDistance = 10.0f;
	m_camera->position.setValue(direction * defaultDistance);
	m_camera->focalDistance.setValue(defaultDistance);

	if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
		static_cast<SoOrthographicCamera*>(m_camera)->height.setValue(defaultDistance);
	}

	// If we have valid scene bounds, position camera based on scene size
	if (!sceneBounds.isEmpty()) {
		SbVec3f center = sceneBounds.getCenter();
		float radius = (sceneBounds.getMax() - sceneBounds.getMin()).length() / 2.0f;
		if (radius < 2.0f) {
			radius = 2.0f;
		}
		m_camera->position.setValue(center + direction * (radius * 2.0f));
		m_camera->focalDistance.setValue(radius * 2.0f);
		if (m_camera->isOfType(SoOrthographicCamera::getClassTypeId())) {
			static_cast<SoOrthographicCamera*>(m_camera)->height.setValue(radius * 2.0f);
		}
	}
}

// Unified error handling implementation
void SceneManager::handleError(ErrorCategory category, ErrorSeverity severity, const std::string& message,
	const std::exception* e, std::function<void()> recoveryAction) {

	// Build error message
	std::string fullMessage = message;
	if (e) {
		fullMessage += ": " + std::string(e->what());
	}

	// Category-specific error counting and limits
	static std::map<ErrorCategory, int> errorCounts;
	static std::map<ErrorCategory, int> errorLimits = {
		{ErrorCategory::RENDERING, 3},
		{ErrorCategory::GEOMETRY, 5},
		{ErrorCategory::LIGHTING, 3},
		{ErrorCategory::GENERAL, 10}
	};

	// Check if we should log this error
	bool shouldLog = true;
	auto it = errorCounts.find(category);
	if (it != errorCounts.end() && it->second >= errorLimits[category]) {
		shouldLog = false;
	}

	// Log error if appropriate
	if (shouldLog) {
		switch (severity) {
		case ErrorSeverity::LOW:
#ifdef _DEBUG
			LOG_WRN_S("[LOW] " + fullMessage);
#endif
			break;
		case ErrorSeverity::MEDIUM:
			LOG_WRN_S("[MEDIUM] " + fullMessage);
			break;
		case ErrorSeverity::HIGH:
			LOG_ERR_S("[HIGH] " + fullMessage);
			break;
		case ErrorSeverity::CRITICAL:
			LOG_ERR_S("[CRITICAL] " + fullMessage);
			break;
		}
		errorCounts[category]++;
	}

	// Execute recovery action if provided
	if (recoveryAction) {
		try {
			recoveryAction();
		} catch (const std::exception& recoveryException) {
#ifdef _DEBUG
			LOG_ERR_S("Error during recovery action: " + std::string(recoveryException.what()));
#endif
		}
	}
}

// Multi-pass rendering optimization methods
int SceneManager::determineOptimalPassCount() {
	// Base pass count for anti-aliasing
	int passCount = 2;

	// Increase passes for scenes with transparent objects
	if (hasTransparentObjects()) {
		passCount = 3; // Additional pass for better transparency sorting
	}

	// Cap at reasonable maximum for performance
	return std::min(passCount, 4);
}

bool SceneManager::hasTransparentObjects() const {
	// TODO: Implement proper transparency detection
	// For now, assume no transparent objects to keep simple
	// This can be enhanced later by checking material properties
	// or geometry appearance settings
	return false;
}

// Deferred update system implementation
void SceneManager::deferUpdate(UpdateType type, std::function<void()> action, int priority, const std::string& description) {
	// Check if we already have a similar update pending
	for (auto& update : m_deferredUpdates) {
		if (update.type == type) {
			// Replace with higher priority update if this one has higher priority
			if (priority > update.priority) {
				update.action = action;
				update.priority = priority;
				update.description = description;
			}
			return;
		}
	}

	// Add new deferred update
	m_deferredUpdates.push_back({type, action, priority, description});
#ifdef _DEBUG
	LOG_INF_S("Deferred update queued: " + description + " (priority: " + std::to_string(priority) + ")");
#endif
}

void SceneManager::processDeferredUpdates() {
	if (m_deferredUpdates.empty()) {
		return;
	}

#ifdef _DEBUG
	LOG_INF_S("Processing " + std::to_string(m_deferredUpdates.size()) + " deferred updates");
#endif

	// Sort by priority (higher priority first)
	std::sort(m_deferredUpdates.begin(), m_deferredUpdates.end(),
		[](const DeferredUpdate& a, const DeferredUpdate& b) {
			return a.priority > b.priority;
		});

	// Execute updates, but limit to prevent frame drops
	const size_t maxUpdatesPerFrame = 5;
	size_t processed = 0;

	for (auto it = m_deferredUpdates.begin(); it != m_deferredUpdates.end() && processed < maxUpdatesPerFrame; ) {
		try {
			it->action();
#ifdef _DEBUG
			LOG_INF_S("Executed deferred update: " + it->description);
#endif
			it = m_deferredUpdates.erase(it);
			processed++;
		} catch (const std::exception& e) {
			handleError(ErrorCategory::GENERAL, ErrorSeverity::HIGH,
				"Exception during deferred update: " + it->description, &e);
			it = m_deferredUpdates.erase(it);
			processed++;
		}
	}

	// If we still have updates remaining, they'll be processed in the next frame
	if (!m_deferredUpdates.empty()) {
#ifdef _DEBUG
		LOG_INF_S(std::to_string(m_deferredUpdates.size()) + " deferred updates remaining for next frame");
#endif
	}
}

bool SceneManager::hasDeferredUpdates() const {
	return !m_deferredUpdates.empty();
}

void SceneManager::clearDeferredUpdates() {
	if (!m_deferredUpdates.empty()) {
		LOG_WRN_S("Clearing " + std::to_string(m_deferredUpdates.size()) + " pending deferred updates");
		m_deferredUpdates.clear();
	}
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
	// Use deferred update to batch visibility changes and reduce refresh calls
	deferUpdate(UpdateType::COORDINATE_SYSTEM_UPDATE, [this, visible]() {
		if (m_coordSystemRenderer) {
			m_coordSystemRenderer->setVisible(visible);

			// Single refresh call instead of multiple methods
			if (m_sceneRoot) {
				m_sceneRoot->touch();
			}
			if (m_canvas) {
				m_canvas->Refresh(true);
			}
			if (m_canvas && m_canvas->getRefreshManager()) {
				m_canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::GEOMETRY_CHANGED, true);
			}

			LOG_INF_S("Set coordinate system visibility: " + std::string(visible ? "visible" : "hidden"));
		}
		else {
			LOG_WRN_S("Coordinate system renderer not available");
		}
	}, 1, "Set coordinate system visibility to " + std::string(visible ? "visible" : "hidden"));
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
		handleError(ErrorCategory::GENERAL, ErrorSeverity::HIGH,
			"Exception during scene rebuild", &e);
	} catch (...) {
		handleError(ErrorCategory::GENERAL, ErrorSeverity::HIGH,
			"Unknown exception during scene rebuild");
	}
}

// Geometry validation optimization methods
void SceneManager::markGeometryDirty() {
	m_forceGeometryValidation = true;
}

void SceneManager::invalidateGeometryCache() {
	m_lastGeometryCount = 0; // Reset cache to force recount
	m_forceGeometryValidation = true;
}

// Scene bounds optimization methods
void SceneManager::markBoundsDirty() {
	m_forceBoundsUpdate = true;
}

void SceneManager::forceBoundsUpdate() {
	m_forceBoundsUpdate = true;
	updateSceneBounds();
}

// Optimized geometry validation with caching and frame skipping
void SceneManager::validateAndRepairGeometries() {
	if (!m_canvas || !m_canvas->getOCCViewer()) {
		return;
	}

	// Check if validation is needed
	auto geometries = m_canvas->getOCCViewer()->getAllGeometry();
	size_t currentGeometryCount = geometries.size();

	// Always validate if forced, or if geometry count changed, or periodically
	bool needsValidation = m_forceGeometryValidation ||
		(currentGeometryCount != m_lastGeometryCount) ||
		(++m_geometryValidationFrameSkip >= GEOMETRY_VALIDATION_INTERVAL);

	if (!needsValidation) {
		return; // Skip validation this frame
	}

	// Reset validation state
	m_forceGeometryValidation = false;
	m_lastGeometryCount = currentGeometryCount;
	m_geometryValidationFrameSkip = 0;

	// Perform actual validation
	int validGeometries = 0;
	int repairedGeometries = 0;

	for (const auto& geometry : geometries) {
		if (geometry) {
			auto coinNode = geometry->getCoinNode();
			if (coinNode) {
				validGeometries++;
			} else {
#ifdef _DEBUG
				static int rebuildWarningCount = 0;
				if (rebuildWarningCount < 3) { // Limit rebuild warnings
					LOG_WRN_S("SceneManager::render: Geometry '" + geometry->getName() + "' has null Coin3D node, rebuilding...");
					rebuildWarningCount++;
				}
#endif
				try {
					// Force rebuild the Coin3D representation
					MeshParameters defaultParams;
					geometry->forceCoinRepresentationRebuild(defaultParams);
					repairedGeometries++;
#ifdef _DEBUG
					static int rebuildSuccessCount = 0;
					if (rebuildSuccessCount < 3) { // Limit success messages
						LOG_INF_S("SceneManager::render: Successfully rebuilt Coin3D node for geometry '" + geometry->getName() + "'");
						rebuildSuccessCount++;
					}
#endif
				} catch (const std::exception& e) {
					handleError(ErrorCategory::GEOMETRY, ErrorSeverity::MEDIUM,
						"Failed to rebuild geometry '" + geometry->getName() + "'", &e);
				} catch (...) {
					handleError(ErrorCategory::GEOMETRY, ErrorSeverity::MEDIUM,
						"Failed to rebuild geometry '" + geometry->getName() + "' (unknown exception)");
				}
			}
		}
	}

	if (repairedGeometries > 0) {
#ifdef _DEBUG
		static int repairStatsCount = 0;
		if (repairStatsCount < 5) { // Limit repair statistics messages
			LOG_INF_S("SceneManager::render: Repaired " + std::to_string(repairedGeometries) +
				" geometries with invalid Coin3D nodes");
			repairStatsCount++;
		}
#endif
	}
}

// Unified lighting configuration method
void SceneManager::setupLightingFromConfig(bool isUpdate, bool isNoShading) {
	if (!m_sceneRoot) {
		LOG_ERR_S("Cannot setup lighting: Scene root not available");
		return;
	}

	if (isUpdate) {
		LOG_INF_S("Updating lighting from configuration");
	} else {
		LOG_INF_S("Initializing lighting from configuration");
	}

	// Clear existing lighting nodes
	int objectIndex = m_objectRoot ? m_sceneRoot->findChild(m_objectRoot) : -1;
	for (int i = m_sceneRoot->getNumChildren() - 1; i >= 0; --i) {
		if (objectIndex >= 0 && i >= objectIndex) continue; // do not remove anything after objects
		SoNode* child = m_sceneRoot->getChild(i);
		if (child->isOfType(SoLightModel::getClassTypeId()) ||
			child->isOfType(SoEnvironment::getClassTypeId()) ||
			child->isOfType(SoDirectionalLight::getClassTypeId()) ||
			child->isOfType(SoPointLight::getClassTypeId()) ||
			child->isOfType(SoSpotLight::getClassTypeId())) {
			m_sceneRoot->removeChild(i);
		}
	}

	// Get lighting configuration
	LightingConfig& config = LightingConfig::getInstance();

	// Set light model based on display mode (only for updates)
	if (isUpdate) {
		SoLightModel* lightModel = new SoLightModel;
		lightModel->ref(); // Reference for our management
		if (isNoShading) {
			// Use BASE_COLOR for no shading - no lighting calculations, direct color
			lightModel->model.setValue(SoLightModel::BASE_COLOR);
			LOG_INF_S("SceneManager::setupLightingFromConfig: Using BASE_COLOR light model for NoShading");
		} else {
			// Use PHONG for normal lighting
			lightModel->model.setValue(SoLightModel::PHONG);
		}

		// Insert light model at the beginning of scene root (after camera)
		m_sceneRoot->insertChild(lightModel, 1);
		lightModel->unref(); // Scene graph now holds the reference
	}

	// Add environment settings
	auto envSettings = config.getEnvironmentSettings();
	SoEnvironment* environment = new SoEnvironment;
	environment->ref(); // Reference for our management

	// Convert Quantity_Color to SbColor
	Standard_Real r = 0.9, g = 0.9, b = 0.9; // Default neutral values
	if (isUpdate && isNoShading) {
		// For NoShading mode, use neutral ambient color with moderate intensity
		environment->ambientColor.setValue(0.9f, 0.9f, 0.9f); // Light gray ambient
		environment->ambientIntensity.setValue(0.5f); // Moderate ambient intensity
		LOG_INF_S("SceneManager::setupLightingFromConfig: Using neutral ambient lighting for NoShading");
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
		environment->unref(); // Scene graph now holds the reference
	}

	// Use appropriate intensity value for logging
	float logIntensity = (isUpdate && isNoShading) ? 0.5f : static_cast<float>(envSettings.ambientIntensity);
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
				light->ref(); // Reference for our management

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
					light->unref(); // Scene graph now holds the reference
				}

				// Store reference to main light for compatibility
				if (lightSettings.name == "Main Light") {
					if (isUpdate && m_light) { m_light->unref(); }
					m_light = light;
					m_light->ref(); // Keep our own reference for main light
				}

				LOG_INF_S("Added directional light: " + lightSettings.name);
			}
			else if (lightSettings.type == "point") {
				SoPointLight* light = new SoPointLight;
				light->ref(); // Reference for our management

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
					light->unref(); // Scene graph now holds the reference
				}

				LOG_INF_S("Added point light: " + lightSettings.name);
			}
			else if (lightSettings.type == "spot") {
				SoSpotLight* light = new SoSpotLight;
				light->ref(); // Reference for our management

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
					light->unref(); // Scene graph now holds the reference
				}

				LOG_INF_S("Added spot light: " + lightSettings.name);
			}
		}
		catch (const std::exception& e) {
			handleError(ErrorCategory::LIGHTING, ErrorSeverity::HIGH,
				"Exception while creating light " + lightSettings.name, &e);
		}
		catch (...) {
			handleError(ErrorCategory::LIGHTING, ErrorSeverity::HIGH,
				"Unknown exception while creating light " + lightSettings.name);
		}
	}

	// Ensure we have a main light reference for compatibility
	if (!m_light) {
		// Create a default main light if none exists
		m_light = new SoDirectionalLight;
		m_light->ref(); // Our reference
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
			// Don't unref here - we keep our own reference for main light
		}
		if (isUpdate) {
			LOG_INF_S("Created default main light for compatibility");
		}
	}

	// Force scene update
	if (m_sceneRoot) {
		m_sceneRoot->touch();
		if (isUpdate) {
			LOG_INF_S("Touched scene root to force lighting update");
		}
	}
}

void SceneManager::invalidateCoin3DCache() {
	LOG_WRN_S("SceneManager::invalidateCoin3DCache: Forcing Coin3D cache invalidation");
	m_forceCacheClearCounter++;
}

bool SceneManager::validateGLContextHealth() {
	// Deep validation of GL context health with detailed logging
	const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
	const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	
	if (!vendor || !renderer || !version) {
		LOG_ERR_S("SceneManager::validateGLContextHealth: GL context is invalid - glGetString returned NULL");
		LOG_ERR_S("  vendor=" + std::string(vendor ? vendor : "NULL") + 
			", renderer=" + std::string(renderer ? renderer : "NULL") + 
			", version=" + std::string(version ? version : "NULL"));
		return false;
	}
	
	// Test if we can actually perform GL operations
	GLint maxTexSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
	GLenum err = glGetError();
	if (err != GL_NO_ERROR || maxTexSize == 0) {
		LOG_ERR_S("SceneManager::validateGLContextHealth: GL context appears corrupted - cannot query GL state");
		LOG_ERR_S("  glGetError=" + std::to_string(err) + ", maxTexSize=" + std::to_string(maxTexSize));
		return false;
	}
	
	// Log resource limits periodically (every 100 validations)
	static int validationCount = 0;
	if (++validationCount % 100 == 1) {
		LOG_INF_S("SceneManager GL Resource Limits: maxTexSize=" + std::to_string(maxTexSize));
	}
	
	return true;
}

void SceneManager::recordRenderTime() {
	m_lastRenderTime = std::chrono::steady_clock::now();
	m_isFirstRender = false;
}
