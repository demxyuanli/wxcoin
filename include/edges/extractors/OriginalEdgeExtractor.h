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

/**
 * @brief Parameters for original edge extraction
 */
struct OriginalEdgeParams {
    double samplingDensity = 80.0;
    double minLength = 0.01;
    bool showLinesOnly = false;
    bool highlightIntersectionNodes = false;
    double intersectionTolerance = 0.005;
    
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
    OriginalEdgeExtractor();
    ~OriginalEdgeExtractor() = default;
    
    // BaseEdgeExtractor interface
    bool canExtract(const TopoDS_Shape& shape) const override;
    const char* getName() const override { return "OriginalEdgeExtractor"; }

    /**
     * @brief Find edge intersections
     */
    void findEdgeIntersections(
        const TopoDS_Shape& shape,
        std::vector<gp_Pnt>& intersectionPoints,
        double tolerance = 0.005);

private:
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
     * @brief Edge data structure for spatial partitioning
     */
    struct EdgeData {
        TopoDS_Edge edge;
        Handle(Geom_Curve) curve;
        Standard_Real first, last;
        AABB bbox;
        int gridX, gridY, gridZ;
    };

    /**
     * @brief Find edge intersections from a list of edges
     */
    void findEdgeIntersectionsFromEdges(
        const std::vector<TopoDS_Edge>& edges,
        std::vector<gp_Pnt>& intersectionPoints,
        double tolerance = 0.005);

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
