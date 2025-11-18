#include "edges/extractors/OriginalEdgeExtractor.h"
#include "edges/EdgeGeometryCache.h"
#include "logger/AsyncLogger.h"
#include "edges/EdgeIntersectionAccelerator.h"  // BVH acceleration
#include <TopoDS.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
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
#include <tbb/tbb.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/concurrent_vector.h>
#include <shared_mutex>

OriginalEdgeExtractor::OriginalEdgeExtractor() {}

// EdgeData constructor implementation
OriginalEdgeExtractor::EdgeData::EdgeData(const FilteredEdge& filteredEdge, double bboxMargin) {
    curve = filteredEdge.curve;
    first = filteredEdge.first;
    last = filteredEdge.last;
    length = filteredEdge.length;

    // Pre-compute bounding box
    Bnd_Box bndBox;
    TopoDS_Edge edge = filteredEdge.edge;
    BRepBndLib::Add(edge, bndBox);

    double exmin, eymin, ezmin, exmax, eymax, ezmax;
    bndBox.Get(exmin, eymin, ezmin, exmax, eymax, ezmax);

    bbox.Add(gp_Pnt(exmin, eymin, ezmin));
    bbox.Add(gp_Pnt(exmax, eymax, ezmax));
    bbox.Enlarge(bboxMargin);
}

// EdgeData constructor for TopoDS_Edge
OriginalEdgeExtractor::EdgeData::EdgeData(const TopoDS_Edge& edge, double bboxMargin) {
    // Get curve and parameters
    Standard_Real first, last;
    curve = BRep_Tool::Curve(edge, first, last);
    this->first = first;
    this->last = last;

    // Calculate length
    gp_Pnt startPoint = curve->Value(first);
    gp_Pnt endPoint = curve->Value(last);
    length = startPoint.Distance(endPoint);

    // Pre-compute bounding box
    Bnd_Box bndBox;
    BRepBndLib::Add(edge, bndBox);

    double exmin, eymin, ezmin, exmax, eymax, ezmax;
    bndBox.Get(exmin, eymin, ezmin, exmax, eymax, ezmax);

    bbox.Add(gp_Pnt(exmin, eymin, ezmin));
    bbox.Add(gp_Pnt(exmax, eymax, ezmax));
    bbox.Enlarge(bboxMargin);
}

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
        // Single-pass edge collection and filtering
        std::vector<FilteredEdge> filteredEdges;
        collectAndFilterEdges(shape, p, filteredEdges);

        // For large models, use progressive loading
        if (filteredEdges.size() > 1000) {
            return extractProgressiveFiltered(filteredEdges, p);
        }

        // Use sequential processing to maintain topology order
        return extractEdgesFiltered(filteredEdges, p);
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

void OriginalEdgeExtractor::collectAndFilterEdges(const TopoDS_Shape& shape, const OriginalEdgeParams& params,
                                                 std::vector<FilteredEdge>& filteredEdges) {
    // Single-pass edge collection with pre-filtering and property computation
    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        TopoDS_Edge edge = TopoDS::Edge(exp.Current());

        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);

        if (curve.IsNull()) {
            continue; // Skip invalid edges
        }

        // Pre-compute edge properties
        gp_Pnt startPoint = curve->Value(first);
        gp_Pnt endPoint = curve->Value(last);
        double edgeLength = startPoint.Distance(endPoint);

        // Quick check for closed edges
        bool isClosed = (edge.Closed() || edgeLength < 1e-6);
        if (isClosed) {
            double paramRange = last - first;
            if (paramRange <= params.minLength) {
                continue; // Skip too short closed edges
            }
            edgeLength = paramRange;
        } else if (edgeLength < params.minLength) {
            continue; // Skip too short open edges
        }

        // Check curve type for line-only filtering
        bool isLineOnly = false;
        if (params.showLinesOnly) {
            BRepAdaptor_Curve adaptor(edge);
            GeomAbs_CurveType curveType = adaptor.GetType();
            if (curveType != GeomAbs_Line) {
                continue; // Skip non-line curves when line-only is enabled
            }
            isLineOnly = true;
        }

        // Edge passed all filters - add to result
        FilteredEdge filteredEdge;
        filteredEdge.edge = edge;
        filteredEdge.curve = curve;
        filteredEdge.first = first;
        filteredEdge.last = last;
        filteredEdge.length = edgeLength;
        filteredEdge.isLineOnly = isLineOnly;

        filteredEdges.push_back(filteredEdge);
    }
}

std::vector<gp_Pnt> OriginalEdgeExtractor::extractEdgesFiltered(const std::vector<FilteredEdge>& edges,
                                                              const OriginalEdgeParams& params) {
    std::vector<gp_Pnt> result;

    // More accurate memory pre-allocation based on filtered edges
    size_t estimatedSize = 0;
    for (const auto& filteredEdge : edges) {
        // Estimate points based on curve type and length
        if (filteredEdge.isLineOnly) {
            estimatedSize += 2; // Start and end points for lines
        } else {
            // Adaptive sampling based on curve length and complexity
            double samples = std::max(4.0, filteredEdge.length * params.samplingDensity * 0.1);
            estimatedSize += static_cast<size_t>(samples);
        }
    }
    result.reserve(estimatedSize);

    // Process filtered edges directly
    for (const FilteredEdge& filteredEdge : edges) {
        std::vector<gp_Pnt> edgePoints = extractSingleEdgeFast(filteredEdge.edge, params);
        if (!edgePoints.empty()) {
            result.insert(result.end(), edgePoints.begin(), edgePoints.end());
        }
    }

    return result;
}

