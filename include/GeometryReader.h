#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include "OCCGeometry.h"

/**
 * @brief Base interface for all geometry file readers
 * 
 * Provides a common interface for reading different geometry formats
 * and converting them to OCCGeometry objects
 */
class GeometryReader {
public:
    using ProgressCallback = std::function<void(int /*percent*/, const std::string& /*stage*/)>;

    /**
     * @brief Result structure for geometry file reading
     */
    struct ReadResult {
        bool success;
        std::string errorMessage;
        std::vector<std::shared_ptr<OCCGeometry>> geometries;
        TopoDS_Shape rootShape;
        double importTime; // Time taken for import in milliseconds
        std::string formatName; // Name of the format that was read

        ReadResult() : success(false), importTime(0.0) {}
    };

    /**
     * @brief Optimization options for geometry import
     */
    struct OptimizationOptions {
        bool enableParallelProcessing = true;
        bool enableShapeAnalysis = false;
        bool enableCaching = true;
        bool enableBatchOperations = true;
        int maxThreads = 4;
        double precision = 0.01;
        double meshDeflection = 0.1;
        double angularDeflection = 0.1;

        OptimizationOptions() = default;
    };

    /**
     * @brief Virtual destructor
     */
    virtual ~GeometryReader() = default;

    /**
     * @brief Read a geometry file and return geometry objects
     * @param filePath Path to the geometry file
     * @param options Optimization options
     * @param progress Progress callback function
     * @return ReadResult containing success status and geometry objects
     */
    virtual ReadResult readFile(const std::string& filePath,
        const OptimizationOptions& options = OptimizationOptions(),
        ProgressCallback progress = nullptr) = 0;

    /**
     * @brief Check if a file has a valid extension for this reader
     * @param filePath Path to check
     * @return true if file has valid extension
     */
    virtual bool isValidFile(const std::string& filePath) const = 0;

    /**
     * @brief Get the file extensions supported by this reader
     * @return Vector of supported extensions (e.g., {".step", ".stp"})
     */
    virtual std::vector<std::string> getSupportedExtensions() const = 0;

    /**
     * @brief Get the format name
     * @return Human-readable format name
     */
    virtual std::string getFormatName() const = 0;

    /**
     * @brief Get the file filter string for file dialogs
     * @return File filter string (e.g., "STEP files (*.step;*.stp)|*.step;*.stp")
     */
    virtual std::string getFileFilter() const = 0;

protected:
    /**
     * @brief Helper function to create OCCGeometry from TopoDS_Shape
     * @param shape The shape to convert
     * @param name Name for the geometry object
     * @param options Optimization options
     * @return Shared pointer to OCCGeometry
     */
    static std::shared_ptr<OCCGeometry> createGeometryFromShape(
        const TopoDS_Shape& shape,
        const std::string& name,
        const OptimizationOptions& options
    );

    /**
     * @brief Helper function to validate file exists and is readable
     * @param filePath Path to validate
     * @param errorMessage Output error message if validation fails
     * @return true if file is valid
     */
    static bool validateFile(const std::string& filePath, std::string& errorMessage);
};

/**
 * @brief Factory class for creating geometry readers
 */
class GeometryReaderFactory {
public:
    /**
     * @brief Get all available geometry readers
     * @return Vector of reader instances
     */
    static std::vector<std::unique_ptr<GeometryReader>> getAllReaders();

    /**
     * @brief Get reader for specific file extension
     * @param extension File extension (e.g., ".step", ".obj")
     * @return Reader instance or nullptr if not supported
     */
    static std::unique_ptr<GeometryReader> getReaderForExtension(const std::string& extension);

    /**
     * @brief Get reader for specific file path
     * @param filePath Path to analyze
     * @return Reader instance or nullptr if not supported
     */
    static std::unique_ptr<GeometryReader> getReaderForFile(const std::string& filePath);

    /**
     * @brief Get combined file filter for all supported formats
     * @return Combined file filter string
     */
    static std::string getAllSupportedFileFilter();

    /**
     * @brief Get all supported file extensions
     * @return Vector of all supported extensions
     */
    static std::vector<std::string> getAllSupportedExtensions();
};
