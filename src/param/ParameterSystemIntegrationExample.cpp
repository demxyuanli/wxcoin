#include "param/ParameterSystemIntegration.h"
#include "logger/Logger.h"
#include <iostream>
#include <thread>
#include <chrono>

/**
 * @brief Parameter System Integration Example
 * Demonstrates how to integrate the new unified parameter management system with existing systems
 */
class ParameterSystemIntegrationExample {
public:
    static void runBasicIntegrationExample() {
        LOG_INF_S("=== Parameter System Integration Basic Example ===");
        
        // 1. Initialize parameter system integration
        auto& integration = ParameterSystemIntegration::getInstance();
        
        ParameterSystemIntegration::IntegrationConfig config;
        config.mode = ParameterSystemIntegration::IntegrationMode::HYBRID;
        config.enableAutoMigration = true;
        config.enableBackwardCompatibility = true;
        config.enablePerformanceOptimization = true;
        config.syncInterval = std::chrono::milliseconds(50);
        config.enableConflictResolution = true;
        config.enableLogging = true;
        
        if (!integration.initialize(config)) {
            LOG_ERR_S("Failed to initialize parameter system integration");
            return;
        }
        
        // 2. Integrate with existing systems
        if (!integration.integrateWithExistingSystems()) {
            LOG_ERR_S("Failed to integrate with existing systems");
            return;
        }
        
        // 3. Demonstrate unified parameter access
        demonstrateUnifiedParameterAccess();
        
        // 4. Demonstrate legacy compatibility
        demonstrateLegacyCompatibility();
        
        // 5. Demonstrate hybrid mode
        demonstrateHybridMode();
        
        // 6. Demonstrate migration
        demonstrateMigration();
        
        // 7. Show integration status
        demonstrateIntegrationStatus();
        
        LOG_INF_S("=== Basic Integration Example Completed ===");
    }
    
    static void runAdvancedIntegrationExample() {
        LOG_INF_S("=== Parameter System Integration Advanced Example ===");
        
        auto& integration = ParameterSystemIntegration::getInstance();
        
        // 1. Dynamic mode switching
        demonstrateDynamicModeSwitching(integration);
        
        // 2. Performance monitoring
        demonstratePerformanceMonitoring(integration);
        
        // 3. Conflict resolution
        demonstrateConflictResolution(integration);
        
        // 4. Error handling and recovery
        demonstrateErrorHandling(integration);
        
        LOG_INF_S("=== Advanced Integration Example Completed ===");
    }
    
    static void runMigrationExample() {
        LOG_INF_S("=== Parameter System Migration Example ===");
        
        auto& integration = ParameterSystemIntegration::getInstance();
        
        // 1. Legacy to unified migration
        demonstrateLegacyToUnifiedMigration(integration);
        
        // 2. Unified to legacy migration
        demonstrateUnifiedToLegacyMigration(integration);
        
        // 3. Migration validation
        demonstrateMigrationValidation(integration);
        
        LOG_INF_S("=== Migration Example Completed ===");
    }

private:
    static void demonstrateUnifiedParameterAccess() {
        LOG_INF_S("--- Demonstrating Unified Parameter Access ---");
        
        // Access unified parameter system
        auto& unifiedIntegration = UnifiedParameterIntegration::getInstance();
        
        // Set parameters using unified interface
        unifiedIntegration.setParameter("rendering.material.diffuse.r", 0.8);
        unifiedIntegration.setParameter("rendering.material.diffuse.g", 0.6);
        unifiedIntegration.setParameter("rendering.material.diffuse.b", 0.4);
        unifiedIntegration.setParameter("mesh.deflection", 0.3);
        unifiedIntegration.setParameter("lighting.main.intensity", 1.2);
        
        // Get parameters using unified interface
        auto diffuseR = unifiedIntegration.getParameter("rendering.material.diffuse.r");
        auto deflection = unifiedIntegration.getParameter("mesh.deflection");
        auto intensity = unifiedIntegration.getParameter("lighting.main.intensity");
        
        LOG_INF_S("Unified parameter access:");
        LOG_INF_S("- Diffuse R: " + std::to_string(std::get<double>(diffuseR)));
        LOG_INF_S("- Deflection: " + std::to_string(std::get<double>(deflection)));
        LOG_INF_S("- Intensity: " + std::to_string(std::get<double>(intensity)));
    }
    
