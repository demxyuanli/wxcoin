#include "SceneManager.h"
#include "Canvas.h"
#include "CoordinateSystemRenderer.h"
#include "PickingAidManager.h"
#include "Logger.h"
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoEnvironment.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/SbLinear.h>
#include <GL/gl.h>

SceneManager::SceneManager(Canvas* canvas)
    : m_canvas(canvas)
    , m_sceneRoot(nullptr)
    , m_camera(nullptr)
    , m_light(nullptr)
    , m_objectRoot(nullptr)
    , m_isPerspectiveCamera(true)
{
    LOG_INF("SceneManager initializing");
}

SceneManager::~SceneManager() {
    cleanup();
    LOG_INF("SceneManager destroyed");
}

bool SceneManager::initScene() {
    try {
        m_sceneRoot = new SoSeparator;
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

        SoEnvironment* env = new SoEnvironment;
        env->ambientColor.setValue(1.0f, 1.0f, 1.0f);
        env->ambientIntensity.setValue(0.3f);
        m_sceneRoot->addChild(env);

        m_light = new SoDirectionalLight;
        m_light->ref();
        m_light->direction.setValue(0.5f, 0.5f, -1.0f);
        m_light->intensity.setValue(0.8f);
        m_light->color.setValue(1.0f, 1.0f, 1.0f);
        m_light->on.setValue(true);
        m_sceneRoot->addChild(m_light);

        SoDirectionalLight* fillLight = new SoDirectionalLight;
        fillLight->direction.setValue(-0.3f, -0.3f, -0.5f);
        fillLight->intensity.setValue(0.4f);
        fillLight->color.setValue(0.9f, 0.9f, 1.0f);
        fillLight->on.setValue(true);
        m_sceneRoot->addChild(fillLight);

        m_objectRoot = new SoSeparator;
        m_objectRoot->ref();
        m_sceneRoot->addChild(m_objectRoot);

        m_coordSystemRenderer = std::make_unique<CoordinateSystemRenderer>(m_objectRoot);
        m_pickingAidManager = std::make_unique<PickingAidManager>(this, m_canvas);

        resetView();
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during scene initialization: " + std::string(e.what()));
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
    if (m_objectRoot) {
        m_objectRoot->unref();
        m_objectRoot = nullptr;
    }
}

void SceneManager::resetView() {
    if (!m_camera || !m_sceneRoot) {
        LOG_ERR("Failed to reset view: Invalid camera or scene");
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

    m_canvas->Refresh(true);
}

void SceneManager::toggleCameraMode() {
    if (!m_sceneRoot || !m_camera) {
        LOG_ERR("Failed to toggle camera mode: Invalid context or scene");
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

    m_canvas->Refresh(true);
    LOG_INF(m_isPerspectiveCamera ? "Switched to Perspective Camera" : "Switched to Orthographic Camera");
}

void SceneManager::setView(const std::string& viewName) {
    if (!m_camera || !m_sceneRoot) {
        LOG_ERR("Failed to set view: Invalid camera or scene");
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
        LOG_WAR("Invalid view name: " + viewName);
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

    LOG_INF("Switched to view: " + viewName);
    m_canvas->Refresh(true);
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    renderAction.apply(m_sceneRoot);

    glDisable(GL_BLEND); // Disable blending afterwards
}

void SceneManager::updateAspectRatio(const wxSize& size) {
    if (m_camera && size.x > 0 && size.y > 0) {
        m_camera->aspectRatio.setValue(static_cast<float>(size.x) / static_cast<float>(size.y));
    }
}

bool SceneManager::screenToWorld(const wxPoint& screenPos, SbVec3f& worldPos) {
    if (!m_camera) {
        LOG_ERR("Cannot convert screen to world: Invalid camera");
        return false;
    }

    wxSize size = m_canvas->GetClientSize();
    if (size.x <= 0 || size.y <= 0) {
        LOG_ERR("Invalid viewport size");
        return false;
    }

    float x = static_cast<float>(screenPos.x) / size.GetWidth();
    float y = 1.0f - static_cast<float>(screenPos.y) / size.GetHeight();

    SbViewportRegion viewport(size.GetWidth(), size.GetHeight());
    SbVec2f normalizedPos(x, y);
    SbLine lineFromCamera;
    m_camera->getViewVolume().projectPointToLine(normalizedPos, lineFromCamera);

    SbPlane targetPlane(SbVec3f(0, 0, 1), 0.0f);

    if (targetPlane.intersect(lineFromCamera, worldPos)) {
        return true;
    }
    else {
        LOG_WAR("Ray did not intersect the target plane.");
        worldPos = lineFromCamera.getPosition() + lineFromCamera.getDirection() * m_camera->focalDistance.getValue();
        return false;
    }
}