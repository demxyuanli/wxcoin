#include "Canvas.h"
#include "Logger.h"
#include <wx/dcclient.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoEnvironment.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCylinder.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoVertexProperty.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include "MouseHandler.h"
#include "NavigationStyle.h"
#include "PositionDialog.h"
#include <GL/gl.h>
#include <memory>

// Define static constants
const int Canvas::RENDER_INTERVAL = 16; // ~60 FPS (milliseconds)
const int Canvas::MOTION_INTERVAL = 10; // Mouse motion throttling (milliseconds)
const float Canvas::COORD_PLANE_SIZE = 4.0f; // Configurable coordinate plane size
const float Canvas::COORD_PLANE_TRANSPARENCY = 1.0f; // Reduced for better visibility

// Define static canvas attributes
const int Canvas::s_canvasAttribs[] = {
    WX_GL_RGBA,
    WX_GL_DOUBLEBUFFER,
    WX_GL_DEPTH_SIZE, 24,
    WX_GL_STENCIL_SIZE, 8,
    0 // Terminator
};

BEGIN_EVENT_TABLE(Canvas, wxGLCanvas)
EVT_PAINT(Canvas::onPaint)
EVT_SIZE(Canvas::onSize)
EVT_ERASE_BACKGROUND(Canvas::onEraseBackground)
EVT_LEFT_DOWN(Canvas::onMouseButton)
EVT_LEFT_UP(Canvas::onMouseButton)
EVT_RIGHT_DOWN(Canvas::onMouseButton)
EVT_RIGHT_UP(Canvas::onMouseButton)
EVT_MOTION(Canvas::onMouseMotion)
EVT_MOUSEWHEEL(Canvas::onMouseWheel)
END_EVENT_TABLE()

Canvas::Canvas(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size)
    : wxGLCanvas(parent, id, s_canvasAttribs, pos, size, wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS | wxBORDER_NONE)
    , m_glContext(nullptr)
    , m_sceneRoot(nullptr)
    , m_camera(nullptr)
    , m_light(nullptr)
    , m_objectRoot(nullptr)
    , m_mouseHandler(nullptr)
    , m_navStyle(nullptr)
    , m_isRendering(false)
    , m_lastRenderTime(0)
    , m_isPerspectiveCamera(true)
    , m_pickingAidSeparator(nullptr)
    , m_pickingAidTransform(nullptr)
    , m_pickingAidVisible(false)
{
    LOG_INF("Canvas initializing with specified attributes");

    SetName("Canvas");

    // Ensure minimum size
    wxSize clientSize = GetClientSize();
    if (clientSize.x <= 0 || clientSize.y <= 0) {
        clientSize = wxSize(400, 300);
        SetSize(clientSize);
        SetMinSize(clientSize);
    }

    // Create OpenGL context
    m_glContext = new wxGLContext(this);
    if (!m_glContext) {
        LOG_ERR("Failed to create GL context.");
        return;
    }

    if (!SetCurrent(*m_glContext)) {
        LOG_ERR("Failed to set GL context as current.");
        delete m_glContext;
        m_glContext = nullptr;
        return;
    }

    // Check OpenGL version
    const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (!glVersion || std::string(glVersion).find("3.") == std::string::npos) {
        LOG_WAR("OpenGL 3.0 or higher required. Found: " + std::string(glVersion ? glVersion : "unknown"));
    }
    LOG_INF("GL Context created. OpenGL version: " + std::string(glVersion ? glVersion : "unknown"));

    // Initialize scene
    if (!initScene()) {
        LOG_ERR("Failed to initialize scene.");
        return;
    }

    // Force initial render
    LOG_INF("Forcing initial paint event.");
    Refresh(true);
    Update();

    LOG_INF("Canvas initialized successfully.");
}

bool Canvas::initScene()
{
    if (!m_glContext || !SetCurrent(*m_glContext)) {
        LOG_ERR("No GL context available during scene initialization");
        return false;
    }

    try {
        m_sceneRoot = new SoSeparator;
        if (!m_sceneRoot) {
            LOG_ERR("Failed to create scene root");
            return false;
        }
        m_sceneRoot->ref();

        // Create camera (default to perspective)
        m_camera = new SoPerspectiveCamera;
        if (!m_camera) {
            LOG_ERR("Failed to create camera");
            return false;
        }
        m_camera->ref();
        m_camera->position.setValue(0.0f, 0.0f, 100.0f);
        m_camera->nearDistance.setValue(0.1f);
        m_camera->farDistance.setValue(100.0f);
        m_camera->focalDistance.setValue(5.0f);

        wxSize size = GetClientSize();
        if (size.x > 0 && size.y > 0) {
            float aspect = static_cast<float>(size.x) / static_cast<float>(size.y);
            m_camera->aspectRatio.setValue(aspect);
        }
        m_sceneRoot->addChild(m_camera);

        // Add environment node
        SoEnvironment* env = new SoEnvironment;
        env->ambientColor.setValue(1.0f, 1.0f, 1.0f);
        env->ambientIntensity.setValue(0.8f); // Increased for side face brightness
        m_sceneRoot->addChild(env);

        // Create directional light
        m_light = new SoDirectionalLight;
        if (!m_light) {
            LOG_ERR("Failed to create light");
            return false;
        }
        m_light->ref();
        m_light->direction.setValue(0.5f, 0.5f, -1.0f); // Adjusted for side face illumination
        m_light->intensity.setValue(1.0f);
        m_light->color.setValue(1.0f, 1.0f, 1.0f);
        m_light->on.setValue(true);
        m_sceneRoot->addChild(m_light);

        // Create object root
        m_objectRoot = new SoSeparator;
        if (!m_objectRoot) {
            LOG_ERR("Failed to create object root");
            return false;
        }
        m_objectRoot->ref();
        m_sceneRoot->addChild(m_objectRoot);

        createPickingAidLines();

        // Set up OpenGL state
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glEnable(GL_COLOR_MATERIAL);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glShadeModel(GL_SMOOTH);
        glClearColor(0.8f, 0.8f, 0.9f, 1.0f);

        createCoordinateSystem();
        resetView();

        wxSize clientSize = GetClientSize();
        if (clientSize.GetWidth() > 0 && clientSize.GetHeight() > 0) {
            glViewport(0, 0, clientSize.GetWidth(), clientSize.GetHeight());
            render();
        }

        AddTestCube();

        return true;
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during scene initialization: " + std::string(e.what()));
        cleanup();
        return false;
    }
}

