#include "param/ParameterSystemIntegration.h"
#include "logger/Logger.h"
#include <algorithm>
#include <chrono>

// ParameterSystemIntegration implementation
ParameterSystemIntegration& ParameterSystemIntegration::getInstance() {
    static ParameterSystemIntegration instance;
    return instance;
}

ParameterSystemIntegration::ParameterSystemIntegration()
    : m_currentMode(IntegrationMode::LEGACY_ONLY)
    , m_initialized(false) {
    
    // Initialize performance metrics
    m_metrics.unifiedParameterCount = 0;
    m_metrics.legacyParameterCount = 0;
    m_metrics.syncOperationsPerformed = 0;
    m_metrics.averageSyncTime = std::chrono::milliseconds(0);
    m_metrics.migrationOperationsCompleted = 0;
    m_metrics.conflictResolutionsPerformed = 0;
    
    LOG_INF_S("ParameterSystemIntegration: Created integration manager");
}

ParameterSystemIntegration::~ParameterSystemIntegration() {
    shutdown();
}

bool ParameterSystemIntegration::initialize(const IntegrationConfig& config) {
    if (m_initialized) {
        LOG_WRN_S("ParameterSystemIntegration: Already initialized");
        return true;
    }
    
    m_config = config;
    m_currentMode = config.mode;
    
    LOG_INF_S("ParameterSystemIntegration: Initializing with mode " + 
              std::to_string(static_cast<int>(m_currentMode)));
    
    try {
        switch (m_currentMode) {
            case IntegrationMode::UNIFIED_ONLY:
                initializeUnifiedSystem();
                break;
            case IntegrationMode::LEGACY_ONLY:
                initializeLegacySystem();
                break;
            case IntegrationMode::HYBRID:
                initializeHybridMode();
                break;
            case IntegrationMode::MIGRATION:
                initializeLegacySystem();
                if (config.enableAutoMigration) {
                    migrateFromLegacyToUnified();
                }
                break;
        }
        
        if (m_config.enableAutoMigration) {
            setupAutoSync();
        }
        
        if (m_config.enableConflictResolution) {
            setupConflictResolution();
        }
        
        m_initialized = true;
        LOG_INF_S("ParameterSystemIntegration: Initialization completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("ParameterSystemIntegration: Initialization failed: " + std::string(e.what()));
        return false;
    }
}

void ParameterSystemIntegration::shutdown() {
    if (!m_initialized) {
        return;
    }
    
    LOG_INF_S("ParameterSystemIntegration: Shutting down");
    
    // Shutdown unified system if enabled
    if (isUnifiedSystemEnabled()) {
        auto& integration = UnifiedParameterIntegration::getInstance();
        integration.shutdown();
    }
    
    m_initialized = false;
    LOG_INF_S("ParameterSystemIntegration: Shutdown completed");
}

bool ParameterSystemIntegration::integrateWithExistingSystems() {
    if (!m_initialized) {
        LOG_ERR_S("ParameterSystemIntegration: Not initialized");
        return false;
    }
    
    LOG_INF_S("ParameterSystemIntegration: Integrating with existing systems");
    
    try {
        // Integrate with RenderingConfig
        auto& renderingConfig = RenderingConfig::getInstance();
        if (isUnifiedSystemEnabled()) {
            auto& integration = UnifiedParameterIntegration::getInstance();
            integration.integrateRenderingConfig(&renderingConfig);
        }
        
        // Integrate with MeshParameterManager
        auto& meshManager = MeshParameterManager::getInstance();
        if (isUnifiedSystemEnabled()) {
            auto& integration = UnifiedParameterIntegration::getInstance();
            integration.integrateMeshParameterManager(&meshManager);
        }
        
        // Integrate with LightingConfig
        auto& lightingConfig = LightingConfig::getInstance();
        if (isUnifiedSystemEnabled()) {
            auto& integration = UnifiedParameterIntegration::getInstance();
            integration.integrateLightingConfig(&lightingConfig);
        }
        
        // Perform initial synchronization
        if (m_currentMode == IntegrationMode::HYBRID) {
            LegacyCompatibilityLayer::syncMeshParameters();
            LegacyCompatibilityLayer::syncRenderingParameters();
            LegacyCompatibilityLayer::syncLightingParameters();
        }
        
        updatePerformanceMetrics();
        LOG_INF_S("ParameterSystemIntegration: Integration completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("ParameterSystemIntegration: Integration failed: " + std::string(e.what()));
        return false;
    }
}

bool ParameterSystemIntegration::enableUnifiedSystem() {
    if (!m_initialized) {
        LOG_ERR_S("ParameterSystemIntegration: Not initialized");
        return false;
    }
    
    LOG_INF_S("ParameterSystemIntegration: Enabling unified system");
    
    try {
        initializeUnifiedSystem();
        m_currentMode = IntegrationMode::UNIFIED_ONLY;
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("ParameterSystemIntegration: Failed to enable unified system: " + std::string(e.what()));
        return false;
    }
}

bool ParameterSystemIntegration::enableLegacySystem() {
    if (!m_initialized) {
        LOG_ERR_S("ParameterSystemIntegration: Not initialized");
        return false;
    }
    
    LOG_INF_S("ParameterSystemIntegration: Enabling legacy system");
    
    try {
        initializeLegacySystem();
        m_currentMode = IntegrationMode::LEGACY_ONLY;
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("ParameterSystemIntegration: Failed to enable legacy system: " + std::string(e.what()));
        return false;
    }
}

bool ParameterSystemIntegration::enableHybridMode() {
    if (!m_initialized) {
        LOG_ERR_S("ParameterSystemIntegration: Not initialized");
        return false;
    }
    
    LOG_INF_S("ParameterSystemIntegration: Enabling hybrid mode");
    
    try {
        initializeHybridMode();
        m_currentMode = IntegrationMode::HYBRID;
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("ParameterSystemIntegration: Failed to enable hybrid mode: " + std::string(e.what()));
        return false;
    }
}

bool ParameterSystemIntegration::migrateFromLegacyToUnified() {
    LOG_INF_S("ParameterSystemIntegration: Starting migration from legacy to unified system");
    
    try {
        // Initialize unified system
        initializeUnifiedSystem();
        
        // Migrate mesh parameters
        auto& meshManager = MeshParameterManager::getInstance();
        auto& integration = UnifiedParameterIntegration::getInstance();
        integration.integrateMeshParameterManager(&meshManager);
        
        // Migrate rendering parameters
        auto& renderingConfig = RenderingConfig::getInstance();
        integration.integrateRenderingConfig(&renderingConfig);
        
        // Migrate lighting parameters
        auto& lightingConfig = LightingConfig::getInstance();
        integration.integrateLightingConfig(&lightingConfig);
        
        // Perform synchronization
        integration.syncFromExistingSystems();
        
        m_currentMode = IntegrationMode::UNIFIED_ONLY;
        
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        m_metrics.migrationOperationsCompleted++;
        
        LOG_INF_S("ParameterSystemIntegration: Migration completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("ParameterSystemIntegration: Migration failed: " + std::string(e.what()));
        return false;
    }
}

bool ParameterSystemIntegration::migrateFromUnifiedToLegacy() {
    LOG_INF_S("ParameterSystemIntegration: Starting migration from unified to legacy system");
    
    try {
        // Initialize legacy system
        initializeLegacySystem();
        
        // Sync parameters back to legacy systems
        if (isUnifiedSystemEnabled()) {
            auto& integration = UnifiedParameterIntegration::getInstance();
            integration.syncToExistingSystems();
        }
        
        m_currentMode = IntegrationMode::LEGACY_ONLY;
        
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        m_metrics.migrationOperationsCompleted++;
        
        LOG_INF_S("ParameterSystemIntegration: Reverse migration completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("ParameterSystemIntegration: Reverse migration failed: " + std::string(e.what()));
        return false;
    }
}

bool ParameterSystemIntegration::validateMigration() {
    LOG_INF_S("ParameterSystemIntegration: Validating migration");
    
    try {
        bool isValid = true;
        
        // Validate unified system if enabled
        if (isUnifiedSystemEnabled()) {
            auto& integration = UnifiedParameterIntegration::getInstance();
            if (!integration.validateAllParameters()) {
                LOG_ERR_S("ParameterSystemIntegration: Unified system validation failed");
                isValid = false;
            }
        }
        
        // Validate legacy systems
        auto& meshManager = MeshParameterManager::getInstance();
        if (!meshManager.validateCurrentParameters()) {
            LOG_ERR_S("ParameterSystemIntegration: Mesh parameter validation failed");
            isValid = false;
        }
        
        LOG_INF_S("ParameterSystemIntegration: Migration validation " + 
                  std::string(isValid ? "passed" : "failed"));
        return isValid;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("ParameterSystemIntegration: Migration validation failed: " + std::string(e.what()));
        return false;
    }
}

bool ParameterSystemIntegration::isUnifiedSystemEnabled() const {
    return m_currentMode == IntegrationMode::UNIFIED_ONLY || 
           m_currentMode == IntegrationMode::HYBRID ||
           m_currentMode == IntegrationMode::MIGRATION;
}

bool ParameterSystemIntegration::isLegacySystemEnabled() const {
    return m_currentMode == IntegrationMode::LEGACY_ONLY || 
           m_currentMode == IntegrationMode::HYBRID ||
           m_currentMode == IntegrationMode::MIGRATION;
}

bool ParameterSystemIntegration::isHybridModeEnabled() const {
    return m_currentMode == IntegrationMode::HYBRID;
}

std::string ParameterSystemIntegration::getIntegrationStatus() const {
    std::ostringstream oss;
    oss << "Parameter System Integration Status:\n";
    oss << "- Current Mode: " << static_cast<int>(m_currentMode) << "\n";
    oss << "- Initialized: " << (m_initialized ? "Yes" : "No") << "\n";
    oss << "- Unified System: " << (isUnifiedSystemEnabled() ? "Enabled" : "Disabled") << "\n";
    oss << "- Legacy System: " << (isLegacySystemEnabled() ? "Enabled" : "Disabled") << "\n";
    oss << "- Hybrid Mode: " << (isHybridModeEnabled() ? "Enabled" : "Disabled") << "\n";
    
    if (m_initialized) {
        auto metrics = getPerformanceMetrics();
        oss << "- Unified Parameters: " << metrics.unifiedParameterCount << "\n";
        oss << "- Legacy Parameters: " << metrics.legacyParameterCount << "\n";
        oss << "- Sync Operations: " << metrics.syncOperationsPerformed << "\n";
        oss << "- Migration Operations: " << metrics.migrationOperationsCompleted << "\n";
    }
    
    return oss.str();
}

std::vector<std::string> ParameterSystemIntegration::getIntegrationDiagnostics() const {
    std::vector<std::string> diagnostics;
    
    diagnostics.push_back("=== Parameter System Integration Diagnostics ===");
    diagnostics.push_back("Current Mode: " + std::to_string(static_cast<int>(m_currentMode)));
    diagnostics.push_back("Initialized: " + std::string(m_initialized ? "Yes" : "No"));
    diagnostics.push_back("Unified System: " + std::string(isUnifiedSystemEnabled() ? "Enabled" : "Disabled"));
    diagnostics.push_back("Legacy System: " + std::string(isLegacySystemEnabled() ? "Enabled" : "Disabled"));
    diagnostics.push_back("Hybrid Mode: " + std::string(isHybridModeEnabled() ? "Enabled" : "Disabled"));
    
    if (m_initialized) {
        auto metrics = getPerformanceMetrics();
        diagnostics.push_back("Unified Parameters: " + std::to_string(metrics.unifiedParameterCount));
        diagnostics.push_back("Legacy Parameters: " + std::to_string(metrics.legacyParameterCount));
        diagnostics.push_back("Sync Operations: " + std::to_string(metrics.syncOperationsPerformed));
        diagnostics.push_back("Average Sync Time: " + std::to_string(metrics.averageSyncTime.count()) + "ms");
        diagnostics.push_back("Migration Operations: " + std::to_string(metrics.migrationOperationsCompleted));
        diagnostics.push_back("Conflict Resolutions: " + std::to_string(metrics.conflictResolutionsPerformed));
    }
    
    diagnostics.push_back("=== End Diagnostics ===");
    return diagnostics;
}

ParameterSystemIntegration::IntegrationPerformanceMetrics ParameterSystemIntegration::getPerformanceMetrics() const {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    return m_metrics;
}

void ParameterSystemIntegration::resetPerformanceMetrics() {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    m_metrics.unifiedParameterCount = 0;
    m_metrics.legacyParameterCount = 0;
    m_metrics.syncOperationsPerformed = 0;
    m_metrics.averageSyncTime = std::chrono::milliseconds(0);
    m_metrics.migrationOperationsCompleted = 0;
    m_metrics.conflictResolutionsPerformed = 0;
    
    LOG_INF_S("ParameterSystemIntegration: Performance metrics reset");
}

// Private methods
void ParameterSystemIntegration::initializeUnifiedSystem() {
    LOG_INF_S("ParameterSystemIntegration: Initializing unified system");
    
    auto& integration = UnifiedParameterIntegration::getInstance();
    
    UnifiedParameterIntegration::IntegrationConfig config;
    config.autoSyncEnabled = m_config.enableAutoMigration;
    config.bidirectionalSync = true;
    config.syncInterval = m_config.syncInterval;
    config.enableSmartBatching = m_config.enablePerformanceOptimization;
    config.enableDependencyTracking = true;
    config.enablePerformanceMonitoring = true;
    
    if (!integration.initialize(config)) {
        throw std::runtime_error("Failed to initialize unified parameter system");
    }
}

void ParameterSystemIntegration::initializeLegacySystem() {
    LOG_INF_S("ParameterSystemIntegration: Initializing legacy system");
    
    // Legacy systems are already initialized as singletons
    // Just ensure they are accessible
    auto& meshManager = MeshParameterManager::getInstance();
    auto& renderingConfig = RenderingConfig::getInstance();
    auto& lightingConfig = LightingConfig::getInstance();
    
    LOG_INF_S("ParameterSystemIntegration: Legacy systems initialized");
}

void ParameterSystemIntegration::initializeHybridMode() {
    LOG_INF_S("ParameterSystemIntegration: Initializing hybrid mode");
    
    // Initialize both systems
    initializeLegacySystem();
    initializeUnifiedSystem();
    
    // Set up integration
    integrateWithExistingSystems();
    
    LOG_INF_S("ParameterSystemIntegration: Hybrid mode initialized");
}

void ParameterSystemIntegration::setupAutoSync() {
    LOG_INF_S("ParameterSystemIntegration: Setting up auto sync");
    
    if (isUnifiedSystemEnabled()) {
        auto& integration = UnifiedParameterIntegration::getInstance();
        integration.enableAutoSync(true);
        integration.setSyncInterval(m_config.syncInterval);
    }
}

void ParameterSystemIntegration::setupConflictResolution() {
    LOG_INF_S("ParameterSystemIntegration: Setting up conflict resolution");
    
    // This would implement conflict resolution logic
    // For now, just log that it's set up
    LOG_INF_S("ParameterSystemIntegration: Conflict resolution configured");
}

void ParameterSystemIntegration::updatePerformanceMetrics() {
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    
    if (isUnifiedSystemEnabled()) {
        auto& integration = UnifiedParameterIntegration::getInstance();
        auto report = integration.getPerformanceReport();
        m_metrics.unifiedParameterCount = report.totalParameters;
    }
    
    // Update legacy parameter count
    auto& meshManager = MeshParameterManager::getInstance();
    // This would need to be implemented in MeshParameterManager
    // m_metrics.legacyParameterCount = meshManager.getParameterCount();
    
    m_metrics.syncOperationsPerformed++;
}

// LegacyCompatibilityLayer implementation
MeshParameterManager& ParameterSystemIntegration::LegacyCompatibilityLayer::getMeshParameterManager() {
    return MeshParameterManager::getInstance();
}

void ParameterSystemIntegration::LegacyCompatibilityLayer::syncMeshParameters() {
    LOG_DBG_S("LegacyCompatibilityLayer: Syncing mesh parameters");
    
    auto& meshManager = getMeshParameterManager();
    auto& integration = UnifiedParameterIntegration::getInstance();
    
    // Sync mesh parameters to unified system
    integration.integrateMeshParameterManager(&meshManager);
}

RenderingConfig& ParameterSystemIntegration::LegacyCompatibilityLayer::getRenderingConfig() {
    return RenderingConfig::getInstance();
}

void ParameterSystemIntegration::LegacyCompatibilityLayer::syncRenderingParameters() {
    LOG_DBG_S("LegacyCompatibilityLayer: Syncing rendering parameters");
    
    auto& renderingConfig = getRenderingConfig();
    auto& integration = UnifiedParameterIntegration::getInstance();
    
    // Sync rendering parameters to unified system
    integration.integrateRenderingConfig(&renderingConfig);
}

LightingConfig& ParameterSystemIntegration::LegacyCompatibilityLayer::getLightingConfig() {
    return LightingConfig::getInstance();
}

void ParameterSystemIntegration::LegacyCompatibilityLayer::syncLightingParameters() {
    LOG_DBG_S("LegacyCompatibilityLayer: Syncing lighting parameters");
    
    auto& lightingConfig = getLightingConfig();
    auto& integration = UnifiedParameterIntegration::getInstance();
    
    // Sync lighting parameters to unified system
    integration.integrateLightingConfig(&lightingConfig);
}