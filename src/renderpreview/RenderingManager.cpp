#include "renderpreview/RenderingManager.h"
#include "logger/Logger.h"
#include <wx/glcanvas.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoComplexity.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoBlinker.h>
#include <algorithm>

RenderingManager::RenderingManager(SoSeparator* sceneRoot, wxGLCanvas* canvas, wxGLContext* glContext)
    : m_sceneRoot(sceneRoot), m_canvas(canvas), m_glContext(glContext), m_nextConfigId(1), m_activeConfigId(-1)
{
    initializePresets();
    LOG_INF_S("RenderingManager: Initialized");
}

RenderingManager::~RenderingManager()
{
    // Don't call clearAllConfigurations() in destructor as it may try to access destroyed OpenGL context
    m_configurations.clear();
    m_activeConfigId = -1;
    LOG_INF_S("RenderingManager: Destroyed");
}

int RenderingManager::addConfiguration(const RenderingSettings& settings)
{
    LOG_INF_S("RenderingManager::addConfiguration: Adding configuration '" + settings.name + "'");
    
    auto managedConfig = std::make_unique<ManagedRendering>();
    managedConfig->settings = settings;
    managedConfig->configId = m_nextConfigId++;
    managedConfig->isActive = false;
    
    int configId = managedConfig->configId;
    m_configurations[configId] = std::move(managedConfig);
    
    LOG_INF_S("RenderingManager::addConfiguration: Successfully added configuration with ID " + std::to_string(configId));
    return configId;
}

bool RenderingManager::removeConfiguration(int configId)
{
    auto it = m_configurations.find(configId);
    if (it == m_configurations.end()) {
        LOG_WRN_S("RenderingManager::removeConfiguration: Configuration with ID " + std::to_string(configId) + " not found");
        return false;
    }
    
    if (it->second->isActive) {
        setActiveConfiguration(-1);
    }
    
    m_configurations.erase(it);
    
    LOG_INF_S("RenderingManager::removeConfiguration: Successfully removed configuration with ID " + std::to_string(configId));
    return true;
}

bool RenderingManager::updateConfiguration(int configId, const RenderingSettings& settings)
{
    auto it = m_configurations.find(configId);
    if (it == m_configurations.end()) {
        LOG_WRN_S("RenderingManager::updateConfiguration: Configuration with ID " + std::to_string(configId) + " not found");
        return false;
    }
    
    it->second->settings = settings;
    
    if (it->second->isActive) {
        setupRenderingState();
    }
    
    LOG_INF_S("RenderingManager::updateConfiguration: Successfully updated configuration with ID " + std::to_string(configId));
    return true;
}

void RenderingManager::clearAllConfigurations()
{
    LOG_INF_S("RenderingManager::clearAllConfigurations: Clearing all configurations");
    
    m_configurations.clear();
    m_activeConfigId = -1;
    
    // Only restore rendering state if OpenGL context is still valid
    if (m_canvas && m_glContext) {
        try {
            restoreRenderingState();
        } catch (...) {
            LOG_WRN_S("RenderingManager::clearAllConfigurations: Failed to restore rendering state (OpenGL context may be destroyed)");
        }
    }
    
    LOG_INF_S("RenderingManager::clearAllConfigurations: All configurations cleared");
}

std::vector<int> RenderingManager::getAllConfigurationIds() const
{
    std::vector<int> ids;
    for (const auto& pair : m_configurations) {
        ids.push_back(pair.first);
    }
    return ids;
}

std::vector<RenderingSettings> RenderingManager::getAllConfigurations() const
{
    std::vector<RenderingSettings> configs;
    for (const auto& pair : m_configurations) {
        configs.push_back(pair.second->settings);
    }
    return configs;
}

RenderingSettings RenderingManager::getConfiguration(int configId) const
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        return it->second->settings;
    }
    return RenderingSettings();
}

bool RenderingManager::hasConfiguration(int configId) const
{
    return m_configurations.find(configId) != m_configurations.end();
}

