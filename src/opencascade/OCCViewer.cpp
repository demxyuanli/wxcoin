#ifdef _WIN32
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "OCCViewer.h"
#include "OCCGeometry.h"
#include "OCCMeshConverter.h"
#include "SceneManager.h"
#include "Logger.h"
#include "Canvas.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <algorithm>

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
    
    LOG_INF("OCC Viewer initialized");
}

void OCCViewer::addGeometry(std::shared_ptr<OCCGeometry> geometry)
{
    if (!geometry) {
        return;
    }
    
    auto it = std::find_if(m_geometries.begin(), m_geometries.end(),
        [&](const std::shared_ptr<OCCGeometry>& g) {
            return g->getName() == geometry->getName();
        });
    
    if (it != m_geometries.end()) {
        LOG_WRN("Geometry with name '" + geometry->getName() + "' already exists");
        return;
    }
    
    geometry->regenerateMesh(m_meshParams);
    m_geometries.push_back(geometry);
    
    SoSeparator* coinNode = geometry->getCoinNode();
    if (coinNode && m_occRoot) {
        m_occRoot->addChild(coinNode);
    }
    
    LOG_INF("Added OCC geometry: " + geometry->getName());
    
    if (m_sceneManager) {
        m_sceneManager->getCanvas()->Refresh();
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
        
        m_geometries.erase(it);
        LOG_INF("Removed OCC geometry: " + geometry->getName());
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
    
    LOG_INF("Cleared all OCC geometries");
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

void OCCViewer::setGeometryVisible(const std::string& name, bool visible)
{
    if (auto g = findGeometry(name)) g->setVisible(visible);
}

void OCCViewer::setGeometrySelected(const std::string& name, bool selected)
{
    auto geometry = findGeometry(name);
    if (!geometry) return;

        geometry->setSelected(selected);
        
        if (selected) {
        if (std::find(m_selectedGeometries.begin(), m_selectedGeometries.end(), geometry) == m_selectedGeometries.end()) {
                m_selectedGeometries.push_back(geometry);
            }
        } else {
        m_selectedGeometries.erase(std::remove(m_selectedGeometries.begin(), m_selectedGeometries.end(), geometry), m_selectedGeometries.end());
    }
    onSelectionChanged();
}

void OCCViewer::setGeometryColor(const std::string& name, const Quantity_Color& color)
{
    if (auto g = findGeometry(name)) g->setColor(color);
}

void OCCViewer::setGeometryTransparency(const std::string& name, double transparency)
{
    if (auto g = findGeometry(name)) g->setTransparency(transparency);
}

void OCCViewer::hideAll()
{
    for (auto& g : m_geometries) g->setVisible(false);
}

void OCCViewer::showAll()
{
    for (auto& g : m_geometries) g->setVisible(true);
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
    for (auto& g : m_selectedGeometries) g->setSelected(false);
    m_selectedGeometries.clear();
    onSelectionChanged();
}

void OCCViewer::setAllColor(const Quantity_Color& color)
{
    for (auto& g : m_geometries) g->setColor(color);
}

void OCCViewer::fitAll()
{
    LOG_INF("Fit all OCC geometries");
}

void OCCViewer::fitGeometry(const std::string& name)
{
        LOG_INF("Fit geometry: " + name);
}

std::shared_ptr<OCCGeometry> OCCViewer::pickGeometry(int x, int y)
{
    return nullptr;
}

void OCCViewer::setWireframeMode(bool wireframe)
{
    m_wireframeMode = wireframe;
}

void OCCViewer::setShadingMode(bool shaded)
{
    m_shadingMode = shaded;
}

void OCCViewer::setShowEdges(bool showEdges)
{
    m_showEdges = showEdges;
    OCCMeshConverter::setShowEdges(showEdges);
    remeshAllGeometries();
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
    LOG_INF("Selection changed");
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
    if (m_sceneManager) {
        m_sceneManager->getCanvas()->Refresh();
    }
}

void OCCViewer::remeshAllGeometries()
{
    for (auto& geometry : m_geometries) {
        geometry->regenerateMesh(m_meshParams);
    }
    if (m_sceneManager) {
        m_sceneManager->getCanvas()->Refresh();
    }
    LOG_INF("Remeshed all geometries with deflection: " + std::to_string(m_meshParams.deflection));
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
        LOG_INF("LOD " + std::string(enabled ? "enabled" : "disabled"));
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
        LOG_INF("LOD rough deflection set to: " + std::to_string(deflection));
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
        LOG_INF("LOD fine deflection set to: " + std::to_string(deflection));
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
        LOG_INF("LOD transition time set to: " + std::to_string(milliseconds) + "ms");
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
        
        LOG_INF("LOD mode switched to " + std::string(roughMode ? "rough" : "fine") + 
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