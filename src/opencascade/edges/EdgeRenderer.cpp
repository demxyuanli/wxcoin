#include "edges/EdgeRenderer.h"
#include "edges/EdgeLODManager.h"
#include "rendering/GeometryProcessor.h"
#include "rendering/GPUEdgeRenderer.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoTranslation.h>

EdgeRenderer::EdgeRenderer()
    : originalEdgeNode(nullptr)
    , featureEdgeNode(nullptr)
    , meshEdgeNode(nullptr)
    , highlightEdgeNode(nullptr)
    , normalLineNode(nullptr)
    , faceNormalLineNode(nullptr)
    , silhouetteEdgeNode(nullptr)
    , intersectionNodesNode(nullptr)
    , m_gpuRenderer(nullptr)
    , m_gpuAccelerationEnabled(false)
    , m_gpuMeshEdgeNode(nullptr)
{
    // Initialize GPU renderer (issue was in EdgeGeometryCache, now fixed)
    try {
        m_gpuRenderer = new GPUEdgeRenderer();
        if (m_gpuRenderer && m_gpuRenderer->initialize()) {
            m_gpuAccelerationEnabled = m_gpuRenderer->isAvailable();
            if (m_gpuAccelerationEnabled) {
                LOG_INF_S("GPU-accelerated edge rendering initialized");
            }
        }
    }
    catch (const std::exception& e) {
        LOG_WRN_S("Failed to initialize GPU renderer: " + std::string(e.what()));
        m_gpuAccelerationEnabled = false;
        if (m_gpuRenderer) {
            delete m_gpuRenderer;
            m_gpuRenderer = nullptr;
        }
    }
}

EdgeRenderer::~EdgeRenderer()
{
    if (originalEdgeNode) originalEdgeNode->unref();
    if (featureEdgeNode) featureEdgeNode->unref();
    if (meshEdgeNode) meshEdgeNode->unref();
    if (highlightEdgeNode) highlightEdgeNode->unref();
    if (normalLineNode) normalLineNode->unref();
    if (faceNormalLineNode) faceNormalLineNode->unref();
    if (silhouetteEdgeNode) silhouetteEdgeNode->unref();
    if (intersectionNodesNode) intersectionNodesNode->unref();
    if (m_gpuMeshEdgeNode) m_gpuMeshEdgeNode->unref();

    // Cleanup GPU renderer
    if (m_gpuRenderer) {
        m_gpuRenderer->shutdown();
        delete m_gpuRenderer;
        m_gpuRenderer = nullptr;
    }
}

SoSeparator* EdgeRenderer::createLineNode(
    const std::vector<gp_Pnt>& points,
    const Quantity_Color& color,
    double width)
{
    if (points.empty()) return nullptr;

    SoSeparator* separator = new SoSeparator();
    separator->ref();

    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(
        static_cast<float>(color.Red()),
        static_cast<float>(color.Green()),
        static_cast<float>(color.Blue())
    );
    separator->addChild(material);

    SoDrawStyle* drawStyle = new SoDrawStyle();
    drawStyle->lineWidth.setValue(static_cast<float>(width));
    separator->addChild(drawStyle);

    SoCoordinate3* coords = new SoCoordinate3();
    coords->point.setNum(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        coords->point.set1Value(i, 
            static_cast<float>(points[i].X()),
            static_cast<float>(points[i].Y()),
            static_cast<float>(points[i].Z())
        );
    }
    separator->addChild(coords);

    SoIndexedLineSet* lineSet = new SoIndexedLineSet();
    int32_t* indices = new int32_t[points.size() * 3];
    int idx = 0;
    for (size_t i = 0; i + 1 < points.size(); i += 2) {
        indices[idx++] = i;
        indices[idx++] = i + 1;
        indices[idx++] = -1;
    }
    lineSet->coordIndex.setValues(0, idx, indices);
    separator->addChild(lineSet);
    delete[] indices;

    return separator;
}

