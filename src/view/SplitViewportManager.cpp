#include "SplitViewportManager.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "RenderingEngine.h"
#include "config/ConfigManager.h"
#include "logger/Logger.h"
#include "DPIManager.h"

#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/SbViewportRegion.h>
#include <GL/gl.h>
#include <string>

SplitViewportManager::SplitViewportManager(Canvas* canvas, SceneManager* sceneManager)
    : m_canvas(canvas)
    , m_sceneManager(sceneManager)
    , m_currentMode(SplitMode::SINGLE)
    , m_activeViewportIndex(0)
    , m_enabled(false)
    , m_dpiScale(1.0f)
    , m_borderWidth(2)
    , m_cameraSyncEnabled(true)
    , m_lastCanvasSize(0, 0) {
    
    LOG_INF_S("SplitViewportManager: Initializing");
    
    if (!m_canvas) {
        LOG_ERR_S("SplitViewportManager: Canvas is null");
        return;
    }
    
    if (!m_sceneManager) {
        LOG_ERR_S("SplitViewportManager: SceneManager is null");
        return;
    }
    
    auto& dpiManager = DPIManager::getInstance();
    m_dpiScale = dpiManager.getDPIScale();
    m_borderWidth = dpiManager.getScaledSize(2);
    
    initializeViewports();
    createViewportScenes();
}

SplitViewportManager::~SplitViewportManager() {
    LOG_INF_S("SplitViewportManager: Destroying");
    
    for (auto& viewport : m_viewports) {
        if (viewport.camera) {
            viewport.camera->unref();
            viewport.camera = nullptr;
        }
        if (viewport.sceneRoot) {
            viewport.sceneRoot->unref();
            viewport.sceneRoot = nullptr;
        }
    }
    
    m_viewports.clear();
}

void SplitViewportManager::initializeViewports() {
    m_viewports.resize(6);
    
    for (int i = 0; i < 6; i++) {
        m_viewports[i].viewportIndex = i;
        m_viewports[i].isActive = (i == 0);
    }
}

void SplitViewportManager::createViewportScenes() {
    if (!m_sceneManager) {
        LOG_ERR_S("SplitViewportManager: Cannot create viewport scenes - SceneManager is null");
        return;
    }
    
    // CRITICAL: Use objectRoot instead of sceneRoot!
    // sceneRoot contains the main camera, which would conflict with viewport cameras
    // objectRoot only contains the geometry, allowing each viewport to have independent cameras
    SoSeparator* objectRoot = m_sceneManager->getObjectRoot();
    if (!objectRoot) {
        LOG_ERR_S("SplitViewportManager: Cannot create viewport scenes - object root is null");
        return;
    }
    
    SoCamera* mainCamera = m_sceneManager->getCamera();
    
    for (int i = 0; i < 6; i++) {
        m_viewports[i].camera = new SoPerspectiveCamera();
        m_viewports[i].camera->ref();
        
        // Initialize viewport camera with main camera's state
        if (mainCamera) {
            m_viewports[i].camera->position.setValue(mainCamera->position.getValue());
            m_viewports[i].camera->orientation.setValue(mainCamera->orientation.getValue());
            m_viewports[i].camera->aspectRatio.setValue(mainCamera->aspectRatio.getValue());
            m_viewports[i].camera->nearDistance.setValue(mainCamera->nearDistance.getValue());
            m_viewports[i].camera->farDistance.setValue(mainCamera->farDistance.getValue());
            m_viewports[i].camera->focalDistance.setValue(mainCamera->focalDistance.getValue());
            
            if (mainCamera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
                SoPerspectiveCamera* mainPersp = static_cast<SoPerspectiveCamera*>(mainCamera);
                SoPerspectiveCamera* viewportPersp = static_cast<SoPerspectiveCamera*>(m_viewports[i].camera);
                viewportPersp->heightAngle.setValue(mainPersp->heightAngle.getValue());
            }
        }
        
        m_viewports[i].sceneRoot = new SoSeparator();
        m_viewports[i].sceneRoot->ref();
        
        SoDirectionalLight* light = new SoDirectionalLight();
        light->direction.setValue(0, 0, -1);
        m_viewports[i].sceneRoot->addChild(light);
        
        m_viewports[i].sceneRoot->addChild(m_viewports[i].camera);
        m_viewports[i].sceneRoot->addChild(objectRoot);  // Use objectRoot instead of sceneRoot
    }
    
    LOG_INF_S("SplitViewportManager: Created viewport scenes with independent cameras");
}

