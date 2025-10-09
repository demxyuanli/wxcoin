#include "edges/EdgeExtractor.h"
#include "logger/Logger.h"
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <gp_Vec.hxx>
#include <GeomAbs_CurveType.hxx>
#include <execution>
#include <mutex>
#include <atomic>

EdgeExtractor::EdgeExtractor()
{
}

struct EdgeData {
    TopoDS_Edge edge;
    Handle(Geom_Curve) curve;
    Standard_Real first, last;
    GeomAbs_CurveType curveType;
    bool isValid;
    bool passesLengthFilter;
    size_t pointCount;
    std::vector<gp_Pnt> sampledPoints;
};

std::vector<gp_Pnt> EdgeExtractor::extractOriginalEdges(
    const TopoDS_Shape& shape, 
    double samplingDensity, 
    double minLength, 
    bool showLinesOnly,
    std::vector<gp_Pnt>* intersectionPoints)
{
    std::vector<EdgeData> edgeData;
    std::vector<TopoDS_Edge> allEdges;

    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        allEdges.push_back(TopoDS::Edge(exp.Current()));
    }

    edgeData.reserve(allEdges.size());

    std::mutex edgeDataMutex;
    std::atomic<size_t> validEdges{0};

    std::for_each(std::execution::par, allEdges.begin(), allEdges.end(), [&](const TopoDS_Edge& edge) {
        EdgeData data;
        data.edge = edge;

        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);

        if (curve.IsNull()) {
            data.isValid = false;
        } else {
            data.curve = curve;
            data.first = first;
            data.last = last;

            BRepAdaptor_Curve adaptor(edge);
            data.curveType = adaptor.GetType();
            data.isValid = true;

            validEdges++;
        }

        std::lock_guard<std::mutex> lock(edgeDataMutex);
        edgeData.push_back(data);
    });

    std::atomic<size_t> edgesPassingFilter{0};

    std::for_each(std::execution::par, edgeData.begin(), edgeData.end(), [&](EdgeData& data) {
        if (!data.isValid) return;

        gp_Pnt startPoint = data.curve->Value(data.first);
        gp_Pnt endPoint = data.curve->Value(data.last);
        double edgeLength = startPoint.Distance(endPoint);

        if (edgeLength >= minLength) {
            data.passesLengthFilter = true;
            edgesPassingFilter++;
        }
    });

    std::atomic<size_t> totalPoints{0};

    std::for_each(std::execution::par, edgeData.begin(), edgeData.end(), [&](EdgeData& data) {
        if (!data.isValid || !data.passesLengthFilter) return;

        if (data.curveType == GeomAbs_Line || showLinesOnly) {
            data.sampledPoints.push_back(data.curve->Value(data.first));
            data.sampledPoints.push_back(data.curve->Value(data.last));
            data.pointCount = 2;
        } else {
            Standard_Real curveLength = data.last - data.first;
            int baseSamples = std::max(4, static_cast<int>(curveLength * samplingDensity * 0.5));

            switch (data.curveType) {
                case GeomAbs_Circle:
                case GeomAbs_Ellipse:
                    baseSamples = std::max(baseSamples, 16);
                    break;
                case GeomAbs_BSplineCurve:
                case GeomAbs_BezierCurve:
                    baseSamples = std::max(baseSamples, 12);
                    break;
                default:
                    break;
            }

            int numSamples = std::min(100, baseSamples);

            data.sampledPoints.reserve(numSamples + 1);
            for (int i = 0; i <= numSamples; ++i) {
                Standard_Real t = data.first + (data.last - data.first) * i / numSamples;
                data.sampledPoints.push_back(data.curve->Value(t));
            }
            data.pointCount = data.sampledPoints.size();
        }

        totalPoints += data.pointCount;
    });

    std::vector<gp_Pnt> points;
    points.reserve(totalPoints);

    for (const auto& data : edgeData) {
        if (data.isValid && data.passesLengthFilter) {
            points.insert(points.end(), data.sampledPoints.begin(), data.sampledPoints.end());
        }
    }

    if (intersectionPoints) {
        findEdgeIntersections(shape, *intersectionPoints);
    }

    return points;
}

