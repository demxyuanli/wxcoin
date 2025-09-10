#include "viewer/OutlinePassManager.h"
#include "SceneManager.h"
#include "logger/Logger.h"

OutlinePassManager::OutlinePassManager(SceneManager* sceneManager, SoSeparator* captureRoot)
    : m_sceneManager(sceneManager), m_captureRoot(captureRoot) {
    
    LOG_INF("OutlinePassManager constructed", "OutlinePassManager");
    
    // Initialize with enhanced mode by default
    initializePasses();
    setOutlineMode(OutlineMode::Enhanced);
}

OutlinePassManager::~OutlinePassManager() {
    LOG_INF("OutlinePassManager destructor", "OutlinePassManager");
    
    setEnabled(false);
    
    // Cleanup is handled by unique_ptr
    m_legacyPass.reset();
    m_enhancedPass.reset();
}

void OutlinePassManager::setEnabled(bool enabled) {
    if (m_enabled == enabled) return;
    
    m_enabled = enabled;
    LOG_INF(("setEnabled " + std::string(enabled ? "true" : "false")).c_str(), "OutlinePassManager");
    
    if (m_currentMode == OutlineMode::Legacy && m_legacyPass) {
        m_legacyPass->setEnabled(enabled);
    } else if (m_currentMode == OutlineMode::Enhanced && m_enhancedPass) {
        m_enhancedPass->setEnabled(enabled);
    }
}

bool OutlinePassManager::isEnabled() const {
    return m_enabled;
}

void OutlinePassManager::setOutlineMode(OutlineMode mode) {
    if (m_currentMode == mode) return;
    
    std::string modeStr = "Switching outline mode to " + 
                         (mode == OutlineMode::Legacy ? "Legacy" : "Enhanced");
    LOG_INF(modeStr.c_str(), "OutlinePassManager");
    
    // Disable current mode
    if (m_currentMode == OutlineMode::Legacy && m_legacyPass) {
        m_legacyPass->setEnabled(false);
    } else if (m_currentMode == OutlineMode::Enhanced && m_enhancedPass) {
        m_enhancedPass->setEnabled(false);
    }
    
    // Migrate parameters
    if (mode == OutlineMode::Enhanced) {
        migrateLegacyToEnhanced();
    } else {
        migrateEnhancedToLegacy();
    }
    
    m_currentMode = mode;
    
    // Enable new mode
    if (m_enabled) {
        setEnabled(true);
    }
}

void OutlinePassManager::setLegacyParams(const ImageOutlineParams& params) {
    m_legacyParams = params;
    
    if (m_currentMode == OutlineMode::Legacy && m_legacyPass) {
        m_legacyPass->setParams(params);
    }
    
    LOG_DBG("Legacy parameters updated", "OutlinePassManager");
}

void OutlinePassManager::setEnhancedParams(const EnhancedOutlineParams& params) {
    m_enhancedParams = params;
    
    if (m_currentMode == OutlineMode::Enhanced && m_enhancedPass) {
        m_enhancedPass->setParams(params);
    }
    
    LOG_DBG("Enhanced parameters updated", "OutlinePassManager");
}

ImageOutlineParams OutlinePassManager::getLegacyParams() const {
    if (m_currentMode == OutlineMode::Legacy && m_legacyPass) {
        return m_legacyPass->getParams();
    }
    return m_legacyParams;
}

EnhancedOutlineParams OutlinePassManager::getEnhancedParams() const {
    if (m_currentMode == OutlineMode::Enhanced && m_enhancedPass) {
        return m_enhancedPass->getParams();
    }
    return m_enhancedParams;
}

void OutlinePassManager::setSelectionRoot(SoSelection* selectionRoot) {
    m_selectionRoot = selectionRoot;
    
    if (m_enhancedPass) {
        m_enhancedPass->setSelectionRoot(selectionRoot);
    }
}

void OutlinePassManager::setHoveredObject(int objectId) {
    if (m_enhancedPass) {
        m_enhancedPass->setHoveredObject(objectId);
    }
}

void OutlinePassManager::clearHover() {
    if (m_enhancedPass) {
        m_enhancedPass->clearHover();
    }
}

