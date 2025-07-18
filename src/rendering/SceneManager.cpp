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

SceneManager::SceneManager(Canvas* canvas)
    : m_canvas(canvas)
    , m_sceneRoot(nullptr)
    , m_camera(nullptr)
    , m_light(nullptr)
    , m_lightRoot(nullptr)  // Add initialization for m_lightRoot
    , m_objectRoot(nullptr)
    , m_isPerspectiveCamera(true)
{
    LOG_INF_S("SceneManager initializing");
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

        // Add an environment node for ambient light for overall brightness
        SoEnvironment* environment = new SoEnvironment;
        environment->ambientColor.setValue(0.4f, 0.4f, 0.4f); // Moderate ambient light
        environment->ambientIntensity.setValue(1.0f);
        m_lightRoot->addChild(environment);

        // Main directional light, made stronger
        m_light = new SoDirectionalLight;
        m_light->ref();
        m_light->direction.setValue(0.5f, 0.5f, -1.0f);
        m_light->intensity.setValue(1.0f); // Increased from 0.8
        m_light->color.setValue(1.0f, 1.0f, 1.0f);
        m_light->on.setValue(true);
        m_lightRoot->addChild(m_light);

        // Fill light, made stronger
        SoDirectionalLight* fillLight = new SoDirectionalLight;
        fillLight->direction.setValue(-0.3f, -0.3f, -0.5f);
        fillLight->intensity.setValue(0.6f); // Increased from 0.4
        fillLight->color.setValue(0.9f, 0.9f, 1.0f);
        fillLight->on.setValue(true);
        m_lightRoot->addChild(fillLight);

        // Back light, made stronger
        SoDirectionalLight* backLight = new SoDirectionalLight;
        backLight->direction.setValue(0.2f, 0.2f, 0.8f);
        backLight->intensity.setValue(0.5f); // Increased from 0.3
        backLight->color.setValue(1.0f, 1.0f, 0.9f);
        backLight->on.setValue(true);
        m_lightRoot->addChild(backLight);

        m_objectRoot = new SoSeparator;
        m_objectRoot->ref();
        m_sceneRoot->addChild(m_objectRoot);

        m_coordSystemRenderer = std::make_unique<CoordinateSystemRenderer>(m_objectRoot);
        m_pickingAidManager = std::make_unique<PickingAidManager>(this, m_canvas, m_canvas->getInputManager());

        // Initialize RenderingConfig callback
        initializeRenderingConfigCallback();
        
        // Initialize LightingConfig callback
        initializeLightingConfigCallback();

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
    if (m_camera) {
        m_camera->aspectRatio.setValue(static_cast<float>(size.x) / static_cast<float>(size.y));
    }

    SbViewportRegion viewport(size.x, size.y);
    SoGLRenderAction renderAction(viewport);
    renderAction.setSmoothing(!fastMode);
    renderAction.setNumPasses(fastMode ? 1 : 2);
    renderAction.setTransparencyType(
        fastMode ? SoGLRenderAction::BLEND : SoGLRenderAction::SORTED_OBJECT_BLEND
    );

    // Explicitly enable blending for line smoothing
    // Note: Let Coin3D handle lighting, don't enable GL_LIGHTING here
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // Disabling GL_COLOR_MATERIAL as it might conflict with the PHONG lighting model used by Coin3D's SoMaterial nodes.
    // This is a likely source of GL_INVALID_ENUM when a complex lighting model is active.
    // glEnable(GL_COLOR_MATERIAL); 
    glEnable(GL_TEXTURE_2D);
    // Removed glTexEnvf to fix OpenGL error 1280 (GL_INVALID_ENUM)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reset OpenGL errors before rendering
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOG_ERR_S("Pre-render: OpenGL error: " + std::to_string(err));
    }
    
    // Reset OpenGL state to prevent errors
    glDisable(GL_TEXTURE_2D);
    // Note: GL_TEXTURE_1D, GL_TEXTURE_3D and GL_TEXTURE_CUBE_MAP may not be available in all OpenGL versions
    // Only disable texture targets that are guaranteed to be available

    GLint lightingEnabled = 0;
    glGetIntegerv(GL_LIGHTING, &lightingEnabled);
    //LOG_INF_S("NavigationCube::render: Lighting enabled: " + std::to_string(lightingEnabled));

    GLint textureEnabled = 0;
    glGetIntegerv(GL_TEXTURE_2D, &textureEnabled);
    //LOG_INF_S("NavigationCube::render: Texture 2D enabled: " + std::to_string(textureEnabled));

    GLint texEnvMode = 0;
    glGetIntegerv(GL_TEXTURE_ENV_MODE, &texEnvMode);
    //LOG_INF_S("NavigationCube::render: Texture env mode: " + std::to_string(texEnvMode) + " (GL_MODULATE=" + std::to_string(GL_MODULATE) + ")");

    // Render the scene
    renderAction.apply(m_sceneRoot);

    // Check for OpenGL errors after rendering
    while ((err = glGetError()) != GL_NO_ERROR) {
        LOG_ERR_S("Post-render: OpenGL error: " + std::to_string(err) + 
                " (GL_INVALID_ENUM=" + std::to_string(GL_INVALID_ENUM) + 
                ", GL_INVALID_VALUE=" + std::to_string(GL_INVALID_VALUE) + 
                ", GL_INVALID_OPERATION=" + std::to_string(GL_INVALID_OPERATION) + ")");
    }

    // glDisable(GL_BLEND); // Disable blending afterwards
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

