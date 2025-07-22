#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "SceneManager.h"
#include "Canvas.h"
#include "config/RenderingConfig.h"
#include "config/LightingConfig.h"
#include "CoordinateSystemRenderer.h"
#include "PickingAidManager.h"
#include "ViewRefreshManager.h"
#include "OCCViewer.h"
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
{
    LOG_INF_S("SceneManager initializing");
    
    // Initialize rendering toolkit with culling
    // m_renderingToolkit = std::make_unique<RenderingToolkitAPI>(); // Removed as per edit hint
    // m_renderingToolkit->setFrustumCullingEnabled(true); // Removed as per edit hint
    // m_renderingToolkit->setOcclusionCullingEnabled(true); // Removed as per edit hint
}

SceneManager::~SceneManager() {
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

        // Set a light model to enable separate two-sided lighting
        SoLightModel* lightModel = new SoLightModel;
        lightModel->model.setValue(SoLightModel::PHONG);
        m_lightRoot->addChild(lightModel);

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

void SceneManager::resetView() {
    if (!m_camera || !m_sceneRoot) {
        LOG_ERR_S("Failed to reset view: Invalid camera or scene");
        return;
    }

    m_camera->position.setValue(5.0f, -5.0f, 5.0f);
    SbVec3f r_position = m_camera->position.getValue();
    SbVec3f r_viewDir(-r_position[0], -r_position[1], -r_position[2]);
    r_viewDir.normalize();
    SbVec3f r_defaultDir(0, 0, -1);
    SbRotation r_rotation(r_defaultDir, r_viewDir);
    m_camera->orientation.setValue(r_rotation);
    m_camera->focalDistance.setValue(8.66f);

    SbViewportRegion viewport(m_canvas->GetClientSize().x, m_canvas->GetClientSize().y);
    
    m_camera->viewAll(m_sceneRoot, viewport, 1.1f);

    m_camera->nearDistance.setValue(0.001f);
    m_camera->farDistance.setValue(10000.0f);

    updateSceneBounds(); // Update bounds after view reset

    if (m_canvas->getRefreshManager()) {
        m_canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::CAMERA_MOVED, true);
    } else {
        m_canvas->Refresh(true);
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

    m_camera->nearDistance.setValue(0.001f);
    m_camera->farDistance.setValue(10000.0f);

    if (m_canvas->getRefreshManager()) {
        m_canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::CAMERA_MOVED, true);
    } else {
        m_canvas->Refresh(true);
    }
    LOG_INF_S(m_isPerspectiveCamera ? "Switched to Perspective Camera" : "Switched to Orthographic Camera");
}

