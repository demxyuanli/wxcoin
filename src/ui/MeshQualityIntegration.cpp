#include "MeshQualityDialog.h"
#include "MeshParameterManager.h"
#include "logger/Logger.h"
#include <memory>

/**
 * Integration layer between the old MeshQualityDialog and the new MeshParameterManager
 * Provides backward compatibility while using the unified parameter system
 */
class MeshQualityIntegration {
public:
    static void integrateMeshParameterManager() {
        LOG_INF_S("=== INTEGRATING MESH PARAMETER MANAGER ===");
        
        auto& paramManager = MeshParameterManager::getInstance();
        
        // Load current configuration if available
        paramManager.loadFromConfig();
        
        // Register global callback for system-wide parameter changes
        paramManager.registerParameterChangeCallback([](const MeshParameterManager::ParameterChange& change) {
            LOG_DBG_S("Global parameter change: " + change.name + 
                     " [" + std::to_string(change.oldValue) + " -> " + std::to_string(change.newValue) + "]");
        });
        
        LOG_INF_S("Mesh parameter manager integration completed");
    }
    
    /**
     * Legacy interface for applying presets
     * Maintains compatibility with existing code
     */
    static void applyLegacyPreset(std::shared_ptr<OCCViewer> viewer, 
                                 double deflection, bool lodEnabled,
                                 double roughDeflection, double fineDeflection,
                                 bool parallelProcessing) {
        LOG_INF_S("Applying legacy preset parameters");
        
        auto& paramManager = MeshParameterManager::getInstance();
        
        // Update parameter manager with legacy values
        paramManager.setParameter(MeshParameterManager::Category::BASIC_MESH, 
                                MeshParamNames::BasicMesh::DEFLECTION, deflection);
        paramManager.setParameter(MeshParameterManager::Category::LOD, 
                                MeshParamNames::LOD::ENABLED, lodEnabled ? 1.0 : 0.0);
        paramManager.setParameter(MeshParameterManager::Category::LOD, 
                                MeshParamNames::LOD::ROUGH_DEFLECTION, roughDeflection);
        paramManager.setParameter(MeshParameterManager::Category::LOD, 
                                MeshParamNames::LOD::FINE_DEFLECTION, fineDeflection);
        
        // Apply parameter-dependent optimizations
        paramManager.validateAndAdjustParameters();
        
        // Apply to all geometries
        if (viewer) {
            paramManager.regenerateAllGeometries(viewer);
        }
        
        LOG_INF_S("Legacy preset applied successfully");
    }
    
    /**
     * Legacy interface for applying surface presets
     */
    static void applyLegacySurfacePreset(std::shared_ptr<OCCViewer> viewer,
                                       double deflection, double angularDeflection,
                                       bool subdivisionEnabled, int subdivisionLevel,
                                       bool smoothingEnabled, int smoothingIterations, double smoothingStrength,
                                       bool lodEnabled, double lodFineDeflection, double lodRoughDeflection,
                                       int tessellationQuality, double featurePreservation, double smoothingCreaseAngle) {
        LOG_INF_S("Applying legacy surface preset parameters");
        
        auto& paramManager = MeshParameterManager::getInstance();
        
        // Set all parameters atomically
        std::map<std::pair<MeshParameterManager::Category, std::string>, double> params;
        params[{{MeshParameterManager::Category::BASIC_MESH, MeshParamNames::BasicMesh::DEFLECTION}}] = deflection;
        params[{{MeshParameterManager::Category::BASIC_MESH, MeshParamNames::BasicMesh::ANGULAR_DEFLECTION}}] = angularDeflection;
        params[{{MeshParameterManager::Category::SUBDIVISION, MeshParamNames::Subdivision::LEVEL}}] = static_cast<double>(subdivisionLevel);
        params[{{MeshParameterManager::Category::SUBDIVISION, MeshParamNames::Subdivision::ENABLED}}] = subdivisionEnabled ? 1.0 : 0.0;
        params[{{MeshParameterManager::Category::SMOOTHING, MeshParamNames::Smoothing::ITERATIONS}}] = static_cast<double>(smoothingIterations);
        params[{{MeshParameterManager::Category::SMOOTHING, MeshParamNames::Smoothing::STRENGTH}}] = smoothingStrength;
        params[{{MeshParameterManager::Category::SMOOTHING, MeshParamNames::Smoothing::ENABLED}}] = smoothingEnabled ? 1.0 : 0.0;
        params[{{MeshParameterManager::Category::LOD, MeshParamNames::LOD::ENABLED}}] = lodEnabled ? 1.0 : 0.0;
        params[{{MeshParameterManager::Category::LOD, MeshParamNames::LOD::FINE_DEFLECTION}}] = lodFineDeflection;
        params[{{MeshParameterManager::Category::LOD, MeshParamNames::LOD::ROUGH_DEFLECTION}}] = lodRoughDeflection;
        params[{{MeshParameterManager::Category::TESSELATION, MeshParamNames::Tessellation::QUALITY}}] = static_cast<double>(tessellationQuality);
        params[{{MeshParameterManager::Category::TESSELATION, MeshParamNames::Tessellation::FEATURE_PRESERVATION}}] = featurePreservation;
        
        paramManager.setParameters(params);
        
        // Apply automatic parameter optimization
        paramManager.validateAndAdjustParameters();
        
        // Apply to all geometries
        if (viewer) {
            paramManager.regenerateAllGeometries(viewer);
        }
        
        LOG_INF_S("Legacy surface preset applied successfully");
    }
    
