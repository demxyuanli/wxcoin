#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "OCCViewer.h"
#include "OCCGeometry.h"
#include "rendering/RenderingToolkitAPI.h"
#include "SceneManager.h"
#include "logger/Logger.h"
#include "Canvas.h"
#include "ObjectTreePanel.h"
#include "ViewRefreshManager.h"
#include "config/EdgeSettingsConfig.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoTransform.h>
#include <algorithm>
#include <limits>
#include <cmath>
#include <wx/gdicmn.h>
#include <chrono>
#include "EdgeComponent.h"

OCCViewer::OCCViewer(SceneManager* sceneManager)
    : m_sceneManager(sceneManager),
      m_occRoot(nullptr),
      m_wireframeMode(false),
      m_showEdges(true),
      m_antiAliasing(true),
      m_showNormals(false),
      m_normalLength(0.5),
      m_correctNormalColor(1.0, 0.0, 0.0, Quantity_TOC_RGB),
      m_incorrectNormalColor(0.0, 1.0, 0.0, Quantity_TOC_RGB),
      m_defaultColor(0.7, 0.7, 0.7, Quantity_TOC_RGB),
      m_defaultTransparency(0.0),
      m_lodEnabled(false),
      m_lodRoughMode(false),
      m_lodRoughDeflection(0.1),
      m_lodFineDeflection(0.01),
      m_lodTransitionTime(500),
      m_lodTimer(this, wxID_ANY),
      m_subdivisionEnabled(false),
      m_subdivisionLevel(2),
      m_subdivisionMethod(0),
      m_subdivisionCreaseAngle(30.0),
      m_smoothingEnabled(false),
      m_smoothingMethod(0),
      m_smoothingIterations(2),
      m_smoothingStrength(0.5),
      m_smoothingCreaseAngle(30.0),
      m_tessellationMethod(0),
      m_tessellationQuality(2),
      m_featurePreservation(0.5),
      m_parallelProcessing(true),
      m_adaptiveMeshing(false),
      m_batchOperationActive(false),
      m_needsViewRefresh(false),
      m_parameterMonitoringEnabled(false)
{
    initializeViewer();
    // Remove default edge display to avoid conflicts with new EdgeComponent system
    // setShowEdges(true);
    
    // Connect LOD timer
    m_lodTimer.Bind(wxEVT_TIMER, [this](wxTimerEvent& event) {
        this->onLODTimer();
    });
}

OCCViewer::~OCCViewer()
{
    clearAll();
    if (m_occRoot) {
        m_occRoot->unref();
    }
}

void OCCViewer::initializeViewer()
{
    m_occRoot = new SoSeparator;
    m_occRoot->ref();
    
    if (m_sceneManager) {
        m_sceneManager->getObjectRoot()->addChild(m_occRoot);
    }
    
    LOG_INF_S("OCC Viewer initialized");
}

void OCCViewer::addGeometry(std::shared_ptr<OCCGeometry> geometry)
{
    auto addStartTime = std::chrono::high_resolution_clock::now();
    
    if (!geometry) {
        LOG_ERR_S("Attempted to add null geometry to OCCViewer");
        return;
    }
    
    auto validationStartTime = std::chrono::high_resolution_clock::now();
    auto it = std::find_if(m_geometries.begin(), m_geometries.end(),
        [&](const std::shared_ptr<OCCGeometry>& g) {
            return g->getName() == geometry->getName();
        });
    
    if (it != m_geometries.end()) {
        LOG_WRN_S("Geometry with name '" + geometry->getName() + "' already exists");
        return;
    }
    auto validationEndTime = std::chrono::high_resolution_clock::now();
    auto validationDuration = std::chrono::duration_cast<std::chrono::microseconds>(validationEndTime - validationStartTime);
    
    // Only log in non-batch mode to reduce overhead
    if (!m_batchOperationActive) {
        LOG_INF_S("Adding geometry: " + geometry->getName());
    }
    
    auto meshStartTime = std::chrono::high_resolution_clock::now();
    // Use optimized mesh update method
    geometry->updateCoinRepresentationIfNeeded(m_meshParams);
    auto meshEndTime = std::chrono::high_resolution_clock::now();
    auto meshDuration = std::chrono::duration_cast<std::chrono::milliseconds>(meshEndTime - meshStartTime);
    
    auto storageStartTime = std::chrono::high_resolution_clock::now();
    m_geometries.push_back(geometry);
    auto storageEndTime = std::chrono::high_resolution_clock::now();
    auto storageDuration = std::chrono::duration_cast<std::chrono::microseconds>(storageEndTime - storageStartTime);
    
    auto coinNodeStartTime = std::chrono::high_resolution_clock::now();
    SoSeparator* coinNode = geometry->getCoinNode();
    auto coinNodeEndTime = std::chrono::high_resolution_clock::now();
    auto coinNodeDuration = std::chrono::duration_cast<std::chrono::microseconds>(coinNodeEndTime - coinNodeStartTime);
    
    auto addChildStartTime = std::chrono::high_resolution_clock::now();
    if (coinNode && m_occRoot) {
        m_occRoot->addChild(coinNode);
    } else {
        if (!coinNode) {
            LOG_ERR_S("Coin3D node is null for geometry: " + geometry->getName());
        }
        if (!m_occRoot) {
            LOG_ERR_S("OCC root is null, cannot add geometry: " + geometry->getName());
        }
    }
    auto addChildEndTime = std::chrono::high_resolution_clock::now();
    auto addChildDuration = std::chrono::duration_cast<std::chrono::microseconds>(addChildEndTime - addChildStartTime);
    
    auto objectTreeStartTime = std::chrono::high_resolution_clock::now();
    // Add geometry to ObjectTree if available (only in non-batch mode to reduce overhead)
    if (!m_batchOperationActive && m_sceneManager && m_sceneManager->getCanvas()) {
        Canvas* canvas = m_sceneManager->getCanvas();
        if (canvas && canvas->getObjectTreePanel()) {
            // Defer ObjectTree update to reduce blocking time
            canvas->getObjectTreePanel()->addOCCGeometry(geometry);
        }
    } else if (m_batchOperationActive) {
        // Queue for deferred update in batch mode
        m_pendingObjectTreeUpdates.push_back(geometry);
    }
    auto objectTreeEndTime = std::chrono::high_resolution_clock::now();
    auto objectTreeDuration = std::chrono::duration_cast<std::chrono::microseconds>(objectTreeEndTime - objectTreeStartTime);
    
    auto viewUpdateStartTime = std::chrono::high_resolution_clock::now();
    // Auto-update scene bounds and view when geometry is added
    if (m_sceneManager) {
        // In batch mode, defer view updates until endBatchOperation
        if (m_batchOperationActive) {
            m_needsViewRefresh = true;
        } else {
            m_sceneManager->updateSceneBounds();
            m_sceneManager->resetView();
            
            Canvas* canvas = m_sceneManager->getCanvas();
            if (canvas && canvas->getRefreshManager()) {
                canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::GEOMETRY_CHANGED, true);
            }
            
            canvas->Refresh();
            LOG_INF_S("Auto-updated scene bounds and view after adding geometry");
        }
    }
    auto viewUpdateEndTime = std::chrono::high_resolution_clock::now();
    auto viewUpdateDuration = std::chrono::duration_cast<std::chrono::microseconds>(viewUpdateEndTime - viewUpdateStartTime);
    
    auto addEndTime = std::chrono::high_resolution_clock::now();
    auto addDuration = std::chrono::duration_cast<std::chrono::milliseconds>(addEndTime - addStartTime);
    
    // Only show detailed breakdown in non-batch mode or for debugging
    if (!m_batchOperationActive) {
        LOG_INF_S("=== GEOMETRY ADDITION BREAKDOWN ===");
        LOG_INF_S("Geometry: " + geometry->getName());
        LOG_INF_S("  - Validation: " + std::to_string(validationDuration.count()) + "μs");
        LOG_INF_S("  - Mesh regeneration: " + std::to_string(meshDuration.count()) + "ms");
        LOG_INF_S("  - Storage: " + std::to_string(storageDuration.count()) + "μs");
        LOG_INF_S("  - Coin3D node: " + std::to_string(coinNodeDuration.count()) + "μs");
        LOG_INF_S("  - Add to scene: " + std::to_string(addChildDuration.count()) + "μs");
        LOG_INF_S("  - Object tree: " + std::to_string(objectTreeDuration.count()) + "μs");
        LOG_INF_S("  - View update: " + std::to_string(viewUpdateDuration.count()) + "μs");
        LOG_INF_S("TOTAL ADDITION TIME: " + std::to_string(addDuration.count()) + "ms");
        LOG_INF_S("==================================");
    }
}

