#pragma once

#include "GeometryReader.h"
#include <OpenCASCADE/IGESControl_Reader.hxx>
#include <OpenCASCADE/IGESCAFControl_Reader.hxx>
#include <OpenCASCADE/Interface_Static.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TopoDS_Compound.hxx>
#include <OpenCASCADE/BRep_Builder.hxx>
#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/Standard_Failure.hxx>
#include <OpenCASCADE/ShapeFix_Shape.hxx>
#include <OpenCASCADE/BRepCheck_Analyzer.hxx>
#include <OpenCASCADE/TDF_LabelSequence.hxx>
#include <OpenCASCADE/XCAFDoc_ShapeTool.hxx>
#include <OpenCASCADE/XCAFDoc_ColorTool.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include <mutex>
#include <future>
#include <thread>
#include <execution>

/**
 * @brief IGES file reader for importing CAD models
 *
 * Provides functionality to read IGES files and convert them to OCCGeometry objects
 * with optimized performance through parallel processing and caching
 */
class IGESReader : public GeometryReader {
public:
    /**
     * @brief Constructor
     */
    IGESReader() = default;

    /**
     * @brief Destructor
     */
    ~IGESReader() override = default;

    /**
     * @brief Read an IGES file and return geometry objects
     * @param filePath Path to the IGES file
     * @param options Optimization options
     * @param progress Progress callback function
     * @return ReadResult containing success status and geometry objects
     */
    ReadResult readFile(const std::string& filePath,
        const OptimizationOptions& options = OptimizationOptions(),
        ProgressCallback progress = nullptr) override;

    /**
     * @brief Check if a file has a valid IGES extension
     * @param filePath Path to check
     * @return true if file has .iges or .igs extension
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
     * @brief Initialize the IGES reader
     */
    void initialize();

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
        const std::string& baseName,
        const OptimizationOptions& options
    );

    /**
     * @brief Process shapes with CAF color and name information
     * @param shapes Vector of shapes to process
     * @param labels CAF labels corresponding to shapes
     * @param shapeTool CAF shape tool
     * @param colorTool CAF color tool
     * @param baseName Base name for geometry objects
     * @param options Optimization options
     * @param progress Progress callback
     * @return Vector of processed geometry objects with colors and names
     */
    std::vector<std::shared_ptr<OCCGeometry>> processShapesWithCAF(
        const std::vector<TopoDS_Shape>& shapes,
        const TDF_LabelSequence& labels,
        const Handle(XCAFDoc_ShapeTool)& shapeTool,
        const Handle(XCAFDoc_ColorTool)& colorTool,
        const std::string& baseName,
        const OptimizationOptions& options,
        ProgressCallback progress = nullptr
    );

    // Static members for caching
    static std::unordered_map<std::string, ReadResult> s_cache;
    static std::mutex s_cacheMutex;
    static bool s_initialized;
};
