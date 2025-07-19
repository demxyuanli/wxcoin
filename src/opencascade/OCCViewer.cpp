#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "OCCViewer.h"
#include "OCCGeometry.h"
#include "OCCMeshConverter.h"
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

OCCViewer::OCCViewer(SceneManager* sceneManager)
    : m_sceneManager(sceneManager),
      m_occRoot(nullptr),
      m_normalRoot(nullptr),
      m_wireframeMode(false),
      m_shadingMode(true),
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
      m_lodTimer(this, wxID_ANY)
{
    initializeViewer();
    setShowEdges(true);
    
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
    if (m_normalRoot) {
        m_normalRoot->unref();
    }
}

void OCCViewer::initializeViewer()
{
    m_occRoot = new SoSeparator;
    m_occRoot->ref();
    
    m_normalRoot = new SoSeparator;
    m_normalRoot->ref();
    
    if (m_sceneManager) {
        m_sceneManager->getObjectRoot()->addChild(m_occRoot);
        m_sceneManager->getObjectRoot()->addChild(m_normalRoot);
    }
    
    LOG_INF_S("OCC Viewer initialized");
}

void OCCViewer::addGeometry(std::shared_ptr<OCCGeometry> geometry)
{
    if (!geometry) {
        LOG_ERR_S("Attempted to add null geometry to OCCViewer");
        return;
    }
    
    auto it = std::find_if(m_geometries.begin(), m_geometries.end(),
        [&](const std::shared_ptr<OCCGeometry>& g) {
            return g->getName() == geometry->getName();
        });
    
    if (it != m_geometries.end()) {
        LOG_WRN_S("Geometry with name '" + geometry->getName() + "' already exists");
        return;
    }
    
    LOG_INF_S("Adding geometry to OCCViewer: " + geometry->getName());
    
    geometry->regenerateMesh(m_meshParams);
    m_geometries.push_back(geometry);
    
    SoSeparator* coinNode = geometry->getCoinNode();
    LOG_INF_S("Got Coin3D node for geometry: " + geometry->getName() + " - Node: " + (coinNode ? "valid" : "null"));
    
    if (coinNode && m_occRoot) {
        m_occRoot->addChild(coinNode);
        LOG_INF_S("Added Coin3D node to OCC root for geometry: " + geometry->getName());
    } else {
        if (!coinNode) {
            LOG_ERR_S("Coin3D node is null for geometry: " + geometry->getName());
        }
        if (!m_occRoot) {
            LOG_ERR_S("OCC root is null, cannot add geometry: " + geometry->getName());
        }
    }
    
    LOG_INF_S("Added OCC geometry: " + geometry->getName());
    
    // Add geometry to ObjectTree if available
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        Canvas* canvas = m_sceneManager->getCanvas();
        if (canvas && canvas->getObjectTreePanel()) {
            canvas->getObjectTreePanel()->addOCCGeometry(geometry);
        }
    }
    
    // Auto-update scene bounds and view when geometry is added
    if (m_sceneManager) {
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

void OCCViewer::setShadingMode(bool shaded)
{
    m_shadingMode = shaded;
    // Update all geometries to shading mode
    for (auto& geometry : m_geometries) {
        if (geometry) {
            geometry->setShadingMode(shaded);
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

void OCCViewer::setShowEdges(bool showEdges)
{
    m_showEdges = showEdges;
    
    // Update EdgeSettingsConfig
    EdgeSettingsConfig& edgeConfig = EdgeSettingsConfig::getInstance();
    edgeConfig.setGlobalShowEdges(showEdges);
    
    // Update OCCMeshConverter
    OCCMeshConverter::setShowEdges(showEdges);
    
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

bool OCCViewer::isShadingMode() const
{
    return m_shadingMode;
}

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
    updateNormalsDisplay();
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
    if (!m_normalRoot) return;
    m_normalRoot->removeAllChildren();
    
    if (!m_showNormals) {
        if (m_sceneManager && m_sceneManager->getCanvas()) {
            auto* refreshManager = m_sceneManager->getCanvas()->getRefreshManager();
            if (refreshManager) {
                refreshManager->requestRefresh(ViewRefreshManager::RefreshReason::NORMALS_TOGGLED, true);
            }
        }
        return;
    }
    
    // Create normals for all visible geometries
    for (auto& geometry : m_geometries) {
        if (!geometry->isVisible()) continue;
        
        // Get mesh data from geometry
        SoSeparator* geomNode = geometry->getCoinNode();
        if (!geomNode) continue;
        
        // Extract mesh data and create normal visualization
        createNormalVisualization(geometry);
    }
    
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        auto* refreshManager = m_sceneManager->getCanvas()->getRefreshManager();
        if (refreshManager) {
            refreshManager->requestRefresh(ViewRefreshManager::RefreshReason::NORMALS_TOGGLED, true);
        }
    }
}

void OCCViewer::createNormalVisualization(std::shared_ptr<OCCGeometry> geometry)
{
    // Convert OCC shape to mesh to get normals
    OCCMeshConverter::TriangleMesh mesh = OCCMeshConverter::convertToMesh(
        geometry->getShape(), m_meshParams);
    
    if (mesh.vertices.empty() || mesh.normals.empty()) {
        return;
    }
    
    SoSeparator* normalGroup = new SoSeparator;
    
    // Get the geometry's position to apply to normal positions
    gp_Pnt geometryPosition = geometry->getPosition();
    
    // Create coordinate points for normal lines
    std::vector<SbVec3f> linePoints;
    std::vector<int32_t> lineIndices;
    
    for (size_t i = 0; i < mesh.vertices.size(); ++i) {
        const gp_Pnt& vertex = mesh.vertices[i];
        const gp_Vec& normal = mesh.normals[i];  
        
        // Start point (vertex position with geometry offset)
        SbVec3f startPoint(
            static_cast<float>(vertex.X() + geometryPosition.X()),
            static_cast<float>(vertex.Y() + geometryPosition.Y()),
            static_cast<float>(vertex.Z() + geometryPosition.Z())
        );
        
        // End point (vertex position + normal vector with geometry offset)
        SbVec3f endPoint(
            static_cast<float>(vertex.X() + geometryPosition.X() + normal.X() * m_normalLength),
            static_cast<float>(vertex.Y() + geometryPosition.Y() + normal.Y() * m_normalLength),
            static_cast<float>(vertex.Z() + geometryPosition.Z() + normal.Z() * m_normalLength)
        );
        
        linePoints.push_back(startPoint);
        linePoints.push_back(endPoint);
        
        // Line indices
        int baseIndex = static_cast<int>(linePoints.size()) - 2;
        lineIndices.push_back(baseIndex);
        lineIndices.push_back(baseIndex + 1);
        lineIndices.push_back(SO_END_LINE_INDEX);
    }
    
    // Create coordinate node
    SoCoordinate3* coords = new SoCoordinate3;
    coords->point.setNum(static_cast<int>(linePoints.size()));
    SbVec3f* points = coords->point.startEditing();
    for (size_t i = 0; i < linePoints.size(); ++i) {
        points[i] = linePoints[i];
    }
    coords->point.finishEditing();
    normalGroup->addChild(coords);
    
    // Create material for normals
    SoMaterial* normalMaterial = new SoMaterial;
    // Use correct normal color (green for correct, red for incorrect)
    // For now, assume all normals are correct
    normalMaterial->diffuseColor.setValue(
        static_cast<float>(m_correctNormalColor.Red()),
        static_cast<float>(m_correctNormalColor.Green()),
        static_cast<float>(m_correctNormalColor.Blue())
    );
    normalGroup->addChild(normalMaterial);
    
    // Create line set
    SoIndexedLineSet* lineSet = new SoIndexedLineSet;
    lineSet->coordIndex.setValues(0, static_cast<int>(lineIndices.size()), 
                                  lineIndices.data());
    normalGroup->addChild(lineSet);
    
    m_normalRoot->addChild(normalGroup);
}

void OCCViewer::remeshAllGeometries()
{
    for (auto& geometry : m_geometries) {
        geometry->regenerateMesh(m_meshParams);
    }
    
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        auto* refreshManager = m_sceneManager->getCanvas()->getRefreshManager();
        if (refreshManager) {
            refreshManager->requestRefresh(ViewRefreshManager::RefreshReason::GEOMETRY_CHANGED);
        }
    }
    
    LOG_INF_S("Remeshed all geometries with deflection: " + std::to_string(m_meshParams.deflection));
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
    if (m_lodEnabled) {
        // Switch to rough mode immediately
        setLODMode(true);
        
        // Start timer to switch back to fine mode
        m_lodTimer.Start(m_lodTransitionTime, wxTIMER_ONE_SHOT);
    }
}

void OCCViewer::requestViewRefresh()
{
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        Canvas* canvas = m_sceneManager->getCanvas();
        canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::MATERIAL_CHANGED, true);
    }
}

