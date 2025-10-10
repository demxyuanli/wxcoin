#include "edges/EdgeExtractor.h"
#include "edges/EdgeGeometryCache.h"
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
#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRepTools.hxx>
#include <execution>
#include <mutex>
#include <atomic>
#include <thread>
#include <limits>
#include <cmath>
#include <sstream>

EdgeExtractor::EdgeExtractor()
{
}

// Axis-Aligned Bounding Box structure
struct AABB {
    double minX, minY, minZ;
    double maxX, maxY, maxZ;

    AABB() : minX(0), minY(0), minZ(0), maxX(0), maxY(0), maxZ(0) {}

    bool intersects(const AABB& other) const {
        return !(maxX < other.minX || other.maxX < minX ||
                 maxY < other.minY || other.maxY < minY ||
                 maxZ < other.minZ || other.maxZ < minZ);
    }

    void expand(const gp_Pnt& point) {
        minX = std::min(minX, point.X());
        minY = std::min(minY, point.Y());
        minZ = std::min(minZ, point.Z());
        maxX = std::max(maxX, point.X());
        maxY = std::max(maxY, point.Y());
        maxZ = std::max(maxZ, point.Z());
    }

    void expand(double margin) {
        minX -= margin;
        minY -= margin;
        minZ -= margin;
        maxX += margin;
        maxY += margin;
        maxZ += margin;
    }
};

struct EdgeData {
    TopoDS_Edge edge;
    Handle(Geom_Curve) curve;
    Standard_Real first, last;
    GeomAbs_CurveType curveType;
    bool isValid;
    bool passesLengthFilter;
    size_t pointCount;
    std::vector<gp_Pnt> sampledPoints;
    AABB bbox;
    int gridX, gridY, gridZ;
};

std::vector<gp_Pnt> EdgeExtractor::extractOriginalEdges(
    const TopoDS_Shape& shape, 
    double samplingDensity, 
    double minLength, 
    bool showLinesOnly,
    std::vector<gp_Pnt>* intersectionPoints)
{
    // Try to use cache if no intersection points are requested
    if (intersectionPoints == nullptr) {
        // Generate cache key based on shape pointer and parameters
        std::ostringstream keyStream;
        keyStream << "original_" 
                  << reinterpret_cast<uintptr_t>(&shape.TShape()) << "_"
                  << samplingDensity << "_"
                  << minLength << "_"
                  << (showLinesOnly ? "1" : "0");
        std::string cacheKey = keyStream.str();
        
        // Get from cache or compute
        auto& cache = EdgeGeometryCache::getInstance();
        return cache.getOrCompute(cacheKey, [&]() {
            // Cache miss - compute normally
            return extractOriginalEdgesImpl(shape, samplingDensity, minLength, showLinesOnly, nullptr);
        });
    } else {
        // Intersection points requested - compute without cache
        return extractOriginalEdgesImpl(shape, samplingDensity, minLength, showLinesOnly, intersectionPoints);
    }
}

// Analyze curve curvature to determine adaptive sampling density
double EdgeExtractor::analyzeCurveCurvature(
    const Handle(Geom_Curve)& curve,
    Standard_Real first,
    Standard_Real last,
    GeomAbs_CurveType curveType)
{
    // For lines, curvature is zero
    if (curveType == GeomAbs_Line) {
        return 0.0;
    }

    const int analysisPoints = 10;
    double maxCurvature = 0.0;
    double totalCurvature = 0.0;
    int validPoints = 0;

    try {
        for (int i = 0; i <= analysisPoints; ++i) {
            Standard_Real t = first + (last - first) * i / analysisPoints;

            gp_Pnt p;
            gp_Vec d1, d2;
            curve->D2(t, p, d1, d2);

            // Calculate curvature: |d1 × d2| / |d1|³
            double denominator = d1.Magnitude();
            if (denominator > 1e-10) {
                double curvature = d1.Crossed(d2).Magnitude() / std::pow(denominator, 3.0);
                maxCurvature = std::max(maxCurvature, curvature);
                totalCurvature += curvature;
                validPoints++;
            }
        }
    } catch (const Standard_Failure&) {
        // If curvature calculation fails, use conservative approach
        LOG_WRN_S("Curvature calculation failed, using conservative sampling");
        return 0.1; // Medium curvature assumption
    }

    if (validPoints == 0) {
        return 0.0;
    }

    // Return average curvature, but cap at reasonable maximum
    double avgCurvature = totalCurvature / validPoints;
    return std::min(avgCurvature, 10.0); // Cap at 10 to prevent excessive sampling
}

