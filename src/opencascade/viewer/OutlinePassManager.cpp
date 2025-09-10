#include "viewer/OutlinePassManager.h"

#include "SceneManager.h"
#include "logger/Logger.h"

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSelection.h>

OutlinePassManager::OutlinePassManager(SceneManager* sceneManager, SoSeparator* captureRoot)
    : m_sceneManager(sceneManager), m_captureRoot(captureRoot) {
    
    LOG_INF("OutlinePassManager constructed", "OutlinePassManager");
    
    // Initialize with default settings
    m_currentMode = OutlineMode::Disabled;
    m_performanceMode = false;
    m_qualityMode = false;
    m_debugVisualization = false;
}

OutlinePassManager::~OutlinePassManager() {
    LOG_INF("OutlinePassManager destructor", "OutlinePassManager");
    
    // Clean up passes
    if (m_legacyPass) {
        m_legacyPass->setEnabled(false);
        m_legacyPass.reset();
    }
    
    if (m_enhancedPass) {
        m_enhancedPass->setEnabled(false);
        m_enhancedPass.reset();
    }
}

void OutlinePassManager::setOutlineMode(OutlineMode mode) {
    if (m_currentMode == mode) return;
    
    std::string modeStr = "Switching outline mode to ";
    if (mode == OutlineMode::Legacy) {
        modeStr += "Legacy";
    } else if (mode == OutlineMode::Enhanced) {
        modeStr += "Enhanced";
    } else {
        modeStr += "Disabled";
    }
    
    LOG_INF(modeStr.c_str(), "OutlinePassManager");
    
    // Disable current mode
    if (m_currentMode == OutlineMode::Legacy && m_legacyPass) {
        m_legacyPass->setEnabled(false);
    } else if (m_currentMode == OutlineMode::Enhanced && m_enhancedPass) {
        m_enhancedPass->setEnabled(false);
    }
    
    // Enable new mode
    m_currentMode = mode;
    
    if (mode == OutlineMode::Legacy) {
        if (!m_legacyPass) {
            m_legacyPass = std::make_unique<ImageOutlinePass>(m_sceneManager, m_captureRoot);
        }
        m_legacyPass->setEnabled(true);
    } else if (mode == OutlineMode::Enhanced) {
        if (!m_enhancedPass) {
            m_enhancedPass = std::make_unique<EnhancedOutlinePass>(m_sceneManager, m_captureRoot);
        }
        m_enhancedPass->setEnabled(true);
    }
}

OutlinePassManager::OutlineMode OutlinePassManager::getOutlineMode() const {
    return m_currentMode;
}

void OutlinePassManager::setLegacyParams(const ImageOutlineParams& params) {
    if (m_legacyPass) {
        m_legacyPass->setParams(params);
    }
}

ImageOutlineParams OutlinePassManager::getLegacyParams() const {
    if (m_legacyPass) {
        return m_legacyPass->getParams();
    }
    return ImageOutlineParams();
}

void OutlinePassManager::setEnhancedParams(const EnhancedOutlineParams& params) {
    if (m_enhancedPass) {
        m_enhancedPass->setParams(params);
    }
}

EnhancedOutlineParams OutlinePassManager::getEnhancedParams() const {
    if (m_enhancedPass) {
        return m_enhancedPass->getParams();
    }
    return EnhancedOutlineParams();
}

void OutlinePassManager::setSelectionRoot(SoSelection* selectionRoot) {
    if (m_enhancedPass) {
        m_enhancedPass->setSelectionRoot(selectionRoot);
    }
}

void OutlinePassManager::setSelectedObjects(const std::vector<int>& objectIds) {
    if (m_enhancedPass) {
        m_enhancedPass->setSelectedObjects(objectIds);
    }
}

void OutlinePassManager::setHoveredObject(int objectId) {
    if (m_enhancedPass) {
        m_enhancedPass->setHoveredObject(objectId);
    }
}

