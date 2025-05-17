#include "Canvas.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "NavigationCube.h"
#include "ObjectTreePanel.h"
#include "Logger.h"
#include <wx/dcclient.h>
#include <wx/msgdlg.h>
#include "NavigationCubeConfigDialog.h"

const int Canvas::RENDER_INTERVAL = 16; // ~60 FPS (milliseconds)
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
EVT_LEFT_DOWN(Canvas::onMouseEvent)
EVT_LEFT_UP(Canvas::onMouseEvent)
EVT_RIGHT_DOWN(Canvas::onMouseEvent)
EVT_RIGHT_UP(Canvas::onMouseEvent)
EVT_MOTION(Canvas::onMouseEvent)
EVT_MOUSEWHEEL(Canvas::onMouseEvent)
END_EVENT_TABLE()

Canvas::Canvas(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size)
    : wxGLCanvas(parent, id, s_canvasAttribs, pos, size, wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS | wxBORDER_NONE)
    , m_glContext(nullptr)
    , m_isRendering(false)
    , m_isInitialized(false)
    , m_lastRenderTime(0)
    , m_objectTreePanel(nullptr)
    , m_commandManager(nullptr)
    , m_dpiScale(1.0f)
    , m_enableNavCube(true) // Default: enable navigation cube
{
    LOG_INF("Canvas::Canvas: Initializing");

    SetName("Canvas");
    wxSize clientSize = GetClientSize();
    if (clientSize.x <= 0 || clientSize.y <= 0) {
        clientSize = wxSize(400, 300);
        SetSize(clientSize);
        SetMinSize(clientSize);
    }

    try {
        m_glContext = new wxGLContext(this);
        if (!m_glContext || !SetCurrent(*m_glContext)) {
            LOG_ERR("Canvas::Canvas: Failed to create/set GL context");
            showErrorDialog("Failed to initialize OpenGL context. Please check your graphics drivers.");
            throw std::runtime_error("GL context initialization failed");
        }

        m_dpiScale = GetContentScaleFactor();
        LOG_INF("Canvas::Canvas: DPI scale factor: " + std::to_string(m_dpiScale));

        m_sceneManager = std::make_unique<SceneManager>(this);
        m_inputManager = std::make_unique<InputManager>(this);

        // Initialize navigation cube only if enabled
        if (m_enableNavCube) {
            auto cubeCallback = [this](const std::string& view) {
                m_sceneManager->setView(view);
                Refresh(true);
                };
            m_navCube = std::make_unique<NavigationCube>(cubeCallback, m_dpiScale, clientSize.x, clientSize.y);
            m_navCube->setRotationChangedCallback([this]() {
                SyncMainCameraToNavigationCube();
                Refresh(true);
            });
            LOG_INF("Canvas::Canvas: Navigation cube initialized");
        }

        // Initialize layout positions (set navigation cube to top-right)
        if (clientSize.x > 0 && clientSize.y > 0) {
            m_cubeLayout.size = 120; // Default cube size
            m_cubeLayout.update(clientSize.x - m_cubeLayout.size - 10, // Right edge - size - margin
                                clientSize.y - m_cubeLayout.size - 10, // Top edge - margin 
                m_cubeLayout.size, clientSize, m_dpiScale);
            LOG_INF("Canvas::Canvas: Initialized navigation cube position: x=" + std::to_string(m_cubeLayout.x) +
                ", y=" + std::to_string(m_cubeLayout.y) + ", size=" + std::to_string(m_cubeLayout.size));
        }

        const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        LOG_INF("Canvas::Canvas: GL Context created. OpenGL version: " + std::string(glVersion ? glVersion : "unknown"));

        if (m_sceneManager && !m_sceneManager->initScene()) {
            LOG_ERR("Canvas::Canvas: Failed to initialize main scene");
            showErrorDialog("Failed to initialize 3D scene. The application may not function correctly.");
            throw std::runtime_error("Scene initialization failed");
        }

        m_isInitialized = true;
        Refresh(true);
        Update();
        LOG_INF("Canvas::Canvas: Initialized successfully");
    }
    catch (const std::exception& e) {
        LOG_ERR("Canvas::Canvas: Initialization failed: " + std::string(e.what()));
        throw;
    }
}