void Canvas::render(bool fastMode)
{
    if (!IsShown() || !m_glContext || !m_sceneRoot) {
        LOG_WAR("Render skipped: Canvas not shown or context/scene invalid");
        return;
    }

    if (m_isRendering) {
        LOG_WAR("Render called while already rendering");
        return;
    }

    wxLongLong currentTime = wxGetLocalTimeMillis();
    if (currentTime - m_lastRenderTime < RENDER_INTERVAL) {
        return;
    }

    m_isRendering = true;
    m_lastRenderTime = currentTime;

    try {
        if (!SetCurrent(*m_glContext)) {
            LOG_ERR("Failed to set GL context during render");
            m_isRendering = false;
            return;
        }

        wxSize size = GetClientSize();
        if (size.x <= 0 || size.y <= 0) {
            LOG_WAR("Invalid viewport size: " + std::to_string(size.x) + "x" + std::to_string(size.y));
            m_isRendering = false;
            return;
        }

        glViewport(0, 0, size.x, size.y);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (m_camera) {
            float aspect = static_cast<float>(size.x) / static_cast<float>(size.y);
            m_camera->aspectRatio.setValue(aspect);
        }

        SbViewportRegion viewport(size.x, size.y);
        SoGLRenderAction renderAction(viewport);
        renderAction.setSmoothing(!fastMode);
        renderAction.setNumPasses(fastMode ? 1 : 2);
        renderAction.setTransparencyType(
            fastMode ? SoGLRenderAction::BLEND : SoGLRenderAction::SORTED_OBJECT_BLEND
        );

        renderAction.apply(m_sceneRoot);
        SwapBuffers();
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during render: " + std::string(e.what()));
    }

    m_isRendering = false;
}

void Canvas::resetView()
{
    if (!m_camera || !m_sceneRoot || !m_glContext || !SetCurrent(*m_glContext)) {
        LOG_ERR("Failed to reset view: Invalid camera or context");
        return;
    }

    m_camera->position.setValue(0.0f, 0.0f, 5.0f);
    m_camera->orientation.setValue(SbVec3f(0, 0, -1), 0);
    m_camera->focalDistance.setValue(5.0f);

    SbViewportRegion viewport(GetClientSize().x, GetClientSize().y);
    m_camera->viewAll(m_sceneRoot, viewport);

    Refresh(true);
    Update();
}

void Canvas::toggleCameraMode()
{
    if (!m_sceneRoot || !m_camera || !m_glContext || !SetCurrent(*m_glContext)) {
        LOG_ERR("Failed to toggle camera mode: Invalid context or scene");
        return;
    }

    SbVec3f position = m_camera->position.getValue();
    SbRotation orientation = m_camera->orientation.getValue();
    float focalDistance = m_camera->focalDistance.getValue();

    m_sceneRoot->removeChild(m_camera);
    m_camera->unref();

    m_isPerspectiveCamera = !m_isPerspectiveCamera;
    if (m_isPerspectiveCamera) {
        m_camera = new SoPerspectiveCamera;
    }
    else {
        m_camera = new SoOrthographicCamera;
    }
    if (!m_camera) {
        LOG_ERR("Failed to create new camera");
        return;
    }
    m_camera->ref();

    m_camera->position.setValue(position);
    m_camera->orientation.setValue(orientation);
    m_camera->focalDistance.setValue(focalDistance);
    m_camera->nearDistance.setValue(0.1f);
    m_camera->farDistance.setValue(100.0f);

    wxSize size = GetClientSize();
    if (size.x > 0 && size.y > 0) {
        float aspect = static_cast<float>(size.x) / static_cast<float>(size.y);
        m_camera->aspectRatio.setValue(aspect);
    }

    m_sceneRoot->insertChild(m_camera, 0);
    Refresh(true);
    Update();
}

void Canvas::cleanup() {
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
    if (m_pickingAidSeparator)
    {
        m_pickingAidSeparator->unref();
        m_pickingAidSeparator = nullptr;
    }
}

