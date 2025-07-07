#pragma once

#include <string>
#include <vector>
#include <memory>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Compound.hxx>
#include "OCCGeometry.h"

/**
 * @brief STEP file reader for importing CAD models
 * 
 * Provides functionality to read STEP files and convert them to OCCGeometry objects
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
        
        ReadResult() : success(false) {}
    };
    
    /**
     * @brief Read a STEP file and return geometry objects
     * @param filePath Path to the STEP file
     * @return ReadResult containing success status and geometry objects
     */
    static ReadResult readSTEPFile(const std::string& filePath);
    
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
     * @brief Convert a TopoDS_Shape to OCCGeometry objects
     * @param shape The shape to convert
     * @param baseName Base name for the geometry objects
     * @return Vector of OCCGeometry objects
     */
    static std::vector<std::shared_ptr<OCCGeometry>> shapeToGeometries(
        const TopoDS_Shape& shape, 
        const std::string& baseName = "ImportedGeometry"
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
}; 