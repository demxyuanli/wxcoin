#include "DynamicSilhouetteRenderer.h"
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SoPath.h>
#include <opencascade/TopoDS.hxx>
#include <opencascade/TopExp_Explorer.hxx>
#include <opencascade/TopExp.hxx>
#include <opencascade/TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <opencascade/BRep_Tool.hxx>
#include <opencascade/BRepAdaptor_Surface.hxx>
#include <opencascade/GeomAPI_ProjectPointOnSurf.hxx>
#include <opencascade/gp_Vec.hxx>
#include "logger/Logger.h"
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/SoDB.h>
// #include <Inventor/elements/SoCameraElement.h>

DynamicSilhouetteRenderer::DynamicSilhouetteRenderer(SoSeparator* sceneRoot)
    : m_sceneRoot(sceneRoot)
    , m_enabled(false)
    , m_needsUpdate(true)
{
    // Create Coin3D nodes
    m_silhouetteNode = new SoSeparator;
    m_silhouetteNode->ref();
    
    m_material = new SoMaterial;
    m_material->diffuseColor.setValue(1.0, 1.0, 0.0); // Yellow
    m_material->ambientColor.setValue(1.0, 1.0, 0.0);
    m_material->emissiveColor.setValue(1.0, 1.0, 0.0);
    m_material->specularColor.setValue(1.0, 1.0, 0.0);
    
    m_drawStyle = new SoDrawStyle;
    m_drawStyle->lineWidth = 2.0;
    m_drawStyle->style = SoDrawStyle::LINES;
    
    m_coordinates = new SoCoordinate3;
    m_lineSet = new SoIndexedLineSet;
    
    m_renderCallback = new SoCallback;
    m_renderCallback->setCallback(renderCallback, this);
    
    // Add nodes to separator
    m_silhouetteNode->addChild(m_material);
    m_silhouetteNode->addChild(m_drawStyle);
    m_silhouetteNode->addChild(m_renderCallback);
    m_silhouetteNode->addChild(m_coordinates);
    m_silhouetteNode->addChild(m_lineSet);
}

DynamicSilhouetteRenderer::~DynamicSilhouetteRenderer() {
    if (m_silhouetteNode) {
        m_silhouetteNode->unref();
    }
}

void DynamicSilhouetteRenderer::setShape(const TopoDS_Shape& shape) {
    m_shape = shape;
    m_needsUpdate = true;
}

SoSeparator* DynamicSilhouetteRenderer::getSilhouetteNode() {
    return m_silhouetteNode;
}

void DynamicSilhouetteRenderer::updateSilhouettes(const gp_Pnt& cameraPos, const SbMatrix* modelMatrix) {
    if (!m_enabled) return;
    calculateSilhouettes(cameraPos, modelMatrix);
}

void DynamicSilhouetteRenderer::setEnabled(bool enabled) {
    m_enabled = enabled;
    if (enabled) {
        m_needsUpdate = true;
    }
}

bool DynamicSilhouetteRenderer::isEnabled() const {
    return m_enabled;
}

