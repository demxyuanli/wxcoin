#pragma once

#include <string>
#include <vector>
#include <OpenCASCADE/TopoDS_Shape.hxx>

// Forward declarations
class SoSeparator;
class SoCoordinate3;
class SoIndexedFaceSet;

/**
 * @brief OpenCASCADE BREP converter
 * 
 * Converts between OpenCASCADE geometry and other formats
 */
class OCCBrepConverter {
public:
    // File I/O
    static bool saveToSTEP(const TopoDS_Shape& shape, const std::string& filename);
    static bool saveToIGES(const TopoDS_Shape& shape, const std::string& filename);
    static bool saveToBREP(const TopoDS_Shape& shape, const std::string& filename);
    static bool saveToSTL(const TopoDS_Shape& shape, const std::string& filename, bool asciiMode = false);
    static bool saveToVRML(const TopoDS_Shape& shape, const std::string& filename);
    
    static TopoDS_Shape loadFromSTEP(const std::string& filename);
    static TopoDS_Shape loadFromIGES(const std::string& filename);
    static TopoDS_Shape loadFromBREP(const std::string& filename);
    
    // Multi-shape file support
    static std::vector<TopoDS_Shape> loadMultipleFromSTEP(const std::string& filename);
    static std::vector<TopoDS_Shape> loadMultipleFromIGES(const std::string& filename);
    
    // Coin3D conversion
    static SoSeparator* convertToCoin3D(const TopoDS_Shape& shape, double deflection = 0.1);
    static void updateCoin3DNode(const TopoDS_Shape& shape, SoSeparator* node, double deflection = 0.1);
    
    // Meshing
    struct MeshData {
        std::vector<float> vertices;    // x,y,z coordinates
        std::vector<int> indices;       // triangle indices
        std::vector<float> normals;     // normal vectors
        std::vector<float> uvs;         // texture coordinates (optional)
    };
    
    static MeshData convertToMesh(const TopoDS_Shape& shape, double deflection = 0.1);
    static bool exportMeshToOBJ(const MeshData& mesh, const std::string& filename);
    
    // Shape information extraction
    static int getVertexCount(const TopoDS_Shape& shape);
    static int getEdgeCount(const TopoDS_Shape& shape);
    static int getFaceCount(const TopoDS_Shape& shape);
    static int getSolidCount(const TopoDS_Shape& shape);
    
    // Mass properties
    static double calculateVolume(const TopoDS_Shape& shape);
    static double calculateSurfaceArea(const TopoDS_Shape& shape);
    static void calculateCenterOfMass(const TopoDS_Shape& shape, double& x, double& y, double& z);
    static void calculateMomentOfInertia(const TopoDS_Shape& shape, 
                                         double& ixx, double& iyy, double& izz,
                                         double& ixy, double& ixz, double& iyz);

private:
    static void addShapeToNode(const TopoDS_Shape& shape, SoSeparator* node, double deflection);
    static SoCoordinate3* createCoordinates(const MeshData& mesh);
    static SoIndexedFaceSet* createFaceSet(const MeshData& mesh);
}; 