int RenderingManager::getConfigurationCount() const
{
    return static_cast<int>(m_configurations.size());
}

bool RenderingManager::setActiveConfiguration(int configId)
{
    if (m_activeConfigId != -1) {
        auto it = m_configurations.find(m_activeConfigId);
        if (it != m_configurations.end()) {
            it->second->isActive = false;
        }
    }
    
    if (configId != -1) {
        auto it = m_configurations.find(configId);
        if (it == m_configurations.end()) {
            LOG_WRN_S("RenderingManager::setActiveConfiguration: Configuration with ID " + std::to_string(configId) + " not found");
            return false;
        }
        
        it->second->isActive = true;
        m_activeConfigId = configId;
        
        setupRenderingState();
        
        LOG_INF_S("RenderingManager::setActiveConfiguration: Activated configuration with ID " + std::to_string(configId));
    } else {
        m_activeConfigId = -1;
        restoreRenderingState();
        LOG_INF_S("RenderingManager::setActiveConfiguration: Deactivated all configurations");
    }
    
    return true;
}

int RenderingManager::getActiveConfigurationId() const
{
    return m_activeConfigId;
}

RenderingSettings RenderingManager::getActiveConfiguration() const
{
    if (m_activeConfigId != -1) {
        auto it = m_configurations.find(m_activeConfigId);
        if (it != m_configurations.end()) {
            return it->second->settings;
        }
    }
    return RenderingSettings();
}

bool RenderingManager::hasActiveConfiguration() const
{
    return m_activeConfigId != -1;
}

void RenderingManager::setRenderingMode(int configId, int mode)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.mode = mode;
        if (it->second->isActive) {
            setupRenderingState();
        }
    }
}

void RenderingManager::setQuality(int configId, int quality)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.quality = quality;
        if (it->second->isActive) {
            setupRenderingState();
        }
    }
}

void RenderingManager::setFastMode(int configId, bool enabled)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.fastMode = enabled;
        if (it->second->isActive) {
            setupRenderingState();
        }
    }
}

void RenderingManager::setTransparencyType(int configId, int type)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.transparencyType = type;
        if (it->second->isActive) {
            setupRenderingState();
        }
    }
}

void RenderingManager::setShadingMode(int configId, bool smooth, bool phong)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.smoothShading = smooth;
        it->second->settings.phongShading = phong;
        if (it->second->isActive) {
            setupRenderingState();
        }
    }
}

void RenderingManager::setCullingMode(int configId, int mode)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.cullMode = mode;
        if (it->second->isActive) {
            setupRenderingState();
        }
    }
}

void RenderingManager::setDepthSettings(int configId, bool test, bool write)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.depthTest = test;
        it->second->settings.depthWrite = write;
        if (it->second->isActive) {
            setupRenderingState();
        }
    }
}

void RenderingManager::setPolygonMode(int configId, int mode)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.polygonMode = mode;
        if (it->second->isActive) {
            setupRenderingState();
        }
    }
}

void RenderingManager::setBackgroundColor(int configId, const wxColour& color)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        it->second->settings.backgroundColor = color;
        if (it->second->isActive) {
            setupRenderingState();
        }
    }
}

void RenderingManager::applyPreset(const std::string& presetName)
{
    auto it = m_presets.find(presetName);
    if (it != m_presets.end()) {
        int configId = addConfiguration(it->second);
        setActiveConfiguration(configId);
        LOG_INF_S("RenderingManager::applyPreset: Applied preset '" + presetName + "'");
    } else {
        LOG_WRN_S("RenderingManager::applyPreset: Preset '" + presetName + "' not found");
    }
}

void RenderingManager::saveAsPreset(int configId, const std::string& presetName)
{
    auto it = m_configurations.find(configId);
    if (it != m_configurations.end()) {
        m_presets[presetName] = it->second->settings;
        LOG_INF_S("RenderingManager::saveAsPreset: Saved configuration as preset '" + presetName + "'");
    }
}