// Adaptive curve sampling based on curvature analysis
std::vector<gp_Pnt> EdgeExtractor::adaptiveSampleCurve(
    const Handle(Geom_Curve)& curve,
    Standard_Real first,
    Standard_Real last,
    GeomAbs_CurveType curveType,
    double baseSamplingDensity)
{
    std::vector<gp_Pnt> points;

    // Fast path for lines - always use 2 points
    if (curveType == GeomAbs_Line) {
        points.push_back(curve->Value(first));
        points.push_back(curve->Value(last));
        return points;
    }

    // Analyze curvature to determine sampling density
    double maxCurvature = analyzeCurveCurvature(curve, first, last, curveType);

    // Determine base sample count based on curvature
    int baseSamples;
    if (maxCurvature < 0.001) {
        baseSamples = 4;      // Very low curvature - minimal sampling
    } else if (maxCurvature < 0.01) {
        baseSamples = 6;      // Low curvature
    } else if (maxCurvature < 0.1) {
        baseSamples = 8;      // Medium curvature
    } else if (maxCurvature < 1.0) {
        baseSamples = 12;     // High curvature
    } else if (maxCurvature < 5.0) {
        baseSamples = 16;     // Very high curvature
    } else {
        baseSamples = 20;     // Extreme curvature
    }

    // Adjust for curve type complexity
    switch (curveType) {
        case GeomAbs_Circle:
        case GeomAbs_Ellipse:
            // Circles and ellipses need more samples for smooth appearance
            baseSamples = std::max(baseSamples, 12);
            break;
        case GeomAbs_BSplineCurve:
        case GeomAbs_BezierCurve:
            // B-splines and Bezier curves may need more samples
            baseSamples = std::max(baseSamples, 10);
            break;
        case GeomAbs_Hyperbola:
        case GeomAbs_Parabola:
            // Conic sections need moderate sampling
            baseSamples = std::max(baseSamples, 8);
            break;
        default:
            break;
    }

    // Apply sampling density factor
    double curveLength = last - first;
    int densitySamples = std::max(4, static_cast<int>(curveLength * baseSamplingDensity * 0.3));

    // Take the maximum of curvature-based and density-based sampling
    int finalSamples = std::max(baseSamples, densitySamples);

    // Cap maximum samples to prevent performance issues
    finalSamples = std::min(finalSamples, 64);

    // Generate sample points
    points.reserve(finalSamples + 1);
    for (int i = 0; i <= finalSamples; ++i) {
        Standard_Real t = first + (last - first) * i / finalSamples;
        try {
            points.push_back(curve->Value(t));
        } catch (const Standard_Failure&) {
            // If evaluation fails, skip this point
            LOG_WRN_S("Failed to evaluate curve at parameter " + std::to_string(t));
        }
    }

    // Ensure we have at least 2 points
    if (points.size() < 2) {
        points.clear();
        points.push_back(curve->Value(first));
        points.push_back(curve->Value(last));
    }

    return points;
}