void OutlinePassManager::clearSelection() {
    if (m_enhancedPass) {
        m_enhancedPass->setSelectedObjects({});
    }
}

void OutlinePassManager::clearHover() {
    if (m_enhancedPass) {
        m_enhancedPass->clearHover();
    }
}

void OutlinePassManager::setEnabled(bool enabled) {
    std::string enabledStr = enabled ? "true" : "false";
    LOG_INF(("setEnabled " + enabledStr).c_str(), "OutlinePassManager");
    
    if (enabled) {
        // Enable current mode
        if (m_currentMode == OutlineMode::Legacy && m_legacyPass) {
            m_legacyPass->setEnabled(true);
        } else if (m_currentMode == OutlineMode::Enhanced && m_enhancedPass) {
            m_enhancedPass->setEnabled(true);
        }
    } else {
        // Disable all modes
        if (m_legacyPass) {
            m_legacyPass->setEnabled(false);
        }
        if (m_enhancedPass) {
            m_enhancedPass->setEnabled(false);
        }
    }
}

bool OutlinePassManager::isEnabled() const {
    if (m_currentMode == OutlineMode::Legacy && m_legacyPass) {
        return m_legacyPass->isEnabled();
    } else if (m_currentMode == OutlineMode::Enhanced && m_enhancedPass) {
        return m_enhancedPass->isEnabled();
    }
    return false;
}

void OutlinePassManager::setPerformanceMode(bool enabled) {
    m_performanceMode = enabled;
    m_qualityMode = false;
    
    if (m_enhancedPass) {
        m_enhancedPass->setDownsampleFactor(2);
        m_enhancedPass->setEarlyCullingEnabled(true);
        m_enhancedPass->setMultiSampleEnabled(false);
    }
    
    LOG_INF("Performance mode enabled", "OutlinePassManager");
}

void OutlinePassManager::setQualityMode(bool enabled) {
    m_qualityMode = enabled;
    m_performanceMode = false;
    
    if (m_enhancedPass) {
        m_enhancedPass->setDownsampleFactor(1);
        m_enhancedPass->setEarlyCullingEnabled(false);
        m_enhancedPass->setMultiSampleEnabled(true);
    }
    
    LOG_INF("Quality mode enabled", "OutlinePassManager");
}

void OutlinePassManager::setBalancedMode() {
    m_performanceMode = false;
    m_qualityMode = false;
    
    if (m_enhancedPass) {
        m_enhancedPass->setDownsampleFactor(1);
        m_enhancedPass->setEarlyCullingEnabled(true);
        m_enhancedPass->setMultiSampleEnabled(false);
    }
    
    LOG_INF("Balanced mode enabled", "OutlinePassManager");
}

void OutlinePassManager::setDebugMode(OutlineDebugMode mode) {
    if (m_currentMode == OutlineMode::Legacy && m_legacyPass) {
        m_legacyPass->setDebugOutput(static_cast<ImageOutlinePass::DebugOutput>(static_cast<int>(mode)));
    } else if (m_currentMode == OutlineMode::Enhanced && m_enhancedPass) {
        m_enhancedPass->setDebugMode(mode);
    }
}

void OutlinePassManager::enableDebugVisualization(bool enabled) {
    m_debugVisualization = enabled;
    
    if (enabled) {
        setDebugMode(OutlineDebugMode::ShowEdges);
    } else {
        setDebugMode(OutlineDebugMode::None);
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
        m_legacyPass->forceUpdate();
    } else if (m_currentMode == OutlineMode::Enhanced && m_enhancedPass) {
        m_enhancedPass->forceUpdate();
    }
}

void OutlinePassManager::updatePerformanceSettings() {
    if (m_performanceMode) {
        setPerformanceMode(true);
    } else if (m_qualityMode) {
        setQualityMode(true);
    } else {
        setBalancedMode();
    }
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