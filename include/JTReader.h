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
#include <OpenCASCADE/Geom_Plane.hxx>
#include <OpenCASCADE/GeomAPI_PointsToBSpline.hxx>
#include <OpenCASCADE/GeomAPI_Interpolate.hxx>
#include <OpenCASCADE/TColgp_Array1OfPnt.hxx>
#include <vector>
#include <string>
#include <fstream>
#include <memory>

/**
 * @brief JT (Jupiter Tessellation) file reader for importing 3D models
 *
 * Provides functionality to read JT files and convert them to OCCGeometry objects
 * JT is Siemens' proprietary format for 3D data exchange
 * 
 * Note: This implementation provides a basic framework for JT support.
 * For full JT support, consider integrating with Siemens JT Open Toolkit
 * or other specialized JT libraries.
 */
class JTReader : public GeometryReader {
public:
    /**
     * @brief Constructor
     */
    JTReader() = default;

    /**
     * @brief Destructor
     */
    ~JTReader() override = default;

    /**
     * @brief Read a JT file and return geometry objects
     * @param filePath Path to the JT file
     * @param options Optimization options
     * @param progress Progress callback function
     * @return ReadResult containing success status and geometry objects
     */
    ReadResult readFile(const std::string& filePath,
        const OptimizationOptions& options = OptimizationOptions(),
        ProgressCallback progress = nullptr) override;

    /**
     * @brief Check if a file has a valid JT extension
     * @param filePath Path to check
     * @return true if file has .jt extension
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
     * @brief JT file format version
     */
    enum class JTFormat {
        JT8_0,
        JT8_1,
        JT9_0,
        JT9_1,
        JT9_2,
        JT9_3,
        JT9_4,
        JT9_5,
        JT10_0,
        JT10_1,
        JT10_2,
        JT10_3,
        JT10_4,
        JT10_5,
        Unknown
    };

    /**
     * @brief Triangle structure for JT parsing
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
     * @brief JT mesh data structure
     */
    struct JTMesh {
        std::vector<gp_Pnt> vertices;
        std::vector<Triangle> triangles;
        std::string name;
        gp_Pnt center;
        double boundingRadius;
        
        JTMesh() : center(0, 0, 0), boundingRadius(0) {}
    };

    /**
     * @brief Detect JT file format version
     * @param filePath Path to JT file
     * @return Detected format version
     */
    JTFormat detectFormat(const std::string& filePath);

    /**
     * @brief Parse JT file header
     * @param filePath Path to JT file
     * @param format Output detected format
     * @param progress Progress callback
     * @return true if header parsing successful
     */
    bool parseHeader(const std::string& filePath, JTFormat& format, ProgressCallback progress = nullptr);

    /**
     * @brief Parse JT file content (basic implementation)
     * @param filePath Path to JT file
     * @param meshes Output vector of meshes
     * @param progress Progress callback
     * @return true if parsing successful
     */
    bool parseJTFile(const std::string& filePath,
        std::vector<JTMesh>& meshes,
        ProgressCallback progress = nullptr);

    /**
     * @brief Parse JT file using basic binary format detection
     * @param filePath Path to JT file
     * @param meshes Output vector of meshes
     * @param progress Progress callback
     * @return true if parsing successful
     */
    bool parseBasicJT(const std::string& filePath,
        std::vector<JTMesh>& meshes,
        ProgressCallback progress = nullptr);

    /**
     * @brief Create TopoDS_Shape from parsed JT data
     * @param meshes Vector of meshes
     * @param baseName Base name for geometry
     * @param options Optimization options
     * @return TopoDS_Shape containing the geometry
     */
    TopoDS_Shape createShapeFromJTData(
        const std::vector<JTMesh>& meshes,
        const std::string& baseName,
        const OptimizationOptions& options
    );

    /**
     * @brief Create a face from triangle
     * @param triangle Triangle data
     * @return TopoDS_Face or null shape if creation fails
     */
    TopoDS_Shape createFaceFromTriangle(const Triangle& triangle);

    /**
     * @brief Read binary data from file
     * @param file Input file stream
     * @param data Pointer to data buffer
     * @param size Number of bytes to read
     * @return true if read successful
     */
    bool readBinaryData(std::ifstream& file, void* data, size_t size);

    /**
     * @brief Convert JT format enum to string
     * @param format JT format
     * @return String representation
     */
    std::string formatToString(JTFormat format);

    // Static members for caching
    static std::unordered_map<std::string, ReadResult> s_cache;
    static std::mutex s_cacheMutex;
};