Canvas::~Canvas()
{
    LOG_INF("Canvas destroying");
    if (m_glContext && SetCurrent(*m_glContext)) {
        cleanup();
    }
    delete m_glContext;
}

void Canvas::setMouseHandler(std::unique_ptr<MouseHandler> mouseHandler)
{
    m_mouseHandler = std::move(mouseHandler);
    LOG_INF("MouseHandler set for Canvas (unique_ptr)");
}

void Canvas::setMouseHandler(MouseHandler* mouseHandler)
{
    m_mouseHandler = std::unique_ptr<MouseHandler>(mouseHandler);
    LOG_INF("MouseHandler set for Canvas (raw pointer)");
}

void Canvas::setNavigationStyle(std::unique_ptr<NavigationStyle> navStyle)
{
    m_navStyle = std::move(navStyle);
    LOG_INF("NavigationStyle set for Canvas (unique_ptr)");
}

void Canvas::setNavigationStyle(NavigationStyle* navStyle)
{
    m_navStyle = std::unique_ptr<NavigationStyle>(navStyle);
    LOG_INF("NavigationStyle set for Canvas (raw pointer)");
}

void Canvas::onPaint(wxPaintEvent& event)
{
    if (!m_glContext || !m_sceneRoot) {
        LOG_WAR("Paint event skipped: Invalid context or scene");
        event.Skip();
        return;
    }

    wxPaintDC dc(this);
    render(false);
    event.Skip();
}

void Canvas::onSize(wxSizeEvent& event)
{
    wxSize size = event.GetSize();
    static wxSize lastSize(-1, -1);
    static wxLongLong lastEventTime = 0;
    wxLongLong currentTime = wxGetLocalTimeMillis();

    if (size == lastSize && (currentTime - lastEventTime) < 100) {
        LOG_DBG("Redundant size event ignored: " + std::to_string(size.x) + "x" + std::to_string(size.y));
        event.Skip();
        return;
    }

    lastSize = size;
    lastEventTime = currentTime;
    LOG_INF("Handling size event: " + std::to_string(size.x) + "x" + std::to_string(size.y));

    if (size.x > 0 && size.y > 0 && m_camera && m_glContext && SetCurrent(*m_glContext)) {
        float aspect = static_cast<float>(size.x) / static_cast<float>(size.y);
        m_camera->aspectRatio.setValue(aspect);
        Refresh();
    }
    else {
        LOG_WAR("Size event skipped: Invalid size or context");
    }
    event.Skip();
}

void Canvas::onEraseBackground(wxEraseEvent& event)
{
    // Do nothing to prevent flickering
}

void Canvas::onMouseButton(wxMouseEvent& event) {
    if (!m_mouseHandler || !m_glContext || !SetCurrent(*m_glContext)) {
        LOG_WAR("Mouse button event skipped: Invalid handler or context");
        event.Skip();
        return;
    }
    if (g_isPickingPosition && event.LeftDown()) {
        LOG_INF("Picking position with mouse click");
        SbVec3f worldPos;
        if (screenToWorld(event.GetPosition(), worldPos)) {
            LOG_INF("Picked position: " + std::to_string(worldPos[0]) + ", " + std::to_string(worldPos[1]) + ", " + std::to_string(worldPos[2]));

            wxWindow* dialog = wxWindow::FindWindowByName("PositionDialog");
            if (dialog) {
                PositionDialog* posDialog = dynamic_cast<PositionDialog*>(dialog);
                if (posDialog) {
                    posDialog->SetPosition(worldPos);

                    posDialog->Show(true);
                    LOG_INF("Position dialog updated and shown");

                    g_isPickingPosition = false;
                }
                else {
                    LOG_ERR("Failed to cast dialog to PositionDialog");
                }
            }
            else {
                LOG_ERR("PositionDialog not found");
            }
        }
        else {
            LOG_WAR("Failed to convert screen position to world coordinates");
        }

    }
    m_mouseHandler->handleMouseButton(event);
    event.Skip();
}

void Canvas::onMouseMotion(wxMouseEvent& event)
{
    if (!m_mouseHandler || !m_glContext || !SetCurrent(*m_glContext)) {
        LOG_WAR("Mouse motion event skipped: Invalid handler or context");
        event.Skip();
        return;
    }

    static wxLongLong lastMotionTime = 0;
    wxLongLong currentTime = wxGetLocalTimeMillis();
    if (currentTime - lastMotionTime >= MOTION_INTERVAL) {
        m_mouseHandler->handleMouseMotion(event);
        lastMotionTime = currentTime;
    }
    event.Skip();
}

void Canvas::onMouseWheel(wxMouseEvent& event)
{
    if (!m_navStyle || !m_glContext || !SetCurrent(*m_glContext)) {
        LOG_WAR("Mouse wheel event skipped: Invalid navigation or context");
        event.Skip();
        return;
    }

    static float accumulatedDelta = 0.0f;
    accumulatedDelta += event.GetWheelRotation();

    if (std::abs(accumulatedDelta) >= event.GetWheelDelta()) {
        m_navStyle->handleMouseWheel(event);
        accumulatedDelta -= std::copysign(event.GetWheelDelta(), accumulatedDelta);
    }
    event.Skip();
}