void DynamicSilhouetteRenderer::calculateSilhouettes(const gp_Pnt& cameraPos, const SbMatrix* modelMatrix) {
    m_silhouettePoints.clear();
    m_silhouetteIndices.clear();
    if (m_shape.IsNull()) return;
    int pointIndex = 0;
    int totalEdges = 0;
    int edgesWithTwoFaces = 0;
    int silhouetteCount = 0;
    TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
    TopExp::MapShapesAndAncestors(m_shape, TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);
    for (TopExp_Explorer exp(m_shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        totalEdges++;
        TopoDS_Edge edge = TopoDS::Edge(exp.Current());
        const TopTools_ListOfShape& faces = edgeFaceMap.FindFromKey(edge);
        if (faces.Extent() != 2) continue;
        edgesWithTwoFaces++;
        TopoDS_Face face1 = TopoDS::Face(faces.First());
        TopoDS_Face face2 = TopoDS::Face(faces.Last());
        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
        if (curve.IsNull()) continue;
        Standard_Real mid = (first + last) / 2.0;
        gp_Pnt midPnt = curve->Value(mid);
        gp_Vec n1 = getNormalAt(face1, midPnt);
        gp_Vec n2 = getNormalAt(face2, midPnt);
        gp_Vec view = midPnt.XYZ() - cameraPos.XYZ();
        if (view.Magnitude() < 1e-6) continue;
        view.Normalize();
        double dot1 = n1.Dot(view);
        double dot2 = n2.Dot(view);
        bool f1Front = dot1 > 0;
        bool f2Front = dot2 > 0;
        gp_Pnt p1 = curve->Value(first);
        gp_Pnt p2 = curve->Value(last);
        // 直接使用shape的原始坐标，不应用modelMatrix变换
        // 因为几何体已经通过setPosition设置了正确的世界坐标
        if (f1Front != f2Front) {
            m_silhouettePoints.push_back(p1);
            m_silhouettePoints.push_back(p2);
            m_silhouetteIndices.push_back(pointIndex++);
            m_silhouetteIndices.push_back(pointIndex++);
            m_silhouetteIndices.push_back(SO_END_LINE_INDEX);
            silhouetteCount++;
            LOG_INF_S("[SilhouetteDebug] silhouette edge: (" + std::to_string(p1.X()) + ", " + std::to_string(p1.Y()) + ", " + std::to_string(p1.Z()) + ") -> (" + std::to_string(p2.X()) + ", " + std::to_string(p2.Y()) + ", " + std::to_string(p2.Z()) + ")");
        }
    }
    m_coordinates->point.setNum(m_silhouettePoints.size());
    for (size_t i = 0; i < m_silhouettePoints.size(); ++i) {
        const gp_Pnt& p = m_silhouettePoints[i];
        m_coordinates->point.set1Value(i, p.X(), p.Y(), p.Z());
    }
    m_lineSet->coordIndex.setValues(0, m_silhouetteIndices.size(), m_silhouetteIndices.data());
}

gp_Vec DynamicSilhouetteRenderer::getNormalAt(const TopoDS_Face& face, const gp_Pnt& p) {
    BRepAdaptor_Surface surf(face, true);
    Standard_Real u, v;
    Handle(Geom_Surface) hSurf = BRep_Tool::Surface(face);
    GeomAPI_ProjectPointOnSurf projector(p, hSurf);
    projector.LowerDistanceParameters(u, v);
    
    gp_Pnt surfPnt;
    gp_Vec dU, dV;
    surf.D1(u, v, surfPnt, dU, dV);
    gp_Vec n = dU.Crossed(dV);
    n.Normalize();
    
    if (face.Orientation() == TopAbs_REVERSED) {
        n.Reverse();
    }
    
    return n;
}

SoCamera* findCameraRecursive(SoNode* node) {
    if (!node) return nullptr;
    SoCamera* cam = dynamic_cast<SoCamera*>(node);
    if (cam) return cam;
    SoGroup* group = dynamic_cast<SoGroup*>(node);
    if (group) {
        for (int i = 0; i < group->getNumChildren(); ++i) {
            SoCamera* found = findCameraRecursive(group->getChild(i));
            if (found) return found;
        }
    }
    return nullptr;
}

void DynamicSilhouetteRenderer::renderCallback(void* userData, SoAction* action) {
    LOG_INF_S("[SilhouetteDebug] renderCallback called");
    DynamicSilhouetteRenderer* renderer = static_cast<DynamicSilhouetteRenderer*>(userData);
    if (!renderer->m_enabled) return;
    gp_Pnt cameraPos(10.0, 10.0, 10.0);
    // 用保存的sceneRoot递归查找SoCamera
    if (renderer->m_sceneRoot) { 
        SoCamera* camera = findCameraRecursive(renderer->m_sceneRoot);
        if (camera) {
            SbVec3f pos = camera->position.getValue();
            cameraPos = gp_Pnt(pos[0], pos[1], pos[2]);
        }
    }
    LOG_INF_S("[SilhouetteDebug] cameraPos: (" + std::to_string(cameraPos.X()) + ", " + std::to_string(cameraPos.Y()) + ", " + std::to_string(cameraPos.Z()) + ")");
    SoState* state = action->getState();
    SbMatrix modelMatrix = SoModelMatrixElement::get(state);
    renderer->calculateSilhouettes(cameraPos, &modelMatrix);
} 