void SplitViewportManager::setSplitMode(SplitMode mode) {
    if (m_currentMode == mode) {
        return;
    }
    
    LOG_INF_S("SplitViewportManager: Changing split mode to " + std::to_string(static_cast<int>(mode)));
    
    // Before changing mode, if switching to single view, sync main camera with active viewport
    if (mode == SplitMode::SINGLE && m_activeViewportIndex >= 0 && m_activeViewportIndex < static_cast<int>(m_viewports.size())) {
        syncMainCameraToViewport(m_activeViewportIndex);
        LOG_INF_S("SplitViewportManager: Restored main camera for single view mode");
    }
    
    m_currentMode = mode;
    m_activeViewportIndex = 0;
    
    if (m_canvas) {
        wxSize canvasSize = m_canvas->GetClientSize();
        updateViewportLayouts(canvasSize);
        m_canvas->Refresh();
    }
}

void SplitViewportManager::setEnabled(bool enabled) {
    if (m_enabled == enabled) {
        return;
    }
    
    m_enabled = enabled;
    
    if (m_enabled) {
        LOG_INF_S("SplitViewportManager: Enabled");
    } else {
        // When disabling split view, restore the active viewport's camera state to main camera
        // This ensures the user's current view is preserved when returning to single view mode
        if (m_activeViewportIndex >= 0 && m_activeViewportIndex < static_cast<int>(m_viewports.size())) {
            syncMainCameraToViewport(m_activeViewportIndex);
            LOG_INF_S("SplitViewportManager: Restored main camera from active viewport");
        }
        LOG_INF_S("SplitViewportManager: Disabled");
    }
    
    if (m_canvas) {
        m_canvas->Refresh();
    }
}