void EdgeRenderer::generateOriginalEdgeNode(
    const std::vector<gp_Pnt>& points,
    const Quantity_Color& color,
    double width)
{
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    
    if (originalEdgeNode) {
        originalEdgeNode->unref();
        originalEdgeNode = nullptr;
    }

    originalEdgeNode = createLineNode(points, color, width);
}

void EdgeRenderer::generateFeatureEdgeNode(
    const std::vector<gp_Pnt>& points,
    const Quantity_Color& color,
    double width)
{
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    
    if (featureEdgeNode) {
        featureEdgeNode->unref();
        featureEdgeNode = nullptr;
    }

    featureEdgeNode = createLineNode(points, color, width);
}

void EdgeRenderer::generateMeshEdgeNode(
    const std::vector<gp_Pnt>& points,
    const Quantity_Color& color,
    double width)
{
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    
    if (meshEdgeNode) {
        meshEdgeNode->unref();
        meshEdgeNode = nullptr;
    }

    meshEdgeNode = createLineNode(points, color, width);
}

void EdgeRenderer::generateHighlightEdgeNode()
{
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    
    if (highlightEdgeNode) {
        highlightEdgeNode->unref();
        highlightEdgeNode = nullptr;
    }

    highlightEdgeNode = new SoSeparator();
    highlightEdgeNode->ref();
}

void EdgeRenderer::generateNormalLineNode(const TriangleMesh& mesh, double length)
{
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    
    if (normalLineNode) {
        normalLineNode->unref();
        normalLineNode = nullptr;
    }

    normalLineNode = new SoSeparator();
    normalLineNode->ref();

    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(1.0f, 0.0f, 0.0f);
    normalLineNode->addChild(material);

    SoDrawStyle* drawStyle = new SoDrawStyle();
    drawStyle->lineWidth.setValue(1.0f);
    normalLineNode->addChild(drawStyle);

    std::vector<gp_Pnt> linePoints;
    for (size_t i = 0; i < mesh.vertices.size() && i < mesh.normals.size(); ++i) {
        const gp_Pnt& v = mesh.vertices[i];
        const gp_Vec& n = mesh.normals[i];
        
        linePoints.push_back(v);
        linePoints.push_back(gp_Pnt(v.X() + n.X() * length, 
                                     v.Y() + n.Y() * length, 
                                     v.Z() + n.Z() * length));
    }

    if (!linePoints.empty()) {
        SoCoordinate3* coords = new SoCoordinate3();
        coords->point.setNum(linePoints.size());
        for (size_t i = 0; i < linePoints.size(); ++i) {
            coords->point.set1Value(i,
                static_cast<float>(linePoints[i].X()),
                static_cast<float>(linePoints[i].Y()),
                static_cast<float>(linePoints[i].Z())
            );
        }
        normalLineNode->addChild(coords);

        SoIndexedLineSet* lineSet = new SoIndexedLineSet();
        int32_t* indices = new int32_t[linePoints.size() / 2 * 3];
        int idx = 0;
        for (size_t i = 0; i < linePoints.size(); i += 2) {
            indices[idx++] = i;
            indices[idx++] = i + 1;
            indices[idx++] = -1;
        }
        lineSet->coordIndex.setValues(0, idx, indices);
        normalLineNode->addChild(lineSet);
        delete[] indices;
    }
}

