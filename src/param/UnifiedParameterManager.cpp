#include "param/UnifiedParameterManager.h"
#include "param/ParameterTreeBuilder.h"
#include "param/ParameterUpdateManagerInitializer.h"
#include "param/ParameterSynchronizerInitializer.h"
#include "OCCGeometry.h"
#include "config/RenderingConfig.h"

// UnifiedParameterManager implementation
UnifiedParameterManager& UnifiedParameterManager::getInstance() {
    static UnifiedParameterManager instance;
    return instance;
}

UnifiedParameterManager::UnifiedParameterManager()
    : m_initialized(false)
    , m_inBatchOperation(false) {
}

void UnifiedParameterManager::initialize() {
    if (m_initialized) {
        return;
    }
    
    // Initialize all subsystems
    UnifiedParameterManagerInitializer::initialize();
    
    // Set up system integration
    setupSystemIntegration();
    
    // Set up default parameters
    setupDefaultParameters();
    
    // Set up update callbacks
    setupUpdateCallbacks();
    
    m_initialized = true;
}

void UnifiedParameterManager::shutdown() {
    if (!m_initialized) {
        return;
    }
    
    // Clean up resources
    m_initialized = false;
}

bool UnifiedParameterManager::setParameter(const std::string& path, const ParameterValue& value) {
    if (!m_initialized) {
        return false;
    }
    
    auto& tree = ParameterTree::getInstance();
    bool success = tree.setParameterValue(path, value);
    
    if (success && m_inBatchOperation) {
        m_batchChangedParameters.push_back(path);
    }
    
    return success;
}

ParameterValue UnifiedParameterManager::getParameter(const std::string& path) const {
    if (!m_initialized) {
        return ParameterValue{};
    }
    
    auto& tree = ParameterTree::getInstance();
    return tree.getParameterValue(path);
}

bool UnifiedParameterManager::hasParameter(const std::string& path) const {
    if (!m_initialized) {
        return false;
    }
    
    auto& tree = ParameterTree::getInstance();
    return tree.hasParameter(path);
}

void UnifiedParameterManager::beginBatchOperation() {
    if (!m_initialized) {
        return;
    }
    
    m_inBatchOperation = true;
    m_batchChangedParameters.clear();
    
    // Begin batch operations in subsystems
    ParameterTree::getInstance().beginBatchUpdate();
    ParameterUpdateManager::getInstance().beginBatchUpdate();
    ParameterSynchronizer::getInstance().beginBatchSync();
}

void UnifiedParameterManager::endBatchOperation() {
    if (!m_initialized || !m_inBatchOperation) {
        return;
    }
    
    // End batch operations in subsystems
    ParameterSynchronizer::getInstance().endBatchSync();
    ParameterUpdateManager::getInstance().endBatchUpdate();
    ParameterTree::getInstance().endBatchUpdate();
    
    m_inBatchOperation = false;
    m_batchChangedParameters.clear();
}

bool UnifiedParameterManager::isInBatchOperation() const {
    return m_inBatchOperation;
}

void UnifiedParameterManager::registerGeometry(std::shared_ptr<OCCGeometry> geometry) {
    if (!m_initialized || !geometry) {
        return;
    }
    
    // Register with synchronizer
    auto& synchronizer = ParameterSynchronizer::getInstance();
    synchronizer.synchronizeGeometry(geometry);
    
    // Register with update manager
    auto& updateManager = ParameterUpdateManager::getInstance();
    auto updateInterface = std::make_shared<GeometryUpdateInterface>(geometry);
    updateManager.registerUpdateInterface(updateInterface);
}

void UnifiedParameterManager::unregisterGeometry(std::shared_ptr<OCCGeometry> geometry) {
    if (!m_initialized || !geometry) {
        return;
    }
    
    // Unregister from synchronizer
    auto& synchronizer = ParameterSynchronizer::getInstance();
    synchronizer.unsynchronizeGeometry(geometry);
    
    // Note: Update interfaces are managed by shared_ptr, so they'll be cleaned up automatically
}

void UnifiedParameterManager::registerRenderingConfig(RenderingConfig* config) {
    if (!m_initialized || !config) {
        return;
    }
    
    // Register with synchronizer
    auto& synchronizer = ParameterSynchronizer::getInstance();
    synchronizer.synchronizeRenderingConfig(config);
    
    // Register with update manager
    auto& updateManager = ParameterUpdateManager::getInstance();
    auto updateInterface = std::make_shared<RenderingConfigUpdateInterface>(config);
    updateManager.registerUpdateInterface(updateInterface);
}

