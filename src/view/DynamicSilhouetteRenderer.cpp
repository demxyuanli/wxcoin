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

DynamicSilhouetteRenderer::DynamicSilhouetteRenderer(SoSeparator* sceneRoot)
    : m_sceneRoot(sceneRoot)
    , m_enabled(false)
    , m_needsUpdate(true)
{
    m_silhouetteNode = new SoSeparator;
    m_silhouetteNode->ref();
    
    m_material = new SoMaterial;
    m_material->diffuseColor.setValue(1.0, 1.0, 0.0);
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
    auto now = std::chrono::steady_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastUpdateTs).count();

    if (m_fastMode) {
        if (m_needsUpdate || m_cachedBoundaryPoints.empty()) {
            buildBoundaryOnlyCache();
            m_needsUpdate = false;
        }
        m_coordinates->point.setNum(static_cast<int>(m_cachedBoundaryPoints.size()));
        for (size_t i = 0; i < m_cachedBoundaryPoints.size(); ++i) {
            const gp_Pnt& p = m_cachedBoundaryPoints[i];
            m_coordinates->point.set1Value(static_cast<int>(i), static_cast<float>(p.X()), static_cast<float>(p.Y()), static_cast<float>(p.Z()));
        }
        m_lineSet->coordIndex.setValues(0, static_cast<int>(m_cachedBoundaryIndices.size()), m_cachedBoundaryIndices.data());
        return;
    }

    double dx = cameraPos.X() - m_lastCameraPos.X();
    double dy = cameraPos.Y() - m_lastCameraPos.Y();
    double dz = cameraPos.Z() - m_lastCameraPos.Z();
    double moveDist2 = dx*dx + dy*dy + dz*dz;
    if (!m_needsUpdate && moveDist2 < (m_minCameraMove * m_minCameraMove) && elapsedMs < m_minUpdateIntervalMs) {
        return;
    }
    m_lastUpdateTs = now;
    m_lastCameraPos = cameraPos;

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

static inline gp_Pnt transformPoint(const gp_Pnt& p, const SbMatrix* m) {
    if (!m) return p;
    SbVec3f v(static_cast<float>(p.X()), static_cast<float>(p.Y()), static_cast<float>(p.Z()));
    SbVec3f out;
    m->multVecMatrix(v, out);
    return gp_Pnt(out[0], out[1], out[2]);
}

static inline gp_Vec transformVector(const gp_Vec& v, const SbMatrix* m) {
    if (!m) return v;
    float a[4][4];
    m->getValue(a);
    double x = v.X(), y = v.Y(), z = v.Z();
    double tx = a[0][0]*x + a[0][1]*y + a[0][2]*z;
    double ty = a[1][0]*x + a[1][1]*y + a[1][2]*z;
    double tz = a[2][0]*x + a[2][1]*y + a[2][2]*z;
    gp_Vec out(tx, ty, tz);
    if (out.Magnitude() > 1e-9) out.Normalize();
    return out;
}

