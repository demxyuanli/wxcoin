#pragma once

#include "UnifiedParameterIntegration.h"
#include "MeshParameterManager.h"
#include "config/RenderingConfig.h"
#include "config/LightingConfig.h"

/**
 * @brief Parameter system integration configuration
 * Provides configuration and initialization for integrating unified parameter system with existing systems
 */
class ParameterSystemIntegration {
public:
    // Integration modes
    enum class IntegrationMode {
        UNIFIED_ONLY,       // Use only unified parameter system
        LEGACY_ONLY,        // Use only legacy parameter system
        HYBRID,             // Use both systems with automatic synchronization
        MIGRATION           // Migrate from legacy to unified system
    };

    // Integration configuration
    struct IntegrationConfig {
        IntegrationMode mode = IntegrationMode::HYBRID;
        bool enableAutoMigration = true;
        bool enableBackwardCompatibility = true;
        bool enablePerformanceOptimization = true;
        std::chrono::milliseconds syncInterval = std::chrono::milliseconds(100);
        bool enableConflictResolution = true;
        bool enableLogging = true;
    };

    static ParameterSystemIntegration& getInstance();

    // Initialization
    bool initialize(const IntegrationConfig& config = IntegrationConfig{});
    void shutdown();

    // System integration
    bool integrateWithExistingSystems();
    bool enableUnifiedSystem();
    bool enableLegacySystem();
    bool enableHybridMode();

    // Migration support
    bool migrateFromLegacyToUnified();
    bool migrateFromUnifiedToLegacy();
    bool validateMigration();

    // Compatibility layer
    class LegacyCompatibilityLayer {
    public:
        // MeshParameterManager compatibility
        static MeshParameterManager& getMeshParameterManager();
        static void syncMeshParameters();
        
        // RenderingConfig compatibility
        static RenderingConfig& getRenderingConfig();
        static void syncRenderingParameters();
        
        // LightingConfig compatibility
        static LightingConfig& getLightingConfig();
        static void syncLightingParameters();
    };

    // Status and diagnostics
    IntegrationMode getCurrentMode() const { return m_currentMode; }
    bool isUnifiedSystemEnabled() const;
    bool isLegacySystemEnabled() const;
    bool isHybridModeEnabled() const;
    
    std::string getIntegrationStatus() const;
    std::vector<std::string> getIntegrationDiagnostics() const;

    // Performance monitoring
    struct IntegrationPerformanceMetrics {
        size_t unifiedParameterCount;
        size_t legacyParameterCount;
        size_t syncOperationsPerformed;
        std::chrono::milliseconds averageSyncTime;
        size_t migrationOperationsCompleted;
        size_t conflictResolutionsPerformed;
    };
    
    IntegrationPerformanceMetrics getPerformanceMetrics() const;
    void resetPerformanceMetrics();

private:
    ParameterSystemIntegration();
    ~ParameterSystemIntegration();
    ParameterSystemIntegration(const ParameterSystemIntegration&) = delete;
    ParameterSystemIntegration& operator=(const ParameterSystemIntegration&) = delete;

    IntegrationConfig m_config;
    IntegrationMode m_currentMode;
    bool m_initialized;
    
    // Performance metrics
    mutable std::mutex m_metricsMutex;
    IntegrationPerformanceMetrics m_metrics;
    
    // Internal methods
    void initializeUnifiedSystem();
    void initializeLegacySystem();
    void initializeHybridMode();
    void setupAutoSync();
    void setupConflictResolution();
    void updatePerformanceMetrics();
};

// Convenience macros for backward compatibility
#ifdef LEGACY_PARAMETER_SYSTEM_ENABLED
#define GET_MESH_PARAM_MANAGER() ParameterSystemIntegration::LegacyCompatibilityLayer::getMeshParameterManager()
#define GET_RENDERING_CONFIG() ParameterSystemIntegration::LegacyCompatibilityLayer::getRenderingConfig()
#define GET_LIGHTING_CONFIG() ParameterSystemIntegration::LegacyCompatibilityLayer::getLightingConfig()
#endif

#ifdef UNIFIED_PARAMETER_SYSTEM_ENABLED
#define GET_UNIFIED_PARAM_INTEGRATION() UnifiedParameterIntegration::getInstance()
#define UNIFIED_PARAM_SET(path, value) GET_UNIFIED_PARAM_INTEGRATION().setParameter(path, value)
#define UNIFIED_PARAM_GET(path) GET_UNIFIED_PARAM_INTEGRATION().getParameter(path)
#endif

// Hybrid mode macros
#ifdef PARAMETER_SYSTEM_INTEGRATION_ENABLED
#define GET_PARAM_INTEGRATION() ParameterSystemIntegration::getInstance()
#define SYNC_ALL_PARAMETERS() GET_PARAM_INTEGRATION().integrateWithExistingSystems()
#endif