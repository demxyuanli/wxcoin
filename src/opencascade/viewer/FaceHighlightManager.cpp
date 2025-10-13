#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "viewer/FaceHighlightManager.h"
#include "SceneManager.h"
#include "Canvas.h"
#include "viewer/PickingService.h"
#include "OCCGeometry.h"
#include "logger/Logger.h"

#include <opencascade/TopoDS.hxx>
#include <opencascade/TopExp_Explorer.hxx>
#include <opencascade/BRep_Tool.hxx>
#include <opencascade/BRepAdaptor_Surface.hxx>
#include <opencascade/Poly_Triangulation.hxx>
#include <opencascade/BRepMesh_IncrementalMesh.hxx>
#include <opencascade/TopLoc_Location.hxx>

FaceHighlightManager::FaceHighlightManager(SceneManager* sceneManager,
                                           SoSeparator* occRoot,
                                           PickingService* pickingService)
    : m_sceneManager(sceneManager)
    , m_occRoot(occRoot)
    , m_pickingService(pickingService)
    , m_enabled(true)
    , m_lastFaceId(-1)
    , m_highlightAttached(false)
{
    m_highlightNode = new SoSeparator;
    m_highlightNode->ref();
    
    m_material = new SoMaterial;
    m_material->diffuseColor.setValue(1.0f, 0.6f, 0.0f);
    m_material->ambientColor.setValue(1.0f, 0.6f, 0.0f);
    m_material->emissiveColor.setValue(0.8f, 0.4f, 0.0f);
    m_material->transparency = 0.3f;
    
    m_drawStyle = new SoDrawStyle;
    m_drawStyle->lineWidth = 3.0f;
    m_drawStyle->style = SoDrawStyle::LINES;
    
    m_coordinates = new SoCoordinate3;
    m_lineSet = new SoIndexedLineSet;
    
    m_highlightNode->addChild(m_material);
    m_highlightNode->addChild(m_drawStyle);
    m_highlightNode->addChild(m_coordinates);
    m_highlightNode->addChild(m_lineSet);
}

FaceHighlightManager::~FaceHighlightManager() {
    detachHighlightFromScene();
    if (m_highlightNode) {
        m_highlightNode->unref();
    }
}

void FaceHighlightManager::setEnabled(bool enabled) {
    m_enabled = enabled;
    if (!enabled) {
        clearHighlight();
    }
}

void FaceHighlightManager::setHighlightColor(float r, float g, float b) {
    if (m_material) {
        m_material->diffuseColor.setValue(r, g, b);
        m_material->ambientColor.setValue(r, g, b);
        m_material->emissiveColor.setValue(r * 0.8f, g * 0.8f, b * 0.8f);
    }
}

void FaceHighlightManager::setHighlightLineWidth(float width) {
    if (m_drawStyle) {
        m_drawStyle->lineWidth = width;
    }
}

void FaceHighlightManager::updateHoverHighlightAt(const wxPoint& screenPos) {
    if (!m_enabled || !m_pickingService) {
        return;
    }
    
    // Invalid screen position - clear highlight
    if (screenPos.x < 0 || screenPos.y < 0) {
        clearHighlight();
        return;
    }
    
    auto result = m_pickingService->pickDetailedAtScreen(screenPos);
    
    if (!result.geometry || result.geometryFaceId < 0) {
        clearHighlight();
        return;
    }
    
    auto currentGeometry = m_lastGeometry.lock();
    if (currentGeometry.get() == result.geometry.get() && 
        m_lastFaceId == result.geometryFaceId) {
        return;
    }
    
    m_lastGeometry = result.geometry;
    m_lastFaceId = result.geometryFaceId;
    
    highlightFace(result.geometry, result.geometryFaceId);
}

void FaceHighlightManager::highlightFace(std::shared_ptr<OCCGeometry> geometry, int faceId) {
    if (!geometry) {
        clearHighlight();
        return;
    }
    
    TopoDS_Shape shape = geometry->getShape();
    if (shape.IsNull()) {
        LOG_WRN_S("FaceHighlightManager: Shape is null");
        clearHighlight();
        return;
    }
    
    int currentFaceIndex = 0;
    TopoDS_Face targetFace;
    bool found = false;
    
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        if (currentFaceIndex == faceId) {
            targetFace = TopoDS::Face(exp.Current());
            found = true;
            break;
        }
        currentFaceIndex++;
    }
    
    if (!found || targetFace.IsNull()) {
        LOG_WRN_S("FaceHighlightManager: Face not found, faceId=" + std::to_string(faceId));
        clearHighlight();
        return;
    }
    
    buildFaceHighlight(targetFace);
    attachHighlightToScene();
    
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        m_sceneManager->getCanvas()->Refresh(false);
    }
}

void FaceHighlightManager::buildFaceHighlight(const TopoDS_Face& face) {
    TopLoc_Location loc;
    Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, loc);
    
    if (triangulation.IsNull()) {
        BRepMesh_IncrementalMesh mesh(face, 0.1);
        triangulation = BRep_Tool::Triangulation(face, loc);
    }
    
    if (triangulation.IsNull()) {
        LOG_WRN_S("FaceHighlightManager: Failed to get triangulation for face");
        return;
    }
    
    gp_Trsf transform = loc.Transformation();
    
    std::vector<SbVec3f> points;
    std::vector<int32_t> indices;
    
    for (int i = 1; i <= triangulation->NbNodes(); i++) {
        gp_Pnt p = triangulation->Node(i);
        p.Transform(transform);
        points.push_back(SbVec3f(static_cast<float>(p.X()), 
                                static_cast<float>(p.Y()), 
                                static_cast<float>(p.Z())));
    }
    
    for (int i = 1; i <= triangulation->NbTriangles(); i++) {
        const Poly_Triangle& triangle = triangulation->Triangle(i);
        int n1, n2, n3;
        triangle.Get(n1, n2, n3);
        
        indices.push_back(n1 - 1);
        indices.push_back(n2 - 1);
        indices.push_back(-1);
        
        indices.push_back(n2 - 1);
        indices.push_back(n3 - 1);
        indices.push_back(-1);
        
        indices.push_back(n3 - 1);
        indices.push_back(n1 - 1);
        indices.push_back(-1);
    }
    
    m_coordinates->point.setNum(static_cast<int>(points.size()));
    for (size_t i = 0; i < points.size(); i++) {
        m_coordinates->point.set1Value(static_cast<int>(i), points[i]);
    }
    
    m_lineSet->coordIndex.setValues(0, static_cast<int>(indices.size()), indices.data());
}

void FaceHighlightManager::attachHighlightToScene() {
    if (!m_highlightAttached && m_occRoot && m_highlightNode) {
        bool alreadyChild = false;
        for (int i = 0; i < m_occRoot->getNumChildren(); ++i) {
            if (m_occRoot->getChild(i) == m_highlightNode) {
                alreadyChild = true;
                break;
            }
        }
        if (!alreadyChild) {
            m_occRoot->addChild(m_highlightNode);
            m_highlightAttached = true;
        }
    }
}

void FaceHighlightManager::detachHighlightFromScene() {
    if (m_highlightAttached && m_occRoot && m_highlightNode) {
        int index = m_occRoot->findChild(m_highlightNode);
        if (index >= 0) {
            m_occRoot->removeChild(index);
        }
        m_highlightAttached = false;
    }
}

void FaceHighlightManager::clearHighlight() {
    detachHighlightFromScene();
    m_lastFaceId = -1;
    m_lastGeometry.reset();
    
    if (m_sceneManager && m_sceneManager->getCanvas()) {
        m_sceneManager->getCanvas()->Refresh(false);
    }
}
