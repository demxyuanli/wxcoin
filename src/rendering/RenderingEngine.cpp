#include "RenderingEngine.h"
#include "SceneManager.h"
#include "NavigationCubeManager.h"
#include "logger/Logger.h"
#include "config/RenderingConfig.h"
#include <wx/msgdlg.h>
#include <stdexcept>
#include <GL/gl.h>
#include <chrono> // Added for rendering timing

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
    LOG_INF_S("RenderingEngine::RenderingEngine: Initializing");
}

RenderingEngine::~RenderingEngine() {
    LOG_INF_S("RenderingEngine::~RenderingEngine: Destroying");
}

bool RenderingEngine::initialize() {
    if (m_isInitialized) {
        LOG_WRN_S("RenderingEngine::initialize: Already initialized");
        return true;
    }

    try {
        setupGLContext();
        m_isInitialized = true;
        
        const char* glVersion = reinterpret_cast<const char*>(glGetString(GL_VERSION));
        LOG_INF_S("RenderingEngine::initialize: GL Context created. OpenGL version: " + 
                std::string(glVersion ? glVersion : "unknown"));
        
        return true;
    }
    catch (const std::exception& e) {
        LOG_ERR_S("RenderingEngine::initialize: Failed: " + std::string(e.what()));
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
    renderWithoutSwap(fastMode);
    presentFrame();
}

void RenderingEngine::renderWithoutSwap(bool fastMode) {
    auto renderStartTime = std::chrono::high_resolution_clock::now();
    
    if (!m_isInitialized) {
        return;
    }

    if (!m_canvas->IsShown() || !m_glContext || !m_sceneManager) {
        return;
    }

    if (m_isRendering) {
        return;
    }

    wxLongLong currentTime = wxGetLocalTimeMillis();
    if (currentTime - m_lastRenderTime < RENDER_INTERVAL) {
        return;
    }

    m_isRendering = true;
    m_lastRenderTime = currentTime;

    try {
        auto contextStartTime = std::chrono::high_resolution_clock::now();
        if (!m_canvas->SetCurrent(*m_glContext)) {
            LOG_ERR_S("Failed to set GL context");
            m_isRendering = false;
            return;
        }
        auto contextEndTime = std::chrono::high_resolution_clock::now();
        auto contextDuration = std::chrono::duration_cast<std::chrono::microseconds>(contextEndTime - contextStartTime);

        wxSize size = m_canvas->GetClientSize();
        if (size.x <= 0 || size.y <= 0) {
            m_isRendering = false;
            return;
        }

        auto clearStartTime = std::chrono::high_resolution_clock::now();
        clearBuffers();
        auto clearEndTime = std::chrono::high_resolution_clock::now();
        auto clearDuration = std::chrono::duration_cast<std::chrono::microseconds>(clearEndTime - clearStartTime);

        // Set viewport with DPI scaling
        auto viewportStartTime = std::chrono::high_resolution_clock::now();
        glViewport(0, 0, size.x, size.y);
        auto viewportEndTime = std::chrono::high_resolution_clock::now();
        auto viewportDuration = std::chrono::duration_cast<std::chrono::microseconds>(viewportEndTime - viewportStartTime);

        // Render main scene
        auto sceneStartTime = std::chrono::high_resolution_clock::now();
        m_sceneManager->render(size, fastMode);
        auto sceneEndTime = std::chrono::high_resolution_clock::now();
        auto sceneDuration = std::chrono::duration_cast<std::chrono::milliseconds>(sceneEndTime - sceneStartTime);

        // Render navigation cube
        if (m_navigationCubeManager) {
            auto navCubeStartTime = std::chrono::high_resolution_clock::now();
            m_navigationCubeManager->render();
            auto navCubeEndTime = std::chrono::high_resolution_clock::now();
            auto navCubeDuration = std::chrono::duration_cast<std::chrono::milliseconds>(navCubeEndTime - navCubeStartTime);
        }

        auto renderEndTime = std::chrono::high_resolution_clock::now();
        auto renderDuration = std::chrono::duration_cast<std::chrono::milliseconds>(renderEndTime - renderStartTime);
        
        // Only log if render time is significant
        if (renderDuration.count() > 16) {
            LOG_INF_S("=== RENDERING ENGINE PERFORMANCE ===");
            LOG_INF_S("GL context: " + std::to_string(contextDuration.count()) + "μs");
            LOG_INF_S("Buffer clear: " + std::to_string(clearDuration.count()) + "μs");
            LOG_INF_S("Viewport set: " + std::to_string(viewportDuration.count()) + "μs");
            LOG_INF_S("Scene render: " + std::to_string(sceneDuration.count()) + "ms");
            LOG_INF_S("Total engine render: " + std::to_string(renderDuration.count()) + "ms");
            LOG_INF_S("Engine FPS: " + std::to_string(1000.0 / renderDuration.count()));
            LOG_INF_S("=====================================");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Exception during render: " + std::string(e.what()));
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

void RenderingEngine::swapBuffers() {
    presentFrame();
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
        LOG_WRN_S("RenderingEngine::handleResize: Not initialized or invalid context");
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
        LOG_WRN_S("RenderingEngine::handleResize: Skipped: Invalid size or context");
    }
}

void RenderingEngine::updateViewport(const wxSize& size, float dpiScale) {
    if (!m_isInitialized) {
        return;
    }
    
    glViewport(0, 0, static_cast<int>(size.x * dpiScale), static_cast<int>(size.y * dpiScale));
}