Canvas::~Canvas() {
    LOG_INF("Canvas::Canvas: Destroying");
    delete m_glContext;
}

void Canvas::showErrorDialog(const std::string& message) const {
    wxMessageDialog dialog(nullptr, message, "Error", wxOK | wxICON_ERROR);
    dialog.ShowModal();
}

void Canvas::render(bool fastMode) {
    if (!m_isInitialized) {
        LOG_WAR("Canvas::render: Skipped: Canvas not initialized");
        return;
    }

    if (!IsShown() || !m_glContext || !m_sceneManager) {
        LOG_WAR("Canvas::render: Skipped: Canvas not shown or context/scene invalid");
        return;
    }

    if (m_isRendering) {
        LOG_WAR("Canvas::render: Skipped: Already rendering");
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
            LOG_ERR("Canvas::render: Failed to set GL context");
            m_isRendering = false;
            showErrorDialog("Failed to set OpenGL context. Rendering cannot proceed.");
            return;
        }

        wxSize size = GetClientSize();
        if (size.x <= 0 || size.y <= 0) {
            LOG_WAR("Canvas::render: Invalid viewport size: " + std::to_string(size.x) + "x" + std::to_string(size.y));
            m_isRendering = false;
            return;
        }

        glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render main scene
        glViewport(0, 0, static_cast<int>(size.x * m_dpiScale), static_cast<int>(size.y * m_dpiScale));
        m_sceneManager->render(size, fastMode);

        SyncNavigationCubeCamera();

        // Render navigation cube
        if (m_navCube && m_navCube->isEnabled()) {
            int cubeX = static_cast<int>(m_cubeLayout.x * m_dpiScale);
            int cubeY = static_cast<int>(m_cubeLayout.y * m_dpiScale);
            m_navCube->render(cubeX, cubeY, wxSize(m_cubeLayout.size, m_cubeLayout.size));
            LOG_DBG("Canvas::render: Rendering navigation cube: x=" + std::to_string(m_cubeLayout.x) +
                ", y=" + std::to_string(m_cubeLayout.y) + ", size=" + std::to_string(m_cubeLayout.size) +
                ", dpiScale=" + std::to_string(m_dpiScale));
        }

        SwapBuffers();
    }
    catch (const std::exception& e) {
        LOG_ERR("Canvas::render: Exception during render: " + std::string(e.what()));
        glViewport(0, 0, GetClientSize().x, GetClientSize().y);
        glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
        m_isRendering = false;
        showErrorDialog("Rendering failed: " + std::string(e.what()) + ". Please check system resources or restart the application.");
    }

    m_isRendering = false;
}

void Canvas::onPaint(wxPaintEvent& event) {
    if (!m_isInitialized || !m_glContext || !m_sceneManager) {
        LOG_WAR("Canvas::onPaint: Skipped: Invalid context, scene, or initialization");
        event.Skip();
        return;
    }

    wxPaintDC dc(this);
    render(false);
    event.Skip();
}

void Canvas::onSize(wxSizeEvent& event) {
    wxSize size = event.GetSize();
    static wxSize lastSize(-1, -1);
    static wxLongLong lastEventTime = 0;
    wxLongLong currentTime = wxGetLocalTimeMillis();

    if (size == lastSize && (currentTime - lastEventTime) < 100) {
        LOG_DBG("Canvas::onSize: Redundant size event ignored: " + std::to_string(size.x) + "x" + std::to_string(size.y));
        event.Skip();
        return;
    }

    lastSize = size;
    lastEventTime = currentTime;
    LOG_INF("Canvas::onSize: Handling size event: " + std::to_string(size.x) + "x" + std::to_string(size.y));

    if (size.x > 0 && size.y > 0 && m_glContext && SetCurrent(*m_glContext)) {
        m_dpiScale = GetContentScaleFactor();
        // Update cube layout to maintain top-right position
        m_cubeLayout.update(size.x - m_cubeLayout.size - 10, // Right edge - size - margin
                            size.y - m_cubeLayout.size - 10, // Top edge - margin
            m_cubeLayout.size, size, m_dpiScale);
        m_sceneManager->updateAspectRatio(size);
        if (m_navCube) {
            m_navCube->setWindowSize(size.x, size.y);
        }
        Refresh();
    }
    else {
        LOG_WAR("Canvas::onSize: Skipped: Invalid size or context");
    }
    event.Skip();
}

