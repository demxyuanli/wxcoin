#include "edges/EdgeRenderer.h"
#include "rendering/GeometryProcessor.h"
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
{
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
    int32_t* indices = new int32_t[points.size() + points.size() / 2];
    int idx = 0;
    for (size_t i = 0; i < points.size(); i += 2) {
        if (i + 1 < points.size()) {
            indices[idx++] = i;
            indices[idx++] = i + 1;
            indices[idx++] = -1;
        }
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
    for (const auto& tri : mesh.triangles) {
        if (tri.v1 >= mesh.vertices.size() || tri.v2 >= mesh.vertices.size() || tri.v3 >= mesh.vertices.size())
            continue;

        const gp_Pnt& p1 = mesh.vertices[tri.v1];
        const gp_Pnt& p2 = mesh.vertices[tri.v2];
        const gp_Pnt& p3 = mesh.vertices[tri.v3];

        gp_Pnt center((p1.X() + p2.X() + p3.X()) / 3.0,
                      (p1.Y() + p2.Y() + p3.Y()) / 3.0,
                      (p1.Z() + p2.Z() + p3.Z()) / 3.0);

        gp_Vec v1(p1, p2);
        gp_Vec v2(p1, p3);
        gp_Vec normal = v1.Crossed(v2);
        
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
    if (intersectionNodesNode) {
        parentNode->addChild(intersectionNodesNode);
    }
}
