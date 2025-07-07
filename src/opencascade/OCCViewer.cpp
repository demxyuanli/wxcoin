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
    : m_sceneManager(sceneManager)
    , m_occRoot(nullptr)
    , m_normalRoot(nullptr)
    , m_wireframeMode(false)
    , m_shadingMode(true)
    , m_showEdges(false)
    , m_antiAliasing(true)
    , m_showNormals(false)
    , m_normalLength(0.5)
    , m_correctNormalColor(1.0, 0.0, 0.0, Quantity_TOC_RGB)    // Red for correct normals
    , m_incorrectNormalColor(0.0, 1.0, 0.0, Quantity_TOC_RGB)  // Green for incorrect normals
    , m_defaultColor(0.7, 0.7, 0.7, Quantity_TOC_RGB)
    , m_defaultTransparency(0.0)
    , m_meshDeflection(0.1)
{
    initializeViewer();
    // Enable edges display by default
    setShowEdges(true);
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
    OCCMeshConverter::setShowEdges(showEdges);
    // Force each geometry to rebuild its Coin3D node
    if (m_occRoot) {
        m_occRoot->removeAllChildren();
        for (auto& geom : m_geometries) {
            geom->forceRefresh();
            SoSeparator* node = geom->getCoinNode();
            if (node) {
                m_occRoot->addChild(node);
            }
        }
    }
    // Trigger redraw
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        m_sceneManager->getCanvas()->Refresh(true);
    }
}