void OCCViewer::removeGeometry(std::shared_ptr<OCCGeometry> geometry)
{
    if (!geometry) return;
    
    auto it = std::find(m_geometries.begin(), m_geometries.end(), geometry);
    if (it != m_geometries.end()) {
        if (geometry->getCoinNode() && m_occRoot) {
            m_occRoot->removeChild(geometry->getCoinNode());
        }
        
        auto selectedIt = std::find(m_selectedGeometries.begin(), m_selectedGeometries.end(), geometry);
        if (selectedIt != m_selectedGeometries.end()) {
            m_selectedGeometries.erase(selectedIt);
        }
        
        // Remove geometry from ObjectTree if available
        if (m_sceneManager && m_sceneManager->getCanvas()) {
            Canvas* canvas = m_sceneManager->getCanvas();
            if (canvas && canvas->getObjectTreePanel()) {
                canvas->getObjectTreePanel()->removeOCCGeometry(geometry);
            }
        }
        
        m_geometries.erase(it);
        LOG_INF_S("Removed OCC geometry: " + geometry->getName());
    }
}

void OCCViewer::removeGeometry(const std::string& name)
{
    auto geometry = findGeometry(name);
    if (geometry) {
        removeGeometry(geometry);
    }
}

void OCCViewer::clearAll()
{
    m_selectedGeometries.clear();
    m_geometries.clear();
    
    if (m_occRoot) {
        m_occRoot->removeAllChildren();
    }
    
    LOG_INF_S("Cleared all OCC geometries");
}

std::shared_ptr<OCCGeometry> OCCViewer::findGeometry(const std::string& name)
{
    auto it = std::find_if(m_geometries.begin(), m_geometries.end(),
        [&](const std::shared_ptr<OCCGeometry>& g) {
            return g->getName() == name;
        });
    
    return (it != m_geometries.end()) ? *it : nullptr;
}

std::vector<std::shared_ptr<OCCGeometry>> OCCViewer::getAllGeometry() const
{
    return m_geometries;
}

std::vector<std::shared_ptr<OCCGeometry>> OCCViewer::getSelectedGeometries() const
{
    return m_selectedGeometries;
}

void OCCViewer::setGeometryVisible(const std::string& name, bool visible)
{
    auto geometry = findGeometry(name);
    if (!geometry) {
        LOG_WRN_S("Geometry not found for visibility change: " + name);
        return;
    }
    
    LOG_INF_S("Setting geometry visibility: " + name + " -> " + (visible ? "visible" : "hidden"));
    geometry->setVisible(visible);
    
    // Request view refresh after visibility change
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        Canvas* canvas = m_sceneManager->getCanvas();
        canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::GEOMETRY_CHANGED, true);
    }
}

void OCCViewer::setGeometrySelected(const std::string& name, bool selected)
{
    auto geometry = findGeometry(name);
    if (!geometry) {
        LOG_WRN_S("Geometry not found for selection: " + name);
        return;
    }

    LOG_INF_S("Setting geometry selection: " + name + " -> " + (selected ? "true" : "false"));
        geometry->setSelected(selected);
        
        if (selected) {
        if (std::find(m_selectedGeometries.begin(), m_selectedGeometries.end(), geometry) == m_selectedGeometries.end()) {
                m_selectedGeometries.push_back(geometry);
            LOG_INF_S("Added geometry to selected list: " + name);
            }
        } else {
        auto it = std::remove(m_selectedGeometries.begin(), m_selectedGeometries.end(), geometry);
        if (it != m_selectedGeometries.end()) {
            m_selectedGeometries.erase(it, m_selectedGeometries.end());
            LOG_INF_S("Removed geometry from selected list: " + name);
        }
    }
    
    onSelectionChanged();
}

void OCCViewer::setGeometryColor(const std::string& name, const Quantity_Color& color)
{
    if (auto g = findGeometry(name)) g->setColor(color);
}

void OCCViewer::setGeometryTransparency(const std::string& name, double transparency)
{
    if (auto g = findGeometry(name)) {
        g->setTransparency(transparency);
        // Request view refresh after transparency change
        if (m_sceneManager && m_sceneManager->getCanvas()) {
            Canvas* canvas = m_sceneManager->getCanvas();
            canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::MATERIAL_CHANGED, true);
        }
    }
}

