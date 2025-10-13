#include "edges/extractors/OriginalEdgeExtractor.h"
#include "edges/EdgeGeometryCache.h"
#include "logger/Logger.h"
#include <TopoDS.hxx>
#include <TopExp_Explorer.hxx>
#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <GeomAbs_CurveType.hxx>
#include <execution>
#include <mutex>
#include <atomic>
#include <sstream>
#include <thread>
#include <future>
#include <limits>
#include <cmath>
#include <algorithm>

OriginalEdgeExtractor::OriginalEdgeExtractor() {}

bool OriginalEdgeExtractor::canExtract(const TopoDS_Shape& shape) const {
    // Can extract from any shape containing edges
    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        return true;
    }
    return false;
}

std::vector<gp_Pnt> OriginalEdgeExtractor::extractTyped(
    const TopoDS_Shape& shape,
    const OriginalEdgeParams* params) {

    // Use default parameters if not provided
    OriginalEdgeParams defaultParams;
    const OriginalEdgeParams& p = params ? *params : defaultParams;
    
    // Try to use cache
    std::ostringstream keyStream;
    keyStream << "original_" 
              << reinterpret_cast<uintptr_t>(&shape.TShape()) << "_"
              << p.samplingDensity << "_"
              << p.minLength << "_"
              << (p.showLinesOnly ? "1" : "0");
    std::string cacheKey = keyStream.str();
    
    auto& cache = EdgeGeometryCache::getInstance();
    return cache.getOrCompute(cacheKey, [&]() {
        std::vector<gp_Pnt> result;
        
        // Fast edge counting for optimization
        int totalEdges = 0;
        for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
            totalEdges++;
        }

        // For large models, use progressive loading
        if (totalEdges > 1000) {
            return extractProgressive(shape, p, totalEdges);
        }

        // Collect all edges for smaller models
        std::vector<TopoDS_Edge> allEdges;
        allEdges.reserve(totalEdges);
        for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
            allEdges.push_back(TopoDS::Edge(exp.Current()));
        }

        // Use sequential processing to maintain topology order
        return extractEdgesBatched(allEdges, p);
    });
}

std::vector<gp_Pnt> OriginalEdgeExtractor::extractProgressive(
    const TopoDS_Shape& shape,
    const OriginalEdgeParams& params,
    int totalEdges) {

    std::vector<gp_Pnt> result;

    // Progressive loading: process in batches
    const int batchSize = 200;
    int processed = 0;

    std::vector<TopoDS_Edge> batch;
    batch.reserve(batchSize);

    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        batch.push_back(TopoDS::Edge(exp.Current()));
        processed++;

        if (batch.size() >= batchSize || processed >= totalEdges) {
            // Process this batch
            std::vector<gp_Pnt> batchResult = extractEdgesBatched(batch, params);
            result.insert(result.end(), batchResult.begin(), batchResult.end());

            // Clear batch for next iteration
            batch.clear();

            // Yield control for UI responsiveness (simulate)
            if (processed % 1000 == 0) {
                std::this_thread::yield();
            }
        }
    }

    return result;
}

std::vector<gp_Pnt> OriginalEdgeExtractor::extractEdgesBatched(
    const std::vector<TopoDS_Edge>& edges,
    const OriginalEdgeParams& params) {

    std::vector<gp_Pnt> result;

    // Pre-allocate result vector for better performance
    size_t estimatedSize = edges.size() * 10; // Rough estimate
    result.reserve(estimatedSize);

    // Process edges sequentially to maintain topology order
    // This is crucial for correct edge connectivity display
    for (const TopoDS_Edge& edge : edges) {
        // Fast edge filtering
        if (!shouldProcessEdge(edge, params)) {
            continue;
        }

        std::vector<gp_Pnt> edgePoints = extractSingleEdgeFast(edge, params);
        if (!edgePoints.empty()) {
            result.insert(result.end(), edgePoints.begin(), edgePoints.end());
        }
    }

    return result;
}

bool OriginalEdgeExtractor::shouldProcessEdge(const TopoDS_Edge& edge, const OriginalEdgeParams& params) {
                Standard_Real first, last;
                Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);

                if (curve.IsNull()) {
        return false;
    }

    // Fast length check
                gp_Pnt startPoint = curve->Value(first);
                gp_Pnt endPoint = curve->Value(last);
                double edgeLength = startPoint.Distance(endPoint);

    // Quick check for closed edges
    bool isClosed = (edge.Closed() || edgeLength < 1e-6);
                if (isClosed) {
                    double paramRange = last - first;
        if (paramRange <= params.minLength) {
            return false;
        }
        edgeLength = paramRange;
    }

    if (edgeLength < params.minLength) {
        return false;
    }

    // Fast curve type check
    if (params.showLinesOnly) {
        BRepAdaptor_Curve adaptor(edge);
        GeomAbs_CurveType curveType = adaptor.GetType();
        if (curveType != GeomAbs_Line) {
            return false;
        }
    }

    return true;
}

