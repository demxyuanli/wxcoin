#pragma once

#include <vector>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TopoDS_Edge.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "EdgeTypes.h"
#include "rendering/GeometryProcessor.h"

/**
 * @brief Edge extraction logic
 * 
 * Handles extraction of different edge types from geometry:
 * - Original edges from CAD geometry
 * - Feature edges based on angle criteria
 * - Mesh edges from triangulated mesh
 */
class EdgeExtractor {
public:
    EdgeExtractor();
    ~EdgeExtractor() = default;

    /**
     * @brief Extract original edges from CAD shape
     * @param shape The CAD shape to extract edges from
     * @param samplingDensity Number of samples per unit length
     * @param minLength Minimum edge length to include
     * @param showLinesOnly If true, only show linear edges
     * @param intersectionPoints Output parameter for edge intersection points
     * @return Vector of sampled points for all edges
     */
    std::vector<gp_Pnt> extractOriginalEdges(
        const TopoDS_Shape& shape, 
        double samplingDensity = 80.0, 
        double minLength = 0.01, 
        bool showLinesOnly = false,
        std::vector<gp_Pnt>* intersectionPoints = nullptr);

    /**
     * @brief Extract feature edges from CAD shape
     * @param shape The CAD shape to extract feature edges from
     * @param featureAngle Angle threshold in degrees for feature detection
     * @param minLength Minimum edge length to include
     * @param onlyConvex If true, only extract convex edges
     * @param onlyConcave If true, only extract concave edges
     * @return Vector of sampled points for feature edges
     */
    std::vector<gp_Pnt> extractFeatureEdges(
        const TopoDS_Shape& shape, 
        double featureAngle, 
        double minLength, 
        bool onlyConvex, 
        bool onlyConcave);

    /**
     * @brief Extract mesh edges from triangulated mesh
     * @param mesh The triangle mesh
     * @return Vector of edge endpoints
     */
    std::vector<gp_Pnt> extractMeshEdges(const TriangleMesh& mesh);

    /**
     * @brief Generate silhouette edges for given camera position
     * @param shape The CAD shape
     * @param cameraPos Camera position for silhouette calculation
     * @return Vector of sampled points for silhouette edges
     */
    std::vector<gp_Pnt> extractSilhouetteEdges(
        const TopoDS_Shape& shape, 
        const gp_Pnt& cameraPos);

private:
    // Helper methods for edge processing
    void findEdgeIntersections(
        const TopoDS_Shape& shape, 
        std::vector<gp_Pnt>& intersectionPoints);
    
    void findEdgeIntersectionsFromEdges(
        const std::vector<TopoDS_Edge>& edges, 
        std::vector<gp_Pnt>& intersectionPoints);
    
    void findEdgeIntersectionsSimple(
        const std::vector<TopoDS_Edge>& edges, 
        std::vector<gp_Pnt>& intersectionPoints);
    
    double computeMinDistanceBetweenCurves(
        const struct EdgeData& data1, 
        const struct EdgeData& data2);
    
    gp_Pnt computeIntersectionPoint(
        const struct EdgeData& data1, 
        const struct EdgeData& data2);
};