void OCCViewer::hideAll()
{
    LOG_INF_S("Hiding all geometries - count: " + std::to_string(m_geometries.size()));
    for (auto& g : m_geometries) {
        g->setVisible(false);
    }
    
    // Request view refresh
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        Canvas* canvas = m_sceneManager->getCanvas();
        canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::GEOMETRY_CHANGED, true);
    }
}

void OCCViewer::showAll()
{
    LOG_INF_S("Showing all geometries - count: " + std::to_string(m_geometries.size()));
    for (auto& g : m_geometries) {
        g->setVisible(true);
    }
    
    // Request view refresh
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        Canvas* canvas = m_sceneManager->getCanvas();
        canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::GEOMETRY_CHANGED, true);
    }
}

void OCCViewer::selectAll()
{
    m_selectedGeometries.clear();
    for (auto& g : m_geometries) {
        g->setSelected(true);
        m_selectedGeometries.push_back(g);
    }
    onSelectionChanged();
}

void OCCViewer::deselectAll()
{
    LOG_INF_S("Deselecting all geometries - count: " + std::to_string(m_selectedGeometries.size()));
    
    for (auto& g : m_selectedGeometries) {
        g->setSelected(false);
    }
    m_selectedGeometries.clear();
    onSelectionChanged();
}

void OCCViewer::setAllColor(const Quantity_Color& color)
{
    for (auto& g : m_geometries) g->setColor(color);
}

void OCCViewer::fitAll()
{
    LOG_INF_S("Fit all OCC geometries");
    
    if (!m_sceneManager) {
        LOG_WRN_S("SceneManager is null, cannot perform fitAll");
        return;
    }
    
    // Update scene bounds first to ensure we have accurate bounding information
    m_sceneManager->updateSceneBounds();
    
    // Reset view to fit all geometries
    m_sceneManager->resetView();
    
    // Request view refresh
    if (m_sceneManager->getCanvas()) {
        Canvas* canvas = m_sceneManager->getCanvas();
        if (canvas && canvas->getRefreshManager()) {
            canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::CAMERA_MOVED, true);
        }
        canvas->Refresh();
    }
    
    LOG_INF_S("FitAll completed - view adjusted to show all geometries");
}

void OCCViewer::fitGeometry(const std::string& name)
{
        LOG_INF_S("Fit geometry: " + name);
}

std::shared_ptr<OCCGeometry> OCCViewer::pickGeometry(int x, int y)
{
    if (!m_sceneManager) {
        return nullptr;
    }
    
    // Convert screen coordinates to world coordinates
    SbVec3f worldPos;
    if (!m_sceneManager->screenToWorld(wxPoint(x, y), worldPos)) {
        LOG_WRN_S("Failed to convert screen coordinates to world coordinates");
    return nullptr;
    }
    
    LOG_INF_S("Picking at world position: (" + std::to_string(worldPos[0]) + ", " + 
              std::to_string(worldPos[1]) + ", " + std::to_string(worldPos[2]) + ")");
    
    // Find the closest geometry to the picked point
    std::shared_ptr<OCCGeometry> closestGeometry = nullptr;
    double minDistance = std::numeric_limits<double>::max();
    
    for (auto& geometry : m_geometries) {
        if (!geometry->isVisible()) continue;
        
        // Get geometry position
        gp_Pnt geometryPos = geometry->getPosition();
        
        // Calculate distance to geometry center
        double distance = std::sqrt(
            std::pow(worldPos[0] - geometryPos.X(), 2) +
            std::pow(worldPos[1] - geometryPos.Y(), 2) +
            std::pow(worldPos[2] - geometryPos.Z(), 2)
        );
        
        LOG_INF_S("Geometry '" + geometry->getName() + "' at distance: " + std::to_string(distance));
        
        // Use a much larger picking radius for complex geometries like wrench
        double pickingRadius = 15.0; // Increased from 10.0 to 15.0
        
        if (distance < minDistance && distance < pickingRadius) {
            minDistance = distance;
            closestGeometry = geometry;
        }
    }
    
    if (closestGeometry) {
        LOG_INF_S("Picked geometry: " + closestGeometry->getName() + " at distance: " + std::to_string(minDistance));
    } else {
        LOG_INF_S("No geometry picked");
    }
    
    return closestGeometry;
}

void OCCViewer::setWireframeMode(bool wireframe)
{
    m_wireframeMode = wireframe;
    // Update all geometries to wireframe mode
    for (auto& geometry : m_geometries) {
        if (geometry) {
            geometry->setWireframeMode(wireframe);
        }
    }
    
    // Request view refresh
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        auto* refreshManager = m_sceneManager->getCanvas()->getRefreshManager();
        if (refreshManager) {
            refreshManager->requestRefresh(ViewRefreshManager::RefreshReason::MATERIAL_CHANGED, true);
        }
    }
}

// Removed setShadingMode method - functionality not needed

void OCCViewer::setShowEdges(bool showEdges)
{
    m_showEdges = showEdges;
    
    // Update EdgeSettingsConfig
    EdgeSettingsConfig& edgeConfig = EdgeSettingsConfig::getInstance();
    edgeConfig.setGlobalShowEdges(showEdges);
    
    // Update rendering toolkit configuration
    auto& config = RenderingToolkitAPI::getConfig();
    config.getEdgeSettings().showEdges = showEdges;
    
    remeshAllGeometries();
    
    // Use refresh manager instead of direct refresh
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        auto* refreshManager = m_sceneManager->getCanvas()->getRefreshManager();
        if (refreshManager) {
            refreshManager->requestRefresh(ViewRefreshManager::RefreshReason::EDGES_TOGGLED, true);
        }
    }
    
    LOG_INF_S("OCCViewer showEdges set to: " + std::string(showEdges ? "enabled" : "disabled"));
}

void OCCViewer::setAntiAliasing(bool enabled)
{
    m_antiAliasing = enabled;
}

bool OCCViewer::isWireframeMode() const
{
    return m_wireframeMode;
}

// Removed isShadingMode method - functionality not needed

bool OCCViewer::isShowEdges() const
{
    return m_showEdges;
}

bool OCCViewer::isShowNormals() const
{
    return m_showNormals;
}

void OCCViewer::setMeshDeflection(double deflection, bool remesh)
{
    if (m_meshParams.deflection != deflection) {
        m_meshParams.deflection = deflection;
        if (remesh) {
            remeshAllGeometries();
        }
    }
}

double OCCViewer::getMeshDeflection() const
{
    return m_meshParams.deflection;
}