std::vector<gp_Pnt> EdgeExtractor::extractFeatureEdges(
    const TopoDS_Shape& shape, 
    double featureAngle, 
    double minLength, 
    bool onlyConvex, 
    bool onlyConcave)
{
    std::vector<gp_Pnt> points;

    TopTools_IndexedDataMapOfShapeListOfShape edgeFaceMap;
    TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, edgeFaceMap);

    double angleThreshold = featureAngle * M_PI / 180.0;

    for (int i = 1; i <= edgeFaceMap.Extent(); ++i) {
        const TopoDS_Edge& edge = TopoDS::Edge(edgeFaceMap.FindKey(i));
        const TopTools_ListOfShape& faces = edgeFaceMap.FindFromIndex(i);

        if (faces.Extent() != 2) continue;

        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
        if (curve.IsNull()) continue;

        gp_Pnt p1 = curve->Value(first);
        gp_Pnt p2 = curve->Value(last);
        if (p1.Distance(p2) < minLength) continue;

        TopoDS_Face face1 = TopoDS::Face(faces.First());
        TopoDS_Face face2 = TopoDS::Face(faces.Last());

        Standard_Real u, v;
        gp_Pnt midPoint = curve->Value((first + last) / 2.0);

        BRepAdaptor_Surface surf1(face1);
        BRepAdaptor_Surface surf2(face2);

        gp_Vec normal1, normal2;
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
            continue;
        }

        if (normal1.Magnitude() < 1e-7 || normal2.Magnitude() < 1e-7) continue;

        normal1.Normalize();
        normal2.Normalize();

        double angle = normal1.Angle(normal2);
        double dotProduct = normal1.Dot(normal2);

        bool isFeatureEdge = false;
        if (angle >= angleThreshold) {
            if (onlyConvex && dotProduct > 0) {
                isFeatureEdge = true;
            } else if (onlyConcave && dotProduct < 0) {
                isFeatureEdge = true;
            } else if (!onlyConvex && !onlyConcave) {
                isFeatureEdge = true;
            }
        }

        if (isFeatureEdge) {
            BRepAdaptor_Curve adaptor(edge);
            int numSamples = std::max(10, static_cast<int>(adaptor.LastParameter() - adaptor.FirstParameter()) * 10);
            numSamples = std::min(numSamples, 50);

            for (int j = 0; j <= numSamples; ++j) {
                Standard_Real t = first + (last - first) * j / numSamples;
                points.push_back(curve->Value(t));
            }
        }
    }

    return points;
}

std::vector<gp_Pnt> EdgeExtractor::extractMeshEdges(const TriangleMesh& mesh)
{
    std::vector<gp_Pnt> points;

    for (const auto& tri : mesh.triangles) {
        if (tri.v1 < mesh.vertices.size() && tri.v2 < mesh.vertices.size() && tri.v3 < mesh.vertices.size()) {
            points.push_back(mesh.vertices[tri.v1]);
            points.push_back(mesh.vertices[tri.v2]);
            points.push_back(mesh.vertices[tri.v2]);
            points.push_back(mesh.vertices[tri.v3]);
            points.push_back(mesh.vertices[tri.v3]);
            points.push_back(mesh.vertices[tri.v1]);
        }
    }

    return points;
}

std::vector<gp_Pnt> EdgeExtractor::extractSilhouetteEdges(
    const TopoDS_Shape& shape, 
    const gp_Pnt& cameraPos)
{
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

        gp_Pnt midPoint = curve->Value((first + last) / 2.0);
        gp_Vec viewDir(midPoint, cameraPos);
        viewDir.Normalize();

        TopoDS_Face face1 = TopoDS::Face(faces.First());
        TopoDS_Face face2 = TopoDS::Face(faces.Last());

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
                normal1.Normalize();
            }

            GeomAPI_ProjectPointOnSurf proj2(midPoint, BRep_Tool::Surface(face2));
            if (proj2.NbPoints() > 0) {
                proj2.Parameters(1, u, v);
                gp_Pnt p; gp_Vec d1u, d1v;
                surf2.D1(u, v, p, d1u, d1v);
                normal2 = d1u.Crossed(d1v);
                if (face2.Orientation() == TopAbs_REVERSED) normal2.Reverse();
                normal2.Normalize();
            }
        } catch (...) {
            continue;
        }

        double dot1 = normal1.Dot(viewDir);
        double dot2 = normal2.Dot(viewDir);

        if ((dot1 > 0 && dot2 < 0) || (dot1 < 0 && dot2 > 0)) {
            int numSamples = 20;
            for (int j = 0; j <= numSamples; ++j) {
                Standard_Real t = first + (last - first) * j / numSamples;
                points.push_back(curve->Value(t));
            }
        }
    }

    return points;
}

void EdgeExtractor::findEdgeIntersections(
    const TopoDS_Shape& shape, 
    std::vector<gp_Pnt>& intersectionPoints)
{
    intersectionPoints.clear();
}

void EdgeExtractor::findEdgeIntersectionsFromEdges(
    const std::vector<TopoDS_Edge>& edges, 
    std::vector<gp_Pnt>& intersectionPoints)
{
}

void EdgeExtractor::findEdgeIntersectionsSimple(
    const std::vector<TopoDS_Edge>& edges, 
    std::vector<gp_Pnt>& intersectionPoints)
{
}

double EdgeExtractor::computeMinDistanceBetweenCurves(
    const struct EdgeData& data1, 
    const struct EdgeData& data2)
{
    return 0.0;
}

gp_Pnt EdgeExtractor::computeIntersectionPoint(
    const struct EdgeData& data1, 
    const struct EdgeData& data2)
{
    return gp_Pnt(0, 0, 0);
}
