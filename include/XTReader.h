#pragma once

#include "GeometryReader.h"
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/BRepTools.hxx>
#include <OpenCASCADE/BRep_Builder.hxx>
#include <OpenCASCADE/TopoDS_Compound.hxx>
#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/ShapeFix_Shape.hxx>
#include <OpenCASCADE/BRepCheck_Analyzer.hxx>
#include <OpenCASCADE/BRepTools_ReShape.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeShape.hxx>
#include <OpenCASCADE/Standard_Failure.hxx>
#include <OpenCASCADE/Standard_ConstructionError.hxx>
#include <mutex>
#include <future>
#include <thread>
#include <execution>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

/**
 * @brief X_T (Parasolid Text) file reader for importing CAD models
 *
 * Provides functionality to read X_T files and convert them to OCCGeometry objects
 * X_T is Parasolid's text-based format for 3D geometry exchange
 * 
 * Note: This implementation provides basic X_T support.
 * For full Parasolid support, consider integrating with Parasolid SDK
 */
class XTReader : public GeometryReader {
public:
    /**
     * @brief Constructor
     */
    XTReader() = default;

    /**
     * @brief Destructor
     */
    ~XTReader() override = default;

    /**
     * @brief Read an X_T file and return geometry objects
     * @param filePath Path to the X_T file
     * @param options Optimization options
     * @param progress Progress callback function
     * @return ReadResult containing success status and geometry objects
     */
    ReadResult readFile(const std::string& filePath,
        const OptimizationOptions& options = OptimizationOptions(),
        ProgressCallback progress = nullptr) override;

    /**
     * @brief Check if a file has a valid X_T extension
     * @param filePath Path to check
     * @return true if file has .x_t or .xmt_txt extension
     */
    bool isValidFile(const std::string& filePath) const override;

    /**
     * @brief Get the file extensions supported by this reader
     * @return Vector of supported extensions
     */
    std::vector<std::string> getSupportedExtensions() const override;

    /**
     * @brief Get the format name
     * @return Human-readable format name
     */
    std::string getFormatName() const override;

    /**
     * @brief Get the file filter string for file dialogs
     * @return File filter string
     */
    std::string getFileFilter() const override;

private:
    /**
     * @brief Initialize the X_T reader
     */
    void initialize();

    /**
     * @brief Parse X_T file header
     * @param filePath Path to X_T file
     * @param progress Progress callback
     * @return true if header parsing successful
     */
    bool parseHeader(const std::string& filePath, ProgressCallback progress = nullptr);

    /**
     * @brief Parse X_T file content
     * @param filePath Path to X_T file
     * @param shapes Output vector of shapes
     * @param progress Progress callback
     * @return true if parsing successful
     */
    bool parseXTFile(const std::string& filePath,
        std::vector<TopoDS_Shape>& shapes,
        ProgressCallback progress = nullptr);

    /**
     * @brief Extract individual shapes from a compound
     * @param compound The compound shape
     * @param shapes Output vector of shapes
     */
    void extractShapes(const TopoDS_Shape& compound, std::vector<TopoDS_Shape>& shapes);

    /**
     * @brief Process shapes in parallel
     * @param shapes Vector of shapes to process
     * @param baseName Base name for geometry objects
     * @param options Optimization options
     * @param progress Progress callback
     * @return Vector of processed geometry objects
     */
    std::vector<std::shared_ptr<OCCGeometry>> processShapesParallel(
        const std::vector<TopoDS_Shape>& shapes,
        const std::string& baseName,
        const OptimizationOptions& options,
        ProgressCallback progress = nullptr
    );

    /**
     * @brief Process a single shape
     * @param shape The shape to process
     * @param name Name for the geometry object
     * @param options Optimization options
     * @return Processed geometry object
     */
    std::shared_ptr<OCCGeometry> processSingleShape(
        const TopoDS_Shape& shape,
        const std::string& name,
        const OptimizationOptions& options
    );

    /**
     * @brief Fix shape if needed
     * @param shape Input shape
     * @return Fixed shape
     */
    TopoDS_Shape fixShape(const TopoDS_Shape& shape);

    /**
     * @brief Parse a line from X_T file
     * @param line Input line
     * @param shapes Output vector of shapes
     * @return true if line was processed successfully
     */
    bool parseLine(const std::string& line, std::vector<TopoDS_Shape>& shapes);

    // Static members for caching
    static std::unordered_map<std::string, ReadResult> s_cache;
    static std::mutex s_cacheMutex;
    static bool s_initialized;
};