void Canvas::createCoordinateSystem()
{
    SoSeparator* coordSystemSep = new SoSeparator;
    SoTransform* originTransform = new SoTransform;
    originTransform->translation.setValue(0.0f, 0.0f, 0.0f);
    originTransform->rotation.setValue(SbRotation::identity());
    originTransform->scaleFactor.setValue(1.0f, 1.0f, 1.0f);
    coordSystemSep->addChild(originTransform);

    SoShapeHints* hints = new SoShapeHints;
    hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    hints->shapeType = SoShapeHints::SOLID;
    coordSystemSep->addChild(hints);

    SoDrawStyle* globalStyle = new SoDrawStyle;
    globalStyle->linePattern = 0xFFFF;
    globalStyle->lineWidth = 1.0f;
    globalStyle->pointSize = 1.0f;
    coordSystemSep->addChild(globalStyle);

    // X plane (YZ plane)
    SoSeparator* xPlaneSep = new SoSeparator;
    SoMaterial* xMaterial = new SoMaterial;
    xMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    xMaterial->transparency.setValue(COORD_PLANE_TRANSPARENCY);
    xPlaneSep->addChild(xMaterial);

    SoDrawStyle* xDrawStyle = new SoDrawStyle;
    xDrawStyle->style = SoDrawStyle::FILLED;
    xPlaneSep->addChild(xDrawStyle);

    SoFaceSet* xFaceSet = new SoFaceSet;
    SoVertexProperty* xVertices = new SoVertexProperty;
    float s = COORD_PLANE_SIZE / 2.0f;
    xVertices->vertex.set1Value(0, SbVec3f(0.0f, -s, -s));
    xVertices->vertex.set1Value(1, SbVec3f(0.0f, s, -s));
    xVertices->vertex.set1Value(2, SbVec3f(0.0f, s, s));
    xVertices->vertex.set1Value(3, SbVec3f(0.0f, -s, s));
    xFaceSet->vertexProperty = xVertices;
    xFaceSet->numVertices.set1Value(0, 4);
    xPlaneSep->addChild(xFaceSet);

    SoSeparator* xLineSep = new SoSeparator;
    SoMaterial* xLineMaterial = new SoMaterial;
    xLineMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    xLineMaterial->transparency.setValue(0.0f);
    xLineSep->addChild(xLineMaterial);

    SoDrawStyle* xLineStyle = new SoDrawStyle;
    xLineStyle->style = SoDrawStyle::LINES;
    xLineStyle->lineWidth = 1.0f;
    xLineSep->addChild(xLineStyle);

    SoIndexedLineSet* xLines = new SoIndexedLineSet;
    xLines->vertexProperty = xVertices;
    xLines->coordIndex.set1Value(0, 0);
    xLines->coordIndex.set1Value(1, 1);
    xLines->coordIndex.set1Value(2, 2);
    xLines->coordIndex.set1Value(3, 3);
    xLines->coordIndex.set1Value(4, 0);
    xLines->coordIndex.set1Value(5, -1);
    xLineSep->addChild(xLines);
    xPlaneSep->addChild(xLineSep);
    coordSystemSep->addChild(xPlaneSep);

    // Y plane (XZ plane)
    SoSeparator* yPlaneSep = new SoSeparator;
    SoMaterial* yMaterial = new SoMaterial;
    yMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    yMaterial->transparency.setValue(COORD_PLANE_TRANSPARENCY);
    yPlaneSep->addChild(yMaterial);

    SoDrawStyle* yDrawStyle = new SoDrawStyle;
    yDrawStyle->style = SoDrawStyle::FILLED;
    yPlaneSep->addChild(yDrawStyle);

    SoFaceSet* yFaceSet = new SoFaceSet;
    SoVertexProperty* yVertices = new SoVertexProperty;
    yVertices->vertex.set1Value(0, SbVec3f(-s, 0.0f, -s));
    yVertices->vertex.set1Value(1, SbVec3f(s, 0.0f, -s));
    yVertices->vertex.set1Value(2, SbVec3f(s, 0.0f, s));
    yVertices->vertex.set1Value(3, SbVec3f(-s, 0.0f, s));
    yFaceSet->vertexProperty = yVertices;
    yFaceSet->numVertices.set1Value(0, 4);
    yPlaneSep->addChild(yFaceSet);

    SoSeparator* yLineSep = new SoSeparator;
    SoMaterial* yLineMaterial = new SoMaterial;
    yLineMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    yLineMaterial->transparency.setValue(0.0f);
    yLineSep->addChild(yLineMaterial);

    SoDrawStyle* yLineStyle = new SoDrawStyle;
    yLineStyle->style = SoDrawStyle::LINES;
    yLineStyle->lineWidth = 1.0f;
    yLineSep->addChild(yLineStyle);

    SoIndexedLineSet* yLines = new SoIndexedLineSet;
    yLines->vertexProperty = yVertices;
    yLines->coordIndex.set1Value(0, 0);
    yLines->coordIndex.set1Value(1, 1);
    yLines->coordIndex.set1Value(2, 2);
    yLines->coordIndex.set1Value(3, 3);
    yLines->coordIndex.set1Value(4, 0);
    yLines->coordIndex.set1Value(5, -1);
    yLineSep->addChild(yLines);
    yPlaneSep->addChild(yLineSep);
    coordSystemSep->addChild(yPlaneSep);

    // Z plane (XY plane)
    SoSeparator* zPlaneSep = new SoSeparator;
    SoMaterial* zMaterial = new SoMaterial;
    zMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    zMaterial->transparency.setValue(COORD_PLANE_TRANSPARENCY);
    zPlaneSep->addChild(zMaterial);

    SoDrawStyle* zDrawStyle = new SoDrawStyle;
    zDrawStyle->style = SoDrawStyle::FILLED;
    zPlaneSep->addChild(zDrawStyle);

    SoFaceSet* zFaceSet = new SoFaceSet;
    SoVertexProperty* zVertices = new SoVertexProperty;
    zVertices->vertex.set1Value(0, SbVec3f(-s, -s, 0.0f));
    zVertices->vertex.set1Value(1, SbVec3f(s, -s, 0.0f));
    zVertices->vertex.set1Value(2, SbVec3f(s, s, 0.0f));
    zVertices->vertex.set1Value(3, SbVec3f(-s, s, 0.0f));
    zFaceSet->vertexProperty = zVertices;
    zFaceSet->numVertices.set1Value(0, 4);
    zPlaneSep->addChild(zFaceSet);

    SoSeparator* zLineSep = new SoSeparator;
    SoMaterial* zLineMaterial = new SoMaterial;
    zLineMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    zLineMaterial->transparency.setValue(0.0f);
    zLineSep->addChild(zLineMaterial);

    SoDrawStyle* zLineStyle = new SoDrawStyle;
    zLineStyle->style = SoDrawStyle::LINES;
    zLineStyle->lineWidth = 1.0f;
    zLineSep->addChild(zLineStyle);

    SoIndexedLineSet* zLines = new SoIndexedLineSet;
    zLines->vertexProperty = zVertices;
    zLines->coordIndex.set1Value(0, 0);
    zLines->coordIndex.set1Value(1, 1);
    zLines->coordIndex.set1Value(2, 2);
    zLines->coordIndex.set1Value(3, 3);
    zLines->coordIndex.set1Value(4, 0);
    zLines->coordIndex.set1Value(5, -1);
    zLineSep->addChild(zLines);
    zPlaneSep->addChild(zLineSep);
    coordSystemSep->addChild(zPlaneSep);

    // X axis
    SoSeparator* xAxisSep = new SoSeparator;
    SoMaterial* xAxisMaterial = new SoMaterial;
    xAxisMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    xAxisMaterial->transparency.setValue(0.0f);
    xAxisSep->addChild(xAxisMaterial);

    SoDrawStyle* xAxisStyle = new SoDrawStyle;
    xAxisStyle->lineWidth = 1.0f;
    xAxisSep->addChild(xAxisStyle);

    SoCoordinate3* xAxisCoords = new SoCoordinate3;
    xAxisCoords->point.set1Value(0, SbVec3f(-s, 0.0f, 0.0f));
    xAxisCoords->point.set1Value(1, SbVec3f(s, 0.0f, 0.0f));
    xAxisSep->addChild(xAxisCoords);

    SoLineSet* xAxisLine = new SoLineSet;
    xAxisLine->numVertices.setValue(2);
    xAxisSep->addChild(xAxisLine);
    coordSystemSep->addChild(xAxisSep);

    // Y axis
    SoSeparator* yAxisSep = new SoSeparator;
    SoMaterial* yAxisMaterial = new SoMaterial;
    yAxisMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    yAxisMaterial->transparency.setValue(0.0f);
    yAxisSep->addChild(yAxisMaterial);

    SoDrawStyle* yAxisStyle = new SoDrawStyle;
    yAxisStyle->lineWidth = 1.0f;
    yAxisSep->addChild(yAxisStyle);

    SoCoordinate3* yAxisCoords = new SoCoordinate3;
    yAxisCoords->point.set1Value(0, SbVec3f(0.0f, -s, 0.0f));
    yAxisCoords->point.set1Value(1, SbVec3f(0.0f, s, 0.0f));
    yAxisSep->addChild(yAxisCoords);

    SoLineSet* yAxisLine = new SoLineSet;
    yAxisLine->numVertices.setValue(2);
    yAxisSep->addChild(yAxisLine);
    coordSystemSep->addChild(yAxisSep);

    // Z axis
    SoSeparator* zAxisSep = new SoSeparator;
    SoMaterial* zAxisMaterial = new SoMaterial;
    zAxisMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    zAxisMaterial->transparency.setValue(0.0f);
    zAxisSep->addChild(zAxisMaterial);

    SoDrawStyle* zAxisStyle = new SoDrawStyle;
    zAxisStyle->lineWidth = 1.0f;
    zAxisSep->addChild(zAxisStyle);

    SoCoordinate3* zAxisCoords = new SoCoordinate3;
    zAxisCoords->point.set1Value(0, SbVec3f(0.0f, 0.0f, -s));
    zAxisCoords->point.set1Value(1, SbVec3f(0.0f, 0.0f, s));
    zAxisSep->addChild(zAxisCoords);

    SoLineSet* zAxisLine = new SoLineSet;
    zAxisLine->numVertices.setValue(2);
    zAxisSep->addChild(zAxisLine);
    coordSystemSep->addChild(zAxisSep);

    m_objectRoot->addChild(coordSystemSep);
}

