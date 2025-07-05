#include "Canvas.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "ObjectTreePanel.h"
#include "Logger.h"
#include "NavigationCubeManager.h"
#include "RenderingEngine.h"
#include "EventCoordinator.h"
#include "ViewportManager.h"
#include <wx/dcclient.h>
#include <wx/msgdlg.h>
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
    , m_objectTreePanel(nullptr)
    , m_commandManager(nullptr)
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
        initializeSubsystems();
        connectSubsystems();
        
        if (m_sceneManager && !m_sceneManager->initScene()) {
            LOG_ERR("Canvas::Canvas: Failed to initialize main scene");
            showErrorDialog("Failed to initialize 3D scene. The application may not function correctly.");
            throw std::runtime_error("Scene initialization failed");
        }

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
}

void Canvas::initializeSubsystems() {
    LOG_INF("Canvas::initializeSubsystems: Creating subsystems");

    // Create core subsystems
    m_renderingEngine = std::make_unique<RenderingEngine>(this);
    m_viewportManager = std::make_unique<ViewportManager>(this);
    m_eventCoordinator = std::make_unique<EventCoordinator>();
    
    m_sceneManager = std::make_unique<SceneManager>(this);
    m_inputManager = std::make_unique<InputManager>(this);
    m_navigationCubeManager = std::make_unique<NavigationCubeManager>(this, m_sceneManager.get());

    // Initialize rendering engine
    if (!m_renderingEngine->initialize()) {
        showErrorDialog("Failed to initialize OpenGL context. Please check your graphics drivers.");
        throw std::runtime_error("RenderingEngine initialization failed");
    }
}

void Canvas::connectSubsystems() {
    LOG_INF("Canvas::connectSubsystems: Connecting subsystems");

    // Connect rendering engine
    m_renderingEngine->setSceneManager(m_sceneManager.get());
    m_renderingEngine->setNavigationCubeManager(m_navigationCubeManager.get());

    // Connect viewport manager
    m_viewportManager->setRenderingEngine(m_renderingEngine.get());
    m_viewportManager->setNavigationCubeManager(m_navigationCubeManager.get());

    // Connect event coordinator
    m_eventCoordinator->setNavigationCubeManager(m_navigationCubeManager.get());
    m_eventCoordinator->setInputManager(m_inputManager.get());
}

void Canvas::showErrorDialog(const std::string& message) const {
    wxMessageDialog dialog(nullptr, message, "Error", wxOK | wxICON_ERROR);
    dialog.ShowModal();
}

void Canvas::render(bool fastMode) {
    if (m_renderingEngine) {
        m_renderingEngine->render(fastMode);
    }
}

void Canvas::onPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    render(false);
    if (m_eventCoordinator) {
        m_eventCoordinator->handlePaintEvent(event);
    }
    event.Skip();
}

void Canvas::onSize(wxSizeEvent& event) {
    wxSize size = event.GetSize();
    if (m_viewportManager) {
        m_viewportManager->handleSizeChange(size);
    }
    if (m_eventCoordinator) {
        m_eventCoordinator->handleSizeEvent(event);
    }
    event.Skip();
}

void Canvas::onEraseBackground(wxEraseEvent& event) {
    // Do nothing to prevent flickering
}

void Canvas::onMouseEvent(wxMouseEvent& event) {
    if (m_eventCoordinator) {
        if (m_eventCoordinator->handleMouseEvent(event)) {
            return; // Event was handled
        }
    }
    event.Skip();
}

void Canvas::setPickingCursor(bool enable) {
    SetCursor(enable ? wxCursor(wxCURSOR_CROSS) : wxCursor(wxCURSOR_DEFAULT));
}

SoCamera* Canvas::getCamera() const {
    if (!m_sceneManager) {
        LOG_WRN("Canvas::getCamera: SceneManager is null");
        return nullptr;
    }
    return m_sceneManager->getCamera();
}

void Canvas::resetView() {
    if (!m_sceneManager) {
        LOG_WRN("Canvas::resetView: SceneManager is null");
        return;
    }
    m_sceneManager->resetView();
}

void Canvas::setNavigationCubeEnabled(bool enabled) {
    if (m_navigationCubeManager) {
        m_navigationCubeManager->setEnabled(enabled);
    }
}

bool Canvas::isNavigationCubeEnabled() const {
    if (m_navigationCubeManager) {
        return m_navigationCubeManager->isEnabled();
    }
    return false;
}

void Canvas::ShowNavigationCubeConfigDialog() {
    if (m_navigationCubeManager) {
        m_navigationCubeManager->showConfigDialog();
    }
}

float Canvas::getDPIScale() const {
    if (m_viewportManager) {
        return m_viewportManager->getDPIScale();
    }
    return GetContentScaleFactor();
}