void OutlinePassManager::setPerformanceMode(bool enabled) {
    m_performanceMode = enabled;
    m_qualityMode = false;
    
    if (enabled) {
        updatePerformanceSettings();
    }
    
    LOG_INF(("Performance mode " + std::string(enabled ? "enabled" : "disabled")).c_str(), 
            "OutlinePassManager");
}

void OutlinePassManager::setQualityMode(bool enabled) {
    m_qualityMode = enabled;
    m_performanceMode = false;
    
    if (enabled) {
        updatePerformanceSettings();
    }
    
    LOG_INF(("Quality mode " + std::string(enabled ? "enabled" : "disabled")).c_str(), 
            "OutlinePassManager");
}

void OutlinePassManager::setBalancedMode() {
    m_performanceMode = false;
    m_qualityMode = false;
    updatePerformanceSettings();
    
    LOG_INF("Balanced mode enabled", "OutlinePassManager");
}

void OutlinePassManager::setDebugMode(int mode) {
    if (m_currentMode == OutlineMode::Legacy && m_legacyPass) {
        m_legacyPass->setDebugOutput(static_cast<ImageOutlinePass::DebugOutput>(mode));
    } else if (m_currentMode == OutlineMode::Enhanced && m_enhancedPass) {
        m_enhancedPass->setDebugMode(static_cast<OutlineDebugMode>(mode));
    }
}

void OutlinePassManager::enableDebugVisualization(bool enabled) {
    m_debugVisualization = enabled;
    
    if (enabled) {
        setDebugMode(static_cast<int>(OutlineDebugMode::ShowEdgeMask));
    } else {
        setDebugMode(static_cast<int>(OutlineDebugMode::Final));
    }
}

void OutlinePassManager::refresh() {
    if (m_currentMode == OutlineMode::Legacy && m_legacyPass) {
        m_legacyPass->refresh();
    } else if (m_currentMode == OutlineMode::Enhanced && m_enhancedPass) {
        m_enhancedPass->refresh();
    }
}

void OutlinePassManager::forceUpdate() {
    if (m_currentMode == OutlineMode::Legacy && m_legacyPass) {
        m_legacyPass->refresh();
    } else if (m_currentMode == OutlineMode::Enhanced && m_enhancedPass) {
        m_enhancedPass->forceUpdate();
    }
}

OutlinePassManager::PerformanceStats OutlinePassManager::getPerformanceStats() const {
    return m_performanceStats;
}

void OutlinePassManager::initializePasses() {
    // Initialize legacy pass
    m_legacyPass = std::make_unique<ImageOutlinePass>(m_sceneManager, m_captureRoot);
    
    // Initialize enhanced pass
    m_enhancedPass = std::make_unique<EnhancedOutlinePass>(m_sceneManager, m_captureRoot);
    
    LOG_INF("Both outline passes initialized", "OutlinePassManager");
}

void OutlinePassManager::migrateLegacyToEnhanced() {
    if (!m_legacyPass || !m_enhancedPass) return;
    
    // Get current legacy parameters
    ImageOutlineParams legacyParams = m_legacyPass->getParams();
    
    // Convert to enhanced parameters
    EnhancedOutlineParams enhancedParams;
    enhancedParams.depthWeight = legacyParams.depthWeight;
    enhancedParams.normalWeight = legacyParams.normalWeight;
    enhancedParams.depthThreshold = legacyParams.depthThreshold;
    enhancedParams.normalThreshold = legacyParams.normalThreshold;
    enhancedParams.edgeIntensity = legacyParams.edgeIntensity;
    enhancedParams.thickness = legacyParams.thickness;
    
    // Set default values for new parameters
    enhancedParams.colorWeight = 0.3f;
    enhancedParams.colorThreshold = 0.1f;
    enhancedParams.glowIntensity = 0.0f;
    enhancedParams.glowRadius = 2.0f;
    enhancedParams.adaptiveThreshold = 1.0f;
    enhancedParams.smoothingFactor = 0.5f;
    enhancedParams.backgroundFade = 0.8f;
    
    // Set colors
    enhancedParams.outlineColor[0] = 0.0f; // Black
    enhancedParams.outlineColor[1] = 0.0f;
    enhancedParams.outlineColor[2] = 0.0f;
    
    enhancedParams.glowColor[0] = 1.0f; // Yellow
    enhancedParams.glowColor[1] = 1.0f;
    enhancedParams.glowColor[2] = 0.0f;
    
    enhancedParams.backgroundColor[0] = 0.2f; // Dark gray
    enhancedParams.backgroundColor[1] = 0.2f;
    enhancedParams.backgroundColor[2] = 0.2f;
    
    // Apply to enhanced pass
    m_enhancedPass->setParams(enhancedParams);
    m_enhancedParams = enhancedParams;
    
    LOG_INF("Parameters migrated from Legacy to Enhanced", "OutlinePassManager");
}