void DynamicSilhouetteRenderer::calculateSilhouettes(const gp_Pnt& cameraPos, const SbMatrix* modelMatrix) {
    m_silhouettePoints.clear();
    m_silhouetteIndices.clear();
    if (m_shape.IsNull()) return;
    int pointIndex = 0;
    TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
    TopExp::MapShapesAndAncestors(m_shape, TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);
    for (TopExp_Explorer exp(m_shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(exp.Current());
        const TopTools_ListOfShape& faces = edgeFaceMap.FindFromKey(edge);
        if (faces.Extent() != 2) continue;
        TopoDS_Face face1 = TopoDS::Face(faces.First());
        TopoDS_Face face2 = TopoDS::Face(faces.Last());
        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
        if (curve.IsNull()) continue;
        const int sampleCount = 8;
        Standard_Real step = (last - first) / static_cast<Standard_Real>(sampleCount);
        auto normalAt = [&](const TopoDS_Face& f, const gp_Pnt& pObj) -> gp_Vec {
            gp_Vec n = getNormalAt(f, pObj);
            return transformVector(n, modelMatrix);
        };
        gp_Pnt prevObj = curve->Value(first);
        gp_Pnt prevW = transformPoint(prevObj, modelMatrix);
        gp_Vec n1Prev = normalAt(face1, prevObj);
        gp_Vec n2Prev = normalAt(face2, prevObj);
        gp_Vec viewPrev = gp_Vec(prevW.XYZ() - cameraPos.XYZ());
        if (viewPrev.Magnitude() > 1e-6) viewPrev.Normalize();
        bool f1FrontPrev = n1Prev.Dot(viewPrev) > 0.0;
        bool f2FrontPrev = n2Prev.Dot(viewPrev) > 0.0;

        for (int s = 1; s <= sampleCount; ++s) {
            Standard_Real t = (s == sampleCount) ? last : (first + step * s);
            gp_Pnt curObj = curve->Value(t);
            gp_Pnt curW = transformPoint(curObj, modelMatrix);
            gp_Vec n1Cur = normalAt(face1, curObj);
            gp_Vec n2Cur = normalAt(face2, curObj);
            gp_Vec viewCur = gp_Vec(curW.XYZ() - cameraPos.XYZ());
            if (viewCur.Magnitude() > 1e-6) viewCur.Normalize();
            bool f1FrontCur = n1Cur.Dot(viewCur) > 0.0;
            bool f2FrontCur = n2Cur.Dot(viewCur) > 0.0;

            bool prevOpp = (f1FrontPrev != f2FrontPrev);
            bool curOpp = (f1FrontCur != f2FrontCur);
            if (prevOpp || curOpp) {
                m_silhouettePoints.push_back(prevW);
                m_silhouettePoints.push_back(curW);
                m_silhouetteIndices.push_back(pointIndex++);
                m_silhouetteIndices.push_back(pointIndex++);
                m_silhouetteIndices.push_back(SO_END_LINE_INDEX);
            }

            prevObj = curObj;
            prevW = curW;
            n1Prev = n1Cur;
            n2Prev = n2Cur;
            f1FrontPrev = f1FrontCur;
            f2FrontPrev = f2FrontCur;
        }
    }
    m_coordinates->point.setNum(static_cast<int>(m_silhouettePoints.size()));
    for (size_t i = 0; i < m_silhouettePoints.size(); ++i) {
        const gp_Pnt& p = m_silhouettePoints[i];
        m_coordinates->point.set1Value(static_cast<int>(i), static_cast<float>(p.X()), static_cast<float>(p.Y()), static_cast<float>(p.Z()));
    }
    m_lineSet->coordIndex.setValues(0, static_cast<int>(m_silhouetteIndices.size()), m_silhouetteIndices.data());
}

void DynamicSilhouetteRenderer::buildBoundaryOnlyCache() {
    m_cachedBoundaryPoints.clear();
    m_cachedBoundaryIndices.clear();
    if (m_shape.IsNull()) return;

    TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
    TopExp::MapShapesAndAncestors(m_shape, TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);
    int pointIndex = 0;
    for (TopExp_Explorer exp(m_shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(exp.Current());
        const TopTools_ListOfShape& faces = edgeFaceMap.FindFromKey(edge);
        if (faces.Extent() != 1) continue;

        Standard_Real first = 0.0, last = 0.0;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
        if (curve.IsNull()) continue;
        gp_Pnt p1 = curve->Value(first);
        gp_Pnt p2 = curve->Value(last);
        m_cachedBoundaryPoints.push_back(p1);
        m_cachedBoundaryPoints.push_back(p2);
        m_cachedBoundaryIndices.push_back(pointIndex++);
        m_cachedBoundaryIndices.push_back(pointIndex++);
        m_cachedBoundaryIndices.push_back(SO_END_LINE_INDEX);
    }
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
    DynamicSilhouetteRenderer* renderer = static_cast<DynamicSilhouetteRenderer*>(userData);
    if (!renderer->m_enabled) return;
    gp_Pnt cameraPos(10.0, 10.0, 10.0);
    SoState* state = action->getState();
    SoNode* rootNode = nullptr;
    if (state) {
        rootNode = renderer->m_sceneRoot;
    }
    if (rootNode) {
        SoCamera* camera = findCameraRecursive(rootNode);
        if (camera) {
            SbVec3f pos = camera->position.getValue();
            cameraPos = gp_Pnt(pos[0], pos[1], pos[2]);
        }
    }
    SbMatrix modelMatrix = SoModelMatrixElement::get(state);
    renderer->calculateSilhouettes(cameraPos, &modelMatrix);
}