void Canvas::AddTestCube()
{
    LOG_INF("Adding test cube to canvas");
    SoSeparator* cubeSeparator = new SoSeparator;

    // Shape hints for correct normals
    SoShapeHints* hints = new SoShapeHints;
    hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    hints->shapeType = SoShapeHints::SOLID;
    cubeSeparator->addChild(hints);

    // Filled cube (red)
    SoSeparator* fillSep = new SoSeparator;
    SoMaterial* cubeMaterial = new SoMaterial;
    cubeMaterial->diffuseColor.setValue(1.0f, 0.0f, 0.0f); // Red
    cubeMaterial->transparency.setValue(0.0f);
    fillSep->addChild(cubeMaterial);
    SoDrawStyle* fillStyle = new SoDrawStyle;
    fillStyle->style = SoDrawStyle::FILLED;
    fillSep->addChild(fillStyle);
    SoCube* cube = new SoCube;
    cube->width.setValue(0.5f);
    cube->height.setValue(0.5f);
    cube->depth.setValue(0.5f);
    fillSep->addChild(cube);
    cubeSeparator->addChild(fillSep);

    // Wireframe cube (blue)
    SoSeparator* wireSep = new SoSeparator;
    SoMaterial* wireframeMaterial = new SoMaterial;
    wireframeMaterial->diffuseColor.setValue(0.5f, 0.8f, 1.0f); // Blue
    wireframeMaterial->transparency.setValue(0.0f);
    wireSep->addChild(wireframeMaterial);
    SoDrawStyle* wireframeStyle = new SoDrawStyle;
    wireframeStyle->style = SoDrawStyle::LINES;
    wireframeStyle->lineWidth = 1.0f;
    wireSep->addChild(wireframeStyle);
    SoCube* wireCube = new SoCube;
    wireCube->width.setValue(0.5f);
    wireCube->height.setValue(0.5f);
    wireCube->depth.setValue(0.5f);
    wireSep->addChild(wireCube);
    cubeSeparator->addChild(wireSep);

    m_objectRoot->addChild(cubeSeparator);
    Refresh(true);
    Update();
}