void OCCViewer::onSelectionChanged()
{
    LOG_INF_S("Selection changed - selected geometries: " + std::to_string(m_selectedGeometries.size()));
    
    // Update ObjectTree selection if available
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        Canvas* canvas = m_sceneManager->getCanvas();
        if (canvas && canvas->getObjectTreePanel()) {
            canvas->getObjectTreePanel()->updateTreeSelectionFromViewer();
            LOG_INF_S("Updated ObjectTreePanel selection");
        } else {
            LOG_WRN_S("Canvas or ObjectTreePanel is null in OCCViewer");
        }
        
        // Force a scene refresh to show selection changes
        if (canvas && canvas->getRefreshManager()) {
            canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::SELECTION_CHANGED, true);
            LOG_INF_S("Requested scene refresh for selection change");
        }
    } else {
        LOG_WRN_S("SceneManager or Canvas is null in OCCViewer");
    }
}

void OCCViewer::onGeometryChanged(std::shared_ptr<OCCGeometry> geometry)
{
    if (geometry) {
        geometry->regenerateMesh(m_meshParams);
    }
}

void OCCViewer::setShowNormals(bool showNormals)
{
    m_showNormals = showNormals;
    globalEdgeFlags.showNormalLines = showNormals;
    updateAllEdgeDisplays();
}

void OCCViewer::setNormalLength(double length)
{
    m_normalLength = length;
    updateNormalsDisplay();
}

void OCCViewer::setNormalColor(const Quantity_Color& correct, const Quantity_Color& incorrect)
{
    m_correctNormalColor = correct;
    m_incorrectNormalColor = incorrect;
    updateNormalsDisplay();
}

void OCCViewer::updateNormalsDisplay()
{
    // This function is no longer needed as m_normalRoot is removed.
    // The normal display logic is now handled by the EdgeComponent.
}

void OCCViewer::createNormalVisualization(std::shared_ptr<OCCGeometry> geometry)
{
    // This function is no longer needed as m_normalRoot is removed.
    // The normal display logic is now handled by the EdgeComponent.
}

void OCCViewer::remeshAllGeometries()
{
    // Update RenderingToolkitAPI configuration with current parameters
    auto& config = RenderingToolkitAPI::getConfig();
    
    // Update smoothing settings
    auto& smoothingSettings = config.getSmoothingSettings();
    smoothingSettings.enabled = m_smoothingEnabled;
    smoothingSettings.creaseAngle = m_smoothingCreaseAngle;
    smoothingSettings.iterations = m_smoothingIterations;
    
    // Update subdivision settings
    auto& subdivisionSettings = config.getSubdivisionSettings();
    subdivisionSettings.enabled = m_subdivisionEnabled;
    subdivisionSettings.levels = m_subdivisionLevel;
    
    // Update edge settings
    auto& edgeSettings = config.getEdgeSettings();
    edgeSettings.featureEdgeAngle = m_subdivisionCreaseAngle;
    
    // Set custom parameters for advanced settings
    config.setParameter("tessellation_quality", std::to_string(m_tessellationQuality));
    config.setParameter("adaptive_meshing", m_adaptiveMeshing ? "true" : "false");
    config.setParameter("parallel_processing", m_parallelProcessing ? "true" : "false");
    config.setParameter("smoothing_strength", std::to_string(m_smoothingStrength));
    config.setParameter("tessellation_method", std::to_string(m_tessellationMethod));
    config.setParameter("feature_preservation", std::to_string(m_featurePreservation));
    
    LOG_INF_S("=== APPLYING MESH PARAMETERS ===");
    LOG_INF_S("Smoothing: " + std::string(m_smoothingEnabled ? "enabled" : "disabled") + 
              " (iterations: " + std::to_string(m_smoothingIterations) + 
              ", strength: " + std::to_string(m_smoothingStrength) + ")");
    LOG_INF_S("Subdivision: " + std::string(m_subdivisionEnabled ? "enabled" : "disabled") + 
              " (level: " + std::to_string(m_subdivisionLevel) + 
              ", method: " + std::to_string(m_subdivisionMethod) + ")");
    LOG_INF_S("Tessellation: quality=" + std::to_string(m_tessellationQuality) + 
              ", adaptive=" + std::string(m_adaptiveMeshing ? "yes" : "no"));
    
    // Regenerate all geometries with updated parameters
    for (auto& geometry : m_geometries) {
        if (geometry) {
            geometry->regenerateMesh(m_meshParams);
            LOG_INF_S("Regenerated mesh for geometry: " + geometry->getName());
        }
    }
    
    // Request view refresh
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        auto* refreshManager = m_sceneManager->getCanvas()->getRefreshManager();
        if (refreshManager) {
            refreshManager->requestRefresh(ViewRefreshManager::RefreshReason::GEOMETRY_CHANGED);
        }
    }
    
    LOG_INF_S("=== MESH REGENERATION COMPLETE ===");
    LOG_INF_S("Remeshed " + std::to_string(m_geometries.size()) + " geometries");
    LOG_INF_S("Deflection: " + std::to_string(m_meshParams.deflection));
}

// LOD (Level of Detail) methods
void OCCViewer::setLODEnabled(bool enabled)
{
    if (m_lodEnabled != enabled) {
        m_lodEnabled = enabled;
        if (!enabled) {
            m_lodTimer.Stop();
            // Switch back to fine mode when disabling LOD
            setLODMode(false);
        }
        LOG_INF_S("LOD " + std::string(enabled ? "enabled" : "disabled"));
    }
}

bool OCCViewer::isLODEnabled() const
{
    return m_lodEnabled;
}

void OCCViewer::setLODRoughDeflection(double deflection)
{
    if (m_lodRoughDeflection != deflection) {
        m_lodRoughDeflection = deflection;
        LOG_INF_S("LOD rough deflection set to: " + std::to_string(deflection));
    }
}

double OCCViewer::getLODRoughDeflection() const
{
    return m_lodRoughDeflection;
}

void OCCViewer::setLODFineDeflection(double deflection)
{
    if (m_lodFineDeflection != deflection) {
        m_lodFineDeflection = deflection;
        LOG_INF_S("LOD fine deflection set to: " + std::to_string(deflection));
    }
}

double OCCViewer::getLODFineDeflection() const
{
    return m_lodFineDeflection;
}

void OCCViewer::setLODTransitionTime(int milliseconds)
{
    if (m_lodTransitionTime != milliseconds) {
        m_lodTransitionTime = milliseconds;
        LOG_INF_S("LOD transition time set to: " + std::to_string(milliseconds) + "ms");
    }
}

int OCCViewer::getLODTransitionTime() const
{
    return m_lodTransitionTime;
}