void UnifiedParameterManager::unregisterRenderingConfig(RenderingConfig* config) {
    if (!m_initialized || !config) {
        return;
    }
    
    // Unregister from synchronizer
    auto& synchronizer = ParameterSynchronizer::getInstance();
    synchronizer.unsynchronizeRenderingConfig(config);
    
    // Note: Update interfaces are managed by shared_ptr, so they'll be cleaned up automatically
}

void UnifiedParameterManager::enableOptimization(bool enable) {
    if (!m_initialized) {
        return;
    }
    
    auto& updateManager = ParameterUpdateManager::getInstance();
    updateManager.enableUpdateOptimization(enable);
}

void UnifiedParameterManager::setUpdateFrequencyLimit(int maxUpdatesPerSecond) {
    if (!m_initialized) {
        return;
    }
    
    auto& updateManager = ParameterUpdateManager::getInstance();
    updateManager.setUpdateFrequencyLimit(maxUpdatesPerSecond);
}

void UnifiedParameterManager::enableDebugMode(bool enable) {
    if (!m_initialized) {
        return;
    }
    
    auto& updateManager = ParameterUpdateManager::getInstance();
    updateManager.enableDebugMode(enable);
}

size_t UnifiedParameterManager::getRegisteredGeometryCount() const {
    if (!m_initialized) {
        return 0;
    }
    
    // This would require the synchronizer to expose this information
    return 0; // Placeholder
}

size_t UnifiedParameterManager::getRegisteredConfigCount() const {
    if (!m_initialized) {
        return 0;
    }
    
    // This would require the synchronizer to expose this information
    return 0; // Placeholder
}

std::vector<std::string> UnifiedParameterManager::getAllParameterPaths() const {
    if (!m_initialized) {
        return {};
    }
    
    auto& tree = ParameterTree::getInstance();
    return tree.getAllParameterPaths();
}

std::vector<std::string> UnifiedParameterManager::getChangedParameters() const {
    if (!m_initialized) {
        return {};
    }
    
    if (m_inBatchOperation) {
        return m_batchChangedParameters;
    }
    
    return {};
}

void UnifiedParameterManager::setupSystemIntegration() {
    // Set up integration between subsystems
    auto& tree = ParameterTree::getInstance();
    auto& updateManager = ParameterUpdateManager::getInstance();
    auto& synchronizer = ParameterSynchronizer::getInstance();
    
    // Connect parameter tree changes to update manager
    tree.addGlobalChangedCallback([&updateManager](const std::string& path, const ParameterValue& value) {
        updateManager.onParameterChanged(path, value);
    });
    
    // Set up batch update callback
    tree.setBatchUpdateCallback([&updateManager](const std::vector<std::string>& changedPaths) {
        updateManager.onBatchUpdate(changedPaths);
    });
}

void UnifiedParameterManager::setupDefaultParameters() {
    // Build all parameter trees
    ParameterTreeBuilder::buildGeometryParameterTree();
    ParameterTreeBuilder::buildRenderingParameterTree();
    ParameterTreeBuilder::buildDisplayParameterTree();
    ParameterTreeBuilder::buildQualityParameterTree();
    ParameterTreeBuilder::buildLightingParameterTree();
    ParameterTreeBuilder::buildMaterialParameterTree();
    ParameterTreeBuilder::buildTextureParameterTree();
    ParameterTreeBuilder::buildShadowParameterTree();
}

void UnifiedParameterManager::setupUpdateCallbacks() {
    // Set up any additional update callbacks needed
}

// UnifiedParameterManagerInitializer implementation
void UnifiedParameterManagerInitializer::initialize() {
    initializeParameterTree();
    initializeUpdateManager();
    initializeSynchronizer();
    setupSystemIntegration();
}

void UnifiedParameterManagerInitializer::initializeParameterTree() {
    // Parameter tree is initialized automatically when first accessed
    // Additional initialization can be done here if needed
}

void UnifiedParameterManagerInitializer::initializeUpdateManager() {
    ParameterUpdateManagerInitializer::initialize();
}

void UnifiedParameterManagerInitializer::initializeSynchronizer() {
    ParameterSynchronizerInitializer::initialize();
}

void UnifiedParameterManagerInitializer::setupSystemIntegration() {
    // Set up integration between different subsystems
    // This is handled in UnifiedParameterManager::setupSystemIntegration()
}