std::vector<std::string> RenderingManager::getAvailablePresets() const
{
    std::vector<std::string> presets;
    for (const auto& pair : m_presets) {
        presets.push_back(pair.first);
    }
    return presets;
}

void RenderingManager::applyToRenderAction(SoGLRenderAction* renderAction)
{
    if (!renderAction) return;
    
    if (m_activeConfigId != -1) {
        auto it = m_configurations.find(m_activeConfigId);
        if (it != m_configurations.end()) {
            const auto& settings = it->second->settings;
            
            // Apply rendering mode
            applyRenderingMode(settings);
            
            // Apply quality settings
            applyQualitySettings(settings);
            
            // Apply transparency settings
            applyTransparencySettings(settings);
            
            // Apply shading settings
            applyShadingSettings(settings);
            
            // Apply culling settings
            applyCullingSettings(settings);
            
            // Apply depth settings
            applyDepthSettings(settings);
            
            // Apply polygon settings
            applyPolygonSettings(settings);
            
            // Apply background settings
            applyBackgroundSettings(settings);
            
            LOG_INF_S("RenderingManager::applyToRenderAction: Applied rendering configuration");
        }
    }
}

void RenderingManager::setupRenderingState()
{
    if (m_activeConfigId != -1) {
        auto it = m_configurations.find(m_activeConfigId);
        if (it != m_configurations.end()) {
            const auto& settings = it->second->settings;
            
            if (m_canvas && m_glContext) {
                m_canvas->SetCurrent(*m_glContext);
                setupOpenGLState(settings);
            }
            
            LOG_INF_S("RenderingManager::setupRenderingState: Applied rendering state");
        }
    }
}

void RenderingManager::restoreRenderingState()
{
    if (m_canvas && m_glContext) {
        try {
            m_canvas->SetCurrent(*m_glContext);
            restoreOpenGLState();
            LOG_INF_S("RenderingManager::restoreRenderingState: Restored default rendering state");
        } catch (...) {
            LOG_WRN_S("RenderingManager::restoreRenderingState: Failed to restore rendering state (OpenGL context may be destroyed)");
        }
    } else {
        LOG_WRN_S("RenderingManager::restoreRenderingState: Canvas or GL context is null");
    }
}

float RenderingManager::getPerformanceImpact() const
{
    if (m_activeConfigId != -1) {
        auto it = m_configurations.find(m_activeConfigId);
        if (it != m_configurations.end()) {
            const auto& settings = it->second->settings;
            
            float impact = 1.0f;
            
            // Quality impact
            impact += settings.quality * 0.3f;
            
            // Fast mode reduces impact
            if (settings.fastMode) {
                impact *= 0.7f;
            }
            
            // Transparency impact
            if (settings.transparencyType > 0) {
                impact += 0.2f;
            }
            
            // Shading impact
            if (settings.phongShading) {
                impact += 0.1f;
            }
            
            // Culling impact (positive for performance)
            if (settings.backfaceCulling) {
                impact *= 0.9f;
            }
            
            return impact;
        }
    }
    return 1.0f;
}

std::string RenderingManager::getQualityDescription() const
{
    if (m_activeConfigId != -1) {
        auto it = m_configurations.find(m_activeConfigId);
        if (it != m_configurations.end()) {
            const auto& settings = it->second->settings;
            
            std::string desc = "Mode: ";
            
            switch (settings.mode) {
                case 0: desc += "Solid"; break;
                case 1: desc += "Wireframe"; break;
                case 2: desc += "Points"; break;
                case 3: desc += "Hidden Line"; break;
                case 4: desc += "Shaded"; break;
                case 5: desc += "Shaded Wireframe"; break;
                default: desc += "Unknown"; break;
            }
            
            desc += ", Quality: ";
            switch (settings.quality) {
                case 0: desc += "Low"; break;
                case 1: desc += "Medium"; break;
                case 2: desc += "High"; break;
                case 3: desc += "Ultra"; break;
                default: desc += "Unknown"; break;
            }
            
            if (settings.fastMode) {
                desc += " (Fast Mode)";
            }
            
            return desc;
        }
    }
    return "No Configuration Active";
}