void Canvas::onEraseBackground(wxEraseEvent& event) {
    // Do nothing to prevent flickering
}

void Canvas::onMouseEvent(wxMouseEvent& event) {
    if (!m_isInitialized || !m_inputManager) {
        LOG_WAR("Canvas::onMouseEvent: Skipped: Canvas not initialized or InputManager invalid");
        event.Skip();
        return;
    }

    // Adjust mouse coordinates for DPI scaling
    float x = event.GetX() / m_dpiScale;
    float y = event.GetY() / m_dpiScale;
    wxSize clientSize = GetClientSize();
    float cubeY = (clientSize.y / m_dpiScale) - y;

    // Log mouse coordinates for debugging
    LOG_DBG("Canvas::onMouseEvent: Mouse event at x=" + std::to_string(x) + ", y=" + std::to_string(y) +
        " (canvas), cubeY=" + std::to_string(cubeY) +
        ", cube region: x=[" + std::to_string(m_cubeLayout.x) + "," + std::to_string(m_cubeLayout.x + m_cubeLayout.size) +
        "], y=[" + std::to_string(m_cubeLayout.y) + "," + std::to_string(m_cubeLayout.y + m_cubeLayout.size) + "]");

    // Check if event is within navigation cube region (right-top corner)
    if (m_navCube && m_navCube->isEnabled()) {
        if (x >= m_cubeLayout.x && x < (m_cubeLayout.x + m_cubeLayout.size) &&
            cubeY >= m_cubeLayout.y && cubeY < (m_cubeLayout.y + m_cubeLayout.size)) {
            
            wxMouseEvent cubeEvent(event);
            // Calculate mouse position relative to the cube's top-left corner, then scale by DPI
            cubeEvent.m_x = static_cast<int>((x - m_cubeLayout.x) * m_dpiScale);
            cubeEvent.m_y = static_cast<int>(((m_cubeLayout.y + m_cubeLayout.size) - cubeY) * m_dpiScale);

            // Calculate the DPI-scaled size of the navigation cube's viewport
            int scaled_cube_dimension = static_cast<int>(m_cubeLayout.size * m_dpiScale);
            wxSize cube_viewport_scaled_size(scaled_cube_dimension, scaled_cube_dimension);

            if (event.GetEventType() == wxEVT_LEFT_DOWN ||
                event.GetEventType() == wxEVT_LEFT_UP ||
                event.GetEventType() == wxEVT_MOTION) {
                m_navCube->handleMouseEvent(cubeEvent, cube_viewport_scaled_size); // Pass scaled size
                Refresh(true);
                return;
            }
        }
    }
    // Forward other events to InputManager
    if (event.GetEventType() == wxEVT_LEFT_DOWN ||
        event.GetEventType() == wxEVT_LEFT_UP ||
        event.GetEventType() == wxEVT_RIGHT_DOWN ||
        event.GetEventType() == wxEVT_RIGHT_UP) {
        m_inputManager->onMouseButton(event);
    }
    else if (event.GetEventType() == wxEVT_MOTION) {
        m_inputManager->onMouseMotion(event);
    }
    else if (event.GetEventType() == wxEVT_MOUSEWHEEL) {
        m_inputManager->onMouseWheel(event);
    }
    else {
        event.Skip();
    }
}

void Canvas::setPickingCursor(bool enable) {
    if (!m_isInitialized) {
        LOG_WAR("Canvas::setPickingCursor: Skipped: Canvas not initialized");
        return;
    }
    SetCursor(enable ? wxCursor(wxCURSOR_CROSS) : wxCursor(wxCURSOR_DEFAULT));
}