void SceneManager::initializeLightingConfigCallback()
{
    // Register callback to update lighting when LightingConfig changes
    LightingConfig& config = LightingConfig::getInstance();
    config.addSettingsChangedCallback([this, &config]() {
        LOG_INF_S("LightingConfig callback triggered - updating scene lighting");
        
        // Clear existing environment nodes from scene root
        if (m_sceneRoot) {
            // Find and remove environment nodes from scene root
            for (int i = m_sceneRoot->getNumChildren() - 1; i >= 0; --i) {
                SoNode* child = m_sceneRoot->getChild(i);
                if (child->isOfType(SoEnvironment::getClassTypeId())) {
                    LOG_INF_S("Removing environment node from scene root");
                    m_sceneRoot->removeChild(i);
                }
            }
        }
        
        // Clear existing lights from light root
        if (m_lightRoot) {
            // Remove all children except the first one (light model)
            while (m_lightRoot->getNumChildren() > 1) {
                m_lightRoot->removeChild(1);
            }
            LOG_INF_S("Cleared existing lights from light root");
        }
        
        // Add environment settings to light root (consistent with initScene)
        const LightSettings& envSettings = config.getEnvironmentSettings();
        SoEnvironment* environment = new SoEnvironment;
        environment->ambientColor.setValue(envSettings.ambientColor.Red(), 
                                         envSettings.ambientColor.Green(), 
                                         envSettings.ambientColor.Blue());
        environment->ambientIntensity.setValue(static_cast<float>(envSettings.ambientIntensity));
        m_lightRoot->addChild(environment);
        LOG_INF_S("Added environment settings to light root");
        
        // Add all lights from config to light root
        const auto& lights = config.getAllLights();
        int lightCount = 0;
        for (const auto& lightSettings : lights) {
            if (!lightSettings.enabled) {
                continue;
            }
            
            if (lightSettings.type == "directional") {
                SoDirectionalLight* light = new SoDirectionalLight;
                light->direction.setValue(static_cast<float>(lightSettings.directionX),
                                        static_cast<float>(lightSettings.directionY),
                                        static_cast<float>(lightSettings.directionZ));
                light->color.setValue(lightSettings.color.Red(),
                                    lightSettings.color.Green(),
                                    lightSettings.color.Blue());
                light->intensity.setValue(static_cast<float>(lightSettings.intensity));
                light->on.setValue(true);
                m_lightRoot->addChild(light);
                
                LOG_INF_S("Added directional light: " + lightSettings.name + 
                         " (dir: " + std::to_string(lightSettings.directionX) + "," + 
                         std::to_string(lightSettings.directionY) + "," + 
                         std::to_string(lightSettings.directionZ) + 
                         ", intensity: " + std::to_string(lightSettings.intensity) + ")");
                lightCount++;
            }
            // Add support for other light types (point, spot) as needed
        }
        
        LOG_INF_S("Total lights added: " + std::to_string(lightCount));
        
        // Debug current lighting state
        debugLightingState();
        
        // Force multiple refresh methods to ensure update
        LOG_INF_S("Forcing scene refresh");
        
        // Method 1: Touch the scene root to force Coin3D update
        if (m_sceneRoot) {
            m_sceneRoot->touch();
            LOG_INF_S("Touched scene root");
        }
        
        // Method 2: Touch the light root
        if (m_lightRoot) {
            m_lightRoot->touch();
            LOG_INF_S("Touched light root");
        }
        
        // Method 3: Force immediate render update
        if (m_canvas) {
            // Force a complete scene rebuild
            m_canvas->Refresh(true);
            m_canvas->Update();
            LOG_INF_S("Forced canvas refresh and update");
        }
        
        // Method 4: RefreshManager
        if (m_canvas && m_canvas->getRefreshManager()) {
            m_canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::RENDERING_CHANGED, true);
            LOG_INF_S("Requested refresh via RefreshManager");
        }
        
        LOG_INF_S("Updated scene lighting from LightingConfig changes - complete");
    });
    
    LOG_INF_S("LightingConfig callback initialized in SceneManager");
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
