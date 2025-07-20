#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <future>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TopoDS_Compound.hxx>
#include "OCCGeometry.h"

/**
 * @brief STEP file reader for importing CAD models
 * 
 * Provides functionality to read STEP files and convert them to OCCGeometry objects
 * with optimized performance through parallel processing and caching
 */
class STEPReader {
public:
    /**
     * @brief Result structure for STEP file reading
     */
    struct ReadResult {
        bool success;
        std::string errorMessage;
        std::vector<std::shared_ptr<OCCGeometry>> geometries;
        TopoDS_Shape rootShape;
        double importTime; // Time taken for import in milliseconds
        
        ReadResult() : success(false), importTime(0.0) {}
    };
    
    /**
     * @brief Optimization options for STEP import
     */
    struct OptimizationOptions {
        bool enableParallelProcessing = true;
        bool enableShapeAnalysis = false; // Disable by default for speed
        bool enableCaching = true;
        bool enableBatchOperations = true;
        int maxThreads = 4;
        double precision = 0.01;
        
        OptimizationOptions() = default;
    };
    
    /**
     * @brief Read a STEP file and return geometry objects with optimization
     * @param filePath Path to the STEP file
     * @param options Optimization options
     * @return ReadResult containing success status and geometry objects
     */
    static ReadResult readSTEPFile(const std::string& filePath, 
                                  const OptimizationOptions& options = OptimizationOptions());
    
    /**
     * @brief Read a STEP file and return a single compound shape
     * @param filePath Path to the STEP file
     * @return TopoDS_Shape containing all geometry from the file
     */
    static TopoDS_Shape readSTEPShape(const std::string& filePath);
    
    /**
     * @brief Check if a file has a valid STEP extension
     * @param filePath Path to check
     * @return true if file has .step or .stp extension
     */
    static bool isSTEPFile(const std::string& filePath);
    
    /**
     * @brief Get supported file extensions
     * @return Vector of supported extensions
     */
    static std::vector<std::string> getSupportedExtensions();
    
    /**
     * @brief Convert a TopoDS_Shape to OCCGeometry objects with optimization
     * @param shape The shape to convert
     * @param baseName Base name for the geometry objects
     * @param options Optimization options
     * @return Vector of OCCGeometry objects
     */
    static std::vector<std::shared_ptr<OCCGeometry>> shapeToGeometries(
        const TopoDS_Shape& shape, 
        const std::string& baseName = "ImportedGeometry",
        const OptimizationOptions& options = OptimizationOptions()
    );
    
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
     * @brief Clear the import cache
     */
    static void clearCache();
    
    /**
     * @brief Get cache statistics
     * @return String with cache statistics
     */
    static std::string getCacheStats();
    
    /**
     * @brief Set global optimization options
     * @param options New optimization options
     */
    static void setGlobalOptimizationOptions(const OptimizationOptions& options);
    
    /**
     * @brief Get current global optimization options
     * @return Current optimization options
     */
    static OptimizationOptions getGlobalOptimizationOptions();
    
private:
    STEPReader() = delete; // Pure static class
    
    /**
     * @brief Initialize the STEP reader (if needed)
     */
    static void initialize();
    
    /**
     * @brief Extract individual shapes from a compound
     * @param compound The compound shape
     * @param shapes Output vector of shapes
     */
    static void extractShapes(const TopoDS_Shape& compound, std::vector<TopoDS_Shape>& shapes);
    
    /**
     * @brief Process shapes in parallel
     * @param shapes Vector of shapes to process
     * @param baseName Base name for geometry objects
     * @param options Optimization options
     * @return Vector of processed geometry objects
     */
    static std::vector<std::shared_ptr<OCCGeometry>> processShapesParallel(
        const std::vector<TopoDS_Shape>& shapes,
        const std::string& baseName,
        const OptimizationOptions& options
    );
    
    /**
     * @brief Process a single shape (for parallel processing)
     * @param shape The shape to process
     * @param name Name for the geometry object
     * @param options Optimization options
     * @return Processed geometry object
     */
    static std::shared_ptr<OCCGeometry> processSingleShape(
        const TopoDS_Shape& shape,
        const std::string& name,
        const OptimizationOptions& options
    );
    
    /**
     * @brief Calculate bounding box for multiple geometries efficiently
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
    
    // Static members for caching and optimization
    static std::unordered_map<std::string, ReadResult> s_cache;
    static std::mutex s_cacheMutex;
    static OptimizationOptions s_globalOptions;
    static bool s_initialized;
}; 