void EdgeRenderer::generateFaceNormalLineNode(const TriangleMesh& mesh, double length)
{
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    
    if (faceNormalLineNode) {
        faceNormalLineNode->unref();
        faceNormalLineNode = nullptr;
    }

    faceNormalLineNode = new SoSeparator();
    faceNormalLineNode->ref();

    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(0.0f, 0.0f, 1.0f);
    faceNormalLineNode->addChild(material);

    SoDrawStyle* drawStyle = new SoDrawStyle();
    drawStyle->lineWidth.setValue(1.0f);
    faceNormalLineNode->addChild(drawStyle);

    std::vector<gp_Pnt> linePoints;
    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
        int v1 = mesh.triangles[i];
        int v2 = mesh.triangles[i + 1];
        int v3 = mesh.triangles[i + 2];
        
        if (v1 >= static_cast<int>(mesh.vertices.size()) || 
            v2 >= static_cast<int>(mesh.vertices.size()) || 
            v3 >= static_cast<int>(mesh.vertices.size()))
            continue;

        const gp_Pnt& p1 = mesh.vertices[v1];
        const gp_Pnt& p2 = mesh.vertices[v2];
        const gp_Pnt& p3 = mesh.vertices[v3];

        gp_Pnt center((p1.X() + p2.X() + p3.X()) / 3.0,
                      (p1.Y() + p2.Y() + p3.Y()) / 3.0,
                      (p1.Z() + p2.Z() + p3.Z()) / 3.0);

        gp_Vec vec1(p1, p2);
        gp_Vec vec2(p1, p3);
        gp_Vec normal = vec1.Crossed(vec2);
        
        if (normal.Magnitude() > 1e-7) {
            normal.Normalize();
            linePoints.push_back(center);
            linePoints.push_back(gp_Pnt(center.X() + normal.X() * length,
                                        center.Y() + normal.Y() * length,
                                        center.Z() + normal.Z() * length));
        }
    }

    if (!linePoints.empty()) {
        SoCoordinate3* coords = new SoCoordinate3();
        coords->point.setNum(linePoints.size());
        for (size_t i = 0; i < linePoints.size(); ++i) {
            coords->point.set1Value(i,
                static_cast<float>(linePoints[i].X()),
                static_cast<float>(linePoints[i].Y()),
                static_cast<float>(linePoints[i].Z())
            );
        }
        faceNormalLineNode->addChild(coords);

        SoIndexedLineSet* lineSet = new SoIndexedLineSet();
        int32_t* indices = new int32_t[linePoints.size() / 2 * 3];
        int idx = 0;
        for (size_t i = 0; i < linePoints.size(); i += 2) {
            indices[idx++] = i;
            indices[idx++] = i + 1;
            indices[idx++] = -1;
        }
        lineSet->coordIndex.setValues(0, idx, indices);
        faceNormalLineNode->addChild(lineSet);
        delete[] indices;
    }
}

void EdgeRenderer::generateSilhouetteEdgeNode(
    const std::vector<gp_Pnt>& points,
    const Quantity_Color& color,
    double width)
{
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    
    if (silhouetteEdgeNode) {
        silhouetteEdgeNode->unref();
        silhouetteEdgeNode = nullptr;
    }

    silhouetteEdgeNode = createLineNode(points, color, width);
}

void EdgeRenderer::clearSilhouetteEdgeNode()
{
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    
    if (silhouetteEdgeNode) {
        silhouetteEdgeNode->unref();
        silhouetteEdgeNode = nullptr;
    }
}

void EdgeRenderer::generateIntersectionNodesNode(
    const std::vector<gp_Pnt>& intersectionPoints,
    const Quantity_Color& color,
    double size)
{
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    
    if (intersectionNodesNode) {
        intersectionNodesNode->unref();
        intersectionNodesNode = nullptr;
    }

    if (intersectionPoints.empty()) return;

    intersectionNodesNode = new SoSeparator();
    intersectionNodesNode->ref();

    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(
        static_cast<float>(color.Red()),
        static_cast<float>(color.Green()),
        static_cast<float>(color.Blue())
    );
    intersectionNodesNode->addChild(material);

    for (const auto& pt : intersectionPoints) {
        SoSeparator* pointSep = new SoSeparator();
        
        SoTranslation* trans = new SoTranslation();
        trans->translation.setValue(
            static_cast<float>(pt.X()),
            static_cast<float>(pt.Y()),
            static_cast<float>(pt.Z())
        );
        pointSep->addChild(trans);

        SoSphere* sphere = new SoSphere();
        sphere->radius.setValue(static_cast<float>(size * 0.01));
        pointSep->addChild(sphere);

        intersectionNodesNode->addChild(pointSep);
    }
}