void SplitViewportManager::render() {
    if (!m_enabled || !m_canvas || !m_sceneManager) {
        return;
    }
    
    // Render the global background once using the main RenderingEngine so that
    // split view uses the same background (solid / gradient / image) as the
    // primary canvas and preview views.
    if (auto renderingEngine = m_canvas->getRenderingEngine()) {
        renderingEngine->renderBackground();
        // Start with a clean depth buffer for per-viewport 3D rendering
        glClear(GL_DEPTH_BUFFER_BIT);
    } else {
        // Fallback: clear with solid background color from config
        wxSize canvasSize = m_canvas->GetClientSize();
        double bgR = ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorR", 0.0);
        double bgG = ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorG", 0.0);
        double bgB = ConfigManager::getInstance().getDouble("Canvas", "BackgroundColorB", 0.0);
        glViewport(0, 0, canvasSize.x, canvasSize.y);
        glClearColor(static_cast<float>(bgR),
                     static_cast<float>(bgG),
                     static_cast<float>(bgB),
                     1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    
    if (m_cameraSyncEnabled) {
        // When sync is enabled, all viewport cameras follow the main camera
        syncAllCamerasToMain();
    } else {
        // When sync is disabled, update the active viewport's camera from main camera
        // Main camera is being manipulated by EventCoordinator, so we copy its state
        // to the active viewport's camera. Other viewports keep their own camera states.
        if (m_activeViewportIndex >= 0 && m_activeViewportIndex < static_cast<int>(m_viewports.size())) {
            SoCamera* mainCamera = m_sceneManager->getCamera();
            SoCamera* activeCamera = m_viewports[m_activeViewportIndex].camera;
            
            if (mainCamera && activeCamera) {
                // Copy main camera state to active viewport camera
                activeCamera->position.setValue(mainCamera->position.getValue());
                activeCamera->orientation.setValue(mainCamera->orientation.getValue());
                activeCamera->aspectRatio.setValue(mainCamera->aspectRatio.getValue());
                activeCamera->nearDistance.setValue(mainCamera->nearDistance.getValue());
                activeCamera->farDistance.setValue(mainCamera->farDistance.getValue());
                activeCamera->focalDistance.setValue(mainCamera->focalDistance.getValue());
                
                if (mainCamera->isOfType(SoPerspectiveCamera::getClassTypeId()) &&
                    activeCamera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
                    SoPerspectiveCamera* mainPersp = static_cast<SoPerspectiveCamera*>(mainCamera);
                    SoPerspectiveCamera* activePersp = static_cast<SoPerspectiveCamera*>(activeCamera);
                    activePersp->heightAngle.setValue(mainPersp->heightAngle.getValue());
                }
            }
        }
    }
    
    int numViewports = 0;
    switch (m_currentMode) {
        case SplitMode::SINGLE:       numViewports = 1; break;
        case SplitMode::HORIZONTAL_2: numViewports = 2; break;
        case SplitMode::VERTICAL_2:   numViewports = 2; break;
        case SplitMode::QUAD:         numViewports = 4; break;
        case SplitMode::SIX:          numViewports = 6; break;
    }
    
    for (int i = 0; i < numViewports; i++) {
        if (i < static_cast<int>(m_viewports.size())) {
            // Background is already rendered once for the full canvas; each
            // viewport only needs to render its own scene.
            renderViewport(m_viewports[i]);
        }
    }
    
    // Draw borders to indicate active/inactive viewports
    drawViewportBorders();
}

void SplitViewportManager::renderViewport(const SplitViewportInfo& viewport) {
    if (!viewport.sceneRoot || !viewport.camera) {
        return;
    }
    
    wxSize canvasSize = m_canvas->GetClientSize();
    
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glPushMatrix();
    
    // Set up viewport
    glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
    
    // Enable scissor test to limit rendering to this viewport
    glEnable(GL_SCISSOR_TEST);
    glScissor(viewport.x, viewport.y, viewport.width, viewport.height);
    
    // Clear only depth buffer for this viewport (color already cleared)
    glClear(GL_DEPTH_BUFFER_BIT);
    
    glDisable(GL_SCISSOR_TEST);
    
    // Match SceneManager render state to ensure correct transparency sorting
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);
    glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    
    // Keep viewport camera aspect ratio in sync with its region
    if (viewport.height > 0) {
        float aspect = static_cast<float>(viewport.width) / static_cast<float>(viewport.height);
        viewport.camera->aspectRatio.setValue(aspect);
    }
    
    // Render the scene using high-quality transparency sorting like the main view
    SbViewportRegion viewportRegion;
    viewportRegion.setWindowSize(SbVec2s(canvasSize.x, canvasSize.y));
    viewportRegion.setViewportPixels(viewport.x, viewport.y, viewport.width, viewport.height);
    
    SoGLRenderAction renderAction(viewportRegion);
    renderAction.setSmoothing(true);
    // Use single-pass sorted-object blending to avoid over-brightening geometry
    renderAction.setNumPasses(1);
    renderAction.setTransparencyType(SoGLRenderAction::SORTED_OBJECT_BLEND);
    renderAction.setCacheContext(1);
    
    renderAction.apply(viewport.sceneRoot);
    
    glPopMatrix();
    glPopAttrib();
}

void SplitViewportManager::drawViewportBackground(const SplitViewportInfo& viewport,
                                                  const double topColor[3],
                                                  const double bottomColor[3]) {
    // Background for split view is now rendered once per frame by
    // RenderingEngine::renderBackground() to ensure consistent visuals with
    // the main canvas and preview views. This function is kept only for
    // backward compatibility and intentionally does nothing.
    (void)viewport;
    (void)topColor;
    (void)bottomColor;
}

void SplitViewportManager::handleSizeChange(const wxSize& canvasSize) {
    m_lastCanvasSize = canvasSize;
    updateViewportLayouts(canvasSize);
}

void SplitViewportManager::updateViewportLayouts(const wxSize& canvasSize) {
    switch (m_currentMode) {
        case SplitMode::SINGLE:
            applySingleViewLayout(canvasSize);
            break;
        case SplitMode::HORIZONTAL_2:
            applyHorizontal2Layout(canvasSize);
            break;
        case SplitMode::VERTICAL_2:
            applyVertical2Layout(canvasSize);
            break;
        case SplitMode::QUAD:
            applyQuadLayout(canvasSize);
            break;
        case SplitMode::SIX:
            applySixViewLayout(canvasSize);
            break;
    }
}

void SplitViewportManager::applySingleViewLayout(const wxSize& canvasSize) {
    m_viewports[0].x = 0;
    m_viewports[0].y = 0;
    m_viewports[0].width = canvasSize.x;
    m_viewports[0].height = canvasSize.y;
    m_viewports[0].isActive = true;
}

void SplitViewportManager::applyHorizontal2Layout(const wxSize& canvasSize) {
    int halfHeight = canvasSize.y / 2;
    int border = m_borderWidth;
    
    m_viewports[0].x = 0;
    m_viewports[0].y = halfHeight + border / 2;
    m_viewports[0].width = canvasSize.x;
    m_viewports[0].height = halfHeight - border / 2;
    m_viewports[0].isActive = (m_activeViewportIndex == 0);
    
    m_viewports[1].x = 0;
    m_viewports[1].y = 0;
    m_viewports[1].width = canvasSize.x;
    m_viewports[1].height = halfHeight - border / 2;
    m_viewports[1].isActive = (m_activeViewportIndex == 1);
}

void SplitViewportManager::applyVertical2Layout(const wxSize& canvasSize) {
    int halfWidth = canvasSize.x / 2;
    int border = m_borderWidth;
    
    m_viewports[0].x = 0;
    m_viewports[0].y = 0;
    m_viewports[0].width = halfWidth - border / 2;
    m_viewports[0].height = canvasSize.y;
    m_viewports[0].isActive = (m_activeViewportIndex == 0);
    
    m_viewports[1].x = halfWidth + border / 2;
    m_viewports[1].y = 0;
    m_viewports[1].width = halfWidth - border / 2;
    m_viewports[1].height = canvasSize.y;
    m_viewports[1].isActive = (m_activeViewportIndex == 1);
}

void SplitViewportManager::applyQuadLayout(const wxSize& canvasSize) {
    int halfWidth = canvasSize.x / 2;
    int halfHeight = canvasSize.y / 2;
    int border = m_borderWidth;
    
    m_viewports[0].x = 0;
    m_viewports[0].y = halfHeight + border / 2;
    m_viewports[0].width = halfWidth - border / 2;
    m_viewports[0].height = halfHeight - border / 2;
    m_viewports[0].isActive = (m_activeViewportIndex == 0);
    
    m_viewports[1].x = halfWidth + border / 2;
    m_viewports[1].y = halfHeight + border / 2;
    m_viewports[1].width = halfWidth - border / 2;
    m_viewports[1].height = halfHeight - border / 2;
    m_viewports[1].isActive = (m_activeViewportIndex == 1);
    
    m_viewports[2].x = 0;
    m_viewports[2].y = 0;
    m_viewports[2].width = halfWidth - border / 2;
    m_viewports[2].height = halfHeight - border / 2;
    m_viewports[2].isActive = (m_activeViewportIndex == 2);
    
    m_viewports[3].x = halfWidth + border / 2;
    m_viewports[3].y = 0;
    m_viewports[3].width = halfWidth - border / 2;
    m_viewports[3].height = halfHeight - border / 2;
    m_viewports[3].isActive = (m_activeViewportIndex == 3);
}

void SplitViewportManager::applySixViewLayout(const wxSize& canvasSize) {
    int thirdWidth = canvasSize.x / 3;
    int halfHeight = canvasSize.y / 2;
    int border = m_borderWidth;
    
    for (int row = 0; row < 2; row++) {
        int yPos = (row == 0) ? halfHeight + border / 2 : 0;
        int rowHeight = halfHeight - border / 2;
        
        for (int col = 0; col < 3; col++) {
            int idx = row * 3 + col;
            m_viewports[idx].x = col * thirdWidth + (col > 0 ? border / 2 : 0);
            m_viewports[idx].y = yPos;
            m_viewports[idx].width = thirdWidth - (col == 0 ? border / 2 : (col == 2 ? border / 2 : border));
            m_viewports[idx].height = rowHeight;
            m_viewports[idx].isActive = (m_activeViewportIndex == idx);
        }
    }
}

void SplitViewportManager::syncAllCamerasToMain() {
    if (!m_cameraSyncEnabled) {
        return;
    }

    if (!m_sceneManager) {
        return;
    }
    
    SoCamera* mainCamera = m_sceneManager->getCamera();
    if (!mainCamera) {
        return;
    }
    
    int numViewports = 0;
    switch (m_currentMode) {
        case SplitMode::SINGLE: numViewports = 1; break;
        case SplitMode::HORIZONTAL_2: numViewports = 2; break;
        case SplitMode::VERTICAL_2: numViewports = 2; break;
        case SplitMode::QUAD: numViewports = 4; break;
        case SplitMode::SIX: numViewports = 6; break;
    }
    
    for (int i = 0; i < numViewports; i++) {
        if (i < static_cast<int>(m_viewports.size()) && m_viewports[i].camera) {
            syncCameraToMain(m_viewports[i].camera);
        }
    }
}

void SplitViewportManager::syncCameraToMain(SoCamera* targetCamera) {
    if (!targetCamera || !m_sceneManager) {
        return;
    }
    
    SoCamera* mainCamera = m_sceneManager->getCamera();
    if (!mainCamera) {
        return;
    }
    
    targetCamera->position.setValue(mainCamera->position.getValue());
    targetCamera->orientation.setValue(mainCamera->orientation.getValue());
    targetCamera->aspectRatio.setValue(mainCamera->aspectRatio.getValue());
    targetCamera->nearDistance.setValue(mainCamera->nearDistance.getValue());
    targetCamera->farDistance.setValue(mainCamera->farDistance.getValue());
    targetCamera->focalDistance.setValue(mainCamera->focalDistance.getValue());
    
    if (mainCamera->isOfType(SoPerspectiveCamera::getClassTypeId()) &&
        targetCamera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
        SoPerspectiveCamera* mainPersp = static_cast<SoPerspectiveCamera*>(mainCamera);
        SoPerspectiveCamera* targetPersp = static_cast<SoPerspectiveCamera*>(targetCamera);
        targetPersp->heightAngle.setValue(mainPersp->heightAngle.getValue());
    }
}

void SplitViewportManager::syncMainCameraToViewport(int viewportIndex) {
    if (!m_sceneManager) {
        return;
    }
    
    if (viewportIndex < 0 || viewportIndex >= static_cast<int>(m_viewports.size())) {
        return;
    }
    
    SoCamera* mainCamera = m_sceneManager->getCamera();
    SoCamera* viewportCamera = m_viewports[viewportIndex].camera;
    
    if (!mainCamera || !viewportCamera) {
        return;
    }
    
    // Copy viewport camera state to main camera
    mainCamera->position.setValue(viewportCamera->position.getValue());
    mainCamera->orientation.setValue(viewportCamera->orientation.getValue());
    mainCamera->aspectRatio.setValue(viewportCamera->aspectRatio.getValue());
    mainCamera->nearDistance.setValue(viewportCamera->nearDistance.getValue());
    mainCamera->farDistance.setValue(viewportCamera->farDistance.getValue());
    mainCamera->focalDistance.setValue(viewportCamera->focalDistance.getValue());
    
    if (mainCamera->isOfType(SoPerspectiveCamera::getClassTypeId()) &&
        viewportCamera->isOfType(SoPerspectiveCamera::getClassTypeId())) {
        SoPerspectiveCamera* mainPersp = static_cast<SoPerspectiveCamera*>(mainCamera);
        SoPerspectiveCamera* viewportPersp = static_cast<SoPerspectiveCamera*>(viewportCamera);
        mainPersp->heightAngle.setValue(viewportPersp->heightAngle.getValue());
    }
}

void SplitViewportManager::drawViewportBorders() {
    if (m_currentMode == SplitMode::SINGLE) {
        return;
    }
    
    int numViewports = 0;
    switch (m_currentMode) {
        case SplitMode::SINGLE: numViewports = 1; break;
        case SplitMode::HORIZONTAL_2: numViewports = 2; break;
        case SplitMode::VERTICAL_2: numViewports = 2; break;
        case SplitMode::QUAD: numViewports = 4; break;
        case SplitMode::SIX: numViewports = 6; break;
    }
    
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
	glDisable(GL_SCISSOR_TEST);
    
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    wxSize canvasSize = m_canvas->GetClientSize();
	glViewport(0, 0, canvasSize.x, canvasSize.y);
    glOrtho(0, canvasSize.x, 0, canvasSize.y, -1, 1);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    for (int i = 0; i < numViewports; i++) {
        if (i < static_cast<int>(m_viewports.size())) {
            drawBorder(m_viewports[i].x, m_viewports[i].y,
                      m_viewports[i].width, m_viewports[i].height,
                      m_viewports[i].isActive);
        }
    }
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    
    glPopAttrib();
}

void SplitViewportManager::drawBorder(int x, int y, int width, int height, bool isActive) {
    glLineWidth(static_cast<float>(m_borderWidth) * 1.5f);
    
    if (isActive) {
        // Active viewport: orange border (more visible)
        glColor3f(1.0f, 0.5f, 0.0f);
    } else {
        // Inactive viewport: gray border
        glColor3f(0.5f, 0.5f, 0.5f);
    }
    
    glBegin(GL_LINE_LOOP);
    glVertex2i(x, y);
    glVertex2i(x + width, y);
    glVertex2i(x + width, y + height);
    glVertex2i(x, y + height);
    glEnd();
}

bool SplitViewportManager::handleMouseEvent(wxMouseEvent& event) {
    if (!m_enabled) {
        return false;
    }
    
    // Handle mouse button down to detect viewport switching
    if (event.ButtonDown(wxMOUSE_BTN_LEFT) || event.ButtonDown(wxMOUSE_BTN_RIGHT) || event.ButtonDown(wxMOUSE_BTN_MIDDLE)) {
        wxPoint pos = event.GetPosition();
        wxSize canvasSize = m_canvas->GetClientSize();
        int glY = canvasSize.y - pos.y;
        
        int viewportIndex = findViewportAtPosition(wxPoint(pos.x, glY));
        if (viewportIndex >= 0 && viewportIndex != m_activeViewportIndex) {
            // Only switch if clicking on a different viewport
            setActiveViewport(viewportIndex);
            if (m_canvas) {
                m_canvas->Refresh();
            }
            // Don't block the event - let it propagate for camera operations
        }
    }
    
    // Always return false to allow event propagation to EventCoordinator
    // This enables camera operations (rotate, pan, zoom) in the active viewport
    return false;
}

int SplitViewportManager::findViewportAtPosition(const wxPoint& pos) {
    int numViewports = 0;
    switch (m_currentMode) {
        case SplitMode::SINGLE: numViewports = 1; break;
        case SplitMode::HORIZONTAL_2: numViewports = 2; break;
        case SplitMode::VERTICAL_2: numViewports = 2; break;
        case SplitMode::QUAD: numViewports = 4; break;
        case SplitMode::SIX: numViewports = 6; break;
    }
    
    for (int i = 0; i < numViewports; i++) {
        if (i < static_cast<int>(m_viewports.size())) {
            const auto& vp = m_viewports[i];
            if (pos.x >= vp.x && pos.x < vp.x + vp.width &&
                pos.y >= vp.y && pos.y < vp.y + vp.height) {
                return i;
            }
        }
    }
    
    return -1;
}

void SplitViewportManager::setActiveViewport(int index) {
    if (index < 0 || index >= static_cast<int>(m_viewports.size())) {
        return;
    }
    
    m_activeViewportIndex = index;
    
    for (size_t i = 0; i < m_viewports.size(); i++) {
        m_viewports[i].isActive = (static_cast<int>(i) == index);
    }
    
    // If camera sync is disabled, switch the main camera to the active viewport's camera
    // This allows independent camera control per viewport
    if (!m_cameraSyncEnabled && m_sceneManager && m_viewports[index].camera) {
        syncMainCameraToViewport(index);
    }
    
    LOG_INF_S("SplitViewportManager: Active viewport changed to " + std::to_string(index));
}

void SplitViewportManager::setCameraSyncEnabled(bool enabled) {
    if (m_cameraSyncEnabled == enabled) {
        return;
    }

    m_cameraSyncEnabled = enabled;
    if (m_cameraSyncEnabled) {
        // When enabling sync, sync all viewport cameras to main camera
        syncAllCamerasToMain();
    } else {
        // When disabling sync, initialize all viewport cameras with current main camera state
        // This gives each viewport a starting point for independent camera control
        syncAllCamerasToMain();
        LOG_INF_S("SplitViewportManager: Initialized all viewport cameras for independent control");
    }
    LOG_INF_S(std::string("SplitViewportManager: Camera sync ") + (enabled ? "enabled" : "disabled"));
}