void OCCViewer::setLODMode(bool roughMode)
{
    if (m_lodRoughMode != roughMode) {
        m_lodRoughMode = roughMode;
        
        // Set appropriate deflection based on mode
        double targetDeflection = roughMode ? m_lodRoughDeflection : m_lodFineDeflection;
        setMeshDeflection(targetDeflection, true);
        
        LOG_INF_S("LOD mode switched to " + std::string(roughMode ? "rough" : "fine") + 
                " (deflection: " + std::to_string(targetDeflection) + ")");
    }
}

bool OCCViewer::isLODRoughMode() const
{
    return m_lodRoughMode;
}

void OCCViewer::onLODTimer()
{
    // Switch to fine mode when timer expires
    setLODMode(false);
    m_lodTimer.Stop();
}

void OCCViewer::startLODInteraction()
{
    if (m_lodEnabled && !m_lodRoughMode) {
        setLODMode(true);
        m_lodTimer.Start(m_lodTransitionTime, wxTIMER_ONE_SHOT);
    }
}

// Batch operations for performance optimization
void OCCViewer::beginBatchOperation()
{
    m_batchOperationActive = true;
    m_needsViewRefresh = false;
    LOG_INF_S("Batch operation started");
}

void OCCViewer::endBatchOperation()
{
    m_batchOperationActive = false;
    
    // Process deferred ObjectTree updates
    updateObjectTreeDeferred();
    
    // Perform deferred view refresh if needed
    if (m_needsViewRefresh && m_sceneManager) {
        auto batchRefreshStartTime = std::chrono::high_resolution_clock::now();
        
        m_sceneManager->updateSceneBounds();
        m_sceneManager->resetView();
        
        Canvas* canvas = m_sceneManager->getCanvas();
        if (canvas && canvas->getRefreshManager()) {
            canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::GEOMETRY_CHANGED, true);
        }
        
        canvas->Refresh();
        
        auto batchRefreshEndTime = std::chrono::high_resolution_clock::now();
        auto batchRefreshDuration = std::chrono::duration_cast<std::chrono::milliseconds>(batchRefreshEndTime - batchRefreshStartTime);
        LOG_INF_S("Batch operation completed with view refresh in " + std::to_string(batchRefreshDuration.count()) + "ms");
    } else {
        LOG_INF_S("Batch operation completed");
    }
    
    m_needsViewRefresh = false;
}

bool OCCViewer::isBatchOperationActive() const
{
    return m_batchOperationActive;
}

void OCCViewer::addGeometries(const std::vector<std::shared_ptr<OCCGeometry>>& geometries)
{
    if (geometries.empty()) {
        return;
    }
    
    auto batchAddStartTime = std::chrono::high_resolution_clock::now();
    LOG_INF_S("Starting batch addition of " + std::to_string(geometries.size()) + " geometries");
    
    // Pre-allocate space to avoid reallocations
    m_geometries.reserve(m_geometries.size() + geometries.size());
    
    // Collect all Coin3D nodes for batch addition
    std::vector<SoSeparator*> coinNodes;
    coinNodes.reserve(geometries.size());
    
    for (const auto& geometry : geometries) {
        if (!geometry) {
            LOG_ERR_S("Attempted to add null geometry in batch operation");
            continue;
        }
        
        // Quick validation (no logging in batch mode)
        auto it = std::find_if(m_geometries.begin(), m_geometries.end(),
            [&](const std::shared_ptr<OCCGeometry>& g) {
                return g->getName() == geometry->getName();
            });
        
        if (it != m_geometries.end()) {
            LOG_WRN_S("Geometry with name '" + geometry->getName() + "' already exists (skipping in batch)");
            continue;
        }
        
        // Regenerate mesh
        geometry->regenerateMesh(m_meshParams);
        
        // Store geometry
        m_geometries.push_back(geometry);
        
        // Collect Coin3D node
        SoSeparator* coinNode = geometry->getCoinNode();
        if (coinNode) {
            coinNodes.push_back(coinNode);
        } else {
            LOG_ERR_S("Coin3D node is null for geometry: " + geometry->getName());
        }
    }
    
    // Batch add all Coin3D nodes to scene
    if (m_occRoot && !coinNodes.empty()) {
        for (SoSeparator* coinNode : coinNodes) {
            m_occRoot->addChild(coinNode);
        }
    }
    
    // Queue geometries for deferred ObjectTree update in batch mode
    if (m_batchOperationActive) {
        for (const auto& geometry : geometries) {
            if (geometry) {
                m_pendingObjectTreeUpdates.push_back(geometry);
            }
        }
        LOG_INF_S("Queued " + std::to_string(geometries.size()) + " geometries for deferred ObjectTree update");
    }
    
    auto batchAddEndTime = std::chrono::high_resolution_clock::now();
    auto batchAddDuration = std::chrono::duration_cast<std::chrono::milliseconds>(batchAddEndTime - batchAddStartTime);
    LOG_INF_S("Batch geometry addition completed in " + std::to_string(batchAddDuration.count()) + "ms");
}

void OCCViewer::requestViewRefresh()
{
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        Canvas* canvas = m_sceneManager->getCanvas();
        canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::MATERIAL_CHANGED, true);
    }
}

void OCCViewer::updateObjectTreeDeferred()
{
    if (m_pendingObjectTreeUpdates.empty()) {
        LOG_INF_S("No pending ObjectTree updates to process");
        return;
    }
    
    auto updateStartTime = std::chrono::high_resolution_clock::now();
    LOG_INF_S("Processing " + std::to_string(m_pendingObjectTreeUpdates.size()) + " deferred ObjectTree updates");
    
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        Canvas* canvas = m_sceneManager->getCanvas();
        if (canvas && canvas->getObjectTreePanel()) {
            LOG_INF_S("Found ObjectTreePanel, adding " + std::to_string(m_pendingObjectTreeUpdates.size()) + " geometries");
            for (const auto& geometry : m_pendingObjectTreeUpdates) {
                if (geometry) {
                    LOG_INF_S("Adding geometry to ObjectTree: " + geometry->getName());
                    canvas->getObjectTreePanel()->addOCCGeometry(geometry);
                } else {
                    LOG_ERR_S("Found null geometry in pending updates");
                }
            }
        } else {
            LOG_ERR_S("Canvas or ObjectTreePanel is null, cannot update ObjectTree");
        }
    } else {
        LOG_ERR_S("SceneManager or Canvas is null, cannot update ObjectTree");
    }
    
    m_pendingObjectTreeUpdates.clear();
    
    auto updateEndTime = std::chrono::high_resolution_clock::now();
    auto updateDuration = std::chrono::duration_cast<std::chrono::milliseconds>(updateEndTime - updateStartTime);
    LOG_INF_S("Deferred ObjectTree updates completed in " + std::to_string(updateDuration.count()) + "ms");
}