std::vector<gp_Pnt> OriginalEdgeExtractor::extractSingleEdgeFast(
    const TopoDS_Edge& edge,
    const OriginalEdgeParams& params) {

    Standard_Real first, last;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);

    if (curve.IsNull()) {
        return {};
    }

    BRepAdaptor_Curve adaptor(edge);
    GeomAbs_CurveType curveType = adaptor.GetType();

    std::vector<gp_Pnt> sampledPoints;

    // Optimized sampling for lines (most common case)
    if (curveType == GeomAbs_Line) {
        sampledPoints.push_back(curve->Value(first));
        sampledPoints.push_back(curve->Value(last));
    } else {
        // Use faster sampling for curved edges
        sampledPoints = adaptiveSampleCurveFast(curve, first, last, curveType, params.samplingDensity);
    }

    // Convert sampled points to line segments
    // This ensures proper edge connectivity by creating point pairs for each line segment
    std::vector<gp_Pnt> edgePoints;
    for (size_t i = 0; i + 1 < sampledPoints.size(); ++i) {
        edgePoints.push_back(sampledPoints[i]);
        edgePoints.push_back(sampledPoints[i + 1]);
    }

    return edgePoints;
}

std::vector<gp_Pnt> OriginalEdgeExtractor::adaptiveSampleCurveFast(
    const Handle(Geom_Curve)& curve,
    Standard_Real first,
    Standard_Real last,
    GeomAbs_CurveType curveType,
    double baseSamplingDensity) const {

    std::vector<gp_Pnt> points;

    // Fast path for simple curves
    if (curveType == GeomAbs_Line) {
        points.push_back(curve->Value(first));
        points.push_back(curve->Value(last));
        return points;
    }

    // Simplified curvature-based sampling
    double maxCurvature = analyzeCurveCurvatureFast(curve, first, last, curveType);

    // Determine sample count based on curvature
    int baseSamples;
    if (maxCurvature < 0.01) baseSamples = 4;
    else if (maxCurvature < 0.1) baseSamples = 6;
    else if (maxCurvature < 1.0) baseSamples = 8;
    else baseSamples = 12;

    // Apply sampling density
    double curveLength = last - first;
    int densitySamples = std::max(3, static_cast<int>(curveLength * baseSamplingDensity * 0.2));
    int finalSamples = std::max(baseSamples, densitySamples);
    finalSamples = std::min(finalSamples, 32); // Limit maximum samples

    // Generate points
    points.reserve(finalSamples + 1);
    for (int i = 0; i <= finalSamples; ++i) {
        Standard_Real t = first + (last - first) * i / finalSamples;
        try {
            points.push_back(curve->Value(t));
        } catch (...) {
            // Fallback to endpoints if evaluation fails
            if (points.empty()) {
                points.push_back(curve->Value(first));
                points.push_back(curve->Value(last));
                return points;
            }
        }
    }

    // Ensure minimum 2 points
    if (points.size() < 2) {
        points.clear();
        points.push_back(curve->Value(first));
        points.push_back(curve->Value(last));
    }

    return points;
}

double OriginalEdgeExtractor::analyzeCurveCurvatureFast(
    const Handle(Geom_Curve)& curve,
    Standard_Real first,
    Standard_Real last,
    GeomAbs_CurveType curveType) const {

    if (curveType == GeomAbs_Line) return 0.0;

    const int analysisPoints = 5; // Reduced from 10 for speed
    double maxCurvature = 0.0;

    try {
        for (int i = 0; i <= analysisPoints; ++i) {
            Standard_Real t = first + (last - first) * i / analysisPoints;

            gp_Pnt p;
            gp_Vec d1, d2;
            curve->D2(t, p, d1, d2);

            double denominator = d1.Magnitude();
            if (denominator > 1e-10) {
                double curvature = d1.Crossed(d2).Magnitude() / std::pow(denominator, 3.0);
                maxCurvature = std::max(maxCurvature, curvature);
            }
        }
    } catch (...) {
        return 0.1; // Default fallback
    }

    return std::min(maxCurvature, 5.0); // Cap maximum curvature
}

