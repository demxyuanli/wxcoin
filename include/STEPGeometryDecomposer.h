#pragma once

#include <vector>
#include <string>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TopoDS_Face.hxx>
#include <OpenCASCADE/TopoDS_Edge.hxx>
#include <OpenCASCADE/TopoDS_Compound.hxx>
#include <OpenCASCADE/Bnd_Box.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Dir.hxx>
#include "GeometryReader.h"

/**
 * @brief Geometry decomposition utility for STEP files
 *
 * Provides intelligent decomposition algorithms to break down complex
 * CAD models into manageable components for visualization and processing.
 */
class STEPGeometryDecomposer {
public:
    /**
     * @brief Face feature structure for intelligent decomposition
     */
    struct FaceFeature {
        TopoDS_Face face;
        int id;
        std::string type; // "PLANE", "CYLINDER", "SPHERE", "CONE", "TORUS", "SURFACE"
        double area;
        gp_Pnt centroid;
        gp_Dir normal;
        std::vector<int> adjacentFaces;

        FaceFeature() : id(0), area(0.0) {}
    };

    /**
     * @brief Comparator for TopoDS_Edge to use in sets
     */
    struct EdgeComparator {
        bool operator()(const TopoDS_Edge& a, const TopoDS_Edge& b) const {
            // Use TShape identity comparison
            return a.TShape().get() < b.TShape().get();
        }
    };

    /**
     * @brief Decompose shape using various strategies
     * @param shape Input shape to decompose
     * @param options Decomposition options
     * @return Vector of decomposed shapes
     */
    static std::vector<TopoDS_Shape> decomposeShape(
        const TopoDS_Shape& shape,
        const GeometryReader::OptimizationOptions& options);

    /**
     * @brief Decompose shape using topology-based approach
     * @param shape Input shape
     * @param level Decomposition level
     * @return Vector of decomposed shapes
     */
    static std::vector<TopoDS_Shape> decomposeByLevelUsingTopo(
        const TopoDS_Shape& shape,
        GeometryReader::DecompositionLevel level);

    /**
     * @brief Feature-based intelligent decomposition (FreeCAD-style)
     * @param shape Input shape to decompose
     * @return Vector of decomposed components
     */
    static std::vector<TopoDS_Shape> decomposeByFeatureRecognition(
        const TopoDS_Shape& shape);

    /**
     * @brief Adjacent faces clustering decomposition
     * @param shape Input shape to decompose
     * @return Vector of decomposed components
     */
    static std::vector<TopoDS_Shape> decomposeByAdjacentFacesClustering(
        const TopoDS_Shape& shape);

    /**
     * @brief Decompose by shell groups (group shells into logical bodies)
     * @param shape Input shape
     * @return Vector of decomposed shapes
     */
    static std::vector<TopoDS_Shape> decomposeByShellGroups(
        const TopoDS_Shape& shape);

    /**
     * @brief FreeCAD-like intelligent decomposition strategy
     * @param shape Input shape to decompose
     * @return Vector of decomposed components
     */
    static std::vector<TopoDS_Shape> decomposeShapeFreeCADLike(
        const TopoDS_Shape& shape);

    /**
     * @brief Basic shape decomposition using multiple strategies
     * @param shape Input shape to decompose
     * @return Vector of decomposed components
     */
    static std::vector<TopoDS_Shape> decomposeShape(
        const TopoDS_Shape& shape);

    /**
     * @brief Decompose by face groups (group faces by geometric similarity)
     * @param shape Input shape
     * @return Vector of decomposed shapes
     */
    static std::vector<TopoDS_Shape> decomposeByFaceGroups(
        const TopoDS_Shape& shape);

    /**
     * @brief Decompose by connectivity (group connected faces)
     * @param shape Input shape
     * @return Vector of decomposed shapes
     */
    static std::vector<TopoDS_Shape> decomposeByConnectivity(
        const TopoDS_Shape& shape);