void SceneManager::setView(const std::string& viewName) {
    if (!m_camera || !m_sceneRoot) {
        LOG_ERR_S("Failed to set view: Invalid camera or scene");
        return;
    }

    // Define view directions (direction, up vector)
    std::map<std::string, std::pair<SbVec3f, SbVec3f>> viewDirections = {
        { "Top", { SbVec3f(0, 0, -1), SbVec3f(0, 1, 0) } },
        { "Bottom", { SbVec3f(0, 0, 1), SbVec3f(0, 1, 0) } },
        { "Front", { SbVec3f(0, -1, 0), SbVec3f(0, 0, 1) } },
        { "Back", { SbVec3f(0, 1, 0), SbVec3f(0, 0, 1) } },
        { "Left", { SbVec3f(-1, 0, 0), SbVec3f(0, 0, 1) } },
        { "Right", { SbVec3f(1, 0, 0), SbVec3f(0, 0, 1) } },
        { "Isometric", { SbVec3f(1, 1, 1), SbVec3f(0, 0, 1) } }
    };

    auto it = viewDirections.find(viewName);
    if (it == viewDirections.end()) {
        LOG_WRN_S("Invalid view name: " + viewName);
        return;
    }

    const auto& [direction, up] = it->second;

    // Set camera orientation
    SbRotation rotation(SbVec3f(0, 0, -1), direction);
    m_camera->orientation.setValue(rotation);

    // Always ensure we have a reasonable default position regardless of scene content
    float defaultDistance = 10.0f;
    m_camera->position.setValue(direction * defaultDistance);
    m_camera->focalDistance.setValue(defaultDistance);

    // Get scene bounding box from the entire scene root, not just object root
    SoGetBoundingBoxAction bboxAction(SbViewportRegion(m_canvas->GetClientSize().x, m_canvas->GetClientSize().y));
    bboxAction.apply(m_sceneRoot);
    SbBox3f bbox = bboxAction.getBoundingBox();

    if (!bbox.isEmpty()) {
        SbVec3f center = bbox.getCenter();
        float radius = (bbox.getMax() - bbox.getMin()).length() / 2.0f;
        
        // Ensure minimum radius for consistency
        if (radius < 2.0f) radius = 2.0f;
        
        m_camera->position.setValue(center + direction * (radius * 2.0f));
        m_camera->focalDistance.setValue(radius * 2.0f);
    }

    // View the entire scene root, not just object root
    SbViewportRegion viewport(m_canvas->GetClientSize().x, m_canvas->GetClientSize().y);
    m_camera->viewAll(m_sceneRoot, viewport, 1.1f); // 1.1 factor to add some margin

    // Ensure reasonable near/far planes
    m_camera->nearDistance.setValue(0.001f);
    m_camera->farDistance.setValue(10000.0f);

    if (m_pickingAidManager) {
        m_pickingAidManager->showReferenceGrid(true);
    }

    LOG_INF_S("Switched to view: " + viewName);
    
    if (m_canvas->getRefreshManager()) {
        m_canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::CAMERA_MOVED, true);
    } else {
        m_canvas->Refresh(true);
    }
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
    SbViewportRegion viewport(size.x, size.y);
    SoGLRenderAction renderAction(viewport);
    renderAction.setSmoothing(!fastMode);
    renderAction.setNumPasses(fastMode ? 1 : 2);
    renderAction.setTransparencyType(
        fastMode ? SoGLRenderAction::BLEND : SoGLRenderAction::SORTED_OBJECT_BLEND
    );
    auto viewportEndTime = std::chrono::high_resolution_clock::now();
    auto viewportDuration = std::chrono::duration_cast<std::chrono::microseconds>(viewportEndTime - viewportStartTime);

    // Explicitly enable blending for line smoothing
    auto glSetupStartTime = std::chrono::high_resolution_clock::now();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reset OpenGL errors before rendering
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOG_ERR_S("Pre-render: OpenGL error: " + std::to_string(err));
    }
    
    // Reset OpenGL state to prevent errors
    glDisable(GL_TEXTURE_2D);
    auto glSetupEndTime = std::chrono::high_resolution_clock::now();
    auto glSetupDuration = std::chrono::duration_cast<std::chrono::microseconds>(glSetupEndTime - glSetupStartTime);

    // Render the scene
    auto coinRenderStartTime = std::chrono::high_resolution_clock::now();
    renderAction.apply(m_sceneRoot);
    auto coinRenderEndTime = std::chrono::high_resolution_clock::now();
    auto coinRenderDuration = std::chrono::duration_cast<std::chrono::milliseconds>(coinRenderEndTime - coinRenderStartTime);
    
    // Check for OpenGL errors after rendering
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOG_ERR_S("Post-render: OpenGL error: " + std::to_string(err));
    }
    
    auto sceneRenderEndTime = std::chrono::high_resolution_clock::now();
    auto sceneRenderDuration = std::chrono::duration_cast<std::chrono::milliseconds>(sceneRenderEndTime - sceneRenderStartTime);
    
    // Only log if render time is significant
    if (sceneRenderDuration.count() > 16) {
        LOG_INF_S("=== SCENE RENDER PERFORMANCE ===");
        LOG_INF_S("Scene size: " + std::to_string(size.x) + "x" + std::to_string(size.y));
        LOG_INF_S("Render mode: " + std::string(fastMode ? "FAST" : "QUALITY"));
        LOG_INF_S("Viewport setup: " + std::to_string(viewportDuration.count()) + "μs");
        LOG_INF_S("GL state setup: " + std::to_string(glSetupDuration.count()) + "μs");
        LOG_INF_S("Coin3D scene render: " + std::to_string(coinRenderDuration.count()) + "ms");
        LOG_INF_S("Total scene render: " + std::to_string(sceneRenderDuration.count()) + "ms");
        LOG_INF_S("Scene render FPS: " + std::to_string(1000.0 / sceneRenderDuration.count()));
        LOG_INF_S("=================================");
    }
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
        LOG_INF_S("Successfully picked 3D point from scene geometry");
        return true;
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
    }
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