std::vector<gp_Pnt> OriginalEdgeExtractor::adaptiveSampleCurve(
    const Handle(Geom_Curve)& curve,
    Standard_Real first,
    Standard_Real last,
    GeomAbs_CurveType curveType,
    double baseSamplingDensity) const {
    
    std::vector<gp_Pnt> points;
    
    // Fast path for lines
    if (curveType == GeomAbs_Line) {
        points.push_back(curve->Value(first));
        points.push_back(curve->Value(last));
        return points;
    }
    
    // Analyze curvature
    double maxCurvature = analyzeCurveCurvature(curve, first, last, curveType);
    
    // Determine sample count based on curvature
    int baseSamples;
    if (maxCurvature < 0.001) baseSamples = 4;
    else if (maxCurvature < 0.01) baseSamples = 6;
    else if (maxCurvature < 0.1) baseSamples = 8;
    else if (maxCurvature < 1.0) baseSamples = 12;
    else if (maxCurvature < 5.0) baseSamples = 16;
    else baseSamples = 20;
    
    // Adjust for curve type
    switch (curveType) {
        case GeomAbs_Circle:
        case GeomAbs_Ellipse:
            baseSamples = std::max(baseSamples, 12);
            break;
        case GeomAbs_BSplineCurve:
        case GeomAbs_BezierCurve:
            baseSamples = std::max(baseSamples, 10);
            break;
        case GeomAbs_Hyperbola:
        case GeomAbs_Parabola:
            baseSamples = std::max(baseSamples, 8);
            break;
        default:
            break;
    }
    
    // Apply sampling density
    double curveLength = last - first;
    int densitySamples = std::max(4, static_cast<int>(curveLength * baseSamplingDensity * 0.3));
    int finalSamples = std::max(baseSamples, densitySamples);
    finalSamples = std::min(finalSamples, 64);
    
    // Generate points
    points.reserve(finalSamples + 1);
    for (int i = 0; i <= finalSamples; ++i) {
        Standard_Real t = first + (last - first) * i / finalSamples;
        try {
            points.push_back(curve->Value(t));
        } catch (...) {
            LOG_WRN_S("Failed to evaluate curve at parameter " + std::to_string(t));
        }
    }
    
    // Ensure minimum 2 points
    if (points.size() < 2) {
        points.clear();
        points.push_back(curve->Value(first));
        points.push_back(curve->Value(last));
    }
    
    return points;
}

double OriginalEdgeExtractor::analyzeCurveCurvature(
    const Handle(Geom_Curve)& curve,
    Standard_Real first,
    Standard_Real last,
    GeomAbs_CurveType curveType) const {
    
    if (curveType == GeomAbs_Line) return 0.0;
    
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
            
            double denominator = d1.Magnitude();
            if (denominator > 1e-10) {
                double curvature = d1.Crossed(d2).Magnitude() / std::pow(denominator, 3.0);
                maxCurvature = std::max(maxCurvature, curvature);
                totalCurvature += curvature;
                validPoints++;
            }
        }
    } catch (...) {
        return 0.1;
    }
    
    if (validPoints == 0) return 0.0;
    
    double avgCurvature = totalCurvature / validPoints;
    return std::min(avgCurvature, 10.0);
}

void OriginalEdgeExtractor::findEdgeIntersections(
    const TopoDS_Shape& shape,
    std::vector<gp_Pnt>& intersectionPoints,
    double tolerance) {

    // Calculate adaptive tolerance based on model size if tolerance is very small
    double adaptiveTolerance = tolerance;
    if (tolerance < 1e-6) {  // If tolerance is effectively zero, use adaptive calculation
        Bnd_Box bbox;
        for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
            BRepBndLib::Add(TopoDS::Edge(exp.Current()), bbox);
        }
        double xmin, ymin, zmin, xmax, ymax, zmax;
        bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        double diagonal = sqrt((xmax - xmin) * (xmax - xmin) +
                              (ymax - ymin) * (ymax - ymin) +
                              (zmax - zmin) * (zmax - zmin));
        adaptiveTolerance = diagonal * 0.001;  // 0.1% of model size
    }


    // Collect all edges
    std::vector<TopoDS_Edge> edges;
    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        edges.push_back(TopoDS::Edge(exp.Current()));
    }

    findEdgeIntersectionsFromEdges(edges, intersectionPoints, adaptiveTolerance);
}