    /**
     * @brief Decompose by geometric features (planes, cylinders, etc.)
     * @param shape Input shape
     * @return Vector of decomposed shapes
     */
    static std::vector<TopoDS_Shape> decomposeByGeometricFeatures(
        const TopoDS_Shape& shape);

    static bool areFacesConnected(const TopoDS_Face& face1, const TopoDS_Face& face2);

private:

    // Helper functions for face analysis
    static std::string classifyFaceType(const TopoDS_Face& face);
    static double calculateFaceArea(const TopoDS_Face& face);
    static gp_Pnt calculateFaceCentroid(const TopoDS_Face& face);
    static gp_Dir calculateFaceNormal(const TopoDS_Face& face);
    static bool areFacesSimilar(const TopoDS_Face& face1, const TopoDS_Face& face2);
    static bool areFacesAdjacent(const TopoDS_Face& face1, const TopoDS_Face& face2);


    // Parallel processing helpers
    static std::vector<FaceFeature> extractFaceFeaturesParallel(
        const std::vector<TopoDS_Face>& faces,
        const std::vector<Bnd_Box>& faceBounds);

    // Optimized clustering algorithms
    static void clusterFacesByFeaturesOptimized(
        const std::vector<FaceFeature>& faceFeatures,
        const std::vector<Bnd_Box>& faceBounds,
        std::vector<std::vector<int>>& featureGroups);

    static void buildFaceAdjacencyGraphOptimized(
        const std::vector<TopoDS_Face>& faces,
        const std::vector<Bnd_Box>& faceBounds,
        std::vector<std::vector<int>>& adjacencyGraph);

    static void clusterAdjacentFacesOptimized(
        const std::vector<TopoDS_Face>& faces,
        const std::vector<std::vector<int>>& adjacencyGraph,
        std::vector<std::vector<int>>& clusters);

    // Validation and creation helpers
    static bool isValidCluster(const std::vector<int>& cluster, const std::vector<TopoDS_Face>& faces);
    static TopoDS_Shape tryCreateSolidFromFaces(const TopoDS_Compound& compound,
                                                const std::vector<FaceFeature>& faceFeatures,
                                                const std::vector<int>& group);
    static TopoDS_Shape tryCreateSolidFromFaceCluster(const TopoDS_Compound& compound,
                                                      const std::vector<TopoDS_Face>& faces,
                                                      const std::vector<int>& cluster);
    static void createComponentsFromGroups(const std::vector<FaceFeature>& faceFeatures,
                                           const std::vector<std::vector<int>>& featureGroups,
                                           std::vector<TopoDS_Shape>& components);
    static void createValidatedComponentsFromClusters(const std::vector<TopoDS_Face>& faces,
                                                      const std::vector<std::vector<int>>& clusters,
                                                      std::vector<TopoDS_Shape>& components);
    static void mergeSmallComponents(std::vector<TopoDS_Shape>& components);
    static void refineComponents(std::vector<TopoDS_Shape>& components);
    static bool areShapesSimilar(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2);
    static void clusterFacesByFeatures(const std::vector<FaceFeature>& faceFeatures, 
                                       std::vector<std::vector<int>>& featureGroups);
    static void buildFaceAdjacencyGraph(const std::vector<TopoDS_Face>& faces, 
                                        std::vector<std::vector<int>>& adjacencyGraph);
    static void clusterAdjacentFaces(const std::vector<TopoDS_Face>& faces, 
                                     const std::vector<std::vector<int>>& adjacencyGraph, 
                                     std::vector<std::vector<int>>& clusters);

    // Spatial optimization helpers
    static std::vector<int> findNearbyFaces(int faceIndex,
                                           const std::vector<std::vector<int>>& spatialGrid,
                                           const std::vector<Bnd_Box>& faceBounds,
                                           int gridSize);
    static bool areFeaturesSimilar(const FaceFeature& f1, const FaceFeature& f2,
                                  const Bnd_Box& b1, const Bnd_Box& b2);
};
