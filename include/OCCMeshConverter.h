#pragma once

#include <vector>
#include <string>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>

// Forward declarations
class SoSeparator;
class SoCoordinate3;
class SoIndexedFaceSet;
class SoNormal;
class Poly_Triangulation;
class TopLoc_Location;

/**
 * @brief OpenCASCADE mesh converter
 * 
 * Converts OpenCASCADE geometry to triangle meshes for rendering and export
 */
class OCCMeshConverter {
public:
    /**
     * @brief Triangle mesh data structure
     */
    struct TriangleMesh {
        std::vector<gp_Pnt> vertices;       // vertex coordinates
        std::vector<int> triangles;         // triangle indices (3 per triangle)
        std::vector<gp_Pnt> normals;        // vertex normals
        std::vector<std::pair<double, double>> uvCoords; // UV texture coordinates
        
        // Statistics
        int getVertexCount() const { return static_cast<int>(vertices.size()); }
        int getTriangleCount() const { return static_cast<int>(triangles.size() / 3); }
        
        void clear() {
            vertices.clear();
            triangles.clear();
            normals.clear();
            uvCoords.clear();
        }
        
        bool isEmpty() const {
            return vertices.empty() || triangles.empty();
        }
    };

    /**
     * @brief Meshing parameters
     */
    struct MeshParameters {
        double deflection;          // mesh deflection
        double angularDeflection;   // angular deflection
        bool relative;              // relative deflection
        bool inParallel;            // parallel computation
        bool internalVerticesMode;  // internal vertices mode
        bool controlSurfaceDeflection; // control surface deflection
        
        MeshParameters() 
            : deflection(0.1)
            , angularDeflection(0.5)
            , relative(false)
            , inParallel(true)
            , internalVerticesMode(true)
            , controlSurfaceDeflection(true) {}
    };

public:
    // Main conversion methods
    static TriangleMesh convertToMesh(const TopoDS_Shape& shape, 
                                      const MeshParameters& params = MeshParameters());
    
    static TriangleMesh convertToMesh(const TopoDS_Shape& shape, double deflection);
    
    // Batch conversion
    static std::vector<TriangleMesh> convertMultipleToMesh(const std::vector<TopoDS_Shape>& shapes,
                                                           const MeshParameters& params = MeshParameters());

    // Coin3D node creation
    static SoSeparator* createCoinNode(const TriangleMesh& mesh);
    static SoSeparator* createCoinNode(const TopoDS_Shape& shape, double deflection = 0.1);
    
    // Update existing Coin3D nodes
    static void updateCoinNode(SoSeparator* node, const TriangleMesh& mesh);
    static void updateCoinNode(SoSeparator* node, const TopoDS_Shape& shape, double deflection = 0.1);

    // Mesh export
    static bool exportToSTL(const TriangleMesh& mesh, const std::string& filename, bool binary = true);
    static bool exportToOBJ(const TriangleMesh& mesh, const std::string& filename);
    static bool exportToPLY(const TriangleMesh& mesh, const std::string& filename, bool binary = true);
    
    // Mesh analysis
    static void calculateBoundingBox(const TriangleMesh& mesh, gp_Pnt& minPt, gp_Pnt& maxPt);
    static double calculateSurfaceArea(const TriangleMesh& mesh);
    static double calculateVolume(const TriangleMesh& mesh);
    static gp_Pnt calculateCentroid(const TriangleMesh& mesh);
    
    // Mesh quality checking
    static bool validateMesh(const TriangleMesh& mesh);
    static std::vector<int> findDegenerateTriangles(const TriangleMesh& mesh);
    static std::vector<int> findDuplicateVertices(const TriangleMesh& mesh, double tolerance = 1e-6);
    
    // Mesh optimization
    static TriangleMesh removeDuplicateVertices(const TriangleMesh& mesh, double tolerance = 1e-6);
    static TriangleMesh removeDegenerateTriangles(const TriangleMesh& mesh);
    static TriangleMesh smoothNormals(const TriangleMesh& mesh, double angleThreshold = 30.0);

    // Utility methods
    static void calculateNormals(TriangleMesh& mesh);
    static void calculateUVCoords(TriangleMesh& mesh);
    static void flipNormals(TriangleMesh& mesh);
    
    // Debug and information
    static void printMeshInfo(const TriangleMesh& mesh);
    static std::string getMeshStatistics(const TriangleMesh& mesh);

    // Control whether edge lines are generated
    static void setShowEdges(bool show);

private:
    static bool s_showEdges;
    static void meshFace(const TopoDS_Shape& face, TriangleMesh& mesh, const MeshParameters& params);
    static void extractTriangulation(const Handle(Poly_Triangulation)& triangulation, 
                                     const TopLoc_Location& location, 
                                     TriangleMesh& mesh, TopAbs_Orientation orientation = TopAbs_FORWARD);
    static void addTriangleToMesh(TriangleMesh& mesh, const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& p3);
    static gp_Pnt calculateTriangleNormal(const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& p3);
    static bool isValidTriangle(const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& p3, double tolerance = 1e-6);
    
    static SoCoordinate3* createCoordinateNode(const TriangleMesh& mesh);
    static SoIndexedFaceSet* createFaceSetNode(const TriangleMesh& mesh);
    static SoNormal* createNormalNode(const TriangleMesh& mesh);
}; 