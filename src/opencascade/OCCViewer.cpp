#include "OCCViewer.h"
#include "OCCGeometry.h"
#include "SceneManager.h"
#include "Logger.h"
#include "Canvas.h"

#include <Inventor/nodes/SoSeparator.h>
#include <algorithm>

OCCViewer::OCCViewer(SceneManager* sceneManager)
    : m_sceneManager(sceneManager)
    , m_occRoot(nullptr)
    , m_wireframeMode(false)
    , m_shadingMode(true)
    , m_showEdges(false)
    , m_antiAliasing(true)
    , m_defaultColor(0.7, 0.7, 0.7, Quantity_TOC_RGB)
    , m_defaultTransparency(0.0)
    , m_meshDeflection(0.1)
{
    initializeViewer();
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
    
    LOG_INF("OCC Viewer initialized");
}

void OCCViewer::addGeometry(std::shared_ptr<OCCGeometry> geometry)
{
    if (!geometry) {
        return;
    }
    
    // Check if geometry already exists
    auto it = std::find_if(m_geometries.begin(), m_geometries.end(),
        [&](const std::shared_ptr<OCCGeometry>& g) {
            return g->getName() == geometry->getName();
        });
    
    if (it != m_geometries.end()) {
        LOG_WRN("Geometry with name '" + geometry->getName() + "' already exists");
        return;
    }
    
    m_geometries.push_back(geometry);
    
    // Add to scene graph
    SoSeparator* coinNode = geometry->getCoinNode();
    if (coinNode && m_occRoot) {
        m_occRoot->addChild(coinNode);
    }
    
    LOG_INF("Added OCC geometry: " + geometry->getName());
    
    // Trigger a redraw to make the new geometry visible
    if (m_sceneManager) {
        m_sceneManager->getCanvas()->Refresh();
    }
}

void OCCViewer::removeGeometry(std::shared_ptr<OCCGeometry> geometry)
{
    if (!geometry) {
        return;
    }
    
    auto it = std::find(m_geometries.begin(), m_geometries.end(), geometry);
    if (it != m_geometries.end()) {
        // Remove from scene graph
        SoSeparator* coinNode = geometry->getCoinNode();
        if (coinNode && m_occRoot) {
            m_occRoot->removeChild(coinNode);
        }
        
        // Remove from selected list if selected
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
    m_geometries.clear();
    m_selectedGeometries.clear();
    
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
    auto geometry = findGeometry(name);
    if (geometry) {
        geometry->setVisible(visible);
    }
}

void OCCViewer::setGeometrySelected(const std::string& name, bool selected)
{
    auto geometry = findGeometry(name);
    if (geometry) {
        geometry->setSelected(selected);
        
        if (selected) {
            // Add to selected list if not already there
            auto it = std::find(m_selectedGeometries.begin(), m_selectedGeometries.end(), geometry);
            if (it == m_selectedGeometries.end()) {
                m_selectedGeometries.push_back(geometry);
            }
        } else {
            // Remove from selected list
            auto it = std::find(m_selectedGeometries.begin(), m_selectedGeometries.end(), geometry);
            if (it != m_selectedGeometries.end()) {
                m_selectedGeometries.erase(it);
            }
        }
        
        onSelectionChanged();
    }
}

void OCCViewer::setGeometryColor(const std::string& name, const Quantity_Color& color)
{
    auto geometry = findGeometry(name);
    if (geometry) {
        geometry->setColor(color);
    }
}

void OCCViewer::setGeometryTransparency(const std::string& name, double transparency)
{
    auto geometry = findGeometry(name);
    if (geometry) {
        geometry->setTransparency(transparency);
    }
}

void OCCViewer::hideAll()
{
    for (auto& geometry : m_geometries) {
        geometry->setVisible(false);
    }
}

void OCCViewer::showAll()
{
    for (auto& geometry : m_geometries) {
        geometry->setVisible(true);
    }
}

void OCCViewer::selectAll()
{
    m_selectedGeometries.clear();
    for (auto& geometry : m_geometries) {
        geometry->setSelected(true);
        m_selectedGeometries.push_back(geometry);
    }
    onSelectionChanged();
}

void OCCViewer::deselectAll()
{
    for (auto& geometry : m_selectedGeometries) {
        geometry->setSelected(false);
    }
    m_selectedGeometries.clear();
    onSelectionChanged();
}

void OCCViewer::setAllColor(const Quantity_Color& color)
{
    for (auto& geometry : m_geometries) {
        geometry->setColor(color);
    }
}

void OCCViewer::fitAll()
{
    // This would integrate with the navigation system
    LOG_INF("Fit all OCC geometries");
}

void OCCViewer::fitGeometry(const std::string& name)
{
    auto geometry = findGeometry(name);
    if (geometry) {
        LOG_INF("Fit geometry: " + name);
    }
}

void OCCViewer::resetView()
{
    LOG_INF("Reset OCC view");
}

std::shared_ptr<OCCGeometry> OCCViewer::pickGeometry(int x, int y)
{
    // This would integrate with the picking system
    LOG_INF("Pick geometry at (" + std::to_string(x) + ", " + std::to_string(y) + ")");
    return nullptr;
}

std::vector<std::shared_ptr<OCCGeometry>> OCCViewer::getSelectedGeometry() const
{
    return m_selectedGeometries;
}

double OCCViewer::measureDistance(const gp_Pnt& point1, const gp_Pnt& point2)
{
    return point1.Distance(point2);
}

double OCCViewer::measureAngle(const gp_Pnt& vertex, const gp_Pnt& point1, const gp_Pnt& point2)
{
    gp_Vec v1(vertex, point1);
    gp_Vec v2(vertex, point2);
    return v1.Angle(v2);
}

void OCCViewer::setWireframeMode(bool wireframe)
{
    m_wireframeMode = wireframe;
    // Update rendering mode for all geometries
    updateAll();
}

void OCCViewer::setShadingMode(bool shaded)
{
    m_shadingMode = shaded;
    updateAll();
}

void OCCViewer::setShowEdges(bool showEdges)
{
    m_showEdges = showEdges;
    updateAll();
}

void OCCViewer::setAntiAliasing(bool enabled)
{
    m_antiAliasing = enabled;
    // This would update the rendering settings
}

void OCCViewer::updateAll()
{
    for (auto& geometry : m_geometries) {
        geometry->updateCoinRepresentation();
    }
    refresh();
}

void OCCViewer::refresh()
{
    // Trigger a redraw
    LOG_INF("Refresh OCC viewer");
}

void OCCViewer::onGeometryChanged(std::shared_ptr<OCCGeometry> geometry)
{
    if (geometry) {
        geometry->updateCoinRepresentation();
        refresh();
    }
}

void OCCViewer::onSelectionChanged()
{
    LOG_INF("Selection changed: " + std::to_string(m_selectedGeometries.size()) + " objects selected");
} 