bool Canvas::screenToWorld(const wxPoint& screenPos, SbVec3f& worldPos)
{
    if (!m_camera || !m_glContext || !SetCurrent(*m_glContext)) {
        LOG_ERR("Cannot convert screen to world: Invalid camera or context");
        return false;
    }

    wxSize size = GetClientSize();
    float x = static_cast<float>(screenPos.x) / size.GetWidth();
    float y = 1.0f - static_cast<float>(screenPos.y) / size.GetHeight();

    SbViewportRegion viewport(size.GetWidth(), size.GetHeight());
    SbVec2f normalizedPos(x, y);
    SbLine line;
    SbViewVolume viewVolume = m_camera->getViewVolume();
    viewVolume.projectPointToLine(normalizedPos, line);
    worldPos = line.getPosition();
    return true;
}

enum GeometryType {
    CUBE,
    SPHERE,
    CYLINDER
};

GeometryType g_selectedGeometryType = CUBE;

void Canvas::CreateGeometryAtPosition(const SbVec3f& position) {
    LOG_INF("Creating geometry at position: " + std::to_string(position[0]) + ", " + std::to_string(position[1]) + ", " + std::to_string(position[2]));
    SoSeparator* geometrySeparator = new SoSeparator;
    SoTransform* transform = new SoTransform;
    transform->translation.setValue(position);
    geometrySeparator->addChild(transform);
    SoMaterial* material = new SoMaterial;
    material->diffuseColor.setValue(0.0, 1.0, 0.0);
    geometrySeparator->addChild(material);
    SoNode* geometry = nullptr;
    std::string geometryType = getCreationGeometryType();
    LOG_INF("Creating geometry of type: " + geometryType);
    if (geometryType == "Box") {
        SoCube* cube = new SoCube;
        cube->width.setValue(0.3);
        cube->height.setValue(0.3);
        cube->depth.setValue(0.3);
        geometry = cube;
    }
    else if (geometryType == "Sphere") {
        SoSphere* sphere = new SoSphere;
        sphere->radius.setValue(0.15);
        geometry = sphere;
    }
    else if (geometryType == "Cylinder") {
        SoCylinder* cylinder = new SoCylinder;
        cylinder->radius.setValue(0.15);
        cylinder->height.setValue(0.3);
        geometry = cylinder;
    }
    else if (geometryType == "Cone") {
        SoCone* cone = new SoCone;
        cone->bottomRadius.setValue(0.15);
        cone->height.setValue(0.3); 
        geometry = cone;
    }
    else {
        LOG_WAR("Unknown geometry type: " + geometryType + ", using Box as fallback");
        SoCube* cube = new SoCube;
        cube->width.setValue(0.3);
        cube->height.setValue(0.3);
        cube->depth.setValue(0.3);
        geometry = cube;
    }
    if (geometry) { geometrySeparator->addChild(geometry); }
    m_objectRoot->addChild(geometrySeparator);
    LOG_INF("Geometry added to scene graph");
    if (m_mouseHandler) {
        LOG_INF("Resetting operation mode to NAVIGATE after geometry creation");
        m_mouseHandler->setOperationMode(MouseHandler::NAVIGATE);
        m_mouseHandler->setCreationGeometryType("");
    }
    else {
        LOG_WAR("Unable to reset operation mode: MouseHandler is null");
    }        Refresh(true);
    Update();
}

