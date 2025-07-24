#include "EdgeComponent.h"
#include "EdgeTypes.h"
#include "logger/Logger.h"
#include <TopoDS_Shape.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepTools.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <vector>
#include <cmath>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoMaterial.h>
#include <rendering/GeometryProcessor.h>

EdgeComponent::EdgeComponent() {

}

EdgeComponent::~EdgeComponent() {
    // 释放所有SoIndexedLineSet节点
    if (originalEdgeNode) originalEdgeNode->unref();
    if (featureEdgeNode) featureEdgeNode->unref();
    if (meshEdgeNode) meshEdgeNode->unref();
    if (highlightEdgeNode) highlightEdgeNode->unref();
    if (normalLineNode) normalLineNode->unref();
    if (faceNormalLineNode) faceNormalLineNode->unref();
}

void EdgeComponent::extractOriginalEdges(const TopoDS_Shape& shape) {
    std::vector<gp_Pnt> points;
    std::vector<int32_t> indices;
    int pointIndex = 0;
    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(exp.Current());
        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
        if (!curve.IsNull()) {
            gp_Pnt p1 = curve->Value(first);
            gp_Pnt p2 = curve->Value(last);
            points.push_back(p1);
            points.push_back(p2);
            indices.push_back(pointIndex++);
            indices.push_back(pointIndex++);
            indices.push_back(SO_END_LINE_INDEX); // -1
        }
    }
    
    if (originalEdgeNode) originalEdgeNode->unref();
    
    // Create material for original edges
    SoMaterial* mat = new SoMaterial;
    mat->diffuseColor.setValue(0, 0, 0); // Black
    
    // Create coordinates
    SoCoordinate3* coords = new SoCoordinate3;
    coords->point.setNum(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        coords->point.set1Value(i, points[i].X(), points[i].Y(), points[i].Z());
    }
    
    // Create line set
    SoIndexedLineSet* lineSet = new SoIndexedLineSet;
    lineSet->coordIndex.setValues(0, indices.size(), indices.data());
    
    // Create separator and add all components
    SoSeparator* sep = new SoSeparator;
    sep->addChild(mat);
    sep->addChild(coords);
    sep->addChild(lineSet);
    
    // Store the separator as originalEdgeNode for proper display management
    originalEdgeNode = sep;
    originalEdgeNode->ref();
}
void EdgeComponent::extractFeatureEdges(const TopoDS_Shape& shape, double featureAngle, double minLength, bool onlyConvex, bool onlyConcave) {
    std::vector<gp_Pnt> points;
    std::vector<int32_t> indices;
    int pointIndex = 0;
    double cosThreshold = std::cos(featureAngle * M_PI / 180.0);
    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(exp.Current());
        TopTools_ListOfShape faceList;
        for (TopExp_Explorer fexp(shape, TopAbs_FACE); fexp.More(); fexp.Next()) {
            TopoDS_Face face = TopoDS::Face(fexp.Current());
            for (TopExp_Explorer eexp(face, TopAbs_EDGE); eexp.More(); eexp.Next()) {
                if (edge.IsSame(eexp.Current())) {
                    faceList.Append(face);
                }
            }
        }
        if (faceList.Extent() == 2) {
            TopoDS_Face face1 = TopoDS::Face(faceList.First());
            TopoDS_Face face2 = TopoDS::Face(faceList.Last());
            Standard_Real first, last;
            Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
            if (curve.IsNull()) continue;
            Standard_Real mid = (first + last) / 2.0;
            gp_Pnt midPnt = curve->Value(mid);
            Standard_Real u1, v1, u2, v2;
            BRepTools::UVBounds(face1, u1, v1, u1, v1);
            BRepTools::UVBounds(face2, u2, v2, u2, v2);
            BRepAdaptor_Surface surf1(face1);
            BRepAdaptor_Surface surf2(face2);
            gp_Vec d1u1, d1v1, d1u2, d1v2;
            surf1.D1(u1, v1, midPnt, d1u1, d1v1);
            surf2.D1(u2, v2, midPnt, d1u2, d1v2);
            gp_Vec n1 = d1u1.Crossed(d1v1);
            gp_Vec n2 = d1u2.Crossed(d1v2);
            n1.Normalize();
            n2.Normalize();
            double cosAngle = n1.Dot(n2);
            if (cosAngle < cosThreshold) {
                gp_Pnt p1 = curve->Value(first);
                gp_Pnt p2 = curve->Value(last);
                if (p1.Distance(p2) < minLength) continue;
                gp_Vec edgeTangent(p1, p2);
                edgeTangent.Normalize();
                double cross = n1.Crossed(n2).Dot(edgeTangent);
                if (onlyConvex && cross <= 0) continue;
                if (onlyConcave && cross >= 0) continue;
                points.push_back(p1);
                points.push_back(p2);
                indices.push_back(pointIndex++);
                indices.push_back(pointIndex++);
                indices.push_back(SO_END_LINE_INDEX);
            }
        }
    }
    if (featureEdgeNode) featureEdgeNode->unref();
    
    // Create material for feature edges
    SoMaterial* mat = new SoMaterial;
    mat->diffuseColor.setValue(0, 0, 1); // Blue
    
    // Create coordinates
    SoCoordinate3* coords = new SoCoordinate3;
    coords->point.setNum(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        coords->point.set1Value(i, points[i].X(), points[i].Y(), points[i].Z());
    }
    
    // Create line set
    SoIndexedLineSet* lineSet = new SoIndexedLineSet;
    lineSet->coordIndex.setValues(0, indices.size(), indices.data());
    
    // Create separator and add all components
    SoSeparator* sep = new SoSeparator;
    sep->addChild(mat);
    sep->addChild(coords);
    sep->addChild(lineSet);
    
    // Store the separator as featureEdgeNode for proper display management
    featureEdgeNode = sep;
    featureEdgeNode->ref();
}
void EdgeComponent::extractMeshEdges(const TriangleMesh& mesh) {
    std::vector<gp_Pnt> points = mesh.vertices;
    std::vector<int32_t> indices;
    for (size_t i = 0; i + 2 < mesh.triangles.size(); i += 3) {
        int a = mesh.triangles[i];
        int b = mesh.triangles[i + 1];
        int c = mesh.triangles[i + 2];
        indices.push_back(a); indices.push_back(b); indices.push_back(SO_END_LINE_INDEX);
        indices.push_back(b); indices.push_back(c); indices.push_back(SO_END_LINE_INDEX);
        indices.push_back(c); indices.push_back(a); indices.push_back(SO_END_LINE_INDEX);
    }
    
    if (meshEdgeNode) meshEdgeNode->unref();
    
    // Create material for mesh edges
    SoMaterial* mat = new SoMaterial;
    mat->diffuseColor.setValue(0, 1, 0); // Green
    
    // Create coordinates
    SoCoordinate3* coords = new SoCoordinate3;
    coords->point.setNum(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        coords->point.set1Value(i, points[i].X(), points[i].Y(), points[i].Z());
    }
    
    // Create line set
    SoIndexedLineSet* lineSet = new SoIndexedLineSet;
    lineSet->coordIndex.setValues(0, indices.size(), indices.data());
    
    // Create separator and add all components
    SoSeparator* sep = new SoSeparator;
    sep->addChild(mat);
    sep->addChild(coords);
    sep->addChild(lineSet);
    
    // Store the separator as meshEdgeNode for proper display management
    meshEdgeNode = sep;
    meshEdgeNode->ref();
}
void EdgeComponent::generateAllEdgeNodes() {
	if (originalEdgeNode) originalEdgeNode->unref();
	if (featureEdgeNode) featureEdgeNode->unref();
	if (meshEdgeNode) meshEdgeNode->unref();
	if (highlightEdgeNode) highlightEdgeNode->unref();
	if (normalLineNode) normalLineNode->unref();
	
	// Initialize all edge nodes as null - they will be created when needed
	originalEdgeNode = nullptr; 
	featureEdgeNode = nullptr; 
	meshEdgeNode = nullptr; 
	highlightEdgeNode = nullptr; 
	normalLineNode = nullptr; 
}
SoSeparator* EdgeComponent::getEdgeNode(EdgeType type) {
    switch(type) {
        case EdgeType::Original: return originalEdgeNode;
        case EdgeType::Feature: return featureEdgeNode;
        case EdgeType::Mesh: return meshEdgeNode;
        case EdgeType::Highlight: return highlightEdgeNode;
        case EdgeType::NormalLine: return normalLineNode;
        case EdgeType::FaceNormalLine: return faceNormalLineNode;
    }
    return nullptr;
}
void EdgeComponent::setEdgeDisplayType(EdgeType type, bool show) {
    switch(type) {
        case EdgeType::Original: edgeFlags.showOriginalEdges = show; break;
        case EdgeType::Feature: edgeFlags.showFeatureEdges = show; break;
        case EdgeType::Mesh: edgeFlags.showMeshEdges = show; break;
        case EdgeType::Highlight: edgeFlags.showHighlightEdges = show; break;
        case EdgeType::NormalLine: edgeFlags.showNormalLines = show; break;
        case EdgeType::FaceNormalLine: edgeFlags.showFaceNormalLines = show; break;
    }
}
bool EdgeComponent::isEdgeDisplayTypeEnabled(EdgeType type) const {
    switch(type) {
        case EdgeType::Original: return edgeFlags.showOriginalEdges;
        case EdgeType::Feature: return edgeFlags.showFeatureEdges;
        case EdgeType::Mesh: return edgeFlags.showMeshEdges;
        case EdgeType::Highlight: return edgeFlags.showHighlightEdges;
        case EdgeType::NormalLine: return edgeFlags.showNormalLines;
        case EdgeType::FaceNormalLine: return edgeFlags.showFaceNormalLines;
    }
    return false;
}
void EdgeComponent::updateEdgeDisplay(SoSeparator* parentNode) {
    if (edgeFlags.showOriginalEdges && originalEdgeNode) {
        if (parentNode->findChild(originalEdgeNode) < 0) {
            parentNode->addChild(originalEdgeNode);
        }
    } else if (originalEdgeNode) {
        int idx = parentNode->findChild(originalEdgeNode);
        if (idx >= 0) parentNode->removeChild(idx);
    }
    if (edgeFlags.showFeatureEdges && featureEdgeNode) {
        if (parentNode->findChild(featureEdgeNode) < 0) {
            parentNode->addChild(featureEdgeNode);
        }
    } else if (featureEdgeNode) {
        int idx = parentNode->findChild(featureEdgeNode);
        if (idx >= 0) parentNode->removeChild(idx);
    }
    if (edgeFlags.showMeshEdges && meshEdgeNode) {
        if (parentNode->findChild(meshEdgeNode) < 0) {
            parentNode->addChild(meshEdgeNode);
        }
    } else if (meshEdgeNode) {
        int idx = parentNode->findChild(meshEdgeNode);
        if (idx >= 0) parentNode->removeChild(idx);
    }
    if (edgeFlags.showHighlightEdges && highlightEdgeNode) {
        if (parentNode->findChild(highlightEdgeNode) < 0) {
            parentNode->addChild(highlightEdgeNode);
        }
    } else if (highlightEdgeNode) {
        int idx = parentNode->findChild(highlightEdgeNode);
        if (idx >= 0) parentNode->removeChild(idx);
    }
    if (edgeFlags.showNormalLines && normalLineNode) {
        if (parentNode->findChild(normalLineNode) < 0) {
            parentNode->addChild(normalLineNode);
            LOG_INF_S("Added normal line node to parent");
        }
    } else if (normalLineNode) {
        int idx = parentNode->findChild(normalLineNode);
        if (idx >= 0) {
            parentNode->removeChild(idx);
            LOG_INF_S("Removed normal line node from parent");
        }
    } else if (edgeFlags.showNormalLines) {
        LOG_WRN_S("Normal lines enabled but normalLineNode is null");
    }
    
    if (edgeFlags.showFaceNormalLines && faceNormalLineNode) {
        if (parentNode->findChild(faceNormalLineNode) < 0) {
            parentNode->addChild(faceNormalLineNode);
            LOG_INF_S("Added face normal line node to parent");
        }
    } else if (faceNormalLineNode) {
        int idx = parentNode->findChild(faceNormalLineNode);
        if (idx >= 0) {
            parentNode->removeChild(idx);
            LOG_INF_S("Removed face normal line node from parent");
        }
    } else if (edgeFlags.showFaceNormalLines) {
        LOG_WRN_S("Face normal lines enabled but faceNormalLineNode is null");
    }
}
void EdgeComponent::generateHighlightEdgeNode() {
    // Generate highlight edge node for selected edges/faces
    // TODO: Implement highlight edge node generation based on selection state
    if (highlightEdgeNode) highlightEdgeNode->unref();
    
    // Create material for highlight edges
    SoMaterial* mat = new SoMaterial;
    mat->diffuseColor.setValue(1, 0, 0); // Red
    
    // Create empty line set for now
    SoIndexedLineSet* lineSet = new SoIndexedLineSet;
    
    // Create separator and add all components
    SoSeparator* sep = new SoSeparator;
    sep->addChild(mat);
    sep->addChild(lineSet);
    
    // Store the separator as highlightEdgeNode for proper display management
    highlightEdgeNode = sep;
    highlightEdgeNode->ref();
}
void EdgeComponent::generateNormalLineNode(const TriangleMesh& mesh, double length) {
    LOG_INF_S("Generating normal line node with " + std::to_string(mesh.vertices.size()) + 
              " vertices and " + std::to_string(mesh.normals.size()) + " normals");
    
    std::vector<gp_Pnt> points;
    std::vector<int32_t> indices;
    int pointIndex = 0;
    int normalCount = 0;
    
    for (size_t i = 0; i < mesh.vertices.size() && i < mesh.normals.size(); ++i) {
        const gp_Pnt& v = mesh.vertices[i];
        const gp_Vec& n = mesh.normals[i];
        
        // Check if normal is valid (not zero)
        if (n.Magnitude() > 0.001) {
            gp_Pnt p2(v.X() + n.X() * length, v.Y() + n.Y() * length, v.Z() + n.Z() * length);
            points.push_back(v);
            points.push_back(p2);
            indices.push_back(pointIndex++);
            indices.push_back(pointIndex++);
            indices.push_back(SO_END_LINE_INDEX);
            normalCount++;
        }
    }
    
    LOG_INF_S("Generated " + std::to_string(normalCount) + " normal lines from " + 
              std::to_string(mesh.vertices.size()) + " vertices");
    
    if (normalLineNode) normalLineNode->unref();
    
    // Create material for normal lines
    SoMaterial* mat = new SoMaterial;
    mat->diffuseColor.setValue(0.5, 0, 0.5); // Purple
    
    // Create coordinates
    SoCoordinate3* coords = new SoCoordinate3;
    coords->point.setNum(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        coords->point.set1Value(i, points[i].X(), points[i].Y(), points[i].Z());
    }
    
    // Create line set
    SoIndexedLineSet* lineSet = new SoIndexedLineSet;
    lineSet->coordIndex.setValues(0, indices.size(), indices.data());
    
    // Create separator and add all components
    normalLineNode = new SoSeparator;
    normalLineNode->ref();
    normalLineNode->addChild(mat);
    normalLineNode->addChild(coords);
    normalLineNode->addChild(lineSet);
}