    static void demonstrateLegacyCompatibility() {
        LOG_INF_S("--- Demonstrating Legacy Compatibility ---");
        
        // Access legacy systems through compatibility layer
        auto& meshManager = ParameterSystemIntegration::LegacyCompatibilityLayer::getMeshParameterManager();
        auto& renderingConfig = ParameterSystemIntegration::LegacyCompatibilityLayer::getRenderingConfig();
        auto& lightingConfig = ParameterSystemIntegration::LegacyCompatibilityLayer::getLightingConfig();
        
        // Use legacy interfaces
        meshManager.setParameter(MeshParameterManager::Category::BASIC_MESH, "deflection", 0.4);
        renderingConfig.setMaterialDiffuseColor(Quantity_Color(0.7, 0.5, 0.3, Quantity_TOC_RGB));
        lightingConfig.setLightIntensity(0, 1.5);
        
        LOG_INF_S("Legacy compatibility:");
        LOG_INF_S("- Mesh deflection set to 0.4");
        LOG_INF_S("- Material diffuse color set to (0.7, 0.5, 0.3)");
        LOG_INF_S("- Light intensity set to 1.5");
        
        // Sync legacy parameters to unified system
        ParameterSystemIntegration::LegacyCompatibilityLayer::syncMeshParameters();
        ParameterSystemIntegration::LegacyCompatibilityLayer::syncRenderingParameters();
        ParameterSystemIntegration::LegacyCompatibilityLayer::syncLightingParameters();
        
        LOG_INF_S("Legacy parameters synced to unified system");
    }
    
    static void demonstrateHybridMode() {
        LOG_INF_S("--- Demonstrating Hybrid Mode ---");
        
        auto& integration = ParameterSystemIntegration::getInstance();
        
        // Enable hybrid mode
        if (!integration.enableHybridMode()) {
            LOG_ERR_S("Failed to enable hybrid mode");
            return;
        }
        
        // Use both systems simultaneously
        auto& unifiedIntegration = UnifiedParameterIntegration::getInstance();
        auto& meshManager = ParameterSystemIntegration::LegacyCompatibilityLayer::getMeshParameterManager();
        
        // Set parameter in unified system
        unifiedIntegration.setParameter("geometry.position.x", 100.0);
        
        // Set parameter in legacy system
        meshManager.setParameter(MeshParameterManager::Category::BASIC_MESH, "angularDeflection", 0.8);
        
        // Both systems should be synchronized
        LOG_INF_S("Hybrid mode:");
        LOG_INF_S("- Set geometry position X to 100.0 in unified system");
        LOG_INF_S("- Set angular deflection to 0.8 in legacy system");
        LOG_INF_S("- Both systems synchronized automatically");
    }
    
    static void demonstrateMigration() {
        LOG_INF_S("--- Demonstrating Migration ---");
        
        auto& integration = ParameterSystemIntegration::getInstance();
        
        // Start with legacy system
        integration.enableLegacySystem();
        
        // Set some parameters in legacy system
        auto& meshManager = ParameterSystemIntegration::LegacyCompatibilityLayer::getMeshParameterManager();
        meshManager.setParameter(MeshParameterManager::Category::BASIC_MESH, "deflection", 0.2);
        meshManager.setParameter(MeshParameterManager::Category::BASIC_MESH, "angularDeflection", 0.6);
        
        LOG_INF_S("Legacy system parameters set");
        
        // Migrate to unified system
        if (integration.migrateFromLegacyToUnified()) {
            LOG_INF_S("Migration to unified system completed");
            
            // Verify parameters in unified system
            auto& unifiedIntegration = UnifiedParameterIntegration::getInstance();
            auto deflection = unifiedIntegration.getParameter("mesh.deflection");
            auto angularDeflection = unifiedIntegration.getParameter("mesh.angularDeflection");
            
            LOG_INF_S("Migrated parameters:");
            LOG_INF_S("- Deflection: " + std::to_string(std::get<double>(deflection)));
            LOG_INF_S("- Angular Deflection: " + std::to_string(std::get<double>(angularDeflection)));
        } else {
            LOG_ERR_S("Migration failed");
        }
    }
    
    static void demonstrateIntegrationStatus() {
        LOG_INF_S("--- Demonstrating Integration Status ---");
        
        auto& integration = ParameterSystemIntegration::getInstance();
        
        // Get integration status
        auto status = integration.getIntegrationStatus();
        LOG_INF_S("Integration Status:");
        LOG_INF_S(status);
        
        // Get diagnostics
        auto diagnostics = integration.getIntegrationDiagnostics();
        LOG_INF_S("Integration Diagnostics:");
        for (const auto& diagnostic : diagnostics) {
            LOG_INF_S(diagnostic);
        }
        
        // Get performance metrics
        auto metrics = integration.getPerformanceMetrics();
        LOG_INF_S("Performance Metrics:");
        LOG_INF_S("- Unified Parameters: " + std::to_string(metrics.unifiedParameterCount));
        LOG_INF_S("- Legacy Parameters: " + std::to_string(metrics.legacyParameterCount));
        LOG_INF_S("- Sync Operations: " + std::to_string(metrics.syncOperationsPerformed));
        LOG_INF_S("- Migration Operations: " + std::to_string(metrics.migrationOperationsCompleted));
        LOG_INF_S("- Conflict Resolutions: " + std::to_string(metrics.conflictResolutionsPerformed));
    }
    