std::string Canvas::getCreationGeometryType() const {
    if (m_mouseHandler) {
        return m_mouseHandler->getCreationGeometryType();
    }    return "Box";
}

void Canvas::createPickingAidLines() {
    m_pickingAidSeparator = new SoSeparator;
    m_pickingAidSeparator->ref();

    m_pickingAidTransform = new SoTransform;
    m_pickingAidSeparator->addChild(m_pickingAidTransform);
    SoDrawStyle* lineStyle = new SoDrawStyle;
    lineStyle->lineWidth.setValue(1.0f);
    lineStyle->linePattern.setValue(0xFFFF);
    m_pickingAidSeparator->addChild(lineStyle);
    SoSeparator* xLineSep = new SoSeparator;
    SoMaterial* xMaterial = new SoMaterial;
    xMaterial->diffuseColor.setValue(0.0f, 1.0f, 0.0f);
    xLineSep->addChild(xMaterial);

    SoCoordinate3* xCoords = new SoCoordinate3;
    xCoords->point.set1Value(0, SbVec3f(-1000.0f, 0.0f, 0.0f));
    xCoords->point.set1Value(1, SbVec3f(1000.0f, 0.0f, 0.0f));
    xLineSep->addChild(xCoords);
    SoLineSet* xLine = new SoLineSet;
    xLine->numVertices.setValue(2);
    xLineSep->addChild(xLine);
    m_pickingAidSeparator->addChild(xLineSep);
    SoSeparator* yLineSep = new SoSeparator;
    SoMaterial* yMaterial = new SoMaterial;
    yMaterial->diffuseColor.setValue(0.0f, 1.0f, 0.0f);
    yLineSep->addChild(yMaterial);
    SoCoordinate3* yCoords = new SoCoordinate3;
    yCoords->point.set1Value(0, SbVec3f(0.0f, -1000.0f, 0.0f));
    yCoords->point.set1Value(1, SbVec3f(0.0f, 1000.0f, 0.0f));
    yLineSep->addChild(yCoords);
    SoLineSet* yLine = new SoLineSet;
    yLine->numVertices.setValue(2);
    yLineSep->addChild(yLine);
    m_pickingAidSeparator->addChild(yLineSep);
    SoSeparator* zLineSep = new SoSeparator;
    SoMaterial* zMaterial = new SoMaterial;
    zMaterial->diffuseColor.setValue(0.0f, 1.0f, 0.0f);
    zLineSep->addChild(zMaterial);
    SoCoordinate3* zCoords = new SoCoordinate3;
    zCoords->point.set1Value(0, SbVec3f(0.0f, 0.0f, -1000.0f));
    zCoords->point.set1Value(1, SbVec3f(0.0f, 0.0f, 1000.0f));
    zLineSep->addChild(zCoords);
    SoLineSet* zLine = new SoLineSet;

    zLine->numVertices.setValue(2);
    zLineSep->addChild(zLine);
    m_pickingAidSeparator->addChild(zLineSep);
    SoSeparator* centerSep = new SoSeparator;
    SoMaterial* centerMaterial = new SoMaterial;
    centerMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
    centerSep->addChild(centerMaterial);
    SoDrawStyle* pointStyle = new SoDrawStyle;
    pointStyle->pointSize.setValue(5.0f);
    centerSep->addChild(pointStyle);
    SoCoordinate3* centerCoord = new SoCoordinate3;
    centerCoord->point.set1Value(0,SbVec3f(0.0f, 0.0f, 0.0f));
    centerSep->addChild(centerCoord);
    SoPointSet* centerPoint = new SoPointSet;
    centerSep->addChild(centerPoint); 
    m_pickingAidSeparator->addChild(centerSep);
    m_pickingAidVisible = false;
}

