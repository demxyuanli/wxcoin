#include "MeshParameterManagerSimple.h"
#include "OCCViewer.h"
#include "rendering/GeometryProcessor.h"
#include "logger/Logger.h"
#include <algorithm>

MeshParameterManagerSimple& MeshParameterManagerSimple::getInstance() {
    static MeshParameterManagerSimple instance;
    return instance;
}

MeshParameterManagerSimple::MeshParameterManagerSimple() 
    : m_deflection(0.1)
    , m_angularDeflection(0.5)
    , m_lodEnabled(true)
    , m_lodRoughDeflection(0.2)
    , m_lodFineDeflection(0.05)
    , m_hasInitialized(false)
{
    LOG_INF_S("=== MESH PARAMETER MANAGER SIMPLE CREATED ===");
    LOG_INF_S("Default deflection: " + std::to_string(m_deflection));
    LOG_INF_S("Default angular deflection: " + std::to_string(m_angularDeflection));
    LOG_INF_S("Default LOD enabled: " + std::string(m_lodEnabled ? "true" : "false"));
}

void MeshParameterManagerSimple::initializeFromViewer(std::shared_ptr<OCCViewer> viewer) {
    if (!viewer) {
        LOG_WRN_S("Cannot initialize from null viewer");
        return;
    }

    LOG_INF_S("=== INITIALIZING PARAMETER MANAGER FROM VIEWER ===");
    
    // Get current parameters from viewer's MeshParameters
    MeshParameters currentParams;
    
    // We'll read from viewer's internal state and update our cache
    m_deflection = currentParams.deflection;
    m_angularDeflection = currentParams.angularDeflection;
    m_lodEnabled = true; // Default assumption
    m_lodRoughDeflection = currentParams.deflection * 2.0; // Rough approximation
    m_lodFineDeflection = currentParams.deflection * 0.5;  // Fine approximation
    
    m_hasInitialized = true;
    LOG_INF_S("Parameter manager initialized from viewer:");
    LOG_INF_S("  Deflection: " + std::to_string(m_deflection));
    LOG_INF_S("  Angular Deflection: " + std::to_string(m_angularDeflection));
    LOG_INF_S("  LOD Enabled: " + std::string(m_lodEnabled ? "true" : "false"));
    LOG_INF_S("  LOD Rough: " + std::to_string(m_lodRoughDeflection));
    LOG_INF_S("  LOD Fine: " + std::to_string(m_lodFineDeflection));
    LOG_INF_S("=== VIEWER INITIALIZATION COMPLETE ===");
}

void MeshParameterManagerSimple::syncToViewer(std::shared_ptr<OCCViewer> viewer) {
    if (!viewer || !m_hasInitialized) {
        return;
    }

    // Update viewer with current parameter values
    MeshParameters params;
    params.deflection = m_deflection;
    params.angularDeflection = m_angularDeflection;
    params.relative = false;
    params.inParallel = true;

    // Apply parameters to viewer - this will trigger actual mesh regeneration
    viewer->setMeshDeflection(m_deflection, true);  // true = remesh immediately
    viewer->setAngularDeflection(m_angularDeflection, true);  // true = remesh immediately
    
    // Apply LOD settings
    viewer->setLODEnabled(m_lodEnabled);
    viewer->setLODRoughDeflection(m_lodRoughDeflection);
    viewer->setLODFineDeflection(m_lodFineDeflection);
    
    // Force complete mesh regeneration for all geometries
    LOG_INF_S("Forcing complete mesh regeneration for Coin3D representation");
    viewer->remeshAllGeometries();
    
    // Force all geometries to regenerate their Coin3D representation
    auto geometries = viewer->getAllGeometry();
    for (auto& geometry : geometries) {
        if (geometry) {
            // Force regeneration by marking as needed and updating
            geometry->setMeshRegenerationNeeded(true);
            geometry->updateCoinRepresentationIfNeeded(params);
            LOG_INF_S("Forced Coin3D regeneration for geometry: " + geometry->getName());
        }
    }
    
    // Request view refresh to update Coin3D representation
    viewer->requestViewRefresh();

    LOG_INF_S("MeshParameterManagerSimple synced parameters to viewer and forced Coin3D mesh regeneration");
}

void MeshParameterManagerSimple::applyPreset(std::shared_ptr<OCCViewer> viewer, 
                                             double deflection, bool lodEnabled,
                                             double roughDeflection, double fineDeflection,
                                             bool parallelProcessing) {
    LOG_INF_S("=== MESH PARAMETER MANAGER APPLYING PRESET ===");
    LOG_INF_S("Input parameters:");
    LOG_INF_S("  Deflection: " + std::to_string(deflection));
    LOG_INF_S("  LOD Enabled: " + std::string(lodEnabled ? "true" : "false"));
    LOG_INF_S("  Rough Deflection: " + std::to_string(roughDeflection));
    LOG_INF_S("  Fine Deflection: " + std::to_string(fineDeflection));
    LOG_INF_S("  Parallel Processing: " + std::string(parallelProcessing ? "true" : "false"));
    
    auto& manager = getInstance();
    manager.setDeflection(deflection);
    manager.setLODEnabled(lodEnabled);
    manager.setLODRoughDeflection(roughDeflection);
    manager.setLODFineDeflection(fineDeflection);

    LOG_INF_S("Parameter manager updated with new values");
    LOG_INF_S("Current manager state:");
    LOG_INF_S("  Deflection: " + std::to_string(manager.getDeflection()));
    LOG_INF_S("  LOD Enabled: " + std::string(manager.isLODEnabled() ? "true" : "false"));
    LOG_INF_S("  LOD Rough: " + std::to_string(manager.getLODRoughDeflection()));
    LOG_INF_S("  LOD Fine: " + std::to_string(manager.getLODFineDeflection()));

    // Force update all parameters to ensure consistency
    manager.forceUpdateAll(viewer);
    
    LOG_INF_S("=== PRESET APPLICATION COMPLETE ===");
}

