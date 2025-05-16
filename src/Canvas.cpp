#include "Canvas.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "NavigationCube.h"
#include "ObjectTreePanel.h"
#include "Logger.h"
#include <wx/dcclient.h>

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
    , m_lastRenderTime(0)
    , m_objectTreePanel(nullptr)
    , m_commandManager(nullptr)
    , m_cubeSize(120)
    , m_cubeX(0)
    , m_cubeY(0)
{
    LOG_INF("Canvas initializing");

    SetName("Canvas");
    wxSize clientSize = GetClientSize();
    if (clientSize.x <= 0 || clientSize.y <= 0) {
        clientSize = wxSize(400, 300);
        SetSize(clientSize);
        SetMinSize(clientSize);
    }

    m_glContext = new wxGLContext(this);
    if (!m_glContext || !SetCurrent(*m_glContext)) {
        LOG_ERR("Failed to create/set GL context");
        return;
    }

    m_sceneManager = std::make_unique<SceneManager>(this);
    m_inputManager = std::make_unique<InputManager>(this);

    // Initialize navigation cube
    auto callback = [this](const std::string& view) {
        m_sceneManager->setView(view);
        Refresh(true);
    };
    m_navCube = std::make_unique<NavigationCube>(callback);

    // Initialize navigation cube position (bottom-right)
    if (clientSize.x > 0 && clientSize.y > 0) {
        m_cubeX = clientSize.x - m_cubeSize - 10; // 10 pixels from right edge
        m_cubeY = clientSize.y - m_cubeSize - 10; // 10 pixels from bottom edge
        LOG_INF("Initialized navigation cube position: bottom-right x=" + std::to_string(m_cubeX) + ", y=" + std::to_string(m_cubeY));
    }

    const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    LOG_INF("GL Context created. OpenGL version: " + std::string(glVersion ? glVersion : "unknown"));

    if (m_sceneManager && !m_sceneManager->initScene()) {
        LOG_ERR("Failed to initialize scene");
    }

    Refresh(true);
    Update();
    LOG_INF("Canvas initialized successfully");
}

Canvas::~Canvas() {
    LOG_INF("Canvas destroying");
    delete m_glContext;
}

void Canvas::render(bool fastMode) {
    if (!IsShown() || !m_glContext || !m_sceneManager) {
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

        glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render main scene
        glViewport(0, 0, size.x, size.y);
        m_sceneManager->render(size, fastMode);

        // Render navigation cube
        if (m_navCube && m_navCube->isEnabled()) {
            m_cubeX = size.x - m_cubeSize - 10; // Update X position
            m_cubeY = size.y - m_cubeSize - 10; // Update Y position for bottom-right
            glViewport(m_cubeX, m_cubeY, m_cubeSize, m_cubeSize); // Set viewport for bottom-right
            m_navCube->render(wxSize(m_cubeSize, m_cubeSize));
            LOG_DBG("Rendering navigation cube: x=" + std::to_string(m_cubeX) + ", y=" + std::to_string(m_cubeY));
        }

        SwapBuffers();
    }
    catch (const std::exception& e) {
        LOG_ERR("Exception during render: " + std::string(e.what()));
    }

    m_isRendering = false;
}

void Canvas::onPaint(wxPaintEvent& event) {
    if (!m_glContext || !m_sceneManager) {
        LOG_WAR("Paint event skipped: Invalid context or scene");
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
        LOG_DBG("Redundant size event ignored: " + std::to_string(size.x) + "x" + std::to_string(size.y));
        event.Skip();
        return;
    }

    lastSize = size;
    lastEventTime = currentTime;
    LOG_INF("Handling size event: " + std::to_string(size.x) + "x" + std::to_string(size.y));

    if (size.x > 0 && size.y > 0 && m_glContext && SetCurrent(*m_glContext)) {
        m_cubeX = size.x - m_cubeSize - 10; // 10 pixels from right edge
        m_cubeY = size.y - m_cubeSize - 10; // 10 pixels from bottom edge
        m_sceneManager->updateAspectRatio(size);
        Refresh();
    }
    else {
        LOG_WAR("Size event skipped: Invalid size or context");
    }
    event.Skip();
}

void Canvas::onEraseBackground(wxEraseEvent& event) {
    // Do nothing to prevent flickering
}

void Canvas::onMouseEvent(wxMouseEvent& event) {
    if (!m_inputManager) {
        event.Skip();
        return;
    }

    // Check if event is within navigation cube region
    if (m_navCube && m_navCube->isEnabled()) {
        wxSize clientSize = GetClientSize();
        m_cubeX = clientSize.x - m_cubeSize - 10;
        m_cubeY = clientSize.y - m_cubeSize - 10;
        int x = event.GetX();
        int y = event.GetY();
        if (x >= m_cubeX && x < m_cubeX + m_cubeSize && y >= m_cubeY && y < m_cubeY + m_cubeSize) {
            wxMouseEvent cubeEvent(event);
            cubeEvent.m_x = x - m_cubeX;
            cubeEvent.m_y = y - m_cubeY;
            if (event.GetEventType() == wxEVT_LEFT_DOWN ||
                event.GetEventType() == wxEVT_LEFT_UP ||
                event.GetEventType() == wxEVT_MOTION) {
                m_navCube->handleMouseEvent(cubeEvent, wxSize(m_cubeSize, m_cubeSize));
                Refresh(true);
                event.Skip();
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
    SetCursor(enable ? wxCursor(wxCURSOR_CROSS) : wxCursor(wxCURSOR_DEFAULT));
}

SoCamera* Canvas::getCamera() const {
    return m_sceneManager->getCamera();
}

void Canvas::resetView() {
    m_sceneManager->resetView();
}

void Canvas::setNavigationCubeEnabled(bool enabled) {
    if (m_navCube) {
        m_navCube->setEnabled(enabled);
        Refresh(true);
    }
}

bool Canvas::isNavigationCubeEnabled() const {
    return m_navCube && m_navCube->isEnabled();
}