void Canvas::showPickingAidLines(const SbVec3f& position) {
    if (!m_pickingAidSeparator) {
        m_pickingAidSeparator = new SoSeparator;
        m_pickingAidSeparator->ref();
        SoTransform* transform = new SoTransform;
        transform->translation.setValue(position);
        m_pickingAidSeparator->addChild(transform);
        SoSeparator* xLineSep = new SoSeparator;
        SoMaterial* xMaterial = new SoMaterial;
        xMaterial->diffuseColor.setValue(0.0f, 1.0f, 0.0f); 
        xLineSep->addChild(xMaterial);
        SoDrawStyle* xDrawStyle = new SoDrawStyle;
        xDrawStyle->lineWidth.setValue(1.0f);
        xLineSep->addChild(xDrawStyle);
        SoCoordinate3* xCoords = new SoCoordinate3;
        xCoords->point.set1Value(0, -1000.0f, 0.0f, 0.0f);
        xCoords->point.set1Value(1, 1000.0f, 0.0f, 0.0f);
        xLineSep->addChild(xCoords);
        SoLineSet* xLineSet = new SoLineSet;
        xLineSet->numVertices.setValue(2);
        xLineSep->addChild(xLineSet);
        m_pickingAidSeparator->addChild(xLineSep);
        SoSeparator* yLineSep = new SoSeparator;
        SoMaterial* yMaterial = new SoMaterial;
        yMaterial->diffuseColor.setValue(0.0f, 1.0f, 0.0f);
        yLineSep->addChild(yMaterial);
        SoDrawStyle* yDrawStyle = new SoDrawStyle;
        yDrawStyle->lineWidth.setValue(1.0f);
        yLineSep->addChild(yDrawStyle);
        SoCoordinate3* yCoords = new SoCoordinate3;
        yCoords->point.set1Value(0, 0.0f, -1000.0f, 0.0f); 
        yCoords->point.set1Value(1, 0.0f, 1000.0f, 0.0f);
        yLineSep->addChild(yCoords);
        SoLineSet* yLineSet = new SoLineSet;
        yLineSet->numVertices.setValue(2);
        yLineSep->addChild(yLineSet);
        m_pickingAidSeparator->addChild(yLineSep);
        SoSeparator* zLineSep = new SoSeparator;
        SoMaterial* zMaterial = new SoMaterial;
        zMaterial->diffuseColor.setValue(0.0f, 1.0f, 0.0f); 
        zLineSep->addChild(zMaterial);
        SoDrawStyle* zDrawStyle = new SoDrawStyle;
        zDrawStyle->lineWidth.setValue(1.0f);
        zLineSep->addChild(zDrawStyle);
        SoCoordinate3* zCoords = new SoCoordinate3;
        zCoords->point.set1Value(0, 0.0f, 0.0f, -1000.0f); 
        zCoords->point.set1Value(1, 0.0f, 0.0f, 1000.0f);
        zLineSep->addChild(zCoords);
        SoLineSet* zLineSet = new SoLineSet;
        zLineSet->numVertices.setValue(2);
        zLineSep->addChild(zLineSet);
        m_pickingAidSeparator->addChild(zLineSep);
        SoSeparator* textSep = new SoSeparator;
        SoMaterial* textMaterial = new SoMaterial;
        textMaterial->diffuseColor.setValue(0.0f, 1.0f, 0.0f); 
        textSep->addChild(textMaterial);
        SoTransform* textTransform = new SoTransform;
        textTransform->translation.setValue(0.1f, 0.1f, 0.1f); 
        textSep->addChild(textTransform);
        SoText2* coordText = new SoText2;
        char coordStr[64];
        snprintf(coordStr, sizeof(coordStr), "(%.2f, %.2f, %.2f)", position[0], position[1], position[2]);
        coordText->string.setValue(coordStr);
        textSep->addChild(coordText);
        m_pickingAidSeparator->addChild(textSep);
        m_sceneRoot->addChild(m_pickingAidSeparator);
        m_pickingAidVisible = true;
    } else {
        SoTransform* transform = dynamic_cast<SoTransform*>(m_pickingAidSeparator->getChild(0));
        if (transform) {
            transform->translation.setValue(position);
        }
        SoSeparator* textSep = nullptr;
        int lastChildIndex = m_pickingAidSeparator->getNumChildren() - 1;
        if (lastChildIndex >= 0) {
            textSep = dynamic_cast<SoSeparator*>(m_pickingAidSeparator->getChild(lastChildIndex));
            if (!textSep || textSep->getNumChildren() == 0 || !dynamic_cast<SoText2*>(textSep->getChild(textSep->getNumChildren() - 1))) {
                textSep = new SoSeparator;
                SoMaterial* textMaterial = new SoMaterial;
                textMaterial->diffuseColor.setValue(0.0f, 1.0f, 0.0f); 
                textSep->addChild(textMaterial);
                SoTransform* textTransform = new SoTransform;
                textTransform->translation.setValue(0.1f, 0.1f, 0.1f);
                textSep->addChild(textTransform);
                SoText2* coordText = new SoText2;
                textSep->addChild(coordText);
                m_pickingAidSeparator->addChild(textSep);
            }
            SoText2* coordText = dynamic_cast<SoText2*>(textSep->getChild(textSep->getNumChildren() - 1));
            if (coordText) {
                char coordStr[64];
                snprintf(coordStr, sizeof(coordStr), "(%.2f, %.2f, %.2f)", position[0], position[1], position[2]);
                coordText->string.setValue(coordStr);
            }
        }
        if (!m_pickingAidVisible) {
            m_sceneRoot->addChild(m_pickingAidSeparator);
            m_pickingAidVisible = true;
        }
    }
    setPickingCursor(true);
    Refresh(true);
    Update();
}

void Canvas::hidePickingAidLines() {
    if (!m_pickingAidSeparator || !m_pickingAidVisible) { return; }
    m_sceneRoot->removeChild(m_pickingAidSeparator);
    m_pickingAidVisible = false;
    setPickingCursor(false);
    Refresh(true);
}

void Canvas::setPickingCursor(bool enable) {
    if (enable) {
        SetCursor(wxCursor(wxCURSOR_CROSS));
    }
    else {
        SetCursor(wxCursor(wxCURSOR_DEFAULT));
    }
}