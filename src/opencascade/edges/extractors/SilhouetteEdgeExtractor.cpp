#include "edges/extractors/SilhouetteEdgeExtractor.h"
#include "logger/Logger.h"
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <gp_Vec.hxx>

SilhouetteEdgeExtractor::SilhouetteEdgeExtractor() {}

bool SilhouetteEdgeExtractor::canExtract(const TopoDS_Shape& shape) const {
    // Can extract from shapes with faces
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        return true;
    }
    return false;
}

std::vector<gp_Pnt> SilhouetteEdgeExtractor::extractTyped(
    const TopoDS_Shape& shape, 
    const SilhouetteEdgeParams* params) {
    
    if (!params) {
        LOG_WRN_S("SilhouetteEdgeExtractor: No camera parameters provided");
        return {};
    }
    
    std::vector<gp_Pnt> points;
    
    TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
    TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);
    
    for (int i = 1; i <= edgeFaceMap.Extent(); ++i) {
        const TopoDS_Edge& edge = TopoDS::Edge(edgeFaceMap.FindKey(i));
        const TopTools_ListOfShape& faces = edgeFaceMap.FindFromIndex(i);
        
        if (faces.Extent() != 2) continue;
        
        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
        if (curve.IsNull()) continue;
        
        TopoDS_Face face1 = TopoDS::Face(faces.First());
        TopoDS_Face face2 = TopoDS::Face(faces.Last());
        
        if (isSilhouetteEdge(edge, face1, face2, params->cameraPosition, params->tolerance)) {
            int numSamples = 20;
            for (int j = 0; j <= numSamples; ++j) {
                Standard_Real t = first + (last - first) * j / numSamples;
                points.push_back(curve->Value(t));
            }
        }
    }
    
    return points;
}

bool SilhouetteEdgeExtractor::isSilhouetteEdge(
    const TopoDS_Edge& edge,
    const TopoDS_Face& face1,
    const TopoDS_Face& face2,
    const gp_Pnt& cameraPos,
    double tolerance) {
    
    Standard_Real first, last;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
    if (curve.IsNull()) return false;
    
    gp_Pnt midPoint = curve->Value((first + last) / 2.0);
    gp_Vec viewDir(midPoint, cameraPos);
    viewDir.Normalize();
    
    gp_Vec normal1 = calculateFaceNormal(face1, edge);
    gp_Vec normal2 = calculateFaceNormal(face2, edge);
    
    if (normal1.Magnitude() < 1e-7 || normal2.Magnitude() < 1e-7) return false;
    
    normal1.Normalize();
    normal2.Normalize();
    
    double dot1 = normal1.Dot(viewDir);
    double dot2 = normal2.Dot(viewDir);
    
    // Silhouette edge: one face visible, one not
    return (dot1 > tolerance && dot2 < -tolerance) || (dot1 < -tolerance && dot2 > tolerance);
}

gp_Vec SilhouetteEdgeExtractor::calculateFaceNormal(
    const TopoDS_Face& face,
    const TopoDS_Edge& edge) const {
    
    Standard_Real first, last;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
    if (curve.IsNull()) return gp_Vec(0, 0, 0);
    
    gp_Pnt midPoint = curve->Value((first + last) / 2.0);
    
    BRepAdaptor_Surface surf(face);
    gp_Vec normal(0, 0, 0);
    Standard_Real u, v;
    
    try {
        GeomAPI_ProjectPointOnSurf proj(midPoint, BRep_Tool::Surface(face));
        if (proj.NbPoints() > 0) {
            proj.Parameters(1, u, v);
            gp_Pnt p; gp_Vec d1u, d1v;
            surf.D1(u, v, p, d1u, d1v);
            normal = d1u.Crossed(d1v);
            if (face.Orientation() == TopAbs_REVERSED) normal.Reverse();
        }
    } catch (...) {
        return gp_Vec(0, 0, 0);
    }
    
    return normal;
}

