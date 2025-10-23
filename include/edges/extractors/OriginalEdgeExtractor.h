#pragma once

#include "BaseEdgeExtractor.h"
#include <OpenCASCADE/Quantity_Color.hxx>
#include <OpenCASCADE/Geom_Curve.hxx>
#include <OpenCASCADE/GeomAbs_CurveType.hxx>
#include <OpenCASCADE/TopoDS_Edge.hxx>
#include <OpenCASCADE/Bnd_Box.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/Standard_Real.hxx>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <tbb/tbb.h>

/**
 * @brief Progress callback for edge extraction
 * @param progress Progress percentage (0-100)
 * @param message Status message describing current operation
 */
using EdgeExtractionProgressCallback = std::function<void(int progress, const std::string& message)>;

/**
 * @brief Parameters for original edge extraction
 */
struct OriginalEdgeParams {
    double samplingDensity = 80.0;
    double minLength = 0.01;
    bool showLinesOnly = false;
    bool highlightIntersectionNodes = false;
    double intersectionTolerance = 0.005;
    EdgeExtractionProgressCallback progressCallback = nullptr;
    
    OriginalEdgeParams() = default;
    OriginalEdgeParams(double density, double minLen, bool linesOnly, bool highlightNodes = false)
        : samplingDensity(density), minLength(minLen), showLinesOnly(linesOnly), 
          highlightIntersectionNodes(highlightNodes) {}
};

/**
 * @brief Original edge extractor
 * 
 * Extracts the original geometric edges from TopoDS_Shape
 * with adaptive sampling and intersection detection
 */
class OriginalEdgeExtractor : public TypedEdgeExtractor<OriginalEdgeParams> {
public:
    /**
     * @brief Axis-Aligned Bounding Box structure
     */
    struct AABB {
        double minX, minY, minZ;
        double maxX, maxY, maxZ;

        AABB() : minX(0), minY(0), minZ(0), maxX(0), maxY(0), maxZ(0) {}

        bool intersects(const AABB& other) const {
            return !(maxX < other.minX || other.maxX < minX ||
                     maxY < other.minY || other.maxY < minY ||
                     maxZ < other.minZ || other.maxZ < minZ);
        }

        void Add(const gp_Pnt& point) {
            minX = std::min(minX, point.X());
            minY = std::min(minY, point.Y());
            minZ = std::min(minZ, point.Z());
            maxX = std::max(maxX, point.X());
            maxY = std::max(maxY, point.Y());
            maxZ = std::max(maxZ, point.Z());
        }

        void Enlarge(double margin) {
            minX -= margin;
            minY -= margin;
            minZ -= margin;
            maxX += margin;
            maxY += margin;
            maxZ += margin;
        }
    };

    /**
     * @brief Pre-filtered edge data with computed properties
     */
    struct FilteredEdge {
        TopoDS_Edge edge;
        Handle(Geom_Curve) curve;
        Standard_Real first, last;
        double length;
        bool isLineOnly; // Cached curve type info
    };

    /**
     * @brief Optimized edge data structure for spatial partitioning with precomputed properties
     */
    struct EdgeData {
        Handle(Geom_Curve) curve;
        Standard_Real first, last;
        AABB bbox;
        int gridX, gridY, gridZ;
        double length; // Precomputed edge length

        // Constructor for efficient initialization from filtered edge
        EdgeData(const FilteredEdge& filteredEdge, double bboxMargin);

        // Constructor for legacy TopoDS_Edge initialization
        EdgeData(const TopoDS_Edge& edge, double bboxMargin);
    };
    OriginalEdgeExtractor();
    ~OriginalEdgeExtractor() = default;
    
    // BaseEdgeExtractor interface
    bool canExtract(const TopoDS_Shape& shape) const override;
    const char* getName() const override { return "OriginalEdgeExtractor"; }

    /**
     * @brief Find edge intersections
     */
    /**
     * @brief Find edge intersections with progressive display support
     * @param shape Input shape
     * @param intersectionPoints Output intersection points
     * @param tolerance Intersection tolerance
     * @param onBatchComplete Callback for each batch completion
     * @param onProgress Progress callback
     */
    void findEdgeIntersectionsProgressive(
        const TopoDS_Shape& shape,
        std::vector<gp_Pnt>& intersectionPoints,
        double tolerance,
        std::function<void(const std::vector<gp_Pnt>&)> onBatchComplete = nullptr,
        std::function<void(int, const std::string&)> onProgress = nullptr);

    /**
     * @brief Find edge intersections (legacy method)
     */
    void findEdgeIntersections(
        const TopoDS_Shape& shape,
        std::vector<gp_Pnt>& intersectionPoints,
        double tolerance = 0.005);

private:

    /**
     * @brief Find edge intersections from a list of edges (legacy version)
     */
    void findEdgeIntersectionsFromEdges(
        const std::vector<TopoDS_Edge>& edges,
        std::vector<gp_Pnt>& intersectionPoints,
        double tolerance = 0.005);

    /**
     * @brief Find edge intersections from pre-filtered edges (optimized version)
     */
    void findEdgeIntersectionsFromFilteredEdges(
        const std::vector<FilteredEdge>& edges,
        std::vector<gp_Pnt>& intersectionPoints,
        double tolerance = 0.005);

    /**
     * @brief Optimized single-pass edge collection and filtering
     */
    void collectAndFilterEdges(const TopoDS_Shape& shape, const OriginalEdgeParams& params,
                              std::vector<FilteredEdge>& filteredEdges);
    std::vector<gp_Pnt> extractEdgesFiltered(const std::vector<FilteredEdge>& edges,
                                           const OriginalEdgeParams& params);
    std::vector<gp_Pnt> extractProgressiveFiltered(const std::vector<FilteredEdge>& edges,
                                                  const OriginalEdgeParams& params);

    /**
     * @brief Simple intersection detection for small number of edges
     */
    void findEdgeIntersectionsSimple(
        const std::vector<TopoDS_Edge>& edges,
        std::vector<gp_Pnt>& intersectionPoints,
        double tolerance);

    /**
     * @brief Check intersection between two edges
     */
    void checkEdgeIntersection(
        const EdgeData& edge1,
        const EdgeData& edge2,
        std::vector<gp_Pnt>& intersectionPoints,
        double tolerance);

    /**
     * @brief Progressive intersection detection with TBB
     */
    void findIntersectionsProgressiveTBB(
        const std::vector<EdgeData>& edgeData,
        std::vector<gp_Pnt>& intersectionPoints,
        double tolerance,
        std::function<void(const std::vector<gp_Pnt>&)> onBatchComplete,
        std::function<void(int, const std::string&)> onProgress);

    /**
     * @brief Generate intersection tasks in batches
     */
    std::vector<std::vector<std::pair<size_t, size_t>>> generateTaskBatches(
        const std::vector<std::vector<size_t>>& gridCells,
        const std::vector<EdgeData>& edgeData,
        size_t batchSize = 100);

    /**
     * @brief Process a batch of intersection tasks
     */
    std::vector<gp_Pnt> processIntersectionBatch(
        const std::vector<std::pair<size_t, size_t>>& batch,
        const std::vector<EdgeData>& edgeData,
        double tolerance);
    
protected:
    std::vector<gp_Pnt> extractTyped(const TopoDS_Shape& shape, const OriginalEdgeParams* params) override;

private:
    /**
     * @brief Progressive extraction for large models
     */
    std::vector<gp_Pnt> extractProgressive(
        const TopoDS_Shape& shape,
        const OriginalEdgeParams& params,
        int totalEdges);

    /**
     * @brief Batched edge processing for better parallelization
     */
    std::vector<gp_Pnt> extractEdgesBatched(
        const std::vector<TopoDS_Edge>& edges,
        const OriginalEdgeParams& params);


    /**
     * @brief Fast edge filtering
     */
    bool shouldProcessEdge(const TopoDS_Edge& edge, const OriginalEdgeParams& params);

    /**
     * @brief Fast single edge extraction
     */
    std::vector<gp_Pnt> extractSingleEdgeFast(
        const TopoDS_Edge& edge,
        const OriginalEdgeParams& params);

    /**
     * @brief Fast adaptive curve sampling based on curvature
     */
    std::vector<gp_Pnt> adaptiveSampleCurveFast(
        const Handle(Geom_Curve)& curve,
        Standard_Real first,
        Standard_Real last,
        GeomAbs_CurveType curveType,
        double baseSamplingDensity) const;

    /**
     * @brief Fast curve curvature analysis
     */
    double analyzeCurveCurvatureFast(
        const Handle(Geom_Curve)& curve,
        Standard_Real first,
        Standard_Real last,
        GeomAbs_CurveType curveType) const;

    /**
     * @brief Adaptive curve sampling based on curvature (legacy)
     */
    std::vector<gp_Pnt> adaptiveSampleCurve(
        const Handle(Geom_Curve)& curve,
        Standard_Real first,
        Standard_Real last,
        GeomAbs_CurveType curveType,
        double baseSamplingDensity) const;

    /**
     * @brief Analyze curve curvature for adaptive sampling (legacy)
     */
    double analyzeCurveCurvature(
        const Handle(Geom_Curve)& curve,
        Standard_Real first,
        Standard_Real last,
        GeomAbs_CurveType curveType) const;
};