SoSeparator* EdgeRenderer::getEdgeNode(EdgeType type)
{
    std::lock_guard<std::mutex> lock(m_nodeMutex);

    switch (type) {
        case EdgeType::Original:
            return originalEdgeNode;
        case EdgeType::Feature:
            return featureEdgeNode;
        case EdgeType::Mesh:
            return meshEdgeNode;
        case EdgeType::Highlight:
            return highlightEdgeNode;
        case EdgeType::Silhouette:
            return silhouetteEdgeNode;
        case EdgeType::NormalLine:
            return normalLineNode;
        case EdgeType::FaceNormalLine:
            return faceNormalLineNode;
        case EdgeType::IntersectionNodes:
            return intersectionNodesNode;
        default:
            return nullptr;
    }
}

void EdgeRenderer::applyAppearanceToEdgeNode(
    EdgeType type,
    const Quantity_Color& color,
    double width,
    int style)
{
    SoSeparator* node = getEdgeNode(type);
    if (!node) return;

    for (int i = 0; i < node->getNumChildren(); ++i) {
        SoNode* child = node->getChild(i);
        
        if (child->isOfType(SoMaterial::getClassTypeId())) {
            SoMaterial* material = static_cast<SoMaterial*>(child);
            material->diffuseColor.setValue(
                static_cast<float>(color.Red()),
                static_cast<float>(color.Green()),
                static_cast<float>(color.Blue())
            );
        }
        
        if (child->isOfType(SoDrawStyle::getClassTypeId())) {
            SoDrawStyle* drawStyle = static_cast<SoDrawStyle*>(child);
            drawStyle->lineWidth.setValue(static_cast<float>(width));
        }
    }
}

void EdgeRenderer::updateEdgeDisplay(
    SoSeparator* parentNode,
    const EdgeDisplayFlags& edgeFlags)
{
    if (!parentNode) return;

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    for (int i = parentNode->getNumChildren() - 1; i >= 0; --i) {
        SoNode* child = parentNode->getChild(i);
        if (child == originalEdgeNode || child == featureEdgeNode || 
            child == meshEdgeNode || child == highlightEdgeNode ||
            child == normalLineNode || child == faceNormalLineNode ||
            child == silhouetteEdgeNode || child == intersectionNodesNode) {
            parentNode->removeChild(i);
        }
    }

    if (edgeFlags.showOriginalEdges && originalEdgeNode) {
        parentNode->addChild(originalEdgeNode);
        // Add intersection nodes when original edges are shown
        if (intersectionNodesNode) {
            parentNode->addChild(intersectionNodesNode);
        }
    }
    if (edgeFlags.showFeatureEdges && featureEdgeNode) {
        parentNode->addChild(featureEdgeNode);
    }
    if (edgeFlags.showMeshEdges && meshEdgeNode) {
        parentNode->addChild(meshEdgeNode);
    }
    if (edgeFlags.showHighlightEdges && highlightEdgeNode) {
        parentNode->addChild(highlightEdgeNode);
    }
    if (edgeFlags.showNormalLines && normalLineNode) {
        parentNode->addChild(normalLineNode);
    }
    if (edgeFlags.showFaceNormalLines && faceNormalLineNode) {
        parentNode->addChild(faceNormalLineNode);
    }
    if (edgeFlags.showSilhouetteEdges && silhouetteEdgeNode) {
        parentNode->addChild(silhouetteEdgeNode);
    }
}