SoCamera* Canvas::getCamera() const {
    if (!m_isInitialized || !m_sceneManager) {
        LOG_WAR("Canvas::getCamera: Invalid state or SceneManager");
        return nullptr;
    }
    return m_sceneManager->getCamera();
}

void Canvas::resetView() {
    if (!m_isInitialized || !m_sceneManager) {
        LOG_WAR("Canvas::resetView: Invalid state or SceneManager");
        return;
    }
    m_sceneManager->resetView();
}

void Canvas::setNavigationCubeEnabled(bool enabled) {
    if (!m_isInitialized) {
        LOG_WAR("Canvas::setNavigationCubeEnabled: Skipped: Canvas not initialized");
        return;
    }
    m_enableNavCube = enabled;
    if (enabled && !m_navCube) {
        try {
            auto cubeCallback = [this](const std::string& view) {
                m_sceneManager->setView(view);
                Refresh(true);
                };
            wxSize clientSize = GetClientSize();
            m_navCube = std::make_unique<NavigationCube>(cubeCallback, m_dpiScale, clientSize.x, clientSize.y);
            m_navCube->setRotationChangedCallback([this]() {
                SyncMainCameraToNavigationCube();
                Refresh(true);
            });
            
            m_cubeLayout.size = 120; 
            m_cubeLayout.update(clientSize.x - m_cubeLayout.size - 10, 
                                clientSize.y - m_cubeLayout.size - 10, 
                m_cubeLayout.size, clientSize, m_dpiScale);
            
            LOG_INF("Canvas::setNavigationCubeEnabled: Navigation cube initialized and positioned at: x=" + 
                    std::to_string(m_cubeLayout.x) + ", y=" + std::to_string(m_cubeLayout.y));
        }
        catch (const std::exception& e) {
            LOG_ERR("Canvas::setNavigationCubeEnabled: Failed to initialize navigation cube: " + std::string(e.what()));
            showErrorDialog("Failed to initialize navigation cube.");
            m_navCube.reset();
            m_enableNavCube = false;
        }
    }
    if (m_navCube) {
        m_navCube->setEnabled(enabled);
    }
    Refresh(true);
}

bool Canvas::isNavigationCubeEnabled() const {
    return m_isInitialized && m_enableNavCube && m_navCube && m_navCube->isEnabled();
}

void Canvas::SetNavigationCubeRect(int x, int y, int size) {
    if (!m_isInitialized) {
        LOG_WAR("Canvas::SetNavigationCubeRect: Skipped: Canvas not initialized");
        return;
    }
    if (size < 50 || x < 0 || y < 0) {
        LOG_WAR("Canvas::SetNavigationCubeRect: Invalid parameters: x=" + std::to_string(x) +
            ", y=" + std::to_string(y) + ", size=" + std::to_string(size));
        return;
    }
    wxSize clientSize = GetClientSize();
    m_cubeLayout.update(x, y, size, clientSize, m_dpiScale);
    Refresh(true);
    LOG_INF("Canvas::SetNavigationCubeRect: Set navigation cube rect: x=" + std::to_string(m_cubeLayout.x) +
        ", y=" + std::to_string(m_cubeLayout.y) + ", size=" + std::to_string(m_cubeLayout.size) +
        ", dpiScale=" + std::to_string(m_dpiScale));
}

void Canvas::SetNavigationCubeColor(const wxColour& color) {
    if (!m_isInitialized || !m_navCube) {
        LOG_WAR("Canvas::SetNavigationCubeColor: Skipped: Canvas not initialized or nav cube not created");
        return;
    }
    LOG_INF("Canvas::SetNavigationCubeColor: Set navigation cube color to R=" + std::to_string(color.GetRed()) +
        ", G=" + std::to_string(color.GetGreen()) + ", B=" + std::to_string(color.GetBlue()));
    Refresh(true);
}

