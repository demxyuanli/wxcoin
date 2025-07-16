#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "Canvas.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "ObjectTreePanel.h"
#include "logger/Logger.h"
#include "NavigationCubeManager.h"
#include "ViewRefreshManager.h"
#include "UnifiedRefreshSystem.h"
#include "RenderingEngine.h"
#include "EventCoordinator.h"
#include "ViewportManager.h"
#include "OCCViewer.h"
#include "GlobalServices.h"
#include <wx/dcclient.h>
#include <wx/msgdlg.h>
#include "MultiViewportManager.h"
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
    : wxGLCanvas(parent, id, s_canvasAttribs, pos, size, wxFULL_REPAINT_ON_RESIZE)
    , m_objectTreePanel(nullptr)
    , m_commandManager(nullptr)
    , m_occViewer(nullptr)
    , m_commandDispatcher(nullptr)
    , m_multiViewportEnabled(false)
{
    LOG_INF_S("Canvas: Initializing");
    
    try {
        initializeSubsystems();
        connectSubsystems();
        
        Bind(wxEVT_PAINT, &Canvas::onPaint, this);
        Bind(wxEVT_SIZE, &Canvas::onSize, this);
        Bind(wxEVT_ERASE_BACKGROUND, &Canvas::onEraseBackground, this);
        Bind(wxEVT_LEFT_DOWN, &Canvas::onMouseEvent, this);
        Bind(wxEVT_LEFT_UP, &Canvas::onMouseEvent, this);
        Bind(wxEVT_MOTION, &Canvas::onMouseEvent, this);
        Bind(wxEVT_MOUSEWHEEL, &Canvas::onMouseEvent, this);
        Bind(wxEVT_RIGHT_DOWN, &Canvas::onMouseEvent, this);
        Bind(wxEVT_RIGHT_UP, &Canvas::onMouseEvent, this);
        
        // Use UnifiedRefreshSystem for initial render if available
        UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
        if (refreshSystem && refreshSystem->isInitialized()) {
            refreshSystem->refreshView("", true);  // Immediate view refresh
            LOG_INF_S("Canvas: Initial render triggered via UnifiedRefreshSystem");
        } else {
            // Fallback to direct refresh
            Refresh();
            LOG_INF_S("Canvas: Initial render triggered via direct refresh");
        }
        
        LOG_INF_S("Canvas: Initialization completed successfully");
    } catch (const std::exception& e) {
        LOG_ERR_S("Canvas: Initialization failed: " + std::string(e.what()));
        showErrorDialog("Canvas initialization failed: " + std::string(e.what()));
        throw;
    }
}

Canvas::~Canvas() {
    LOG_INF_S("Canvas::Canvas: Destroying");
}

void Canvas::initializeSubsystems() {
    LOG_INF_S("Canvas::initializeSubsystems: Creating subsystems");

    // Create core subsystems
    m_refreshManager = std::make_unique<ViewRefreshManager>(this);
    m_renderingEngine = std::make_unique<RenderingEngine>(this);
    m_viewportManager = std::make_unique<ViewportManager>(this);
    m_eventCoordinator = std::make_unique<EventCoordinator>();

    m_sceneManager = std::make_unique<SceneManager>(this);
    
    // Initialize the scene manager
    if (!m_sceneManager->initScene()) {
        showErrorDialog("Failed to initialize scene manager.");
        throw std::runtime_error("SceneManager initialization failed");
    }
    
    m_inputManager = std::make_unique<InputManager>(this);
    m_navigationCubeManager = std::make_unique<NavigationCubeManager>(this, m_sceneManager.get());
    
    // Get unified refresh system from global application services
    m_unifiedRefreshSystem = GlobalServices::GetRefreshSystem();

    // Initialize rendering engine FIRST
    if (!m_renderingEngine->initialize()) {
        showErrorDialog("Failed to initialize OpenGL context. Please check your graphics drivers.");
        throw std::runtime_error("RenderingEngine initialization failed");
    }

    // Create multi-viewport manager AFTER OpenGL context is ready
    // Do NOT create it here - delay until first render
    m_multiViewportEnabled = true;
}