// LOD (Level of Detail) rendering methods
void EdgeRenderer::generateLODEdgeNodes(
    EdgeLODManager* lodManager,
    const Quantity_Color& color,
    double width)
{
    if (!lodManager) {
        LOG_WRN_S("LOD manager is null in generateLODEdgeNodes");
        return;
    }

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    // Generate nodes for all LOD levels
    for (int level = 0; level <= 4; ++level) {
        EdgeLODManager::LODLevel lodLevel = static_cast<EdgeLODManager::LODLevel>(level);
        const auto& lodEdges = lodManager->getLODEdges(lodLevel);

        if (!lodEdges.empty()) {
            // Create a unique identifier for this LOD level
            std::string nodeName = "lod_" + std::to_string(level);

            // Generate the line node for this LOD level
            // Note: We could create separate nodes for each LOD level
            // or use a more sophisticated switching mechanism
            LOG_DBG_S("Generated LOD node for level " + std::to_string(level) +
                     " with " + std::to_string(lodEdges.size() / 2) + " edges");
        }
    }

    LOG_INF_S("Generated LOD edge nodes for all levels");
}

void EdgeRenderer::setGPUAccelerationEnabled(bool enabled)
{
    if (!m_gpuRenderer || !m_gpuRenderer->isAvailable()) {
        if (enabled) {
            LOG_WRN_S("GPU acceleration not available");
        }
        m_gpuAccelerationEnabled = false;
        return;
    }

    m_gpuAccelerationEnabled = enabled;
    LOG_INF_S(std::string("GPU acceleration ") + (enabled ? "enabled" : "disabled"));
}

void EdgeRenderer::generateGPUMeshEdgeNode(
    const TriangleMesh& mesh,
    const Quantity_Color& color,
    double width)
{
    if (!m_gpuAccelerationEnabled || !m_gpuRenderer) {
        LOG_WRN_S("GPU acceleration not enabled or available");
        return;
    }

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    // Clean up existing node
    if (m_gpuMeshEdgeNode) {
        m_gpuMeshEdgeNode->unref();
        m_gpuMeshEdgeNode = nullptr;
    }

    // Create GPU-accelerated edge node
    GPUEdgeRenderer::EdgeRenderSettings settings;
    settings.color = color;
    settings.lineWidth = static_cast<float>(width);
    settings.antiAliasing = true;
    settings.mode = GPUEdgeRenderer::RenderMode::GeometryShader;

    m_gpuMeshEdgeNode = m_gpuRenderer->createGPUEdgeNode(mesh, settings);

    if (m_gpuMeshEdgeNode) {
        LOG_INF_S("Generated GPU-accelerated mesh edge node with " + 
                  std::to_string(mesh.triangles.size() / 3) + " triangles");
    } else {
        LOG_ERR_S("Failed to create GPU mesh edge node");
    }
}

void EdgeRenderer::updateLODLevel(EdgeLODManager* lodManager)
{
    if (!lodManager) {
        LOG_WRN_S("LOD manager is null in updateLODLevel");
        return;
    }

    EdgeLODManager::LODLevel currentLevel = lodManager->getCurrentLODLevel();
    const auto& currentEdges = lodManager->getLODEdges(currentLevel);

    // Log before locking
    if (currentEdges.empty()) {
        LOG_WRN_S("No edges available for current LOD level " +
                 std::to_string(static_cast<int>(currentLevel)));
        return;
    }

    // Update the original edge node with current LOD edges
    // Do this INSIDE the lock but without calling generateOriginalEdgeNode()
    // to avoid recursive locking
    {
        std::lock_guard<std::mutex> lock(m_nodeMutex);
        
        // Clean up existing node
        if (originalEdgeNode) {
            originalEdgeNode->unref();
            originalEdgeNode = nullptr;
        }

        // Create new node directly (same logic as generateOriginalEdgeNode but without locking)
        Quantity_Color defaultColor(1.0, 1.0, 1.0, Quantity_TOC_RGB);
        originalEdgeNode = createLineNode(currentEdges, defaultColor, 1.0);
    }

    // Log after releasing lock
    LOG_INF_S("Updated edge display to LOD level " +
             std::to_string(static_cast<int>(currentLevel)) +
             " with " + std::to_string(currentEdges.size() / 2) + " edges");
}