std::vector<gp_Pnt> EdgeExtractor::extractOriginalEdgesImpl(
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

        // Use adaptive sampling for all curves
        data.sampledPoints = adaptiveSampleCurve(
            data.curve,
            data.first,
            data.last,
            data.curveType,
            samplingDensity
        );
        data.pointCount = data.sampledPoints.size();

        totalPoints += data.pointCount;
    });

    std::vector<gp_Pnt> points;
    points.reserve(totalPoints * 2);

    for (const auto& data : edgeData) {
        if (data.isValid && data.passesLengthFilter) {
            // Convert sampled points to line segments (pairs of points)
            for (size_t i = 0; i + 1 < data.sampledPoints.size(); ++i) {
                points.push_back(data.sampledPoints[i]);
                points.push_back(data.sampledPoints[i + 1]);
            }
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
    
    int totalEdges = edgeFaceMap.Extent();
    int twoFaceEdges = 0;
    int oneFaceEdges = 0;
    int featureEdgesFound = 0;
    int nullCurves = 0;
    int filteredByLength = 0;
    int closedCurves = 0;

    for (int i = 1; i <= edgeFaceMap.Extent(); ++i) {
        const TopoDS_Edge& edge = TopoDS::Edge(edgeFaceMap.FindKey(i));
        const TopTools_ListOfShape& faces = edgeFaceMap.FindFromIndex(i);

        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
        if (curve.IsNull()) {
            nullCurves++;
            continue;
        }

        // For closed curves (circles), check curve length instead of endpoint distance
        BRepAdaptor_Curve adaptor(edge);
        bool isClosed = (edge.Closed() || adaptor.IsClosed());
        
        if (isClosed) {
            closedCurves++;
        } else {
            gp_Pnt p1 = curve->Value(first);
            gp_Pnt p2 = curve->Value(last);
            if (p1.Distance(p2) < minLength) {
                filteredByLength++;
                continue;
            }
        }

        bool isFeatureEdge = false;
        
        // Boundary edges (only one face) are always feature edges
        if (faces.Extent() == 1) {
            oneFaceEdges++;
            isFeatureEdge = true;
        }
        // Edges between two faces - check angle
        else if (faces.Extent() == 2) {
            twoFaceEdges++;
            
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
            double angleDeg = angle * 180.0 / M_PI;
            double dotProduct = normal1.Dot(normal2);

            if (angle >= angleThreshold) {
                if (onlyConvex && dotProduct > 0) {
                    isFeatureEdge = true;
                } else if (onlyConcave && dotProduct < 0) {
                    isFeatureEdge = true;
                } else if (!onlyConvex && !onlyConcave) {
                    isFeatureEdge = true;
                }
            }
            
            // Debug: log first edge angle
            static bool firstLog = true;
            if (firstLog) {
                LOG_INF_S("First edge angle: " + std::to_string(angleDeg) + 
                         " deg, threshold: " + std::to_string(featureAngle) + 
                         " deg, isFeature: " + std::to_string(isFeatureEdge));
                firstLog = false;
            }
        }

        if (isFeatureEdge) {
            featureEdgesFound++;
            int numSamples = std::max(10, static_cast<int>(adaptor.LastParameter() - adaptor.FirstParameter()) * 10);
            numSamples = std::min(numSamples, 50);

            std::vector<gp_Pnt> edgePoints;
            for (int j = 0; j <= numSamples; ++j) {
                Standard_Real t = first + (last - first) * j / numSamples;
                edgePoints.push_back(curve->Value(t));
            }
            
            // Convert to line segments (pairs of points)
            for (size_t j = 0; j + 1 < edgePoints.size(); ++j) {
                points.push_back(edgePoints[j]);
                points.push_back(edgePoints[j + 1]);
            }
        }
    }

    return points;
}

std::vector<gp_Pnt> EdgeExtractor::extractMeshEdges(const TriangleMesh& mesh)
{
    std::vector<gp_Pnt> points;

    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
        int v1 = mesh.triangles[i];
        int v2 = mesh.triangles[i + 1];
        int v3 = mesh.triangles[i + 2];
        
        if (v1 < static_cast<int>(mesh.vertices.size()) && 
            v2 < static_cast<int>(mesh.vertices.size()) && 
            v3 < static_cast<int>(mesh.vertices.size())) {
            points.push_back(mesh.vertices[v1]);
            points.push_back(mesh.vertices[v2]);
            points.push_back(mesh.vertices[v2]);
            points.push_back(mesh.vertices[v3]);
            points.push_back(mesh.vertices[v3]);
            points.push_back(mesh.vertices[v1]);
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
    auto startTime = std::chrono::steady_clock::now();

    // Collect all edges
    std::vector<TopoDS_Edge> edges;
    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        edges.push_back(TopoDS::Edge(exp.Current()));
    }

    findEdgeIntersectionsFromEdges(edges, intersectionPoints);
}

void EdgeExtractor::findEdgeIntersectionsFromEdges(
    const std::vector<TopoDS_Edge>& edges, 
    std::vector<gp_Pnt>& intersectionPoints)
{
    auto startTime = std::chrono::steady_clock::now();

    // For small number of edges, use simpler approach
    if (edges.size() < 50) {
        findEdgeIntersectionsSimple(edges, intersectionPoints);
        return;
    }

    // For larger models, use the optimized spatial approach
    // Calculate global bounding box from edges
    Bnd_Box globalBbox;
    for (const auto& edge : edges) {
        BRepBndLib::Add(edge, globalBbox);
    }
    double xmin, ymin, zmin, xmax, ymax, zmax;
    globalBbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    double diagonal = sqrt((xmax - xmin) * (xmax - xmin) + (ymax - ymin) * (ymax - ymin) + (zmax - zmin) * (zmax - zmin));
    const double tolerance = diagonal * 0.005;
    const double bboxMargin = tolerance * 2.0;

    // Create edge data with bounding boxes
    std::vector<EdgeData> edgeData;
    edgeData.reserve(edges.size());

    // Determine grid size for spatial partitioning
    const int targetEdgesPerCell = 10;
    int gridSize = std::max(1, static_cast<int>(std::cbrt(edges.size() / targetEdgesPerCell)));
    double gridSizeX = (xmax - xmin) / gridSize;
    double gridSizeY = (ymax - ymin) / gridSize;
    double gridSizeZ = (zmax - zmin) / gridSize;

    // Precompute edge data
    for (const auto& edge : edges) {
        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);

        if (curve.IsNull()) continue;

        EdgeData data;
        data.edge = edge;
        data.curve = curve;
        data.first = first;
        data.last = last;
        data.isValid = true;
        data.passesLengthFilter = true;

        // Compute bounding box by sampling the curve
        const int bboxSamples = std::min(20, std::max(5, static_cast<int>((last - first) * 50)));
        data.bbox = AABB();

        for (int i = 0; i <= bboxSamples; ++i) {
            Standard_Real t = first + (last - first) * i / bboxSamples;
            gp_Pnt point = curve->Value(t);
            if (i == 0) {
                data.bbox.minX = data.bbox.maxX = point.X();
                data.bbox.minY = data.bbox.maxY = point.Y();
                data.bbox.minZ = data.bbox.maxZ = point.Z();
            } else {
                data.bbox.expand(point);
            }
        }

        // Expand bounding box with margin
        data.bbox.expand(bboxMargin);

        // Assign to grid cell
        data.gridX = std::max(0, std::min(gridSize - 1, static_cast<int>((data.bbox.minX - xmin) / gridSizeX)));
        data.gridY = std::max(0, std::min(gridSize - 1, static_cast<int>((data.bbox.minY - ymin) / gridSizeY)));
        data.gridZ = std::max(0, std::min(gridSize - 1, static_cast<int>((data.bbox.minZ - zmin) / gridSizeZ)));

        edgeData.push_back(data);
    }

    // Create spatial grid
    std::vector<std::vector<std::vector<std::vector<size_t>>>> grid(
        gridSize, std::vector<std::vector<std::vector<size_t>>>(
        gridSize, std::vector<std::vector<size_t>>(
        gridSize, std::vector<size_t>())));

    // Assign edges to grid cells
    for (size_t i = 0; i < edgeData.size(); ++i) {
        const auto& data = edgeData[i];
        grid[data.gridX][data.gridY][data.gridZ].push_back(i);
    }

    // Process cells for intersections
    std::mutex intersectionMutex;
    std::atomic<size_t> processedComparisons{0};
    std::atomic<size_t> bboxFiltered{0};
    std::atomic<size_t> distanceFiltered{0};

    auto processCellRange = [&](size_t startCell, size_t endCell) {
        for (size_t cellIdx = startCell; cellIdx < endCell; ++cellIdx) {
            size_t gx = cellIdx / (gridSize * gridSize);
            size_t gy = (cellIdx / gridSize) % gridSize;
            size_t gz = cellIdx % gridSize;

            const auto& cellEdges = grid[gx][gy][gz];
            if (cellEdges.size() < 2) continue;

            // Check all pairs within this cell
            for (size_t i = 0; i < cellEdges.size(); ++i) {
                for (size_t j = i + 1; j < cellEdges.size(); ++j) {
                    size_t edgeIdx1 = cellEdges[i];
                    size_t edgeIdx2 = cellEdges[j];

                    processedComparisons++;

                    const auto& data1 = edgeData[edgeIdx1];
                    const auto& data2 = edgeData[edgeIdx2];

                    // AABB intersection test
                    if (!data1.bbox.intersects(data2.bbox)) {
                        bboxFiltered++;
                        continue;
                    }

                    // Compute minimum distance between curves
                    double minDistance = computeMinDistanceBetweenCurves(data1, data2);

                    if (minDistance > tolerance) {
                        distanceFiltered++;
                        continue;
                    }

                    // Found intersection
                    gp_Pnt intersectionPoint = computeIntersectionPoint(data1, data2);

                    // Thread-safe addition to result
                    std::lock_guard<std::mutex> lock(intersectionMutex);

                    // Check if this intersection point is already found
                    bool alreadyFound = false;
                    for (const auto& existingPoint : intersectionPoints) {
                        if (intersectionPoint.Distance(existingPoint) < tolerance) {
                            alreadyFound = true;
                            break;
                        }
                    }

                    if (!alreadyFound) {
                        intersectionPoints.push_back(intersectionPoint);
                    }
                }
            }
        }
    };

    // Use parallel processing for large models
    const size_t totalCells = gridSize * gridSize * gridSize;
    const size_t numThreads = std::min(8u, std::max(1u, std::thread::hardware_concurrency()));
    const size_t cellsPerThread = (totalCells + numThreads - 1) / numThreads;

    std::vector<std::thread> threads;
    for (size_t t = 0; t < numThreads; ++t) {
        size_t startCell = t * cellsPerThread;
        size_t endCell = std::min(startCell + cellsPerThread, totalCells);
        threads.emplace_back(processCellRange, startCell, endCell);
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
}

void EdgeExtractor::findEdgeIntersectionsSimple(
    const std::vector<TopoDS_Edge>& edges, 
    std::vector<gp_Pnt>& intersectionPoints)
{
    // Calculate bounding box for tolerance
    Bnd_Box bbox;
    for (const auto& edge : edges) {
        BRepBndLib::Add(edge, bbox);
    }
    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    double diagonal = sqrt((xmax - xmin) * (xmax - xmin) + (ymax - ymin) * (ymax - ymin) + (zmax - zmin) * (zmax - zmin));
    const double tolerance = diagonal * 0.01;

    // Simple pairwise comparison
    for (size_t i = 0; i < edges.size(); ++i) {
        for (size_t j = i + 1; j < edges.size(); ++j) {
            const TopoDS_Edge& edge1 = edges[i];
            const TopoDS_Edge& edge2 = edges[j];

            Standard_Real first1, last1, first2, last2;
            Handle(Geom_Curve) curve1 = BRep_Tool::Curve(edge1, first1, last1);
            Handle(Geom_Curve) curve2 = BRep_Tool::Curve(edge2, first2, last2);

            if (curve1.IsNull() || curve2.IsNull()) continue;

            // Use simplified intersection detection - check only endpoints and midpoints
            gp_Pnt start1 = curve1->Value(first1);
            gp_Pnt end1 = curve1->Value(last1);
            gp_Pnt mid1 = curve1->Value((first1 + last1) / 2.0);

            gp_Pnt start2 = curve2->Value(first2);
            gp_Pnt end2 = curve2->Value(last2);
            gp_Pnt mid2 = curve2->Value((first2 + last2) / 2.0);

            // Check distances between key points
            double minDistance = std::numeric_limits<double>::max();
            gp_Pnt closestPoint1, closestPoint2;

            std::vector<gp_Pnt> points1 = {start1, mid1, end1};
            std::vector<gp_Pnt> points2 = {start2, mid2, end2};

            for (const auto& p1 : points1) {
                for (const auto& p2 : points2) {
                    double dist = p1.Distance(p2);
                    if (dist < minDistance) {
                        minDistance = dist;
                        closestPoint1 = p1;
                        closestPoint2 = p2;
                    }
                }
            }

            // If the curves are close enough, consider it an intersection
            if (minDistance < tolerance) {
                gp_Pnt intersectionPoint = gp_Pnt(
                    (closestPoint1.X() + closestPoint2.X()) / 2.0,
                    (closestPoint1.Y() + closestPoint2.Y()) / 2.0,
                    (closestPoint1.Z() + closestPoint2.Z()) / 2.0
                );

                // Check if this intersection point is already found
                bool alreadyFound = false;
                for (const auto& existingPoint : intersectionPoints) {
                    if (intersectionPoint.Distance(existingPoint) < tolerance) {
                        alreadyFound = true;
                        break;
                    }
                }

                if (!alreadyFound) {
                    intersectionPoints.push_back(intersectionPoint);
                }
            }
        }
    }
}

double EdgeExtractor::computeMinDistanceBetweenCurves(
    const struct EdgeData& data1, 
    const struct EdgeData& data2)
{
    const int samples = 15;
    double minDistance = std::numeric_limits<double>::max();

    for (int i = 0; i <= samples; ++i) {
        Standard_Real t1 = data1.first + (data1.last - data1.first) * i / samples;
        gp_Pnt p1 = data1.curve->Value(t1);

        for (int j = 0; j <= samples; ++j) {
            Standard_Real t2 = data2.first + (data2.last - data2.first) * j / samples;
            gp_Pnt p2 = data2.curve->Value(t2);

            double dist = p1.Distance(p2);
            if (dist < minDistance) {
                minDistance = dist;
            }
        }
    }

    return minDistance;
}

gp_Pnt EdgeExtractor::computeIntersectionPoint(
    const struct EdgeData& data1, 
    const struct EdgeData& data2)
{
    const int samples = 10;
    double minDistance = std::numeric_limits<double>::max();
    gp_Pnt closest1, closest2;

    for (int i = 0; i <= samples; ++i) {
        Standard_Real t1 = data1.first + (data1.last - data1.first) * i / samples;
        gp_Pnt p1 = data1.curve->Value(t1);

        for (int j = 0; j <= samples; ++j) {
            Standard_Real t2 = data2.first + (data2.last - data2.first) * j / samples;
            gp_Pnt p2 = data2.curve->Value(t2);

            double dist = p1.Distance(p2);
            if (dist < minDistance) {
                minDistance = dist;
                closest1 = p1;
                closest2 = p2;
            }
        }
    }

    return gp_Pnt(
        (closest1.X() + closest2.X()) / 2.0,
        (closest1.Y() + closest2.Y()) / 2.0,
        (closest1.Z() + closest2.Z()) / 2.0
    );
}