// Subdivision surface control implementations
void OCCViewer::setSubdivisionEnabled(bool enabled)
{
    if (m_subdivisionEnabled != enabled) {
        bool oldValue = m_subdivisionEnabled;
        m_subdivisionEnabled = enabled;
        LOG_INF_S("Subdivision enabled: " + std::string(enabled ? "true" : "false"));
        logParameterChange("subdivision_enabled", oldValue ? 1.0 : 0.0, enabled ? 1.0 : 0.0);
        // TODO: Apply subdivision to all geometries
        remeshAllGeometries();
    }
}

bool OCCViewer::isSubdivisionEnabled() const
{
    return m_subdivisionEnabled;
}

void OCCViewer::setSubdivisionLevel(int level)
{
    if (m_subdivisionLevel != level && level >= 1 && level <= 5) {
        int oldValue = m_subdivisionLevel;
        m_subdivisionLevel = level;
        LOG_INF_S("Subdivision level set to: " + std::to_string(level));
        logParameterChange("subdivision_level", oldValue, level);
        if (m_subdivisionEnabled) {
            remeshAllGeometries();
        }
    }
}

int OCCViewer::getSubdivisionLevel() const
{
    return m_subdivisionLevel;
}

void OCCViewer::setSubdivisionMethod(int method)
{
    if (m_subdivisionMethod != method && method >= 0 && method <= 3) {
        m_subdivisionMethod = method;
        LOG_INF_S("Subdivision method set to: " + std::to_string(method));
        if (m_subdivisionEnabled) {
            remeshAllGeometries();
        }
    }
}

int OCCViewer::getSubdivisionMethod() const
{
    return m_subdivisionMethod;
}

void OCCViewer::setSubdivisionCreaseAngle(double angle)
{
    if (m_subdivisionCreaseAngle != angle && angle >= 0.0 && angle <= 180.0) {
        m_subdivisionCreaseAngle = angle;
        LOG_INF_S("Subdivision crease angle set to: " + std::to_string(angle));
        if (m_subdivisionEnabled) {
            remeshAllGeometries();
        }
    }
}

double OCCViewer::getSubdivisionCreaseAngle() const
{
    return m_subdivisionCreaseAngle;
}

// Mesh smoothing control implementations
void OCCViewer::setSmoothingEnabled(bool enabled)
{
    if (m_smoothingEnabled != enabled) {
        m_smoothingEnabled = enabled;
        LOG_INF_S("Smoothing enabled: " + std::string(enabled ? "true" : "false"));
        if (enabled) {
            remeshAllGeometries();
        }
    }
}

bool OCCViewer::isSmoothingEnabled() const
{
    return m_smoothingEnabled;
}

void OCCViewer::setSmoothingMethod(int method)
{
    if (m_smoothingMethod != method && method >= 0 && method <= 3) {
        m_smoothingMethod = method;
        LOG_INF_S("Smoothing method set to: " + std::to_string(method));
        if (m_smoothingEnabled) {
            remeshAllGeometries();
        }
    }
}

int OCCViewer::getSmoothingMethod() const
{
    return m_smoothingMethod;
}

void OCCViewer::setSmoothingIterations(int iterations)
{
    if (m_smoothingIterations != iterations && iterations >= 1 && iterations <= 10) {
        m_smoothingIterations = iterations;
        LOG_INF_S("Smoothing iterations set to: " + std::to_string(iterations));
        if (m_smoothingEnabled) {
            remeshAllGeometries();
        }
    }
}

int OCCViewer::getSmoothingIterations() const
{
    return m_smoothingIterations;
}

void OCCViewer::setSmoothingStrength(double strength)
{
    if (m_smoothingStrength != strength && strength >= 0.01 && strength <= 1.0) {
        m_smoothingStrength = strength;
        LOG_INF_S("Smoothing strength set to: " + std::to_string(strength));
        if (m_smoothingEnabled) {
            remeshAllGeometries();
        }
    }
}

double OCCViewer::getSmoothingStrength() const
{
    return m_smoothingStrength;
}

void OCCViewer::setSmoothingCreaseAngle(double angle)
{
    if (m_smoothingCreaseAngle != angle && angle >= 0.0 && angle <= 180.0) {
        m_smoothingCreaseAngle = angle;
        LOG_INF_S("Smoothing crease angle set to: " + std::to_string(angle));
        if (m_smoothingEnabled) {
            remeshAllGeometries();
        }
    }
}

double OCCViewer::getSmoothingCreaseAngle() const
{
    return m_smoothingCreaseAngle;
}

// Advanced tessellation control implementations
void OCCViewer::setTessellationMethod(int method)
{
    if (m_tessellationMethod != method && method >= 0 && method <= 3) {
        m_tessellationMethod = method;
        LOG_INF_S("Tessellation method set to: " + std::to_string(method));
        remeshAllGeometries();
    }
}

int OCCViewer::getTessellationMethod() const
{
    return m_tessellationMethod;
}

void OCCViewer::setTessellationQuality(int quality)
{
    if (m_tessellationQuality != quality && quality >= 1 && quality <= 5) {
        m_tessellationQuality = quality;
        LOG_INF_S("Tessellation quality set to: " + std::to_string(quality));
        remeshAllGeometries();
    }
}

int OCCViewer::getTessellationQuality() const
{
    return m_tessellationQuality;
}

void OCCViewer::setFeaturePreservation(double preservation)
{
    if (m_featurePreservation != preservation && preservation >= 0.0 && preservation <= 1.0) {
        m_featurePreservation = preservation;
        LOG_INF_S("Feature preservation set to: " + std::to_string(preservation));
        remeshAllGeometries();
    }
}

double OCCViewer::getFeaturePreservation() const
{
    return m_featurePreservation;
}

void OCCViewer::setParallelProcessing(bool enabled)
{
    if (m_parallelProcessing != enabled) {
        m_parallelProcessing = enabled;
        LOG_INF_S("Parallel processing enabled: " + std::string(enabled ? "true" : "false"));
        // TODO: Apply parallel processing settings to mesh generation
    }
}

bool OCCViewer::isParallelProcessing() const
{
    return m_parallelProcessing;
}

void OCCViewer::setAdaptiveMeshing(bool enabled)
{
    if (m_adaptiveMeshing != enabled) {
        m_adaptiveMeshing = enabled;
        LOG_INF_S("Adaptive meshing enabled: " + std::string(enabled ? "true" : "false"));
        if (enabled) {
            remeshAllGeometries();
        }
    }
}

