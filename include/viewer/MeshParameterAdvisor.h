#pragma once

#include "rendering/GeometryProcessor.h"
#include <OpenCASCADE/TopoDS_Shape.hxx>

/**
 * @brief Structure to hold shape complexity analysis results
 */
struct ShapeComplexity {
    double boundingBoxSize = 0.0;    // Diagonal of bounding box
    size_t faceCount = 0;            // Number of faces
    size_t edgeCount = 0;            // Number of edges
    double surfaceArea = 0.0;        // Total surface area
    double volumeToSurfaceRatio = 0.0; // Volume/surface area ratio for solids
    bool hasComplexSurfaces = false; // True if has BSpline or Bezier surfaces
    double avgCurvature = 0.0;       // Simplified curvature metric
};

/**
 * @brief Quality presets for mesh generation
 */
enum class MeshQualityPreset {
    Draft,      // Fast preview, coarse mesh
    Low,        // Basic quality, good for visualization
    Medium,     // Balanced quality and performance
    High,       // High quality, production ready
    VeryHigh    // Maximum quality, detailed analysis
};

/**
 * @brief Intelligent mesh parameter recommendation system
 *
 * Analyzes geometry complexity and recommends optimal mesh parameters
 * for different quality requirements and model types.
 */
class MeshParameterAdvisor {
public:
    /**
     * @brief Analyze shape complexity
     * @param shape The CAD shape to analyze
     * @return Complexity analysis results
     */
    static ShapeComplexity analyzeShape(const TopoDS_Shape& shape);

    /**
     * @brief Recommend optimal mesh parameters for a shape
     * @param shape The CAD shape to analyze
     * @return Recommended mesh parameters
     */
    static MeshParameters recommendParameters(const TopoDS_Shape& shape);

    /**
     * @brief Estimate triangle count for given parameters
     * @param shape The CAD shape
     * @param params Mesh parameters
     * @return Estimated number of triangles
     */
    static size_t estimateTriangleCount(const TopoDS_Shape& shape,
                                       const MeshParameters& params);

    /**
     * @brief Get parameters for a specific quality preset
     * @param shape The CAD shape
     * @param preset Quality preset
     * @return Mesh parameters for the preset
     */
    static MeshParameters getPresetParameters(const TopoDS_Shape& shape,
                                             MeshQualityPreset preset);

    /**
     * @brief Get draft quality preset parameters
     * @param shape The CAD shape
     * @return Draft quality parameters
     */
    static MeshParameters getDraftPreset(const TopoDS_Shape& shape);

    /**
     * @brief Get low quality preset parameters
     * @param shape The CAD shape
     * @return Low quality parameters
     */
    static MeshParameters getLowPreset(const TopoDS_Shape& shape);

    /**
     * @brief Get medium quality preset parameters
     * @param shape The CAD shape
     * @return Medium quality parameters
     */
    static MeshParameters getMediumPreset(const TopoDS_Shape& shape);

    /**
     * @brief Get high quality preset parameters
     * @param shape The CAD shape
     * @return High quality parameters
     */
    static MeshParameters getHighPreset(const TopoDS_Shape& shape);

    /**
     * @brief Get very high quality preset parameters
     * @param shape The CAD shape
     * @return Very high quality parameters
     */
    static MeshParameters getVeryHighPreset(const TopoDS_Shape& shape);

    /**
     * @brief Validate mesh parameters
     * @param params Parameters to validate
     * @param shape Optional shape for context-aware validation
     * @return True if parameters are valid
     */
    static bool validateParameters(const MeshParameters& params,
                                  const TopoDS_Shape* shape = nullptr);

    /**
     * @brief Get recommended deflection based on quality level
     * @param boundingBoxSize Size of model bounding box
     * @param quality Quality level (0.0 = coarse, 1.0 = fine)
     * @return Recommended deflection
     */
    static double getRecommendedDeflection(double boundingBoxSize, double quality);

    /**
     * @brief Estimate memory usage for triangle mesh
     * @param triangleCount Number of triangles
     * @return Estimated memory usage in MB
     */
    static double estimateMemoryUsage(size_t triangleCount);
};