void EdgeComponent::generateFaceNormalLineNode(const TriangleMesh& mesh, double length) {
    LOG_INF_S("Generating face normal line node with " + std::to_string(mesh.vertices.size()) + 
              " vertices and " + std::to_string(mesh.triangles.size() / 3) + " triangles");
    
    std::vector<gp_Pnt> points;
    std::vector<int32_t> indices;
    int pointIndex = 0;
    int faceNormalCount = 0;
    
    // Calculate face normals for each triangle
    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
        if (i + 2 >= mesh.triangles.size()) break;
        
        int idx1 = mesh.triangles[i];
        int idx2 = mesh.triangles[i + 1];
        int idx3 = mesh.triangles[i + 2];
        
        if (idx1 >= 0 && idx1 < static_cast<int>(mesh.vertices.size()) &&
            idx2 >= 0 && idx2 < static_cast<int>(mesh.vertices.size()) &&
            idx3 >= 0 && idx3 < static_cast<int>(mesh.vertices.size())) {
            
            const gp_Pnt& p1 = mesh.vertices[idx1];
            const gp_Pnt& p2 = mesh.vertices[idx2];
            const gp_Pnt& p3 = mesh.vertices[idx3];
            
            // Calculate face center
            gp_Pnt faceCenter(
                (p1.X() + p2.X() + p3.X()) / 3.0,
                (p1.Y() + p2.Y() + p3.Y()) / 3.0,
                (p1.Z() + p2.Z() + p3.Z()) / 3.0
            );
            
            // Calculate face normal
            gp_Vec v1(p1, p2);
            gp_Vec v2(p1, p3);
            gp_Vec faceNormal = v1.Crossed(v2);
            
            // Normalize the face normal
            double normalLength = faceNormal.Magnitude();
            if (normalLength > 0.001) {
                faceNormal.Scale(1.0 / normalLength);
                
                // Create line from face center to normal direction
                gp_Pnt normalEnd(
                    faceCenter.X() + faceNormal.X() * length,
                    faceCenter.Y() + faceNormal.Y() * length,
                    faceCenter.Z() + faceNormal.Z() * length
                );
                
                points.push_back(faceCenter);
                points.push_back(normalEnd);
                indices.push_back(pointIndex++);
                indices.push_back(pointIndex++);
                indices.push_back(SO_END_LINE_INDEX);
                faceNormalCount++;
            }
        }
    }
    
    LOG_INF_S("Generated " + std::to_string(faceNormalCount) + " face normal lines from " + 
              std::to_string(mesh.triangles.size() / 3) + " triangles");
    
    if (faceNormalLineNode) faceNormalLineNode->unref();
    
    // Create material for face normal lines
    SoMaterial* mat = new SoMaterial;
    mat->diffuseColor.setValue(0, 0.5, 0.5); // Cyan
    
    // Create coordinates
    SoCoordinate3* coords = new SoCoordinate3;
    coords->point.setNum(points.size());
    for (size_t i = 0; i < points.size(); ++i) {
        coords->point.set1Value(i, points[i].X(), points[i].Y(), points[i].Z());
    }
    
    // Create line set
    SoIndexedLineSet* lineSet = new SoIndexedLineSet;
    lineSet->coordIndex.setValues(0, indices.size(), indices.data());
    
    // Create separator and add all components
    faceNormalLineNode = new SoSeparator;
    faceNormalLineNode->ref();
    faceNormalLineNode->addChild(mat);
    faceNormalLineNode->addChild(coords);
    faceNormalLineNode->addChild(lineSet);
} 