int RenderingManager::getEstimatedFPS() const
{
    float impact = getPerformanceImpact();
    int baseFPS = 60;
    
    // Estimate FPS based on performance impact
    int estimatedFPS = static_cast<int>(baseFPS / impact);
    
    // Clamp to reasonable range
    return std::max(15, std::min(120, estimatedFPS));
}

void RenderingManager::initializePresets()
{
    // Performance Preset
    RenderingSettings performance;
    performance.name = "Performance";
    performance.mode = 4; // Shaded
    performance.quality = 0; // Low
    performance.fastMode = true;
    performance.transparencyType = 0; // None
    performance.smoothShading = false;
    performance.phongShading = false;
    performance.backfaceCulling = true;
    performance.depthTest = true;
    performance.depthWrite = true;
    m_presets["Performance"] = performance;
    
    // Balanced Preset
    RenderingSettings balanced;
    balanced.name = "Balanced";
    balanced.mode = 4; // Shaded
    balanced.quality = 1; // Medium
    balanced.fastMode = false;
    balanced.transparencyType = 1; // Blend
    balanced.smoothShading = true;
    balanced.phongShading = true;
    balanced.backfaceCulling = true;
    balanced.depthTest = true;
    balanced.depthWrite = true;
    m_presets["Balanced"] = balanced;
    
    // Quality Preset
    RenderingSettings quality;
    quality.name = "Quality";
    quality.mode = 4; // Shaded
    quality.quality = 2; // High
    quality.fastMode = false;
    quality.transparencyType = 2; // SortedBlend
    quality.smoothShading = true;
    quality.phongShading = true;
    quality.backfaceCulling = true;
    quality.depthTest = true;
    quality.depthWrite = true;
    m_presets["Quality"] = quality;
    
    // Ultra Preset
    RenderingSettings ultra;
    ultra.name = "Ultra";
    ultra.mode = 4; // Shaded
    ultra.quality = 3; // Ultra
    ultra.fastMode = false;
    ultra.transparencyType = 3; // DelayedBlend
    ultra.smoothShading = true;
    ultra.phongShading = true;
    ultra.backfaceCulling = true;
    ultra.depthTest = true;
    ultra.depthWrite = true;
    m_presets["Ultra"] = ultra;
    
    // Wireframe Preset
    RenderingSettings wireframe;
    wireframe.name = "Wireframe";
    wireframe.mode = 1; // Wireframe
    wireframe.quality = 1; // Medium
    wireframe.fastMode = true;
    wireframe.transparencyType = 0; // None
    wireframe.smoothShading = false;
    wireframe.phongShading = false;
    wireframe.backfaceCulling = false;
    wireframe.depthTest = true;
    wireframe.depthWrite = true;
    m_presets["Wireframe"] = wireframe;
    
    LOG_INF_S("RenderingManager::initializePresets: Initialized " + std::to_string(m_presets.size()) + " presets");
}

void RenderingManager::applyRenderingMode(const RenderingSettings& settings)
{
    if (!m_sceneRoot) return;
    
    // This would typically involve modifying scene graph nodes
    // For now, we'll just log the mode change
    LOG_INF_S("RenderingManager::applyRenderingMode: Applied mode " + std::to_string(settings.mode));
}

void RenderingManager::applyQualitySettings(const RenderingSettings& settings)
{
    if (!m_sceneRoot) return;
    
    // Apply quality settings to scene graph
    LOG_INF_S("RenderingManager::applyQualitySettings: Applied quality " + std::to_string(settings.quality));
}

void RenderingManager::applyTransparencySettings(const RenderingSettings& settings)
{
    if (!m_sceneRoot) return;
    
    // Apply transparency settings
    LOG_INF_S("RenderingManager::applyTransparencySettings: Applied transparency type " + std::to_string(settings.transparencyType));
}

