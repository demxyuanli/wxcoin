#pragma once

#include "GeometryReader.h"
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TopoDS_Face.hxx>
#include <OpenCASCADE/BRep_Builder.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeFace.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakePolygon.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeVertex.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeWire.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeShell.hxx>
#include <OpenCASCADE/BRepBuilderAPI_MakeSolid.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/TopoDS_Compound.hxx>
#include <OpenCASCADE/TopoDS_Builder.hxx>
#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/BRepAdaptor_Surface.hxx>
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <future>
#include <thread>
#include <execution>

/**
 * @brief OBJ file reader for importing 3D models
 *
 * Provides functionality to read OBJ files and convert them to OCCGeometry objects
 * Supports vertices, faces, and basic materials
 */
class OBJReader : public GeometryReader {
public:
    /**
     * @brief Constructor
     */
    OBJReader() = default;

    /**
     * @brief Destructor
     */
    ~OBJReader() override = default;

    /**
     * @brief Read an OBJ file and return geometry objects
     * @param filePath Path to the OBJ file
     * @param options Optimization options
     * @param progress Progress callback function
     * @return ReadResult containing success status and geometry objects
     */
    ReadResult readFile(const std::string& filePath,
        const OptimizationOptions& options = OptimizationOptions(),
        ProgressCallback progress = nullptr) override;

    /**
     * @brief Check if a file has a valid OBJ extension
     * @param filePath Path to check
     * @return true if file has .obj extension
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
     * @brief Vertex structure for OBJ parsing
     */
    struct Vertex {
        double x, y, z;
        Vertex(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
        gp_Pnt toPoint() const { return gp_Pnt(x, y, z); }
    };

    /**
     * @brief Face structure for OBJ parsing
     */
    struct Face {
        std::vector<int> vertexIndices;
        std::vector<int> normalIndices;  // Normal indices for each vertex
        std::string materialName;
        Face() = default;
    };

    /**
     * @brief Material structure for OBJ parsing
     */
    struct Material {
        std::string name;
        double r, g, b; // RGB color
        Material() : r(0.8), g(0.8), b(0.8) {} // Default gray
    };

    /**
     * @brief Parse OBJ file content
     * @param filePath Path to OBJ file
     * @param vertices Output vector of vertices
     * @param faces Output vector of faces
     * @param materials Output map of materials
     * @param progress Progress callback
     * @return true if parsing successful
     */
    bool parseOBJFile(const std::string& filePath,
        std::vector<Vertex>& vertices,
        std::vector<Face>& faces,
        std::vector<Vertex>& normals,
        std::unordered_map<std::string, Material>& materials,
        ProgressCallback progress = nullptr);

    /**
     * @brief Create TopoDS_Shape from parsed OBJ data
     * @param vertices Vector of vertices
     * @param faces Vector of faces
     * @param baseName Base name for geometry
     * @param options Optimization options
     * @return TopoDS_Shape containing the geometry
     */
    TopoDS_Shape createShapeFromOBJData(
        const std::vector<Vertex>& vertices,
        const std::vector<Face>& faces,
        const std::vector<Vertex>& normals,
        const std::string& baseName,
        const OptimizationOptions& options
    );

    /**
     * @brief Create a face from vertex indices
     * @param vertices Vector of vertices
     * @param faceIndices Indices of vertices for the face
     * @return TopoDS_Face or null shape if creation fails
     */
    TopoDS_Shape createFaceFromVertices(
        const std::vector<Vertex>& vertices,
        const std::vector<int>& faceIndices,
        const std::vector<Vertex>& normals,
        const std::vector<int>& normalIndices
    );

    /**
     * @brief Parse MTL file for materials
     * @param mtlFilePath Path to MTL file
     * @param materials Output map of materials
     * @return true if parsing successful
     */
    bool parseMTLFile(const std::string& mtlFilePath,
        std::unordered_map<std::string, Material>& materials);

    // Static members for caching
    static std::unordered_map<std::string, ReadResult> s_cache;
    static std::mutex s_cacheMutex;
};