void Canvas::SetNavigationCubeViewportSize(int size) {
    if (!m_isInitialized || !m_navCube) {
        LOG_WAR("Canvas::SetNavigationCubeViewportSize: Skipped: Canvas not initialized or nav cube not created");
        return;
    }
    if (size < 50) {
        LOG_WAR("Canvas::SetNavigationCubeViewportSize: Invalid size: " + std::to_string(size));
        return;
    }
    LOG_INF("Canvas::SetNavigationCubeViewportSize: Set navigation cube viewport size to " + std::to_string(size));
    Refresh(true);
}

void Canvas::SyncNavigationCubeCamera() {
    if (!m_isInitialized || !m_navCube || !m_sceneManager) {
        LOG_WAR("Canvas::SyncNavigationCubeCamera: Skipped: Canvas not initialized or components missing");
        return;
    }

    SoCamera* mainCamera = m_sceneManager->getCamera();
    if (mainCamera) {
        SbVec3f mainPos = mainCamera->position.getValue();
        SbRotation mainOrient = mainCamera->orientation.getValue();

        float distance = mainPos.length();
        float angleX = atan2(mainPos[1], sqrt(mainPos[0]*mainPos[0] + mainPos[2]*mainPos[2])) * 180.0f / M_PI;
        float angleY = atan2(mainPos[0], mainPos[2]) * 180.0f / M_PI;

        float navDistance = 5.0f; 
        float radX = angleX * M_PI / 180.0f;
        float radY = angleY * M_PI / 180.0f;
        float x = navDistance * sin(radY) * cos(radX);
        float y = navDistance * sin(radX);
        float z = navDistance * cos(radY) * cos(radX);
        m_navCube->setCameraPosition(SbVec3f(x, y, z));
        m_navCube->setCameraOrientation(mainOrient);

        LOG_DBG("Canvas::SyncNavigationCubeCamera: Synced navigation cube camera angle with main camera - AngleX: " + std::to_string(angleX) + ", AngleY: " + std::to_string(angleY));
    }
}

void Canvas::SyncMainCameraToNavigationCube() {
    if (!m_isInitialized || !m_navCube || !m_sceneManager) {
        LOG_WAR("Canvas::SyncMainCameraToNavigationCube: Skipped: Canvas not initialized or components missing");
        return;
    }

    SoCamera* navCamera = m_navCube->getCamera();
    if (navCamera) {
        SbVec3f navPos = navCamera->position.getValue();
        SbRotation navOrient = navCamera->orientation.getValue();

        SoCamera* mainCamera = m_sceneManager->getCamera();
        if (mainCamera) {
            // 只同步旋转角度，不改变主相机位置
            mainCamera->orientation.setValue(navOrient);
            LOG_DBG("Canvas::SyncMainCameraToNavigationCube: Synced main camera orientation with navigation cube camera");
        }
    }
}

void Canvas::ShowNavigationCubeConfigDialog() {
    if (!m_isInitialized) {
        LOG_WAR("Canvas::ShowNavigationCubeConfigDialog: Skipped: Canvas not initialized");
        return;
    }

    wxSize clientSize = GetClientSize();
    int maxX = static_cast<int>(clientSize.x / m_dpiScale);
    int maxY = static_cast<int>(clientSize.y / m_dpiScale);
   
    NavigationCubeConfigDialog dialog(this, m_cubeLayout.x, m_cubeLayout.y, m_cubeLayout.size, m_cubeLayout.size, wxColour(255, 255, 255), maxX, maxY);
    if (dialog.ShowModal() == wxID_OK) {
        int newX = dialog.GetX();
        int newY = dialog.GetY();
        int newSize = dialog.GetSize();
        int newViewportSize = dialog.GetViewportSize();
        wxColour newColor = dialog.GetColor();
        
        SetNavigationCubeRect(newX, newY, newSize);
        SetNavigationCubeViewportSize(newViewportSize);
        SetNavigationCubeColor(newColor);
        LOG_INF("Canvas::ShowNavigationCubeConfigDialog: Applied new navigation cube settings: x=" + 
                std::to_string(newX) + ", y=" + std::to_string(newY) + ", size=" + std::to_string(newSize) +
                ", viewportSize=" + std::to_string(newViewportSize));
    }
}