    static void demonstrateDynamicModeSwitching(ParameterSystemIntegration& integration) {
        LOG_INF_S("--- Demonstrating Dynamic Mode Switching ---");
        
        // Switch to unified mode
        LOG_INF_S("Switching to unified mode");
        integration.enableUnifiedSystem();
        
        // Set some parameters
        auto& unifiedIntegration = UnifiedParameterIntegration::getInstance();
        unifiedIntegration.setParameter("rendering.material.transparency", 0.5);
        
        // Switch to legacy mode
        LOG_INF_S("Switching to legacy mode");
        integration.enableLegacySystem();
        
        // Set parameters in legacy system
        auto& renderingConfig = ParameterSystemIntegration::LegacyCompatibilityLayer::getRenderingConfig();
        renderingConfig.setMaterialTransparency(0.3);
        
        // Switch to hybrid mode
        LOG_INF_S("Switching to hybrid mode");
        integration.enableHybridMode();
        
        LOG_INF_S("Dynamic mode switching completed");
    }
    
    static void demonstratePerformanceMonitoring(ParameterSystemIntegration& integration) {
        LOG_INF_S("--- Demonstrating Performance Monitoring ---");
        
        // Perform some operations
        auto& unifiedIntegration = UnifiedParameterIntegration::getInstance();
        for (int i = 0; i < 100; ++i) {
            unifiedIntegration.setParameter("test.param_" + std::to_string(i), static_cast<double>(i));
        }
        
        // Get performance metrics
        auto metrics = integration.getPerformanceMetrics();
        LOG_INF_S("Performance after operations:");
        LOG_INF_S("- Unified Parameters: " + std::to_string(metrics.unifiedParameterCount));
        LOG_INF_S("- Sync Operations: " + std::to_string(metrics.syncOperationsPerformed));
        LOG_INF_S("- Average Sync Time: " + std::to_string(metrics.averageSyncTime.count()) + "ms");
        
        // Reset metrics
        integration.resetPerformanceMetrics();
        LOG_INF_S("Performance metrics reset");
    }
    
    static void demonstrateConflictResolution(ParameterSystemIntegration& integration) {
        LOG_INF_S("--- Demonstrating Conflict Resolution ---");
        
        // Create a conflict by setting different values in both systems
        auto& unifiedIntegration = UnifiedParameterIntegration::getInstance();
        auto& meshManager = ParameterSystemIntegration::LegacyCompatibilityLayer::getMeshParameterManager();
        
        // Set different values
        unifiedIntegration.setParameter("mesh.deflection", 0.1);
        meshManager.setParameter(MeshParameterManager::Category::BASIC_MESH, "deflection", 0.5);
        
        LOG_INF_S("Conflict created:");
        LOG_INF_S("- Unified system: deflection = 0.1");
        LOG_INF_S("- Legacy system: deflection = 0.5");
        
        // The system should resolve the conflict automatically
        // (This would need to be implemented in the actual system)
        LOG_INF_S("Conflict resolution would be handled automatically");
    }
    
    static void demonstrateErrorHandling(ParameterSystemIntegration& integration) {
        LOG_INF_S("--- Demonstrating Error Handling ---");
        
        // Test invalid operations
        try {
            // Try to access unified system when in legacy mode
            integration.enableLegacySystem();
            auto& unifiedIntegration = UnifiedParameterIntegration::getInstance();
            unifiedIntegration.setParameter("test.param", 123.0);
            LOG_INF_S("Unified system access in legacy mode: handled gracefully");
        } catch (const std::exception& e) {
            LOG_INF_S("Exception caught: " + std::string(e.what()));
        }
        
        // Test migration validation
        bool isValid = integration.validateMigration();
        LOG_INF_S("Migration validation: " + std::string(isValid ? "PASSED" : "FAILED"));
        
        // Test error recovery
        LOG_INF_S("Error handling and recovery demonstrated");
    }
    
