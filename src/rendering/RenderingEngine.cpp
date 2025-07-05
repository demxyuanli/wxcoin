#include "RenderingEngine.h"
#include "SceneManager.h"
#include "NavigationCubeManager.h"
#include "Logger.h"
#include <wx/msgdlg.h>
#include <stdexcept>
#include <GL/gl.h>

const int RenderingEngine::RENDER_INTERVAL = 16; // ~60 FPS (milliseconds)

RenderingEngine::RenderingEngine(wxGLCanvas* canvas)
    : m_canvas(canvas)
    , m_glContext(nullptr)
    , m_sceneManager(nullptr)
    , m_navigationCubeManager(nullptr)
    , m_isInitialized(false)
    , m_isRendering(false)
    , m_lastRenderTime(0)
{
    LOG_INF("RenderingEngine::RenderingEngine: Initializing");
}

RenderingEngine::~RenderingEngine() {
    LOG_INF("RenderingEngine::~RenderingEngine: Destroying");
}

bool RenderingEngine::initialize() {
    if (m_isInitialized) {
        LOG_WRN("RenderingEngine::initialize: Already initialized");
        return true;
    }

    try {
        setupGLContext();
        m_isInitialized = true;
        
        const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        LOG_INF("RenderingEngine::initialize: GL Context created. OpenGL version: " + 
                std::string(glVersion ? glVersion : "unknown"));
        
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERR("RenderingEngine::initialize: Failed: " + std::string(e.what()));
        return false;
    }
}

void RenderingEngine::setupGLContext() {
    if (!m_canvas) {
        throw std::runtime_error("Canvas is null");
    }

    m_glContext = std::make_unique<wxGLContext>(m_canvas);
    if (!m_glContext || !m_canvas->SetCurrent(*m_glContext)) {
        throw std::runtime_error("Failed to create/set GL context");
    }
}

void RenderingEngine::render(bool fastMode) {
    if (!m_isInitialized) {
        LOG_WRN("RenderingEngine::render: Skipped: Not initialized");
        return;
    }

    if (!m_canvas->IsShown() || !m_glContext || !m_sceneManager) {
        LOG_WRN("RenderingEngine::render: Skipped: Canvas not shown or context/scene invalid");
        return;
    }

    if (m_isRendering) {
        LOG_WRN("RenderingEngine::render: Skipped: Already rendering");
        return;
    }

    wxLongLong currentTime = wxGetLocalTimeMillis();
    if (currentTime - m_lastRenderTime < RENDER_INTERVAL) {
        return;
    }

    m_isRendering = true;
    m_lastRenderTime = currentTime;

    try {
        if (!m_canvas->SetCurrent(*m_glContext)) {
            LOG_ERR("RenderingEngine::render: Failed to set GL context");
            m_isRendering = false;
            return;
        }

        wxSize size = m_canvas->GetClientSize();
        if (size.x <= 0 || size.y <= 0) {
            LOG_WRN("RenderingEngine::render: Invalid viewport size: " + 
                   std::to_string(size.x) + "x" + std::to_string(size.y));
            m_isRendering = false;
            return;
        }

        clearBuffers();
        
        // Set viewport with DPI scaling
        // Note: We don't have direct access to DPI scale here, so we'll use the canvas size directly
        glViewport(0, 0, size.x, size.y);
        
        // Render main scene
        m_sceneManager->render(size, fastMode);

        // Render navigation cube
        if (m_navigationCubeManager) {
            m_navigationCubeManager->render();
        }

        presentFrame();
    }
    catch (const std::exception& e) {
        LOG_ERR("RenderingEngine::render: Exception during render: " + std::string(e.what()));
        clearBuffers();
        m_isRendering = false;
        
        wxMessageDialog dialog(nullptr, 
                              "Rendering failed: " + std::string(e.what()) + 
                              ". Please check system resources or restart the application.",
                              "Rendering Error", wxOK | wxICON_ERROR);
        dialog.ShowModal();
    }

    m_isRendering = false;
}

void RenderingEngine::clearBuffers() {
    glClearColor(0.6f, 0.8f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderingEngine::presentFrame() {
    m_canvas->SwapBuffers();
}

void RenderingEngine::handleResize(const wxSize& size) {
    if (!m_isInitialized || !m_glContext) {
        LOG_WRN("RenderingEngine::handleResize: Not initialized or invalid context");
        return;
    }

    if (size.x > 0 && size.y > 0 && m_canvas->SetCurrent(*m_glContext)) {
        if (m_sceneManager) {
            m_sceneManager->updateAspectRatio(size);
        }
        
        if (m_navigationCubeManager) {
            m_navigationCubeManager->handleSizeChange();
        }
        
        m_canvas->Refresh();
    } else {
        LOG_WRN("RenderingEngine::handleResize: Skipped: Invalid size or context");
    }
}

void RenderingEngine::updateViewport(const wxSize& size, float dpiScale) {
    if (!m_isInitialized) {
        return;
    }
    
    glViewport(0, 0, static_cast<int>(size.x * dpiScale), static_cast<int>(size.y * dpiScale));
} 