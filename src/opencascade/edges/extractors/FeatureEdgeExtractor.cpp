#include "edges/extractors/FeatureEdgeExtractor.h"
#include "logger/Logger.h"
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <gp_Vec.hxx>

FeatureEdgeExtractor::FeatureEdgeExtractor() {}

bool FeatureEdgeExtractor::canExtract(const TopoDS_Shape& shape) const {
    // Can extract from shapes with faces
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        return true;
    }
    return false;
}

std::vector<gp_Pnt> FeatureEdgeExtractor::extractTyped(
    const TopoDS_Shape& shape, 
    const FeatureEdgeParams* params) {
    
    FeatureEdgeParams defaultParams;
    const FeatureEdgeParams& p = params ? *params : defaultParams;
    
    std::vector<gp_Pnt> points;
    
    TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
    TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);
    
    double angleThreshold = p.featureAngle * M_PI / 180.0;
    
    for (int i = 1; i <= edgeFaceMap.Extent(); ++i) {
        const TopoDS_Edge& edge = TopoDS::Edge(edgeFaceMap.FindKey(i));
        const TopTools_ListOfShape& faces = edgeFaceMap.FindFromIndex(i);
        
        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
        if (curve.IsNull()) continue;
        
        // Check length filter
        BRepAdaptor_Curve adaptor(edge);
        bool isClosed = (edge.Closed() || adaptor.IsClosed());
        
        if (!isClosed) {
            gp_Pnt p1 = curve->Value(first);
            gp_Pnt p2 = curve->Value(last);
            if (p1.Distance(p2) < p.minLength) continue;
        }
        
        bool isFeature = false;
        
        // Boundary edges (only one face) are always features
        if (faces.Extent() == 1) {
            isFeature = true;
        }
        // Edges between two faces - check angle
        else if (faces.Extent() == 2) {
            TopoDS_Face face1 = TopoDS::Face(faces.First());
            TopoDS_Face face2 = TopoDS::Face(faces.Last());

            double angle = calculateFaceAngle(edge, face1, face2);

            // Skip if normals couldn't be calculated (angle = 0.0)
            if (angle < 1e-10) continue;

            if (angle >= angleThreshold) {
                // Calculate normals to determine convex/concave
                gp_Pnt midPoint = curve->Value((first + last) / 2.0);
                
                BRepAdaptor_Surface surf1(face1);
                BRepAdaptor_Surface surf2(face2);
                
                gp_Vec normal1, normal2;
                Standard_Real u, v;
                
                try {
                    // Skip expensive normal calculations if we don't need to check convex/concave
                    if (!p.onlyConvex && !p.onlyConcave) {
                        isFeature = true;
                    } else {
                        // Only compute normals when needed for convex/concave filtering
                        GeomAPI_ProjectPointOnSurf proj1(midPoint, BRep_Tool::Surface(face1));
                        if (proj1.NbPoints() > 0) {
                            proj1.Parameters(1, u, v);
                            gp_Pnt pt; gp_Vec d1u, d1v;
                            surf1.D1(u, v, pt, d1u, d1v);
                            normal1 = d1u.Crossed(d1v);
                            if (face1.Orientation() == TopAbs_REVERSED) normal1.Reverse();
                            normal1.Normalize();
                        }

                        GeomAPI_ProjectPointOnSurf proj2(midPoint, BRep_Tool::Surface(face2));
                        if (proj2.NbPoints() > 0) {
                            proj2.Parameters(1, u, v);
                            gp_Pnt pt; gp_Vec d1u, d1v;
                            surf2.D1(u, v, pt, d1u, d1v);
                            normal2 = d1u.Crossed(d1v);
                            if (face2.Orientation() == TopAbs_REVERSED) normal2.Reverse();
                            normal2.Normalize();
                        }

                        double dotProduct = normal1.Dot(normal2);

                        if (p.onlyConvex && dotProduct > 0) {
                            isFeature = true;
                        } else if (p.onlyConcave && dotProduct < 0) {
                            isFeature = true;
                        }
                    }
                } catch (...) {
                    continue;
                }
            }
        }
        
        if (isFeature) {
            int numSamples = std::max(10, static_cast<int>(adaptor.LastParameter() - adaptor.FirstParameter()) * 10);
            numSamples = std::min(numSamples, 50);
            
            std::vector<gp_Pnt> edgePoints;
            for (int j = 0; j <= numSamples; ++j) {
                Standard_Real t = first + (last - first) * j / numSamples;
                edgePoints.push_back(curve->Value(t));
            }
            
            // Convert to line segments
            for (size_t j = 0; j + 1 < edgePoints.size(); ++j) {
                points.push_back(edgePoints[j]);
                points.push_back(edgePoints[j + 1]);
            }
        }
    }
    
    return points;
}

bool FeatureEdgeExtractor::isFeatureEdge(
    const TopoDS_Edge& edge,
    const TopTools_ListOfShape& faces,
    double angleThreshold,
    bool onlyConvex,
    bool onlyConcave) const {
    
    if (faces.Extent() == 1) return true;
    if (faces.Extent() != 2) return false;
    
    const TopoDS_Face& face1 = TopoDS::Face(faces.First());
    const TopoDS_Face& face2 = TopoDS::Face(faces.Last());
    
    double angle = calculateFaceAngle(edge, face1, face2);
    return angle >= angleThreshold;
}

double FeatureEdgeExtractor::calculateFaceAngle(
    const TopoDS_Edge& edge,
    const TopoDS_Face& face1,
    const TopoDS_Face& face2) const {
    
    Standard_Real first, last;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
    if (curve.IsNull()) return 0.0;
    
    gp_Pnt midPoint = curve->Value((first + last) / 2.0);
    
    BRepAdaptor_Surface surf1(face1);
    BRepAdaptor_Surface surf2(face2);
    
    gp_Vec normal1, normal2;
    Standard_Real u, v;
    
    try {
        GeomAPI_ProjectPointOnSurf proj1(midPoint, BRep_Tool::Surface(face1));
        if (proj1.NbPoints() > 0) {
            proj1.Parameters(1, u, v);
            gp_Pnt p; gp_Vec d1u, d1v;
            surf1.D1(u, v, p, d1u, d1v);
            normal1 = d1u.Crossed(d1v);
            if (face1.Orientation() == TopAbs_REVERSED) normal1.Reverse();
        }
        
        GeomAPI_ProjectPointOnSurf proj2(midPoint, BRep_Tool::Surface(face2));
        if (proj2.NbPoints() > 0) {
            proj2.Parameters(1, u, v);
            gp_Pnt p; gp_Vec d1u, d1v;
            surf2.D1(u, v, p, d1u, d1v);
            normal2 = d1u.Crossed(d1v);
            if (face2.Orientation() == TopAbs_REVERSED) normal2.Reverse();
        }
    } catch (...) {
        return 0.0;
    }
    
    if (normal1.Magnitude() < 1e-7 || normal2.Magnitude() < 1e-7) return 0.0;
    
    normal1.Normalize();
    normal2.Normalize();
    
    return normal1.Angle(normal2);
}