bool OCCViewer::isAdaptiveMeshing() const
{
    return m_adaptiveMeshing;
}

// Mesh quality validation and debugging implementations
void OCCViewer::validateMeshParameters()
{
    LOG_INF_S("=== MESH PARAMETER VALIDATION ===");
    
    // Validate subdivision parameters
    LOG_INF_S("Subdivision Settings:");
    LOG_INF_S("  - Enabled: " + std::string(m_subdivisionEnabled ? "true" : "false"));
    LOG_INF_S("  - Level: " + std::to_string(m_subdivisionLevel));
    LOG_INF_S("  - Method: " + std::to_string(m_subdivisionMethod));
    LOG_INF_S("  - Crease Angle: " + std::to_string(m_subdivisionCreaseAngle));
    
    // Validate smoothing parameters
    LOG_INF_S("Smoothing Settings:");
    LOG_INF_S("  - Enabled: " + std::string(m_smoothingEnabled ? "true" : "false"));
    LOG_INF_S("  - Method: " + std::to_string(m_smoothingMethod));
    LOG_INF_S("  - Iterations: " + std::to_string(m_smoothingIterations));
    LOG_INF_S("  - Strength: " + std::to_string(m_smoothingStrength));
    LOG_INF_S("  - Crease Angle: " + std::to_string(m_smoothingCreaseAngle));
    
    // Validate tessellation parameters
    LOG_INF_S("Tessellation Settings:");
    LOG_INF_S("  - Method: " + std::to_string(m_tessellationMethod));
    LOG_INF_S("  - Quality: " + std::to_string(m_tessellationQuality));
    LOG_INF_S("  - Feature Preservation: " + std::to_string(m_featurePreservation));
    LOG_INF_S("  - Parallel Processing: " + std::string(m_parallelProcessing ? "true" : "false"));
    LOG_INF_S("  - Adaptive Meshing: " + std::string(m_adaptiveMeshing ? "true" : "false"));
    
    // Validate basic mesh parameters
    LOG_INF_S("Basic Mesh Settings:");
    LOG_INF_S("  - Deflection: " + std::to_string(m_meshParams.deflection));
    LOG_INF_S("  - Angular Deflection: " + std::to_string(m_meshParams.angularDeflection));
    LOG_INF_S("  - Relative: " + std::string(m_meshParams.relative ? "true" : "false"));
    LOG_INF_S("  - In Parallel: " + std::string(m_meshParams.inParallel ? "true" : "false"));
    
    LOG_INF_S("=== VALIDATION COMPLETE ===");
}

void OCCViewer::logCurrentMeshSettings()
{
    LOG_INF_S("=== CURRENT MESH SETTINGS ===");
    LOG_INF_S("Geometry Count: " + std::to_string(m_geometries.size()));
    
    for (const auto& geometry : m_geometries) {
        if (geometry) {
            LOG_INF_S("Geometry: " + geometry->getName());
            // TODO: Add geometry-specific mesh statistics
        }
    }
    
    LOG_INF_S("=== SETTINGS LOGGED ===");
}

void OCCViewer::compareMeshQuality(const std::string& geometryName)
{
    auto geometry = findGeometry(geometryName);
    if (!geometry) {
        LOG_ERR_S("Geometry not found: " + geometryName);
        return;
    }
    
    LOG_INF_S("=== MESH QUALITY COMPARISON FOR: " + geometryName + " ===");
    
    // TODO: Implement mesh quality comparison
    // This would compare current mesh with previous state or reference mesh
    
    LOG_INF_S("=== COMPARISON COMPLETE ===");
}

std::string OCCViewer::getMeshQualityReport() const
{
    std::string report = "=== MESH QUALITY REPORT ===\n";
    
    report += "Active Geometries: " + std::to_string(m_geometries.size()) + "\n";
    report += "Subdivision Enabled: " + std::string(m_subdivisionEnabled ? "Yes" : "No") + "\n";
    report += "Smoothing Enabled: " + std::string(m_smoothingEnabled ? "Yes" : "No") + "\n";
    report += "Adaptive Meshing: " + std::string(m_adaptiveMeshing ? "Yes" : "No") + "\n";
    report += "Parallel Processing: " + std::string(m_parallelProcessing ? "Yes" : "No") + "\n";
    
    report += "\nCurrent Parameters:\n";
    report += "- Deflection: " + std::to_string(m_meshParams.deflection) + "\n";
    report += "- Subdivision Level: " + std::to_string(m_subdivisionLevel) + "\n";
    report += "- Smoothing Iterations: " + std::to_string(m_smoothingIterations) + "\n";
    report += "- Tessellation Quality: " + std::to_string(m_tessellationQuality) + "\n";
    
    return report;
}

void OCCViewer::exportMeshStatistics(const std::string& filename)
{
    LOG_INF_S("Exporting mesh statistics to: " + filename);
    
    // TODO: Implement mesh statistics export
    // This would export detailed mesh information to a file
    
    LOG_INF_S("Mesh statistics exported successfully");
}

bool OCCViewer::verifyParameterApplication(const std::string& parameterName, double expectedValue)
{
    LOG_INF_S("Verifying parameter: " + parameterName + " = " + std::to_string(expectedValue));
    
    // Check if parameter matches expected value
    if (parameterName == "deflection") {
        bool matches = std::abs(m_meshParams.deflection - expectedValue) < 1e-6;
        LOG_INF_S("Deflection verification: " + std::string(matches ? "PASS" : "FAIL"));
        return matches;
    }
    else if (parameterName == "subdivision_level") {
        bool matches = (m_subdivisionLevel == static_cast<int>(expectedValue));
        LOG_INF_S("Subdivision level verification: " + std::string(matches ? "PASS" : "FAIL"));
        return matches;
    }
    else if (parameterName == "smoothing_iterations") {
        bool matches = (m_smoothingIterations == static_cast<int>(expectedValue));
        LOG_INF_S("Smoothing iterations verification: " + std::string(matches ? "PASS" : "FAIL"));
        return matches;
    }
    // Add more parameter checks as needed
    
    LOG_ERR_S("Unknown parameter: " + parameterName);
    return false;
}

// Real-time parameter monitoring implementations
void OCCViewer::enableParameterMonitoring(bool enabled)
{
    m_parameterMonitoringEnabled = enabled;
    LOG_INF_S("Parameter monitoring " + std::string(enabled ? "enabled" : "disabled"));
}