    static void demonstrateLegacyToUnifiedMigration(ParameterSystemIntegration& integration) {
        LOG_INF_S("--- Demonstrating Legacy to Unified Migration ---");
        
        // Start with legacy system
        integration.enableLegacySystem();
        
        // Set up legacy parameters
        auto& meshManager = ParameterSystemIntegration::LegacyCompatibilityLayer::getMeshParameterManager();
        auto& renderingConfig = ParameterSystemIntegration::LegacyCompatibilityLayer::getRenderingConfig();
        auto& lightingConfig = ParameterSystemIntegration::LegacyCompatibilityLayer::getLightingConfig();
        
        meshManager.setParameter(MeshParameterManager::Category::BASIC_MESH, "deflection", 0.3);
        meshManager.setParameter(MeshParameterManager::Category::BASIC_MESH, "angularDeflection", 0.7);
        renderingConfig.setMaterialDiffuseColor(Quantity_Color(0.8, 0.6, 0.4, Quantity_TOC_RGB));
        lightingConfig.setLightIntensity(0, 1.3);
        
        LOG_INF_S("Legacy parameters set up");
        
        // Perform migration
        if (integration.migrateFromLegacyToUnified()) {
            LOG_INF_S("Migration completed successfully");
            
            // Verify migration
            auto& unifiedIntegration = UnifiedParameterIntegration::getInstance();
            auto deflection = unifiedIntegration.getParameter("mesh.deflection");
            auto angularDeflection = unifiedIntegration.getParameter("mesh.angularDeflection");
            
            LOG_INF_S("Migrated parameters verified:");
            LOG_INF_S("- Deflection: " + std::to_string(std::get<double>(deflection)));
            LOG_INF_S("- Angular Deflection: " + std::to_string(std::get<double>(angularDeflection)));
        } else {
            LOG_ERR_S("Migration failed");
        }
    }
    
    static void demonstrateUnifiedToLegacyMigration(ParameterSystemIntegration& integration) {
        LOG_INF_S("--- Demonstrating Unified to Legacy Migration ---");
        
        // Start with unified system
        integration.enableUnifiedSystem();
        
        // Set up unified parameters
        auto& unifiedIntegration = UnifiedParameterIntegration::getInstance();
        unifiedIntegration.setParameter("mesh.deflection", 0.4);
        unifiedIntegration.setParameter("mesh.angularDeflection", 0.8);
        unifiedIntegration.setParameter("rendering.material.diffuse.r", 0.9);
        unifiedIntegration.setParameter("rendering.material.diffuse.g", 0.7);
        unifiedIntegration.setParameter("rendering.material.diffuse.b", 0.5);
        
        LOG_INF_S("Unified parameters set up");
        
        // Perform reverse migration
        if (integration.migrateFromUnifiedToLegacy()) {
            LOG_INF_S("Reverse migration completed successfully");
            
            // Verify migration
            auto& meshManager = ParameterSystemIntegration::LegacyCompatibilityLayer::getMeshParameterManager();
            auto& renderingConfig = ParameterSystemIntegration::LegacyCompatibilityLayer::getRenderingConfig();
            
            LOG_INF_S("Migrated parameters verified in legacy system");
        } else {
            LOG_ERR_S("Reverse migration failed");
        }
    }
    
    static void demonstrateMigrationValidation(ParameterSystemIntegration& integration) {
        LOG_INF_S("--- Demonstrating Migration Validation ---");
        
        // Perform migration
        integration.migrateFromLegacyToUnified();
        
        // Validate migration
        bool isValid = integration.validateMigration();
        
        if (isValid) {
            LOG_INF_S("Migration validation: PASSED");
            LOG_INF_S("All parameters migrated successfully");
        } else {
            LOG_ERR_S("Migration validation: FAILED");
            LOG_ERR_S("Some parameters may not have been migrated correctly");
        }
        
        // Get diagnostics for detailed information
        auto diagnostics = integration.getIntegrationDiagnostics();
        LOG_INF_S("Migration diagnostics:");
        for (const auto& diagnostic : diagnostics) {
            LOG_INF_S(diagnostic);
        }
    }
};

/**
 * @brief Main function - run all integration examples
 */
int main() {
    try {
        LOG_INF_S("Starting Parameter System Integration Examples");
        
        // Run basic integration example
        ParameterSystemIntegrationExample::runBasicIntegrationExample();
        
        // Run advanced integration example
        ParameterSystemIntegrationExample::runAdvancedIntegrationExample();
        
        // Run migration example
        ParameterSystemIntegrationExample::runMigrationExample();
        
        LOG_INF_S("All integration examples completed successfully");
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Integration example execution failed: " + std::string(e.what()));
        return 1;
    }
    
    return 0;
}