    /**
     * Sync OCCViewer parameters with MeshParameterManager
     */
    static void syncViewerParameters(std::shared_ptr<OCCViewer> viewer) {
        if (!viewer) return;
        
        LOG_INF_S("Syncing OCCViewer parameters with MeshParameterManager");
        
        auto& paramManager = MeshParameterManager::getInstance();
        MeshParameters params = paramManager.getCurrentMeshParameters();
        
        // Update viewer's mesh parameters to match manager
        viewer->setMeshDeflection(params.deflection, false);
        viewer->setAngularDeflection(params.angularDeflection, false);
        
        // Additional viewer-specific syncing would go here
        
        LOG_INF_S("Viewer parameters synced with MeshParameterManager");
    }
    
    /**
     * Get parameter report for debugging
     */
    static std::string getParameterDebugReport() {
        auto& paramManager = MeshParameterManager::getInstance();
        
        std::string report = "=== MESH PARAMETER DEBUG REPORT ===\n";
        report += paramManager.getParameterReport();
        
        // Add validation status
        report += "\nValidation Status: ";
        report += paramManager.validateCurrentParameters() ? "PASS" : "FAIL";
        
        return report;
    }
    
    /**
     * Reset all parameters to defaults
     */
    static void resetToDefaults(std::shared_ptr<OCCViewer> viewer) {
        LOG_INF_S("Resetting all parameters to defaults");
        
        auto& paramManager = MeshParameterManager::getInstance();
        
        // Apply default preset
        paramManager.applyPreset("Balanced");
        
        // Apply to geometries
        if (viewer) {
            paramManager.regenerateAllGeometries(viewer);
        }
        
        LOG_INF_S("Parameters reset to defaults successfully");
    }
};

/**
 * Hook for integrating with existing MeshQualityDialog
 * Replace the old applyPreset and applySurfacePreset methods with these implementations
 */
namespace MeshQualityDialogHooks {
    
    void hookApplyPreset(std::shared_ptr<OCCViewer> viewer, 
                         double deflection, bool lodEnabled,
                         double roughDeflection, double fineDeflection,
                         bool parallelProcessing) {
        MeshQualityIntegration::applyLegacyPreset(viewer, deflection, lodEnabled, 
                                                 roughDeflection, fineDeflection, parallelProcessing);
    }
    
    void hookApplySurfacePreset(std::shared_ptr<OCCViewer> viewer,
                               double deflection, double angularDeflection,
                               bool subdivisionEnabled, int subdivisionLevel,
                               bool smoothingEnabled, int smoothingIterations, double smoothingStrength,
                               bool lodEnabled, double lodFineDeflection, double lodRoughDeflection,
                               int tessellationQuality, double featurePreservation, double smoothingCreaseAngle) {
        MeshQualityIntegration::applyLegacySurfacePreset(viewer, deflection, angularDeflection,
                                                        subdivisionEnabled, subdivisionLevel,
                                                        smoothingEnabled, smoothingIterations, smoothingStrength,
                                                        lodEnabled, lodFineDeflection, lodRoughDeflection,
                                                        tessellationQuality, featurePreservation, smoothingCreaseAngle);
    }
}

/**
 * Startup initialization for mesh parameter management
 */
void initializeMeshParameterManagement(std::shared_ptr<OCCViewer> viewer) {
    LOG_INF_S("=== INITIALIZING MESH PARAMETER MANAGEMENT ===");
    
    // Initialize parameter manager
    MeshQualityIntegration::integrateMeshParameterManager();
    
    // Sync viewer parameters
    MeshQualityIntegration::syncViewerParameters(viewer);
    
    LOG_INF_S("Mesh parameter management initialization completed");
}