void SceneManager::getSceneBoundingBoxMinMax(SbVec3f& min, SbVec3f& max) const {
    if (m_sceneBoundingBox.isEmpty()) {
        min = SbVec3f(-10.0f, -10.0f, 0.0f);
        max = SbVec3f(10.0f, 10.0f, 0.0f);
    } else {
        m_sceneBoundingBox.getBounds(min, max);
    }
}

void SceneManager::initializeRenderingConfigCallback()
{
    // Register callback to update geometries when RenderingConfig changes
    RenderingConfig& config = RenderingConfig::getInstance();
    config.registerSettingsChangedCallback([this]() {
        LOG_INF_S("RenderingConfig callback triggered - updating geometries");
        
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
            } else {
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
        } else {
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
    if (!m_lightRoot) {
        LOG_ERR_S("Cannot update lighting: Light root not available");
        return;
    }
    
    LightingConfig& config = LightingConfig::getInstance();
    
    // Update environment settings first
    auto envSettings = config.getEnvironmentSettings();
    for (int i = 0; i < m_lightRoot->getNumChildren(); ++i) {
        SoNode* child = m_lightRoot->getChild(i);
        if (child->isOfType(SoEnvironment::getClassTypeId())) {
            SoEnvironment* env = static_cast<SoEnvironment*>(child);
            
            // Convert Quantity_Color to SbColor
            Standard_Real r, g, b;
            envSettings.ambientColor.Values(r, g, b, Quantity_TOC_RGB);
            env->ambientColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
            env->ambientIntensity.setValue(static_cast<float>(envSettings.ambientIntensity));
            
            LOG_INF_S("Updated environment lighting - ambient color: " + 
                     std::to_string(r) + "," + std::to_string(g) + "," + std::to_string(b) + 
                     ", intensity: " + std::to_string(envSettings.ambientIntensity));
            break;
        }
    }
    
    // Get lights from configuration
    auto lights = config.getAllLights();
    LOG_INF_S("Processing " + std::to_string(lights.size()) + " lights from configuration");
    
    // Find existing lights and update them, or create new ones if needed
    std::vector<bool> lightProcessed(lights.size(), false);
    
    // First pass: try to update existing lights
    for (int i = 0; i < m_lightRoot->getNumChildren(); ++i) {
        SoNode* child = m_lightRoot->getChild(i);
        
        // Skip non-light nodes
        if (!child->isOfType(SoDirectionalLight::getClassTypeId()) &&
            !child->isOfType(SoPointLight::getClassTypeId()) &&
            !child->isOfType(SoSpotLight::getClassTypeId())) {
            continue;
        }
        
        // Try to match with a light from configuration
        for (size_t lightIndex = 0; lightIndex < lights.size(); ++lightIndex) {
            if (lightProcessed[lightIndex]) continue;
            
            const auto& lightSettings = lights[lightIndex];
            if (!lightSettings.enabled) continue;
            
            bool matched = false;
            
            if (lightSettings.type == "directional" && child->isOfType(SoDirectionalLight::getClassTypeId())) {
                SoDirectionalLight* light = static_cast<SoDirectionalLight*>(child);
                
                // Update light properties
                light->direction.setValue(static_cast<float>(lightSettings.directionX),
                                        static_cast<float>(lightSettings.directionY),
                                        static_cast<float>(lightSettings.directionZ));
                
                Standard_Real r, g, b;
                lightSettings.color.Values(r, g, b, Quantity_TOC_RGB);
                light->color.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
                light->intensity.setValue(static_cast<float>(lightSettings.intensity));
                light->on.setValue(true);
                
                matched = true;
                LOG_INF_S("Updated existing directional light: " + lightSettings.name);
                
            } else if (lightSettings.type == "point" && child->isOfType(SoPointLight::getClassTypeId())) {
                SoPointLight* light = static_cast<SoPointLight*>(child);
                
                // Update light properties
                light->location.setValue(static_cast<float>(lightSettings.positionX),
                                       static_cast<float>(lightSettings.positionY),
                                       static_cast<float>(lightSettings.positionZ));
                
                Standard_Real r, g, b;
                lightSettings.color.Values(r, g, b, Quantity_TOC_RGB);
                light->color.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
                light->intensity.setValue(static_cast<float>(lightSettings.intensity));
                light->on.setValue(true);
                
                matched = true;
                LOG_INF_S("Updated existing point light: " + lightSettings.name);
                
            } else if (lightSettings.type == "spot" && child->isOfType(SoSpotLight::getClassTypeId())) {
                SoSpotLight* light = static_cast<SoSpotLight*>(child);
                
                // Update light properties
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
                
                matched = true;
                LOG_INF_S("Updated existing spot light: " + lightSettings.name);
            }
            
            if (matched) {
                lightProcessed[lightIndex] = true;
                break;
            }
        }
    }
    
    // Second pass: create new lights for unprocessed configurations
    for (size_t lightIndex = 0; lightIndex < lights.size(); ++lightIndex) {
        if (lightProcessed[lightIndex]) continue;
        
        const auto& lightSettings = lights[lightIndex];
        if (!lightSettings.enabled) continue;
        
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
                
                m_lightRoot->addChild(light);
                
                LOG_INF_S("Created new directional light: " + lightSettings.name);
                
            } else if (lightSettings.type == "point") {
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
                
                m_lightRoot->addChild(light);
                
                LOG_INF_S("Created new point light: " + lightSettings.name);
                
            } else if (lightSettings.type == "spot") {
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
                
                m_lightRoot->addChild(light);
                
                LOG_INF_S("Created new spot light: " + lightSettings.name);
            }
        } catch (const std::exception& e) {
            LOG_ERR_S("Exception while creating light " + lightSettings.name + ": " + std::string(e.what()));
        } catch (...) {
            LOG_ERR_S("Unknown exception while creating light " + lightSettings.name);
        }
    }
    
    // Disable lights that are no longer in configuration
    for (int i = 0; i < m_lightRoot->getNumChildren(); ++i) {
        SoNode* child = m_lightRoot->getChild(i);
        
        if (child->isOfType(SoDirectionalLight::getClassTypeId())) {
            SoDirectionalLight* light = static_cast<SoDirectionalLight*>(child);
            bool found = false;
            
            for (const auto& lightSettings : lights) {
                if (lightSettings.type == "directional" && lightSettings.enabled) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                light->on.setValue(false);
                LOG_INF_S("Disabled unused directional light");
            }
            
        } else if (child->isOfType(SoPointLight::getClassTypeId())) {
            SoPointLight* light = static_cast<SoPointLight*>(child);
            bool found = false;
            
            for (const auto& lightSettings : lights) {
                if (lightSettings.type == "point" && lightSettings.enabled) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                light->on.setValue(false);
                LOG_INF_S("Disabled unused point light");
            }
            
        } else if (child->isOfType(SoSpotLight::getClassTypeId())) {
            SoSpotLight* light = static_cast<SoSpotLight*>(child);
            bool found = false;
            
            for (const auto& lightSettings : lights) {
                if (lightSettings.type == "spot" && lightSettings.enabled) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                light->on.setValue(false);
                LOG_INF_S("Disabled unused spot light");
            }
        }
    }
    
    // Force scene update
    if (m_sceneRoot) {
        m_sceneRoot->touch();
        LOG_INF_S("Touched scene root to force lighting update");
    }
    
    // Force update all geometries to apply new lighting
    if (m_canvas && m_canvas->getOCCViewer()) {
        OCCViewer* viewer = m_canvas->getOCCViewer();
        auto allGeometries = viewer->getAllGeometry();
        LOG_INF_S("Forcing update of " + std::to_string(allGeometries.size()) + " geometries for lighting changes");
        
        for (auto& geometry : allGeometries) {
            if (geometry) {
                // Force rebuild by updating material properties
                geometry->setMaterialAmbientColor(geometry->getMaterialAmbientColor());
                geometry->setMaterialDiffuseColor(geometry->getMaterialDiffuseColor());
                geometry->setMaterialSpecularColor(geometry->getMaterialSpecularColor());
                LOG_INF_S("Forced material update for geometry: " + geometry->getName());
            }
        }
    }
    
    // Request refresh
    if (m_canvas) {
        if (m_canvas->getRefreshManager()) {
            m_canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::LIGHTING_CHANGED, true);
        } else {
            m_canvas->Refresh(true);
        }
        LOG_INF_S("Requested scene refresh for lighting changes");
    }
    
    LOG_INF_S("Scene lighting updated successfully");
}

