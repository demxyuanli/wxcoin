#pragma once

#include "GeometryReader.h"
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TopoDS_Face.hxx>
#include <OpenCASCADE/BRep_Builder.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeFace.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakePolygon.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Vec.hxx>
#include <OpenCASCADE/TopoDS_Compound.hxx>
#include <OpenCASCADE/TopoDS_Builder.hxx>
#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/Geom_Plane.hxx>
#include <OpenCASCADE/GeomAPI_PointsToBSpline.hxx>
#include <OpenCASCADE/GeomAPI_Interpolate.hxx>
#include <OpenCASCADE/TColgp_Array1OfPnt.hxx>
#include <vector>
#include <string>
#include <fstream>
#include <mutex>
#include <future>
#include <thread>
#include <execution>

/**
 * @brief STL file reader for importing 3D models
 *
 * Provides functionality to read STL files (both ASCII and binary) and convert them to OCCGeometry objects
 * Supports triangular mesh data with normals
 */
class STLReader : public GeometryReader {
public:
    /**
     * @brief Constructor
     */
    STLReader() = default;

    /**
     * @brief Destructor
     */
    ~STLReader() override = default;

    /**
     * @brief Read an STL file and return geometry objects
     * @param filePath Path to the STL file
     * @param options Optimization options
     * @param progress Progress callback function
     * @return ReadResult containing success status and geometry objects
     */
    ReadResult readFile(const std::string& filePath,
        const OptimizationOptions& options = OptimizationOptions(),
        ProgressCallback progress = nullptr) override;

    /**
     * @brief Check if a file has a valid STL extension
     * @param filePath Path to check
     * @return true if file has .stl extension
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
     * @brief Triangle structure for STL parsing
     */
    struct Triangle {
        gp_Vec normal;
        gp_Pnt vertices[3];
        
        Triangle() = default;
        Triangle(const gp_Vec& n, const gp_Pnt& v1, const gp_Pnt& v2, const gp_Pnt& v3)
            : normal(n)
        {
            vertices[0] = v1;
            vertices[1] = v2;
            vertices[2] = v3;
        }
    };

    /**
     * @brief STL file format type
     */
    enum class STLFormat {
        ASCII,
        Binary,
        Unknown
    };

    /**
     * @brief Detect STL file format
     * @param filePath Path to STL file
     * @return Detected format
     */
    STLFormat detectFormat(const std::string& filePath);

    /**
     * @brief Parse ASCII STL file
     * @param filePath Path to STL file
     * @param triangles Output vector of triangles
     * @param progress Progress callback
     * @return true if parsing successful
     */
    bool parseASCIISTL(const std::string& filePath,
        std::vector<Triangle>& triangles,
        ProgressCallback progress = nullptr);

    /**
     * @brief Parse binary STL file
     * @param filePath Path to STL file
     * @param triangles Output vector of triangles
     * @param progress Progress callback
     * @return true if parsing successful
     */
    bool parseBinarySTL(const std::string& filePath,
        std::vector<Triangle>& triangles,
        ProgressCallback progress = nullptr);

    /**
     * @brief Create TopoDS_Shape from parsed STL data
     * @param triangles Vector of triangles
     * @param baseName Base name for geometry
     * @param options Optimization options
     * @return TopoDS_Shape containing the geometry
     */
    TopoDS_Shape createShapeFromSTLData(
        const std::vector<Triangle>& triangles,
        const std::string& baseName,
        const OptimizationOptions& options
    );

    /**
     * @brief Create a face from triangle
     * @param triangle Triangle data
     * @return TopoDS_Face or null shape if creation fails
     */
    TopoDS_Face createFaceFromTriangle(const Triangle& triangle);

    /**
     * @brief Create a face from triangle with model center for better normal orientation
     * @param triangle Triangle data
     * @param modelCenter Center point of the model for normal direction analysis
     * @return TopoDS_Face or null shape if creation fails
     */
    TopoDS_Face createFaceFromTriangle(const Triangle& triangle, const gp_Pnt& modelCenter);

    /**
     * @brief Read binary data from file
     * @param file Input file stream
     * @param data Pointer to data buffer
     * @param size Number of bytes to read
     * @return true if read successful
     */
    bool readBinaryData(std::ifstream& file, void* data, size_t size);

    // Static members for caching
    static std::unordered_map<std::string, ReadResult> s_cache;
    static std::mutex s_cacheMutex;
};