std::vector<gp_Pnt> OriginalEdgeExtractor::extractProgressiveFiltered(const std::vector<FilteredEdge>& edges,
                                                                     const OriginalEdgeParams& params) {
    std::vector<gp_Pnt> result;

    // Progressive processing with batched chunks
    const int batchSize = 200;
    int processed = 0;

    std::vector<FilteredEdge> batch;
    batch.reserve(batchSize);

    for (const FilteredEdge& filteredEdge : edges) {
        batch.push_back(filteredEdge);
        processed++;

        if (batch.size() >= batchSize || processed >= static_cast<int>(edges.size())) {
            // Process this batch
            std::vector<gp_Pnt> batchResult = extractEdgesFiltered(batch, params);
            result.insert(result.end(), batchResult.begin(), batchResult.end());

            // Clear batch for next iteration
            batch.clear();

            // Yield control for UI responsiveness
            if (processed % 1000 == 0) {
                std::this_thread::yield();
            }
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

    LOG_INF_S_ASYNC("OriginalEdgeExtractor: Detecting intersections, edges=" +
              std::to_string(edges.size()));

    // Generate cache key based on shape pointer and tolerance
    size_t shapeHash = reinterpret_cast<size_t>(shape.TShape().get());
    std::ostringstream keyStream;
    keyStream << "intersections_" << shapeHash << "_" 
              << std::fixed << std::setprecision(6) << adaptiveTolerance;
    std::string cacheKey = keyStream.str();

    LOG_INF_S_ASYNC("OriginalEdgeExtractor: Checking cache for key=" + cacheKey + 
                   ", shapeHash=" + std::to_string(shapeHash) + 
                   ", tolerance=" + std::to_string(adaptiveTolerance) + 
                   ", edges=" + std::to_string(edges.size()));

    // Try to get from cache
    auto& cache = EdgeGeometryCache::getInstance();
    auto cachedIntersections = cache.getOrComputeIntersections(
        cacheKey,
        [this, &edges, adaptiveTolerance]() -> std::vector<gp_Pnt> {
            // Cache miss - compute intersections
            std::vector<gp_Pnt> tempIntersections;
            LOG_INF_S_ASYNC("Computing intersections (cache miss) using optimized spatial grid (" +
                      std::to_string(edges.size()) + " edges)");
            findEdgeIntersectionsFromEdges(edges, tempIntersections, adaptiveTolerance);
            return tempIntersections;
        },
        shapeHash,
        adaptiveTolerance
    );
    
    LOG_INF_S_ASYNC("OriginalEdgeExtractor: Cache lookup complete, got " + 
                   std::to_string(cachedIntersections.size()) + " intersections");

    // Merge cached results into output
    intersectionPoints.insert(intersectionPoints.end(), 
                             cachedIntersections.begin(), 
                             cachedIntersections.end());
}

void OriginalEdgeExtractor::findEdgeIntersectionsFromEdges(
    const std::vector<TopoDS_Edge>& edges,
    std::vector<gp_Pnt>& intersectionPoints,
    double tolerance) {

    // For very small number of edges, use simpler approach
    if (edges.size() < 20) {
        findEdgeIntersectionsSimple(edges, intersectionPoints, tolerance);
        return;
    }
    
    // For larger models (>= 100 edges), use BVH acceleration
    if (edges.size() >= 100) {
        LOG_INF_S_ASYNC("Using BVH acceleration for " + std::to_string(edges.size()) + " edges");
        
        EdgeIntersectionAccelerator accelerator;
        accelerator.buildFromEdges(edges);
        
        // Extract intersections using BVH (handles duplicate checking internally)
        intersectionPoints = accelerator.extractIntersectionsParallel(tolerance);
        
        const auto& stats = accelerator.getStatistics();
        LOG_INF_S_ASYNC("BVH computation complete: " + std::to_string(intersectionPoints.size()) + 
                       " intersections found, pruning ratio: " + 
                       std::to_string(stats.pruningRatio * 100) + "%");
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
    std::vector<EdgeData> allEdgeData;
    allEdgeData.reserve(edges.size());

    // Determine optimal grid size for spatial partitioning
    // Use aspect ratio aware grid sizing instead of simple cubic root
    double sizeX = xmax - xmin + 2 * bboxMargin;
    double sizeY = ymax - ymin + 2 * bboxMargin;
    double sizeZ = zmax - zmin + 2 * bboxMargin;

    // Calculate grid dimensions based on aspect ratios and target cell count
    const int targetEdgesPerCell = 8; // Reduced for better performance
    double totalVolume = sizeX * sizeY * sizeZ;
    double avgCellVolume = totalVolume / (edges.size() / targetEdgesPerCell);
    double cellSize = std::cbrt(avgCellVolume);

    int gridSizeX = std::max(1, static_cast<int>(sizeX / cellSize));
    int gridSizeY = std::max(1, static_cast<int>(sizeY / cellSize));
    int gridSizeZ = std::max(1, static_cast<int>(sizeZ / cellSize));

    // Cap maximum grid size to prevent excessive memory usage
    const int maxGridSize = 32;
    gridSizeX = std::min(gridSizeX, maxGridSize);
    gridSizeY = std::min(gridSizeY, maxGridSize);
    gridSizeZ = std::min(gridSizeZ, maxGridSize);

    // Process edges and prepare data
    std::vector<EdgeData> edgeData;
    edgeData.reserve(edges.size());

    // Create efficient grid storage: vector of vectors for each grid cell
    int totalGridCells = gridSizeX * gridSizeY * gridSizeZ;
    std::vector<std::vector<size_t>> gridCells(totalGridCells);

    for (size_t i = 0; i < edges.size(); ++i) {
        const TopoDS_Edge& edge = edges[i];

        // Skip invalid edges
        Standard_Real first, last;
        Handle(Geom_Curve) testCurve = BRep_Tool::Curve(edge, first, last);
        if (testCurve.IsNull()) continue;

        // Use optimized constructor
        EdgeData data(edge, bboxMargin);

        // Assign to grid cell based on bounding box center
        double centerX = (data.bbox.minX + data.bbox.maxX) / 2.0 - xmin + bboxMargin;
        double centerY = (data.bbox.minY + data.bbox.maxY) / 2.0 - ymin + bboxMargin;
        double centerZ = (data.bbox.minZ + data.bbox.maxZ) / 2.0 - zmin + bboxMargin;

        data.gridX = std::max(0, std::min(gridSizeX - 1, static_cast<int>(centerX / (sizeX / gridSizeX))));
        data.gridY = std::max(0, std::min(gridSizeY - 1, static_cast<int>(centerY / (sizeY / gridSizeY))));
        data.gridZ = std::max(0, std::min(gridSizeZ - 1, static_cast<int>(centerZ / (sizeZ / gridSizeZ))));

        // Store edge data and assign to grid cell
        size_t dataIndex = edgeData.size();
        edgeData.push_back(data);

        // Calculate flat grid index
        int gridIndex = data.gridX * (gridSizeY * gridSizeZ) + data.gridY * gridSizeZ + data.gridZ;
        gridCells[gridIndex].push_back(dataIndex);
    }

    // Thread-safe intersection point management
    std::mutex intersectionMutex;
    auto addIntersectionPoint = [&](const gp_Pnt& point) {
        std::lock_guard<std::mutex> lock(intersectionMutex);
        // Check if already found (optimize by checking recent points first)
        const size_t maxCheck = std::min(size_t(20), intersectionPoints.size());
        bool alreadyFound = false;
        for (size_t k = intersectionPoints.size() - maxCheck; k < intersectionPoints.size(); ++k) {
            if (point.Distance(intersectionPoints[k]) < tolerance) {
                alreadyFound = true;
                break;
            }
        }
        if (!alreadyFound) {
            intersectionPoints.push_back(point);
        }
    };

    // Collect all potential intersection checks
    std::vector<std::pair<size_t, size_t>> intersectionChecks;

    for (int x = 0; x < gridSizeX; ++x) {
        for (int y = 0; y < gridSizeY; ++y) {
            for (int z = 0; z < gridSizeZ; ++z) {
                // Calculate current cell index
                int currentCellIndex = x * (gridSizeY * gridSizeZ) + y * gridSizeZ + z;
                const auto& cellEdges = gridCells[currentCellIndex];
                if (cellEdges.empty()) continue;

                // Check intersections within this cell
                for (size_t i = 0; i < cellEdges.size(); ++i) {
                    for (size_t j = i + 1; j < cellEdges.size(); ++j) {
                        intersectionChecks.emplace_back(cellEdges[i], cellEdges[j]);
                    }
                }

                // Check intersections with neighboring cells (27-cell neighborhood)
                for (int dx = -1; dx <= 1; ++dx) {
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dz = -1; dz <= 1; ++dz) {
                            if (dx == 0 && dy == 0 && dz == 0) continue;

                            int nx = x + dx, ny = y + dy, nz = z + dz;
                            if (nx < 0 || nx >= gridSizeX || ny < 0 || ny >= gridSizeY || nz < 0 || nz >= gridSizeZ) continue;

                            int neighborCellIndex = nx * (gridSizeY * gridSizeZ) + ny * gridSizeZ + nz;
                            const auto& neighborEdges = gridCells[neighborCellIndex];

                            // Only check edges that could potentially intersect (bbox check first)
                            for (size_t i : cellEdges) {
                                for (size_t j : neighborEdges) {
                                    // Ensure consistent ordering to avoid duplicate checks
                                    if (i < j && edgeData[i].bbox.intersects(edgeData[j].bbox)) {
                                        intersectionChecks.emplace_back(i, j);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Perform accurate intersection checks in parallel using OpenCASCADE
    std::for_each(std::execution::par, intersectionChecks.begin(), intersectionChecks.end(),
        [&](const std::pair<size_t, size_t>& check) {
            size_t idx1 = check.first;
            size_t idx2 = check.second;

            // Use OpenCASCADE's accurate intersection calculation
            const EdgeData& data1 = edgeData[idx1];
            const EdgeData& data2 = edgeData[idx2];

            try {
                // Use improved distance-based intersection detection with higher accuracy
                const int fineSamples = 16; // Increased samples for better accuracy
                double minDistance = std::numeric_limits<double>::max();
                gp_Pnt closestPoint1, closestPoint2;

                for (int i = 0; i <= fineSamples; ++i) {
                    Standard_Real t1 = data1.first + (data1.last - data1.first) * i / fineSamples;
                    gp_Pnt p1 = data1.curve->Value(t1);

                    for (int j = 0; j <= fineSamples; ++j) {
                        Standard_Real t2 = data2.first + (data2.last - data2.first) * j / fineSamples;
                        gp_Pnt p2 = data2.curve->Value(t2);

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
                    addIntersectionPoint(intersectionPoint);
                }
            } catch (...) {
                // If geometric intersection fails, use fallback method
                const int fineSamples = 8;
                double minDistance = std::numeric_limits<double>::max();
                gp_Pnt closestPoint1, closestPoint2;

                for (int i = 0; i <= fineSamples; ++i) {
                    Standard_Real t1 = data1.first + (data1.last - data1.first) * i / fineSamples;
                    gp_Pnt p1 = data1.curve->Value(t1);

                    for (int j = 0; j <= fineSamples; ++j) {
                        Standard_Real t2 = data2.first + (data2.last - data2.first) * j / fineSamples;
                        gp_Pnt p2 = data2.curve->Value(t2);

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
                    addIntersectionPoint(intersectionPoint);
                }
            }
        });
}

void OriginalEdgeExtractor::findEdgeIntersectionsFromFilteredEdges(
    const std::vector<FilteredEdge>& filteredEdges,
    std::vector<gp_Pnt>& intersectionPoints,
    double tolerance) {

    // For small number of edges, use simpler approach
    if (filteredEdges.size() < 50) {
        // Convert FilteredEdge back to TopoDS_Edge for simple method
        std::vector<TopoDS_Edge> edges;
        edges.reserve(filteredEdges.size());
        for (const auto& filteredEdge : filteredEdges) {
            edges.push_back(filteredEdge.edge);
        }
        findEdgeIntersectionsSimple(edges, intersectionPoints, tolerance);
        return;
    }

    // For larger models, use the optimized spatial approach
    // Calculate global bounding box from filtered edges
    AABB globalBbox;
    for (const auto& filteredEdge : filteredEdges) {
        // Use edge length to estimate bounding box (approximation)
        // For more accuracy, we could compute actual bbox here, but FilteredEdge doesn't store it
        // This is a trade-off between memory usage and computation
        double approxSize = filteredEdge.length * 0.5; // Rough approximation
        globalBbox.Add(gp_Pnt(-approxSize, -approxSize, -approxSize));
        globalBbox.Add(gp_Pnt(approxSize, approxSize, approxSize));
    }

    double diagonal = sqrt((globalBbox.maxX - globalBbox.minX) * (globalBbox.maxX - globalBbox.minX) +
                          (globalBbox.maxY - globalBbox.minY) * (globalBbox.maxY - globalBbox.minY) +
                          (globalBbox.maxZ - globalBbox.minZ) * (globalBbox.maxZ - globalBbox.minZ));
    const double bboxMargin = tolerance * 2.0;

    // Create edge data with bounding boxes using pre-filtered edges
    std::vector<EdgeData> allEdgeData;
    allEdgeData.reserve(filteredEdges.size());

    // Determine optimal grid size for spatial partitioning
    // Use aspect ratio aware grid sizing instead of simple cubic root
    double sizeX = globalBbox.maxX - globalBbox.minX + 2 * bboxMargin;
    double sizeY = globalBbox.maxY - globalBbox.minY + 2 * bboxMargin;
    double sizeZ = globalBbox.maxZ - globalBbox.minZ + 2 * bboxMargin;

    // Calculate grid dimensions based on aspect ratios and target cell count
    const int targetEdgesPerCell = 8; // Reduced for better performance
    double totalVolume = sizeX * sizeY * sizeZ;
    double avgCellVolume = totalVolume / (filteredEdges.size() / targetEdgesPerCell);
    double cellSize = std::cbrt(avgCellVolume);

    int gridSizeX = std::max(1, static_cast<int>(sizeX / cellSize));
    int gridSizeY = std::max(1, static_cast<int>(sizeY / cellSize));
    int gridSizeZ = std::max(1, static_cast<int>(sizeZ / cellSize));

    // Cap maximum grid size to prevent excessive memory usage
    const int maxGridSize = 32;
    gridSizeX = std::min(gridSizeX, maxGridSize);
    gridSizeY = std::min(gridSizeY, maxGridSize);
    gridSizeZ = std::min(gridSizeZ, maxGridSize);

    // Process filtered edges and prepare data using optimized EdgeData constructor
    std::vector<EdgeData> edgeData;
    edgeData.reserve(filteredEdges.size());

    // Create efficient grid storage: vector of vectors for each grid cell
    int totalGridCells = gridSizeX * gridSizeY * gridSizeZ;
    std::vector<std::vector<size_t>> gridCells(totalGridCells);

    double xmin = globalBbox.minX - bboxMargin;
    double ymin = globalBbox.minY - bboxMargin;
    double zmin = globalBbox.minZ - bboxMargin;

    for (size_t i = 0; i < filteredEdges.size(); ++i) {
        const FilteredEdge& filteredEdge = filteredEdges[i];

        // Use optimized constructor that pre-computes bounding box
        EdgeData data(filteredEdge, bboxMargin);

        // Assign to grid cell based on bounding box center
        double centerX = (data.bbox.minX + data.bbox.maxX) / 2.0 - xmin;
        double centerY = (data.bbox.minY + data.bbox.maxY) / 2.0 - ymin;
        double centerZ = (data.bbox.minZ + data.bbox.maxZ) / 2.0 - zmin;

        data.gridX = std::max(0, std::min(gridSizeX - 1, static_cast<int>(centerX / (sizeX / gridSizeX))));
        data.gridY = std::max(0, std::min(gridSizeY - 1, static_cast<int>(centerY / (sizeY / gridSizeY))));
        data.gridZ = std::max(0, std::min(gridSizeZ - 1, static_cast<int>(centerZ / (sizeZ / gridSizeZ))));

        // Store edge data and assign to grid cell
        size_t dataIndex = edgeData.size();
        edgeData.push_back(data);

        // Calculate flat grid index
        int gridIndex = data.gridX * (gridSizeY * gridSizeZ) + data.gridY * gridSizeZ + data.gridZ;
        gridCells[gridIndex].push_back(dataIndex);
    }

    // Thread-safe intersection point management
    std::mutex intersectionMutex;
    auto addIntersectionPoint = [&](const gp_Pnt& point) {
        std::lock_guard<std::mutex> lock(intersectionMutex);
        // Check if already found (optimize by checking recent points first)
        const size_t maxCheck = std::min(size_t(20), intersectionPoints.size());
        bool alreadyFound = false;
        for (size_t k = intersectionPoints.size() - maxCheck; k < intersectionPoints.size(); ++k) {
            if (point.Distance(intersectionPoints[k]) < tolerance) {
                alreadyFound = true;
                break;
            }
        }
        if (!alreadyFound) {
            intersectionPoints.push_back(point);
        }
    };

    // Collect all potential intersection checks
    std::vector<std::pair<size_t, size_t>> intersectionChecks;

    for (int x = 0; x < gridSizeX; ++x) {
        for (int y = 0; y < gridSizeY; ++y) {
            for (int z = 0; z < gridSizeZ; ++z) {
                // Calculate current cell index
                int currentCellIndex = x * (gridSizeY * gridSizeZ) + y * gridSizeZ + z;
                const auto& cellEdges = gridCells[currentCellIndex];
                if (cellEdges.empty()) continue;

                // Check intersections within this cell
                for (size_t i = 0; i < cellEdges.size(); ++i) {
                    for (size_t j = i + 1; j < cellEdges.size(); ++j) {
                        intersectionChecks.emplace_back(cellEdges[i], cellEdges[j]);
                    }
                }

                // Check intersections with neighboring cells (27-cell neighborhood)
                for (int dx = -1; dx <= 1; ++dx) {
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dz = -1; dz <= 1; ++dz) {
                            if (dx == 0 && dy == 0 && dz == 0) continue;

                            int nx = x + dx, ny = y + dy, nz = z + dz;
                            if (nx < 0 || nx >= gridSizeX || ny < 0 || ny >= gridSizeY || nz < 0 || nz >= gridSizeZ) continue;

                            int neighborCellIndex = nx * (gridSizeY * gridSizeZ) + ny * gridSizeZ + nz;
                            const auto& neighborEdges = gridCells[neighborCellIndex];

                            // Only check edges that could potentially intersect (optimized bbox check)
                            for (size_t i : cellEdges) {
                                const AABB& bbox1 = edgeData[i].bbox;
                                for (size_t j : neighborEdges) {
                                    // Ensure consistent ordering to avoid duplicate checks
                                    if (i < j) {
                                        const AABB& bbox2 = edgeData[j].bbox;
                                        // Fast bbox intersection check with early exit
                                        if (bbox1.intersects(bbox2)) {
                                            intersectionChecks.emplace_back(i, j);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Perform accurate intersection checks in parallel using OpenCASCADE
    std::for_each(std::execution::par, intersectionChecks.begin(), intersectionChecks.end(),
        [&](const std::pair<size_t, size_t>& check) {
            size_t idx1 = check.first;
            size_t idx2 = check.second;

            // Use OpenCASCADE's accurate intersection calculation
            const EdgeData& data1 = edgeData[idx1];
            const EdgeData& data2 = edgeData[idx2];

            try {
                // Use improved distance-based intersection detection with higher accuracy
                const int fineSamples = 16; // Increased samples for better accuracy
                double minDistance = std::numeric_limits<double>::max();
                gp_Pnt closestPoint1, closestPoint2;

                for (int i = 0; i <= fineSamples; ++i) {
                    Standard_Real t1 = data1.first + (data1.last - data1.first) * i / fineSamples;
                    gp_Pnt p1 = data1.curve->Value(t1);

                    for (int j = 0; j <= fineSamples; ++j) {
                        Standard_Real t2 = data2.first + (data2.last - data2.first) * j / fineSamples;
                        gp_Pnt p2 = data2.curve->Value(t2);

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
                    addIntersectionPoint(intersectionPoint);
                }
            } catch (...) {
                // If geometric intersection fails, use fallback method
                const int fineSamples = 8;
                double minDistance = std::numeric_limits<double>::max();
                gp_Pnt closestPoint1, closestPoint2;

                for (int i = 0; i <= fineSamples; ++i) {
                    Standard_Real t1 = data1.first + (data1.last - data1.first) * i / fineSamples;
                    gp_Pnt p1 = data1.curve->Value(t1);

                    for (int j = 0; j <= fineSamples; ++j) {
                        Standard_Real t2 = data2.first + (data2.last - data2.first) * j / fineSamples;
                        gp_Pnt p2 = data2.curve->Value(t2);

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
                    addIntersectionPoint(intersectionPoint);
                }
            }
        });
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

            // Use OpenCASCADE's native extrema algorithm instead of sampling
            try {
                GeomAPI_ExtremaCurveCurve extrema(curve1, curve2, first1, last1, first2, last2);
                
                if (extrema.NbExtrema() > 0) {
                    double minDist = std::numeric_limits<double>::max();
                    int minIndex = -1;
                    
                    for (int k = 1; k <= extrema.NbExtrema(); ++k) {
                        double dist = extrema.Distance(k);
                        if (dist < minDist) {
                            minDist = dist;
                            minIndex = k;
                        }
                    }
                    
                    if (minIndex > 0 && minDist < tolerance) {
                        gp_Pnt p1, p2;
                        extrema.Points(minIndex, p1, p2);
                        
                        // Use midpoint as intersection
                        gp_Pnt intersectionPoint(
                            (p1.X() + p2.X()) / 2.0,
                            (p1.Y() + p2.Y()) / 2.0,
                            (p1.Z() + p2.Z()) / 2.0
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
            } catch (const Standard_Failure&) {
                // Some edge pairs cannot be computed - this is normal
                continue;
            }
        }
    }
}

void OriginalEdgeExtractor::checkEdgeIntersection(
    const EdgeData& edge1, const EdgeData& edge2,
    std::vector<gp_Pnt>& intersectionPoints, double tolerance) {

    // Use OpenCASCADE's native extrema algorithm for accurate intersection
    try {
        GeomAPI_ExtremaCurveCurve extrema(
            edge1.curve, edge2.curve,
            edge1.first, edge1.last,
            edge2.first, edge2.last
        );
        
        if (extrema.NbExtrema() > 0) {
            double minDist = std::numeric_limits<double>::max();
            int minIndex = -1;
            
            for (int i = 1; i <= extrema.NbExtrema(); ++i) {
                double dist = extrema.Distance(i);
                if (dist < minDist) {
                    minDist = dist;
                    minIndex = i;
                }
            }
            
            if (minIndex > 0 && minDist < tolerance) {
                gp_Pnt p1, p2;
                extrema.Points(minIndex, p1, p2);
                
                // Use midpoint as intersection point
                gp_Pnt intersectionPoint(
                    (p1.X() + p2.X()) / 2.0,
                    (p1.Y() + p2.Y()) / 2.0,
                    (p1.Z() + p2.Z()) / 2.0
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
        }
    } catch (const Standard_Failure&) {
        // Some edge pairs cannot be computed - this is normal
        return;
    }
}

// ============================================================================
// Progressive Display Implementation
// ============================================================================

void OriginalEdgeExtractor::findEdgeIntersectionsProgressive(
    const TopoDS_Shape& shape,
    std::vector<gp_Pnt>& intersectionPoints,
    double tolerance,
    std::function<void(const std::vector<gp_Pnt>&)> onBatchComplete,
    std::function<void(int, const std::string&)> onProgress) {
    
    LOG_INF_S_ASYNC("OriginalEdgeExtractor: Starting progressive intersection detection");
    
    // Check cache first - if cached, use cached results and call batch callback
    size_t shapeHash = reinterpret_cast<size_t>(shape.TShape().get());
    std::ostringstream keyStream;
    keyStream << "intersections_" << shapeHash << "_" 
              << std::fixed << std::setprecision(6) << tolerance;
    std::string cacheKey = keyStream.str();
    
    auto& cache = EdgeGeometryCache::getInstance();
    auto cachedPoints = cache.tryGetCached(cacheKey);
    if (cachedPoints && !cachedPoints->empty()) {
        LOG_INF_S_ASYNC("OriginalEdgeExtractor: Using cached intersections (" + 
                       std::to_string(cachedPoints->size()) + " points)");
        
        // Use cached results
        intersectionPoints = *cachedPoints;
        
        // Call batch callback with all cached points (for progressive display)
        if (onBatchComplete) {
            onBatchComplete(intersectionPoints);
        }
        
        // Call progress callback
        if (onProgress) {
            onProgress(100, "Using cached intersections");
        }
        
        return;
    }
    
    LOG_INF_S_ASYNC("OriginalEdgeExtractor: Cache miss, computing intersections");
    
    // Extract edges first
    std::vector<TopoDS_Edge> edges;
    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        edges.push_back(TopoDS::Edge(exp.Current()));
    }
    
    if (edges.empty()) {
        LOG_WRN_S("OriginalEdgeExtractor: No edges found in shape");
        return;
    }
    
    LOG_INF_S_ASYNC("OriginalEdgeExtractor: Found " + std::to_string(edges.size()) + " edges");
    
    // For small number of edges, use simple method
    if (edges.size() < 50) {
        findEdgeIntersectionsSimple(edges, intersectionPoints, tolerance);
        
        // Store in cache after computation
        if (!intersectionPoints.empty()) {
            cache.storeCached(cacheKey, intersectionPoints, shapeHash, tolerance);
            LOG_INF_S_ASYNC("OriginalEdgeExtractor: Stored " + std::to_string(intersectionPoints.size()) + 
                           " intersections in cache");
        }
        
        if (onBatchComplete && !intersectionPoints.empty()) {
            onBatchComplete(intersectionPoints);
        }
        return;
    }
    
    // Build spatial grid and edge data
    std::vector<EdgeData> edgeData;
    edgeData.reserve(edges.size());
    
    // Calculate global bounding box
    AABB globalBbox;
    for (const auto& edge : edges) {
        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
        if (curve.IsNull()) continue;
        
        EdgeData data(edge, tolerance * 2.0);
        edgeData.push_back(data);
        globalBbox.Add(gp_Pnt(data.bbox.minX, data.bbox.minY, data.bbox.minZ));
        globalBbox.Add(gp_Pnt(data.bbox.maxX, data.bbox.maxY, data.bbox.maxZ));
    }
    
    if (edgeData.empty()) {
        LOG_WRN_S("OriginalEdgeExtractor: No valid edges found");
        return;
    }
    
    LOG_INF_S_ASYNC("OriginalEdgeExtractor: Built spatial grid with " + std::to_string(edgeData.size()) + " valid edges");
    
    // Use progressive TBB implementation
    findIntersectionsProgressiveTBB(edgeData, intersectionPoints, tolerance, onBatchComplete, onProgress);
    
    // Store in cache after computation
    if (!intersectionPoints.empty()) {
        cache.storeCached(cacheKey, intersectionPoints, shapeHash, tolerance);
        LOG_INF_S_ASYNC("OriginalEdgeExtractor: Stored " + std::to_string(intersectionPoints.size()) + 
                       " intersections in cache");
    }
}

void OriginalEdgeExtractor::findIntersectionsProgressiveTBB(
    const std::vector<EdgeData>& edgeData,
    std::vector<gp_Pnt>& intersectionPoints,
    double tolerance,
    std::function<void(const std::vector<gp_Pnt>&)> onBatchComplete,
    std::function<void(int, const std::string&)> onProgress) {
    
    LOG_INF_S_ASYNC("OriginalEdgeExtractor: Starting TBB progressive intersection detection");
    
    // Calculate optimal grid dimensions
    AABB globalBbox;
    for (const auto& data : edgeData) {
        globalBbox.Add(gp_Pnt(data.bbox.minX, data.bbox.minY, data.bbox.minZ));
        globalBbox.Add(gp_Pnt(data.bbox.maxX, data.bbox.maxY, data.bbox.maxZ));
    }
    
    double sizeX = globalBbox.maxX - globalBbox.minX;
    double sizeY = globalBbox.maxY - globalBbox.minY;
    double sizeZ = globalBbox.maxZ - globalBbox.minZ;
    
    const int targetEdgesPerCell = 8;
    double totalVolume = sizeX * sizeY * sizeZ;
    double avgCellVolume = totalVolume / (edgeData.size() / targetEdgesPerCell);
    double cellSize = std::cbrt(avgCellVolume);
    
    int gridSizeX = std::max(1, static_cast<int>(sizeX / cellSize));
    int gridSizeY = std::max(1, static_cast<int>(sizeY / cellSize));
    int gridSizeZ = std::max(1, static_cast<int>(sizeZ / cellSize));
    
    LOG_INF_S_ASYNC("OriginalEdgeExtractor: Grid dimensions: " + std::to_string(gridSizeX) + "x" + 
             std::to_string(gridSizeY) + "x" + std::to_string(gridSizeZ));
    
    // Build spatial grid
    int totalGridCells = gridSizeX * gridSizeY * gridSizeZ;
    std::vector<std::vector<size_t>> gridCells(totalGridCells);
    
    double xmin = globalBbox.minX;
    double ymin = globalBbox.minY;
    double zmin = globalBbox.minZ;
    
    for (size_t i = 0; i < edgeData.size(); ++i) {
        const EdgeData& data = edgeData[i];
        
        // Assign to grid cell based on bounding box center
        double centerX = (data.bbox.minX + data.bbox.maxX) / 2.0 - xmin;
        double centerY = (data.bbox.minY + data.bbox.maxY) / 2.0 - ymin;
        double centerZ = (data.bbox.minZ + data.bbox.maxZ) / 2.0 - zmin;
        
        int gridX = std::max(0, std::min(gridSizeX - 1, static_cast<int>(centerX / (sizeX / gridSizeX))));
        int gridY = std::max(0, std::min(gridSizeY - 1, static_cast<int>(centerY / (sizeY / gridSizeY))));
        int gridZ = std::max(0, std::min(gridSizeZ - 1, static_cast<int>(centerZ / (sizeZ / gridSizeZ))));
        
        int gridIndex = gridX * (gridSizeY * gridSizeZ) + gridY * gridSizeZ + gridZ;
        gridCells[gridIndex].push_back(i);
    }
    
    // Generate task batches
    auto taskBatches = generateTaskBatches(gridCells, edgeData, 100); // 100 tasks per batch
    
    LOG_INF_S_ASYNC("OriginalEdgeExtractor: Generated " + std::to_string(taskBatches.size()) + " task batches");
    
    // Process batches progressively
    std::vector<gp_Pnt> allIntersections;
    std::atomic<size_t> totalProcessed{0};
    
    for (size_t batchIndex = 0; batchIndex < taskBatches.size(); ++batchIndex) {
        const auto& batch = taskBatches[batchIndex];
        if (batch.empty()) continue;
        
        // Process batch in parallel using TBB
        tbb::concurrent_vector<gp_Pnt> batchIntersections;
        
        tbb::parallel_for(
            tbb::blocked_range<size_t>(0, batch.size()),
            [&](const tbb::blocked_range<size_t>& range) {
                for (size_t i = range.begin(); i < range.end(); ++i) {
                    const auto& task = batch[i];
                    size_t idx1 = task.first;
                    size_t idx2 = task.second;
                    
                    // Check intersection
                    const EdgeData& data1 = edgeData[idx1];
                    const EdgeData& data2 = edgeData[idx2];
                    
                    // Use improved distance-based intersection detection
                    const int fineSamples = 16;
                    double minDistance = std::numeric_limits<double>::max();
                    gp_Pnt closestPoint1, closestPoint2;
                    
                    for (int i = 0; i <= fineSamples; ++i) {
                        Standard_Real t1 = data1.first + (data1.last - data1.first) * i / fineSamples;
                        gp_Pnt p1 = data1.curve->Value(t1);
                        
                        for (int j = 0; j <= fineSamples; ++j) {
                            Standard_Real t2 = data2.first + (data2.last - data2.first) * j / fineSamples;
                            gp_Pnt p2 = data2.curve->Value(t2);
                            
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
                        
                        batchIntersections.push_back(intersectionPoint);
                    }
                }
            }
        );
        
        // Convert concurrent vector to regular vector
        std::vector<gp_Pnt> batchResults(batchIntersections.begin(), batchIntersections.end());
        
        // Add to total results
        allIntersections.insert(allIntersections.end(), batchResults.begin(), batchResults.end());
        
        // Update progress
        totalProcessed += batch.size();
        int progress = static_cast<int>(((batchIndex + 1) * 100) / taskBatches.size());
        std::string message = "Processed batch " + std::to_string(batchIndex + 1) + 
                            "/" + std::to_string(taskBatches.size()) + 
                            ", found " + std::to_string(batchResults.size()) + " intersections";
        
        // Update progress callback only when computation is complete
        if (onProgress && batchIndex + 1 == taskBatches.size()) {
            onProgress(100, message);
        }
        
        // Report batch completion
        if (onBatchComplete && !batchResults.empty()) {
            onBatchComplete(batchResults);
        }
    }
    
    intersectionPoints = std::move(allIntersections);
    LOG_INF_S_ASYNC("OriginalEdgeExtractor: Progressive intersection detection completed, total: " + 
             std::to_string(intersectionPoints.size()) + " intersections");
}

std::vector<std::vector<std::pair<size_t, size_t>>> OriginalEdgeExtractor::generateTaskBatches(
    const std::vector<std::vector<size_t>>& gridCells,
    const std::vector<EdgeData>& edgeData,
    size_t batchSize) {
    
    std::vector<std::pair<size_t, size_t>> allTasks;
    
    // Generate all intersection tasks
    for (size_t cellIndex = 0; cellIndex < gridCells.size(); ++cellIndex) {
        const auto& cellEdges = gridCells[cellIndex];
        if (cellEdges.empty()) continue;
        
        // Within-cell intersections
        for (size_t i = 0; i < cellEdges.size(); ++i) {
            for (size_t j = i + 1; j < cellEdges.size(); ++j) {
                allTasks.emplace_back(cellEdges[i], cellEdges[j]);
            }
        }
        
        // Cross-cell intersections with neighbors (simplified - check all cells for now)
        for (size_t otherCellIndex = cellIndex + 1; otherCellIndex < gridCells.size(); ++otherCellIndex) {
            const auto& otherEdges = gridCells[otherCellIndex];
            if (otherEdges.empty()) continue;
            
            for (size_t i : cellEdges) {
                for (size_t j : otherEdges) {
                    if (i < j && edgeData[i].bbox.intersects(edgeData[j].bbox)) {
                        allTasks.emplace_back(i, j);
                    }
                }
            }
        }
    }
    
    // Split into batches
    std::vector<std::vector<std::pair<size_t, size_t>>> batches;
    for (size_t i = 0; i < allTasks.size(); i += batchSize) {
        size_t end = std::min(i + batchSize, allTasks.size());
        batches.emplace_back(allTasks.begin() + i, allTasks.begin() + end);
    }
    
    return batches;
}

std::vector<gp_Pnt> OriginalEdgeExtractor::processIntersectionBatch(
    const std::vector<std::pair<size_t, size_t>>& batch,
    const std::vector<EdgeData>& edgeData,
    double tolerance) {
    
    std::vector<gp_Pnt> intersections;
    
    for (const auto& task : batch) {
        size_t idx1 = task.first;
        size_t idx2 = task.second;
        
        const EdgeData& data1 = edgeData[idx1];
        const EdgeData& data2 = edgeData[idx2];
        
        // Check intersection using existing method
        std::vector<gp_Pnt> tempIntersections;
        checkEdgeIntersection(data1, data2, tempIntersections, tolerance);
        
        intersections.insert(intersections.end(), tempIntersections.begin(), tempIntersections.end());
    }
    
    return intersections;
}