bool OCCViewer::isShowingEdges() const
{
    return m_showEdges;
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

void OCCViewer::setShowNormals(bool showNormals)
{
    m_showNormals = showNormals;
    updateNormalDisplay();
}

void OCCViewer::setNormalLength(double length)
{
    m_normalLength = length;
    if (m_showNormals) {
        updateNormalDisplay();
    }
}

void OCCViewer::setNormalColor(const Quantity_Color& correctColor, const Quantity_Color& incorrectColor)
{
    m_correctNormalColor = correctColor;
    m_incorrectNormalColor = incorrectColor;
    if (m_showNormals) {
        updateNormalDisplay();
    }
}

void OCCViewer::updateNormalDisplay()
{
    if (!m_normalRoot) {
        return;
    }
    
    // Clear existing normal display
    m_normalRoot->removeAllChildren();
    
    if (!m_showNormals) {
        return;
    }
    
    createNormalNodes();
    
    // Trigger refresh
    if (m_sceneManager) {
        m_sceneManager->getCanvas()->Refresh();
    }
}

void OCCViewer::createNormalNodes()
{
    if (!m_normalRoot) {
        return;
    }
    
    for (auto& geometry : m_geometries) {
        if (!geometry || !geometry->isVisible()) {
            continue;
        }
        
        TopoDS_Shape shape = geometry->getShape();
        if (shape.IsNull()) {
            continue;
        }
        
        // Convert shape to mesh to get normals
        OCCMeshConverter::TriangleMesh mesh = OCCMeshConverter::convertToMesh(shape, m_meshDeflection);
        
        if (mesh.isEmpty()) {
            continue;
        }
        
        // Create separator for this geometry's normals
        SoSeparator* normalSep = new SoSeparator;
        
        // Create coordinates for normal lines
        SoCoordinate3* coords = new SoCoordinate3;
        SoIndexedLineSet* lineSet = new SoIndexedLineSet;
        
        std::vector<SbVec3f> normalCoords;
        std::vector<int32_t> lineIndices;
        
        // Calculate face normals and create normal lines
        for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
            int i0 = mesh.triangles[i];
            int i1 = mesh.triangles[i + 1];
            int i2 = mesh.triangles[i + 2];
            
            if (i0 >= 0 && i0 < static_cast<int>(mesh.vertices.size()) &&
                i1 >= 0 && i1 < static_cast<int>(mesh.vertices.size()) &&
                i2 >= 0 && i2 < static_cast<int>(mesh.vertices.size())) {
                
                const gp_Pnt& v0 = mesh.vertices[i0];
                const gp_Pnt& v1 = mesh.vertices[i1];
                const gp_Pnt& v2 = mesh.vertices[i2];
                
                // Calculate triangle center
                gp_Pnt center(
                    (v0.X() + v1.X() + v2.X()) / 3.0,
                    (v0.Y() + v1.Y() + v2.Y()) / 3.0,
                    (v0.Z() + v1.Z() + v2.Z()) / 3.0
                );
                
                // Calculate face normal
                gp_Vec edge1(v0, v1);
                gp_Vec edge2(v0, v2);
                gp_Vec normal = edge1.Crossed(edge2);
                
                if (normal.Magnitude() > 1e-6) {
                    normal.Normalize();
                    
                    // Better normal orientation check
                    // Check if normal points outward from the centroid of the entire shape
                    gp_Pnt shapeCentroid(0, 0, 0);
                    int vertexCount = 0;
                    for (const auto& vertex : mesh.vertices) {
                        shapeCentroid.SetX(shapeCentroid.X() + vertex.X());
                        shapeCentroid.SetY(shapeCentroid.Y() + vertex.Y());
                        shapeCentroid.SetZ(shapeCentroid.Z() + vertex.Z());
                        vertexCount++;
                    }
                    if (vertexCount > 0) {
                        shapeCentroid.SetX(shapeCentroid.X() / vertexCount);
                        shapeCentroid.SetY(shapeCentroid.Y() / vertexCount);
                        shapeCentroid.SetZ(shapeCentroid.Z() / vertexCount);
                    }
                    
                    // Vector from shape centroid to triangle center
                    gp_Vec outwardVec(shapeCentroid, center);
                    
                    // If normal and outward vector have positive dot product, normal is pointing outward (correct)
                    bool isCorrectOrientation = (normal.Dot(outwardVec) > 0);
                    
                    // Create normal line
                    SbVec3f startPoint(center.X(), center.Y(), center.Z());
                    SbVec3f endPoint(
                        center.X() + normal.X() * m_normalLength,
                        center.Y() + normal.Y() * m_normalLength,
                        center.Z() + normal.Z() * m_normalLength
                    );
                    
                    int startIndex = normalCoords.size();
                    normalCoords.push_back(startPoint);
                    normalCoords.push_back(endPoint);
                    
                    // Add line indices
                    lineIndices.push_back(startIndex);
                    lineIndices.push_back(startIndex + 1);
                    lineIndices.push_back(-1); // End of line
                    
                    // Create material for this normal based on orientation
                    SoMaterial* material = new SoMaterial;
                    if (isCorrectOrientation) {
                        // Red for correct normals
                        material->diffuseColor.setValue(
                            m_correctNormalColor.Red(),
                            m_correctNormalColor.Green(),
                            m_correctNormalColor.Blue()
                        );
                    } else {
                        // Green for incorrect normals
                        material->diffuseColor.setValue(
                            m_incorrectNormalColor.Red(),
                            m_incorrectNormalColor.Green(),
                            m_incorrectNormalColor.Blue()
                        );
                    }
                    
                    // Create separate line set for each normal to have different colors
                    SoSeparator* singleNormalSep = new SoSeparator;
                    singleNormalSep->addChild(material);
                    
                    SoDrawStyle* drawStyle = new SoDrawStyle;
                    drawStyle->lineWidth.setValue(2.0f);
                    singleNormalSep->addChild(drawStyle);
                    
                    SoCoordinate3* singleCoords = new SoCoordinate3;
                    singleCoords->point.setValues(0, 2, &normalCoords[startIndex]);
                    singleNormalSep->addChild(singleCoords);
                    
                    SoIndexedLineSet* singleLineSet = new SoIndexedLineSet;
                    int32_t singleIndices[] = {0, 1, -1};
                    singleLineSet->coordIndex.setValues(0, 3, singleIndices);
                    singleNormalSep->addChild(singleLineSet);
                    
                    normalSep->addChild(singleNormalSep);
                }
            }
        }
        
        if (normalSep->getNumChildren() > 0) {
            m_normalRoot->addChild(normalSep);
        }
    }
    
    LOG_INF("Created normal display for " + std::to_string(m_geometries.size()) + " geometries");
}

