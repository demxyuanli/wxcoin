#pragma once

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "OCCGeometry.h"
#include "GeometryReader.h"

// Forward declarations
class STEPReader;

/**
 * @brief Geometry conversion utilities for STEP files
 * 
 * Handles conversion of TopoDS_Shape to OCCGeometry objects with
 * decomposition, coloring, and post-processing
 */
class STEPGeometryConverter {
public:
    using ProgressCallback = std::function<void(int /*percent*/, const std::string& /*stage*/)>;

    /**
     * @brief Convert a TopoDS_Shape to OCCGeometry objects with optimization
     * @param shape The shape to convert
     * @param baseName Base name for the geometry objects
     * @param options Optimization options
     * @param progress Progress callback
     * @param progressStart Starting progress percentage
     * @param progressSpan Progress span for this operation
     * @return Vector of OCCGeometry objects
     */
    static std::vector<std::shared_ptr<OCCGeometry>> shapeToGeometries(
        const TopoDS_Shape& shape,
        const std::string& baseName = "ImportedGeometry",
        const GeometryReader::OptimizationOptions& options = GeometryReader::OptimizationOptions(),
        ProgressCallback progress = nullptr,
        int progressStart = 50,
        int progressSpan = 40
    );

    /**
     * @brief Process a single shape (for parallel processing)
     * @param shape The shape to process
     * @param name Name for the geometry object
     * @param baseName Base filename
     * @param options Optimization options
     * @return Processed geometry object
     */
    static std::shared_ptr<OCCGeometry> processSingleShape(
        const TopoDS_Shape& shape,
        const std::string& name,
        const std::string& baseName,
        const GeometryReader::OptimizationOptions& options
    );

    /**
     * @brief Process a single shape with custom color palette and index
     * @param shape The shape to process
     * @param name Name for the geometry object
     * @param baseName Base filename
     * @param options Optimization options
     * @param palette Color palette to use
     * @param hasher Hash function for consistent coloring
     * @param colorIndex Index in the color palette
     * @return Processed geometry object
     */
    static std::shared_ptr<OCCGeometry> processSingleShape(
        const TopoDS_Shape& shape,
        const std::string& name,
        const std::string& baseName,
        const GeometryReader::OptimizationOptions& options,
        const std::vector<Quantity_Color>& palette,
        const std::hash<std::string>& hasher,
        size_t colorIndex
    );

    /**
     * @brief Create geometries from shapes with coloring
     * @param shapes Vector of shapes to convert
     * @param baseName Base name for geometries
     * @param options Optimization options
     * @param palette Color palette
     * @param hasher Hash function for consistent coloring
     * @return Vector of created geometries
     */
    static std::vector<std::shared_ptr<OCCGeometry>> createGeometriesFromShapes(
        const std::vector<TopoDS_Shape>& shapes,
        const std::string& baseName,
        const GeometryReader::OptimizationOptions& options,
        const std::vector<Quantity_Color>& palette,
        const std::hash<std::string>& hasher);

    /**
     * @brief Detect if a shape is a shell model (surface model without volume)
     * @param shape The shape to analyze
     * @return true if the shape is a shell model
     */
    static bool detectShellModel(const TopoDS_Shape& shape);

    /**
     * @brief Scale imported geometry to reasonable size
     * @param geometries Vector of geometry objects to scale
     * @param targetSize Target maximum dimension (0 = auto-detect)
     * @return Scaling factor applied
     */
    static double scaleGeometriesToReasonableSize(
        std::vector<std::shared_ptr<OCCGeometry>>& geometries,
        double targetSize = 0.0
    );

    /**
     * @brief Calculate combined bounding box of multiple geometries
     * @param geometries Vector of geometries
     * @param minPt Output minimum point
     * @param maxPt Output maximum point
     * @return true if valid bounds found
     */
    static bool calculateCombinedBoundingBox(
        const std::vector<std::shared_ptr<OCCGeometry>>& geometries,
        gp_Pnt& minPt,
        gp_Pnt& maxPt
    );
};