void OriginalEdgeExtractor::findEdgeIntersectionsFromEdges(
    const std::vector<TopoDS_Edge>& edges,
    std::vector<gp_Pnt>& intersectionPoints,
    double tolerance) {

    // For small number of edges, use simpler approach
    if (edges.size() < 50) {
        findEdgeIntersectionsSimple(edges, intersectionPoints, tolerance);
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
    const double bboxMargin = tolerance * 2.0;

    // Create edge data with bounding boxes
    std::vector<EdgeData> edgeData;
    edgeData.reserve(edges.size());

    // Determine grid size for spatial partitioning
    const int targetEdgesPerCell = 10;
    int gridSize = std::max(1, static_cast<int>(std::cbrt(edges.size() / targetEdgesPerCell)));
    double gridSizeX = (xmax - xmin + 2 * bboxMargin) / gridSize;
    double gridSizeY = (ymax - ymin + 2 * bboxMargin) / gridSize;
    double gridSizeZ = (zmax - zmin + 2 * bboxMargin) / gridSize;

    // Process edges and assign to grid cells
    for (size_t i = 0; i < edges.size(); ++i) {
        const TopoDS_Edge& edge = edges[i];
        EdgeData data;
        data.edge = edge;

        // Get curve and parameters
        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
        if (curve.IsNull()) continue;

        data.curve = curve;
        data.first = first;
        data.last = last;

        // Calculate bounding box
        Bnd_Box bbox;
        BRepBndLib::Add(edge, bbox);
        double exmin, eymin, ezmin, exmax, eymax, ezmax;
        bbox.Get(exmin, eymin, ezmin, exmax, eymax, ezmax);

        // Expand bounding box
        data.bbox.Add(gp_Pnt(exmin, eymin, ezmin));
        data.bbox.Add(gp_Pnt(exmax, eymax, ezmax));
        data.bbox.Enlarge(bboxMargin);

        // Assign to grid cell
        double centerX = (exmin + exmax) / 2.0 - xmin + bboxMargin;
        double centerY = (eymin + eymax) / 2.0 - ymin + bboxMargin;
        double centerZ = (ezmin + ezmax) / 2.0 - zmin + bboxMargin;

        data.gridX = std::max(0, std::min(gridSize - 1, static_cast<int>(centerX / gridSizeX)));
        data.gridY = std::max(0, std::min(gridSize - 1, static_cast<int>(centerY / gridSizeY)));
        data.gridZ = std::max(0, std::min(gridSize - 1, static_cast<int>(centerZ / gridSizeZ)));

        edgeData.push_back(data);
    }

    // Create spatial grid
    std::vector<std::vector<std::vector<std::vector<size_t>>>> grid(
        gridSize, std::vector<std::vector<std::vector<size_t>>>(
        gridSize, std::vector<std::vector<size_t>>(
        gridSize, std::vector<size_t>())));

    // Assign edges to grid cells
    for (size_t i = 0; i < edgeData.size(); ++i) {
        const EdgeData& data = edgeData[i];
        grid[data.gridX][data.gridY][data.gridZ].push_back(i);
    }

    // Check intersections within each cell and neighboring cells
    for (int x = 0; x < gridSize; ++x) {
        for (int y = 0; y < gridSize; ++y) {
            for (int z = 0; z < gridSize; ++z) {
                const auto& cellEdges = grid[x][y][z];
                if (cellEdges.empty()) continue;

                // Check intersections within this cell
                for (size_t i = 0; i < cellEdges.size(); ++i) {
                    for (size_t j = i + 1; j < cellEdges.size(); ++j) {
                        checkEdgeIntersection(edgeData[cellEdges[i]], edgeData[cellEdges[j]], intersectionPoints, tolerance);
                    }
                }

                // Check intersections with neighboring cells
                for (int dx = -1; dx <= 1; ++dx) {
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dz = -1; dz <= 1; ++dz) {
                            if (dx == 0 && dy == 0 && dz == 0) continue;

                            int nx = x + dx, ny = y + dy, nz = z + dz;
                            if (nx < 0 || nx >= gridSize || ny < 0 || ny >= gridSize || nz < 0 || nz >= gridSize) continue;

                            const auto& neighborEdges = grid[nx][ny][nz];
                            for (size_t i : cellEdges) {
                                for (size_t j : neighborEdges) {
                                    if (edgeData[i].bbox.intersects(edgeData[j].bbox)) {
                                        checkEdgeIntersection(edgeData[i], edgeData[j], intersectionPoints, tolerance);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < intersectionPoints.size(); ++i) {
        const auto& p = intersectionPoints[i];
    }
}

void OriginalEdgeExtractor::findEdgeIntersectionsSimple(
    const std::vector<TopoDS_Edge>& edges,
    std::vector<gp_Pnt>& intersectionPoints,
    double tolerance) {

    for (size_t i = 0; i < edges.size(); ++i) {
        for (size_t j = i + 1; j < edges.size(); ++j) {
            Standard_Real first1, last1, first2, last2;
            Handle(Geom_Curve) curve1 = BRep_Tool::Curve(edges[i], first1, last1);
            Handle(Geom_Curve) curve2 = BRep_Tool::Curve(edges[j], first2, last2);

            if (curve1.IsNull() || curve2.IsNull()) continue;

            // Sample more points for better accuracy
            const int samples = 10;
            std::vector<gp_Pnt> points1, points2;

            for (int k = 0; k <= samples; ++k) {
                Standard_Real t1 = first1 + (last1 - first1) * k / samples;
                Standard_Real t2 = first2 + (last2 - first2) * k / samples;
                points1.push_back(curve1->Value(t1));
                points2.push_back(curve2->Value(t2));
            }

            double minDistance = std::numeric_limits<double>::max();
            gp_Pnt closestPoint1, closestPoint2;

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

            if (minDistance < tolerance) {
                gp_Pnt intersectionPoint(
                    (closestPoint1.X() + closestPoint2.X()) / 2.0,
                    (closestPoint1.Y() + closestPoint2.Y()) / 2.0,
                    (closestPoint1.Z() + closestPoint2.Z()) / 2.0
                );

                // Check if already found
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

void OriginalEdgeExtractor::checkEdgeIntersection(
    const EdgeData& edge1, const EdgeData& edge2,
    std::vector<gp_Pnt>& intersectionPoints, double tolerance) {

    // Two-stage approach: fast coarse check + refined check
    const int coarseSamples = 6;  // Very fast initial check
    double minDistance = std::numeric_limits<double>::max();

    // Stage 1: Coarse check with fewer samples
    for (int i = 0; i <= coarseSamples; ++i) {
        Standard_Real t1 = edge1.first + (edge1.last - edge1.first) * i / coarseSamples;
        gp_Pnt p1 = edge1.curve->Value(t1);

        for (int j = 0; j <= coarseSamples; ++j) {
            Standard_Real t2 = edge2.first + (edge2.last - edge2.first) * j / coarseSamples;
            gp_Pnt p2 = edge2.curve->Value(t2);

            double dist = p1.Distance(p2);
            if (dist < minDistance) {
                minDistance = dist;
            }
        }
    }

    // Early exit if clearly not intersecting
    if (minDistance > tolerance * 2.0) {
        return;
    }

    // Stage 2: Refined check with more samples only if needed
    const int fineSamples = 8;
    minDistance = std::numeric_limits<double>::max();
    gp_Pnt closestPoint1, closestPoint2;

    for (int i = 0; i <= fineSamples; ++i) {
        Standard_Real t1 = edge1.first + (edge1.last - edge1.first) * i / fineSamples;
        gp_Pnt p1 = edge1.curve->Value(t1);

        for (int j = 0; j <= fineSamples; ++j) {
            Standard_Real t2 = edge2.first + (edge2.last - edge2.first) * j / fineSamples;
            gp_Pnt p2 = edge2.curve->Value(t2);

            double dist = p1.Distance(p2);
            if (dist < minDistance) {
                minDistance = dist;
                closestPoint1 = p1;
                closestPoint2 = p2;
            }
        }
    }

    if (minDistance < tolerance) {
        // Use simple average for intersection point
        gp_Pnt intersectionPoint(
            (closestPoint1.X() + closestPoint2.X()) / 2.0,
            (closestPoint1.Y() + closestPoint2.Y()) / 2.0,
            (closestPoint1.Z() + closestPoint2.Z()) / 2.0
        );

        // Check if already found (optimize by checking recent points first)
        bool alreadyFound = false;
        const size_t maxCheck = std::min(size_t(10), intersectionPoints.size()); // Only check last 10 points
        for (size_t k = intersectionPoints.size() - maxCheck; k < intersectionPoints.size(); ++k) {
            if (intersectionPoint.Distance(intersectionPoints[k]) < tolerance) {
                alreadyFound = true;
                break;
            }
        }

        if (!alreadyFound) {
            intersectionPoints.push_back(intersectionPoint);
        }
    }

    for (size_t i = 0; i < intersectionPoints.size(); ++i) {
        const auto& p = intersectionPoints[i];
    }
}