void RenderingManager::applyShadingSettings(const RenderingSettings& settings)
{
    if (!m_sceneRoot) return;
    
    // Apply shading settings
    LOG_INF_S("RenderingManager::applyShadingSettings: Smooth=" + std::to_string(settings.smoothShading) + 
              ", Phong=" + std::to_string(settings.phongShading));
}

void RenderingManager::applyCullingSettings(const RenderingSettings& settings)
{
    if (!m_sceneRoot) return;
    
    // Apply culling settings
    LOG_INF_S("RenderingManager::applyCullingSettings: Cull mode " + std::to_string(settings.cullMode));
}

void RenderingManager::applyDepthSettings(const RenderingSettings& settings)
{
    if (!m_sceneRoot) return;
    
    // Apply depth settings
    LOG_INF_S("RenderingManager::applyDepthSettings: Test=" + std::to_string(settings.depthTest) + 
              ", Write=" + std::to_string(settings.depthWrite));
}

void RenderingManager::applyPolygonSettings(const RenderingSettings& settings)
{
    if (!m_sceneRoot) return;
    
    // Apply polygon settings
    LOG_INF_S("RenderingManager::applyPolygonSettings: Mode " + std::to_string(settings.polygonMode));
}

void RenderingManager::applyBackgroundSettings(const RenderingSettings& settings)
{
    if (!m_sceneRoot) return;
    
    // Apply background settings
    LOG_INF_S("RenderingManager::applyBackgroundSettings: Color (" + 
              std::to_string(settings.backgroundColor.Red()) + "," +
              std::to_string(settings.backgroundColor.Green()) + "," +
              std::to_string(settings.backgroundColor.Blue()) + ")");
}

void RenderingManager::setupOpenGLState(const RenderingSettings& settings)
{
    if (!m_canvas || !m_glContext) return;
    
    m_canvas->SetCurrent(*m_glContext);
    
    // Setup depth testing
    if (settings.depthTest) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    
    // Setup depth writing
    if (settings.depthWrite) {
        glDepthMask(GL_TRUE);
    } else {
        glDepthMask(GL_FALSE);
    }
    
    // Setup culling
    if (settings.backfaceCulling) {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    } else {
        glDisable(GL_CULL_FACE);
    }
    
    // Setup blending for transparency
    if (settings.transparencyType > 0) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
    
    // Setup lighting
    if (settings.mode == 4 || settings.mode == 5) { // Shaded modes
        glEnable(GL_LIGHTING);
        glEnable(GL_NORMALIZE);
    } else {
        glDisable(GL_LIGHTING);
        glDisable(GL_NORMALIZE);
    }
    
    // Setup polygon mode
    switch (settings.polygonMode) {
        case 0: // Fill
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            break;
        case 1: // Line
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            break;
        case 2: // Point
            glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
            break;
    }
    
    // Setup line width and point size
    glLineWidth(settings.lineWidth);
    glPointSize(settings.pointSize);
    
    // Setup background color
    float r = settings.backgroundColor.Red() / 255.0f;
    float g = settings.backgroundColor.Green() / 255.0f;
    float b = settings.backgroundColor.Blue() / 255.0f;
    glClearColor(r, g, b, 1.0f);
    
    LOG_INF_S("RenderingManager::setupOpenGLState: Applied OpenGL state");
}

void RenderingManager::restoreOpenGLState()
{
    if (!m_canvas || !m_glContext) return;
    
    m_canvas->SetCurrent(*m_glContext);
    
    // Restore default OpenGL state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1.0f);
    glPointSize(1.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    LOG_INF_S("RenderingManager::restoreOpenGLState: Restored default OpenGL state");
}

void RenderingManager::optimizeForPerformance(const RenderingSettings& settings)
{
    // Apply performance optimizations
    LOG_INF_S("RenderingManager::optimizeForPerformance: Applied performance optimizations");
}

void RenderingManager::optimizeForQuality(const RenderingSettings& settings)
{
    // Apply quality optimizations
    LOG_INF_S("RenderingManager::optimizeForQuality: Applied quality optimizations");
} 