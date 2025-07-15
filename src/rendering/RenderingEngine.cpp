#include "RenderingEngine.h"
#include "SceneManager.h"
#include "NavigationCubeManager.h"
#include "logger/Logger.h"
#include "config/RenderingConfig.h"
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
    // Load lighting settings from configuration
    RenderingConfig& config = RenderingConfig::getInstance();
    const auto& lightingSettings = config.getLightingSettings();
    
    m_lightAmbientColor = lightingSettings.ambientColor;
    m_lightDiffuseColor = lightingSettings.diffuseColor;
    m_lightSpecularColor = lightingSettings.specularColor;
    m_lightIntensity = lightingSettings.intensity;
    m_lightAmbientIntensity = lightingSettings.ambientIntensity;
    
    LOG_INF_S("RenderingEngine::RenderingEngine: Initializing with configured lighting settings");
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
    if (!m_isInitialized) {
        LOG_WRN_S("RenderingEngine::renderWithoutSwap: Skipped: Not initialized");
        return;
    }

    if (!m_canvas->IsShown() || !m_glContext || !m_sceneManager) {
        LOG_WRN_S("RenderingEngine::renderWithoutSwap: Skipped: Canvas not shown or context/scene invalid");
        return;
    }

    if (m_isRendering) {
        LOG_WRN_S("RenderingEngine::renderWithoutSwap: Skipped: Already rendering");
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
            LOG_ERR_S("RenderingEngine::renderWithoutSwap: Failed to set GL context");
            m_isRendering = false;
            return;
        }

        wxSize size = m_canvas->GetClientSize();
        if (size.x <= 0 || size.y <= 0) {
            LOG_WRN_S("RenderingEngine::renderWithoutSwap: Invalid viewport size: " + 
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
    }
    catch (const std::exception& e) {
        LOG_ERR_S("RenderingEngine::renderWithoutSwap: Exception during render: " + std::string(e.what()));
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

// Lighting settings methods
void RenderingEngine::setLightAmbientColor(const Quantity_Color& color)
{
    m_lightAmbientColor = color;
    LOG_INF_S("RenderingEngine: Set light ambient color to (" + 
             std::to_string(color.Red()) + ", " + 
             std::to_string(color.Green()) + ", " + 
             std::to_string(color.Blue()) + ")");
}

void RenderingEngine::setLightDiffuseColor(const Quantity_Color& color)
{
    m_lightDiffuseColor = color;
    LOG_INF_S("RenderingEngine: Set light diffuse color to (" + 
             std::to_string(color.Red()) + ", " + 
             std::to_string(color.Green()) + ", " + 
             std::to_string(color.Blue()) + ")");
}

void RenderingEngine::setLightSpecularColor(const Quantity_Color& color)
{
    m_lightSpecularColor = color;
    LOG_INF_S("RenderingEngine: Set light specular color to (" + 
             std::to_string(color.Red()) + ", " + 
             std::to_string(color.Green()) + ", " + 
             std::to_string(color.Blue()) + ")");
}

void RenderingEngine::setLightIntensity(double intensity)
{
    m_lightIntensity = std::max(0.0, std::min(1.0, intensity));
    LOG_INF_S("RenderingEngine: Set light intensity to " + std::to_string(m_lightIntensity));
}

void RenderingEngine::setLightAmbientIntensity(double intensity)
{
    m_lightAmbientIntensity = std::max(0.0, std::min(1.0, intensity));
    LOG_INF_S("RenderingEngine: Set light ambient intensity to " + std::to_string(m_lightAmbientIntensity));
}