void OutlinePassManager::migrateEnhancedToLegacy() {
    if (!m_legacyPass || !m_enhancedPass) return;
    
    // Get current enhanced parameters
    EnhancedOutlineParams enhancedParams = m_enhancedPass->getParams();
    
    // Convert to legacy parameters
    ImageOutlineParams legacyParams;
    legacyParams.depthWeight = enhancedParams.depthWeight;
    legacyParams.normalWeight = enhancedParams.normalWeight;
    legacyParams.depthThreshold = enhancedParams.depthThreshold;
    legacyParams.normalThreshold = enhancedParams.normalThreshold;
    legacyParams.edgeIntensity = enhancedParams.edgeIntensity;
    legacyParams.thickness = enhancedParams.thickness;
    
    // Apply to legacy pass
    m_legacyPass->setParams(legacyParams);
    m_legacyParams = legacyParams;
    
    LOG_INF("Parameters migrated from Enhanced to Legacy", "OutlinePassManager");
}

void OutlinePassManager::updatePerformanceSettings() {
    if (!m_enhancedPass) return;
    
    if (m_performanceMode) {
        // Performance mode: prioritize speed
        m_enhancedPass->setDownsampleFactor(2);
        m_enhancedPass->setEarlyCullingEnabled(true);
        m_enhancedPass->setMultiSampleEnabled(false);
        
        // Reduce quality settings
        EnhancedOutlineParams params = m_enhancedPass->getParams();
        params.smoothingFactor = 0.2f;
        params.glowIntensity = 0.0f;
        m_enhancedPass->setParams(params);
        
        m_performanceStats.isOptimized = true;
        
    } else if (m_qualityMode) {
        // Quality mode: prioritize visual quality
        m_enhancedPass->setDownsampleFactor(1);
        m_enhancedPass->setEarlyCullingEnabled(false);
        m_enhancedPass->setMultiSampleEnabled(true);
        
        // Increase quality settings
        EnhancedOutlineParams params = m_enhancedPass->getParams();
        params.smoothingFactor = 0.8f;
        params.glowIntensity = 0.3f;
        m_enhancedPass->setParams(params);
        
        m_performanceStats.isOptimized = false;
        
    } else {
        // Balanced mode: good balance of speed and quality
        m_enhancedPass->setDownsampleFactor(1);
        m_enhancedPass->setEarlyCullingEnabled(true);
        m_enhancedPass->setMultiSampleEnabled(false);
        
        // Balanced quality settings
        EnhancedOutlineParams params = m_enhancedPass->getParams();
        params.smoothingFactor = 0.5f;
        params.glowIntensity = 0.1f;
        m_enhancedPass->setParams(params);
        
        m_performanceStats.isOptimized = true;
    }
    
    logPerformanceInfo();
}

void OutlinePassManager::logPerformanceInfo() {
    std::string mode = m_performanceMode ? "Performance" : 
                      (m_qualityMode ? "Quality" : "Balanced");
    
    std::string message = "Performance settings updated: " + mode + " mode";
    LOG_INF(message.c_str(), "OutlinePassManager");
    
    if (m_enhancedPass) {
        EnhancedOutlineParams params = m_enhancedPass->getParams();
        std::string settingsMsg = "Current settings - smoothing: " + std::to_string(params.smoothingFactor) +
                                 ", glow: " + std::to_string(params.glowIntensity);
        LOG_DBG(settingsMsg.c_str(), "OutlinePassManager");
    }
}