void SceneManager::initializeLightingFromConfig() {
    if (!m_lightRoot) {
        LOG_ERR_S("Cannot initialize lighting: Light root not available");
        return;
    }
    
    LOG_INF_S("Initializing lighting from configuration");
    
    // Clear existing lights (except light model)
    for (int i = m_lightRoot->getNumChildren() - 1; i >= 0; --i) {
        SoNode* child = m_lightRoot->getChild(i);
        if (!child->isOfType(SoLightModel::getClassTypeId())) {
            m_lightRoot->removeChild(i);
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
    
    m_lightRoot->addChild(environment);
    
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
                
                m_lightRoot->addChild(light);
                
                // Store reference to main light for compatibility
                if (lightSettings.name == "Main Light") {
                    m_light = light;
                    m_light->ref();
                }
                
                LOG_INF_S("Added directional light: " + lightSettings.name);
                
            } else if (lightSettings.type == "point") {
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
                
                m_lightRoot->addChild(light);
                
                LOG_INF_S("Added point light: " + lightSettings.name);
                
            } else if (lightSettings.type == "spot") {
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
                
                m_lightRoot->addChild(light);
                
                LOG_INF_S("Added spot light: " + lightSettings.name);
            }
        } catch (const std::exception& e) {
            LOG_ERR_S("Exception while creating light " + lightSettings.name + ": " + std::string(e.what()));
        } catch (...) {
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
        m_lightRoot->addChild(m_light);
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
        } else if (child->isOfType(SoEnvironment::getClassTypeId())) {
            SoEnvironment* env = static_cast<SoEnvironment*>(child);
            SbColor color = env->ambientColor.getValue();
            float intensity = env->ambientIntensity.getValue();
            LOG_INF_S("Child " + std::to_string(i) + ": SoEnvironment (ambient color: " + 
                     std::to_string(color[0]) + "," + std::to_string(color[1]) + "," + std::to_string(color[2]) + 
                     ", intensity: " + std::to_string(intensity) + ")");
        } else if (child->isOfType(SoDirectionalLight::getClassTypeId())) {
            SoDirectionalLight* light = static_cast<SoDirectionalLight*>(child);
            SbVec3f dir = light->direction.getValue();
            SbColor color = light->color.getValue();
            float intensity = light->intensity.getValue();
            bool on = light->on.getValue();
            LOG_INF_S("Child " + std::to_string(i) + ": SoDirectionalLight (direction: " + 
                     std::to_string(dir[0]) + "," + std::to_string(dir[1]) + "," + std::to_string(dir[2]) + 
                     ", color: " + std::to_string(color[0]) + "," + std::to_string(color[1]) + "," + std::to_string(color[2]) + 
                     ", intensity: " + std::to_string(intensity) + ", on: " + std::to_string(on) + ")");
        } else {
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
    } else {
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