void MeshParameterManagerSimple::applySurfacePreset(std::shared_ptr<OCCViewer> viewer,
                                                    double deflection, double angularDeflection,
                                                    bool subdivisionEnabled, int subdivisionLevel,
                                                    bool smoothingEnabled, int smoothingIterations, double smoothingStrength,
                                                    bool lodEnabled, double lodFineDeflection, double lodRoughDeflection,
                                                    int tessellationQuality, double featurePreservation, double smoothingCreaseAngle) {
    LOG_INF_S("=== APPLYING SURFACE PRESET VIA PARAMETER MANAGER ===");
    LOG_INF_S("Input parameters:");
    LOG_INF_S("  Deflection: " + std::to_string(deflection));
    LOG_INF_S("  Angular Deflection: " + std::to_string(angularDeflection));
    LOG_INF_S("  Subdivision Enabled: " + std::string(subdivisionEnabled ? "true" : "false"));
    LOG_INF_S("  Smoothing Enabled: " + std::string(smoothingEnabled ? "true" : "false"));
    LOG_INF_S("  LOD Enabled: " + std::string(lodEnabled ? "true" : "false"));
    
    auto& manager = getInstance();
    manager.setDeflection(deflection);
    manager.setAngularDeflection(angularDeflection);
    manager.setLODEnabled(lodEnabled);
    manager.setLODFineDeflection(lodFineDeflection);
    manager.setLODRoughDeflection(lodRoughDeflection);

    // Apply parameters to viewer - this will trigger actual mesh regeneration
    if (viewer) {
        viewer->setMeshDeflection(deflection, true);  // true = remesh immediately
        viewer->setAngularDeflection(angularDeflection, true);  // true = remesh immediately
        
        // Apply LOD settings
        viewer->setLODEnabled(lodEnabled);
        viewer->setLODRoughDeflection(lodRoughDeflection);
        viewer->setLODFineDeflection(lodFineDeflection);
        
        // Apply advanced parameters if viewer supports them
        viewer->setSubdivisionEnabled(subdivisionEnabled);
        viewer->setSubdivisionLevel(subdivisionLevel);
        viewer->setSmoothingEnabled(smoothingEnabled);
        viewer->setSmoothingIterations(smoothingIterations);
        viewer->setSmoothingStrength(smoothingStrength);
        viewer->setTessellationQuality(tessellationQuality);
        viewer->setFeaturePreservation(featurePreservation);
        
        // Request view refresh to update Coin3D representation
        viewer->requestViewRefresh();
        
        LOG_INF_S("Surface preset applied successfully via MeshParameterManagerSimple");
    } else {
        LOG_ERR_S("Cannot apply surface preset: OCCViewer is null");
    }
    
    // Force update all parameters to ensure consistency
    manager.forceUpdateAll(viewer);
}

void MeshParameterManagerSimple::resetToDefaults() {
    LOG_INF_S("=== RESETTING PARAMETER MANAGER TO DEFAULTS ===");
    
    m_deflection = 0.1;
    m_angularDeflection = 0.5;
    m_lodEnabled = true;
    m_lodRoughDeflection = 0.2;
    m_lodFineDeflection = 0.05;
    
    LOG_INF_S("Parameter manager reset to defaults:");
    LOG_INF_S("  Deflection: " + std::to_string(m_deflection));
    LOG_INF_S("  Angular Deflection: " + std::to_string(m_angularDeflection));
    LOG_INF_S("  LOD Enabled: " + std::string(m_lodEnabled ? "true" : "false"));
    LOG_INF_S("  LOD Rough: " + std::to_string(m_lodRoughDeflection));
    LOG_INF_S("  LOD Fine: " + std::to_string(m_lodFineDeflection));
    
    // Note: RenderingToolkitAPI configuration reset is handled by OCCViewer
    // when we call the individual setter methods below
}

void MeshParameterManagerSimple::forceUpdateAll(std::shared_ptr<OCCViewer> viewer) {
    LOG_INF_S("=== FORCE UPDATING ALL PARAMETERS ===");
    
    if (!viewer) {
        LOG_WRN_S("Cannot force update: viewer is null");
        return;
    }
    
    // Apply current parameter values to viewer
    viewer->setMeshDeflection(m_deflection, true);  // true = remesh immediately
    viewer->setAngularDeflection(m_angularDeflection, true);  // true = remesh immediately
    
    // Apply LOD settings
    viewer->setLODEnabled(m_lodEnabled);
    viewer->setLODRoughDeflection(m_lodRoughDeflection);
    viewer->setLODFineDeflection(m_lodFineDeflection);
    
    // Force complete mesh regeneration for all geometries
    LOG_INF_S("Forcing complete mesh regeneration for Coin3D representation");
    viewer->remeshAllGeometries();
    
    // Force all geometries to regenerate their Coin3D representation
    MeshParameters params;
    params.deflection = m_deflection;
    params.angularDeflection = m_angularDeflection;
    params.relative = false;
    params.inParallel = true;
    
    auto geometries = viewer->getAllGeometry();
    for (auto& geometry : geometries) {
        if (geometry) {
            // Force regeneration by marking as needed and updating
            geometry->setMeshRegenerationNeeded(true);
            geometry->updateCoinRepresentationIfNeeded(params);
            LOG_INF_S("Forced Coin3D regeneration for geometry: " + geometry->getName());
        }
    }
    
    // Request view refresh to update Coin3D representation
    viewer->requestViewRefresh();
    
    LOG_INF_S("All parameters force updated to viewer with Coin3D mesh regeneration");
}