void OCCViewer::fixNormals()
{
    for (auto& geometry : m_geometries) {
        if (!geometry || !geometry->isVisible()) {
            continue;
        }
        
        TopoDS_Shape shape = geometry->getShape();
        if (shape.IsNull()) {
            continue;
        }
        
        // Convert shape to mesh
        OCCMeshConverter::TriangleMesh mesh = OCCMeshConverter::convertToMesh(shape, m_meshDeflection);
        
        if (mesh.isEmpty()) {
            continue;
        }
        
        // Calculate shape centroid
        gp_Pnt shapeCentroid(0, 0, 0);
        for (const auto& vertex : mesh.vertices) {
            shapeCentroid.SetX(shapeCentroid.X() + vertex.X());
            shapeCentroid.SetY(shapeCentroid.Y() + vertex.Y());
            shapeCentroid.SetZ(shapeCentroid.Z() + vertex.Z());
        }
        if (!mesh.vertices.empty()) {
            shapeCentroid.SetX(shapeCentroid.X() / mesh.vertices.size());
            shapeCentroid.SetY(shapeCentroid.Y() / mesh.vertices.size());
            shapeCentroid.SetZ(shapeCentroid.Z() / mesh.vertices.size());
        }
        
        // Count incorrect normals
        int incorrectCount = 0;
        int totalCount = 0;
        
        for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
            int i0 = mesh.triangles[i];
            int i1 = mesh.triangles[i + 1];
            int i2 = mesh.triangles[i + 2];
            
            if (i0 >= 0 && i0 < static_cast<int>(mesh.vertices.size()) &&
                i1 >= 0 && i1 < static_cast<int>(mesh.vertices.size()) &&
                i2 >= 0 && i2 < static_cast<int>(mesh.vertices.size())) {
                
                const gp_Pnt& v0 = mesh.vertices[i0];
                const gp_Pnt& v1 = mesh.vertices[i1];
                const gp_Pnt& v2 = mesh.vertices[i2];
                
                // Calculate triangle center
                gp_Pnt center(
                    (v0.X() + v1.X() + v2.X()) / 3.0,
                    (v0.Y() + v1.Y() + v2.Y()) / 3.0,
                    (v0.Z() + v1.Z() + v2.Z()) / 3.0
                );
                
                // Calculate face normal
                gp_Vec edge1(v0, v1);
                gp_Vec edge2(v0, v2);
                gp_Vec normal = edge1.Crossed(edge2);
                
                if (normal.Magnitude() > 1e-6) {
                    normal.Normalize();
                    
                    // Vector from shape centroid to triangle center
                    gp_Vec outwardVec(shapeCentroid, center);
                    
                    // Check if normal points outward
                    bool isCorrectOrientation = (normal.Dot(outwardVec) > 0);
                    
                    totalCount++;
                    if (!isCorrectOrientation) {
                        incorrectCount++;
                    }
                }
            }
        }
        
        // If more than 50% of normals are incorrect, flip all normals
        if (totalCount > 0 && incorrectCount > totalCount / 2) {
            LOG_INF("Flipping normals for geometry: " + geometry->getName() + 
                   " (" + std::to_string(incorrectCount) + "/" + std::to_string(totalCount) + " incorrect)");
            
            OCCMeshConverter::flipNormals(mesh);
            
            // Update the geometry's representation
            geometry->updateCoinRepresentation();
        }
    }
    
    // Update normal display if it's currently shown
    if (m_showNormals) {
        updateNormalDisplay();
    }
} 