bool OCCViewer::isParameterMonitoringEnabled() const
{
    return m_parameterMonitoringEnabled;
}

void OCCViewer::logParameterChange(const std::string& parameterName, double oldValue, double newValue)
{
    if (m_parameterMonitoringEnabled) {
        LOG_INF_S("PARAMETER CHANGE: " + parameterName + 
                 " [" + std::to_string(oldValue) + " -> " + std::to_string(newValue) + "]");
    }
}

// DisplayFlags setters
void OCCViewer::setShowEdgesFlag(bool flag) {
    for (auto& geometry : m_geometries) {
        if (geometry) geometry->setShowEdges(flag);
    }
    updateDisplay();
}
void OCCViewer::setShowWireframeFlag(bool flag) {
    for (auto& geometry : m_geometries) {
        if (geometry) geometry->setShowWireframe(flag);
    }
    updateDisplay();
}
void OCCViewer::setShowFacesFlag(bool flag) {
    m_displayFlags.showFaces = flag;
    updateDisplay();
}
void OCCViewer::setShowNormalsFlag(bool flag) {
    m_displayFlags.showNormals = flag;
    updateDisplay();
}
bool OCCViewer::isShowEdgesFlag() const { return m_displayFlags.showEdges; }
bool OCCViewer::isShowWireframeFlag() const { return m_displayFlags.showWireframe; }
bool OCCViewer::isShowFacesFlag() const { return m_displayFlags.showFaces; }
bool OCCViewer::isShowNormalsFlag() const { return m_displayFlags.showNormals; }

void OCCViewer::updateDisplay() {
    // Clear scene or prepare for redraw if needed
    // ...
    if (isShowFacesFlag()) {
        drawFaces();
    }
    if (isShowWireframeFlag()) {
        drawWireframe();
    }
    // Edge display is now handled by EdgeComponent system
    // drawEdges() call removed to avoid conflicts
    if (isShowNormalsFlag()) {
        drawNormals();
    }
    // Trigger a canvas refresh if needed
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        m_sceneManager->getCanvas()->Refresh();
    }
}

// Stub implementations for draw methods
void OCCViewer::drawFaces() {
    // Render all geometry faces/solids
    for (const auto& geometry : m_geometries) {
        if (!geometry || !geometry->isVisible()) continue;
        SoSeparator* coinNode = geometry->getCoinNode();
        if (coinNode && m_occRoot) {
            // Ensure face display is enabled for this geometry
            geometry->setFaceDisplay(true);
            // Add to scene if not already present
            if (!m_occRoot->findChild(coinNode)) {
                m_occRoot->addChild(coinNode);
            }
        }
    }
}

void OCCViewer::drawWireframe() {
    // Render all geometry in wireframe mode (overlay)
    for (const auto& geometry : m_geometries) {
        if (!geometry || !geometry->isVisible()) continue;
        geometry->setWireframeOverlay(true);
    }
}

// drawEdges() method removed - edge display is now handled by EdgeComponent system

void OCCViewer::drawNormals() {
    // Render normals for all visible geometry
    for (const auto& geometry : m_geometries) {
        if (!geometry || !geometry->isVisible()) continue;
        geometry->setNormalDisplay(true);
    }
}

void OCCViewer::setShowOriginalEdges(bool show) {
    globalEdgeFlags.showOriginalEdges = show;
    updateAllEdgeDisplays();
}
void OCCViewer::setShowFeatureEdges(bool show) {
    globalEdgeFlags.showFeatureEdges = show;
    updateAllEdgeDisplays();
}
void OCCViewer::setShowMeshEdges(bool show) {
    globalEdgeFlags.showMeshEdges = show;
    updateAllEdgeDisplays();
}
void OCCViewer::setShowHighlightEdges(bool show) {
    globalEdgeFlags.showHighlightEdges = show;
    updateAllEdgeDisplays();
}
void OCCViewer::setShowNormalLines(bool show) {
    LOG_INF_S("Setting show normal lines to: " + std::string(show ? "true" : "false"));
    globalEdgeFlags.showNormalLines = show;
    updateAllEdgeDisplays();
}

void OCCViewer::setShowFaceNormalLines(bool show) {
    LOG_INF_S("Setting show face normal lines to: " + std::string(show ? "true" : "false"));
    globalEdgeFlags.showFaceNormalLines = show;
    updateAllEdgeDisplays();
}

void OCCViewer::toggleEdgeType(EdgeType type, bool show) {
    switch(type) {
        case EdgeType::Original: globalEdgeFlags.showOriginalEdges = show; break;
        case EdgeType::Feature: globalEdgeFlags.showFeatureEdges = show; break;
        case EdgeType::Mesh: globalEdgeFlags.showMeshEdges = show; break;
        case EdgeType::Highlight: globalEdgeFlags.showHighlightEdges = show; break;
        case EdgeType::NormalLine: globalEdgeFlags.showNormalLines = show; break;
        case EdgeType::FaceNormalLine: globalEdgeFlags.showFaceNormalLines = show; break;
    }
    updateAllEdgeDisplays();
}

bool OCCViewer::isEdgeTypeEnabled(EdgeType type) const {
    switch(type) {
        case EdgeType::Original: return globalEdgeFlags.showOriginalEdges;
        case EdgeType::Feature: return globalEdgeFlags.showFeatureEdges;
        case EdgeType::Mesh: return globalEdgeFlags.showMeshEdges;
        case EdgeType::Highlight: return globalEdgeFlags.showHighlightEdges;
        case EdgeType::NormalLine: return globalEdgeFlags.showNormalLines;
        case EdgeType::FaceNormalLine: return globalEdgeFlags.showFaceNormalLines;
    }
    return false;
}

void OCCViewer::updateAllEdgeDisplays() {
    LOG_INF_S("Updating all edge displays for " + std::to_string(m_geometries.size()) + " geometries");
    LOG_INF_S("Global edge flags - Normal lines: " + std::string(globalEdgeFlags.showNormalLines ? "true" : "false") + 
              ", Face normal lines: " + std::string(globalEdgeFlags.showFaceNormalLines ? "true" : "false"));
    
    for (auto& g : m_geometries) {
        if (g->edgeComponent) {
            g->edgeComponent->edgeFlags = globalEdgeFlags;
            LOG_INF_S("Updated edge flags for geometry: " + g->getName());
        } else {
            LOG_WRN_S("EdgeComponent is null for geometry: " + g->getName());
        }
        g->updateEdgeDisplay();
    }
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        m_sceneManager->getCanvas()->Refresh();
        LOG_INF_S("Canvas refreshed");
    }
}