void Canvas::connectSubsystems() {
    LOG_INF_S("Canvas::connectSubsystems: Connecting subsystems");

    // Connect rendering engine
    m_renderingEngine->setSceneManager(m_sceneManager.get());
    m_renderingEngine->setNavigationCubeManager(m_navigationCubeManager.get());

    // Connect viewport manager
    m_viewportManager->setRenderingEngine(m_renderingEngine.get());
    m_viewportManager->setNavigationCubeManager(m_navigationCubeManager.get());

    // Connect event coordinator
    m_eventCoordinator->setNavigationCubeManager(m_navigationCubeManager.get());
    m_eventCoordinator->setInputManager(m_inputManager.get());
    
    // Connect multi-viewport manager
    if (m_multiViewportManager) {
        m_multiViewportManager->setNavigationCubeManager(m_navigationCubeManager.get());
    }
    
    // Set Canvas and SceneManager in global unified refresh system
    if (m_unifiedRefreshSystem) {
        m_unifiedRefreshSystem->setComponents(this, m_occViewer, m_sceneManager.get());
    }
    
    // Use UnifiedRefreshSystem for initial render after all subsystems are connected
    UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
    if (refreshSystem && refreshSystem->isInitialized()) {
        refreshSystem->refreshView("", true);  // Immediate view refresh
        LOG_INF_S("Canvas: Subsystems connected - Initial render triggered via UnifiedRefreshSystem");
    } else {
        // Fallback to direct refresh
        Refresh();
        LOG_INF_S("Canvas: Subsystems connected - Initial render triggered via direct refresh");
    }
}

void Canvas::showErrorDialog(const std::string& message) const {
    wxMessageDialog dialog(nullptr, message, "Error", wxOK | wxICON_ERROR);
    dialog.ShowModal();
}

void Canvas::render(bool fastMode) {
    if (m_renderingEngine) {
        // Create MultiViewportManager on first render when GL context is active
        if (m_multiViewportEnabled && !m_multiViewportManager) {
            try {
                m_multiViewportManager = std::make_unique<MultiViewportManager>(this, m_sceneManager.get());
                m_multiViewportManager->setNavigationCubeManager(m_navigationCubeManager.get());
                m_multiViewportManager->handleSizeChange(GetClientSize());
                LOG_INF_S("Canvas::render: MultiViewportManager created successfully");
            } catch (const std::exception& e) {
                LOG_ERR_S("Canvas::render: Failed to create MultiViewportManager: " + std::string(e.what()));
                m_multiViewportEnabled = false;
            }
        }
        
        // Render main scene first (without swapping buffers)
        m_renderingEngine->renderWithoutSwap(fastMode);
        
        // Render additional viewports on top of main scene
        if (m_multiViewportEnabled && m_multiViewportManager) {
            m_multiViewportManager->render();
        }
        
        // Finally swap buffers to display everything
        m_renderingEngine->swapBuffers();
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
    if (m_multiViewportManager) {
        m_multiViewportManager->handleSizeChange(size);
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
    // Check if this is an interaction event that should trigger LOD
    bool isInteractionEvent = false;
    if (event.GetEventType() == wxEVT_LEFT_DOWN || 
        event.GetEventType() == wxEVT_RIGHT_DOWN ||
        event.GetEventType() == wxEVT_MOTION ||
        event.GetEventType() == wxEVT_MOUSEWHEEL) {
        isInteractionEvent = true;
    }
    
    // Trigger LOD interaction if enabled
    if (isInteractionEvent && m_occViewer) {
        m_occViewer->startLODInteraction();
    }
    
    // Check multi-viewport first - this should have higher priority
    if (m_multiViewportEnabled && m_multiViewportManager) {
        if (m_multiViewportManager->handleMouseEvent(event)) {
            return; // Event was handled
        }
    }
    
    // Only pass to EventCoordinator if MultiViewportManager didn't handle it
    if (m_eventCoordinator) {
        if (m_eventCoordinator->handleMouseEvent(event)) {
            return; // Event was handled
        }
    }
    event.Skip();
}

void Canvas::setMultiViewportEnabled(bool enabled) {
    m_multiViewportEnabled = enabled;
    Refresh();
}

bool Canvas::isMultiViewportEnabled() const {
    return m_multiViewportEnabled;
}

void Canvas::setPickingCursor(bool enable) {
    SetCursor(enable ? wxCursor(wxCURSOR_CROSS) : wxCursor(wxCURSOR_DEFAULT));
}

SoCamera* Canvas::getCamera() const {
    if (!m_sceneManager) {
        LOG_WRN_S("Canvas::getCamera: SceneManager is null");
        return nullptr;
    }
    return m_sceneManager->getCamera();
}

void Canvas::resetView() {
    if (!m_sceneManager) {
        LOG_WRN_S("Canvas::resetView: SceneManager is null");
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

void Canvas::setOCCViewer(OCCViewer* occViewer) {
    m_occViewer = occViewer;
    
    // Update global unified refresh system with the new OCCViewer
    UnifiedRefreshSystem* globalRefreshSystem = GlobalServices::GetRefreshSystem();
    if (globalRefreshSystem) {
        globalRefreshSystem->setComponents(this, occViewer, m_sceneManager.get());
    }
}

