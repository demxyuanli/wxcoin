#pragma once

#include <vector>
#include <string>
#include <map>
#include <set>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Vec.hxx>
#include <OpenCASCADE/TopAbs_Orientation.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "rendering/GeometryProcessor.h"

// Forward declarations
class SoSeparator;
class SoCoordinate3;
class SoIndexedFaceSet;
class SoIndexedLineSet;
class SoNormal;
class Poly_Triangulation;
class TopLoc_Location;

/**
 * @brief OpenCASCADE mesh converter with geometric smoothing
 * 
 * Converts OpenCASCADE geometry to triangle meshes with advanced smoothing capabilities
 */
class OCCMeshConverter {
public:
    /**
     * @brief Meshing parameters
     */
    struct MeshParameters {
        double deflection;          // mesh deflection
        double angularDeflection;   // angular deflection
        bool relative;              // relative deflection
        bool inParallel;            // parallel computation
        
        MeshParameters() 
            : deflection(0.1)
            , angularDeflection(0.5)
            , relative(false)
            , inParallel(true) {}
    };

public:
    // Main conversion methods
    static TriangleMesh convertToMesh(const TopoDS_Shape& shape, 
                                      const MeshParameters& params = MeshParameters());
    static TriangleMesh convertToMesh(const TopoDS_Shape& shape, double deflection);



    // Geometric smoothing methods
    /**
     * @brief FreeCAD-style angle threshold normal averaging
     * 
     * Implements FreeCAD's angle threshold normal averaging algorithm with 4-step logic:
     * 1. Build adjacency relationships (vertex -> adjacent faces -> face normals)
     * 2. Calculate face normals using cross product
     * 3. Apply angle threshold filtering (only faces within threshold participate)
     * 4. Perform iterative weighted averaging with boundary protection
     * 
     * @param mesh Input triangle mesh
     * @param creaseAngle Angle threshold in degrees (default 30.0)
     * @param iterations Number of smoothing iterations (default 2, supports <=2)
     * @return Smoothed triangle mesh
     */
    static TriangleMesh smoothNormals(const TriangleMesh& mesh, double creaseAngle = 30.0, int iterations = 2);
    static TriangleMesh createSubdivisionSurface(const TriangleMesh& mesh, int levels = 2);
    
    // Utility methods
    static void calculateNormals(TriangleMesh& mesh);
    static void flipNormals(TriangleMesh& mesh);
    
    // Control settings
    static void setShowEdges(bool show);
    static void setFeatureEdgeAngle(double angleDegrees);
    static void setSmoothingEnabled(bool enabled);
    static void setSubdivisionEnabled(bool enabled);
    static void setSubdivisionLevels(int levels);
    static void setCreaseAngle(double angle);

    // Calculate triangle face normal
    static gp_Vec calculateTriangleNormalVec(const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& p3);

public:
    // Static member variables for configuration
    static bool s_showEdges;
    static double s_featureEdgeAngle;
    static bool s_smoothingEnabled;
    static bool s_subdivisionEnabled;
    static int s_subdivisionLevels;
    static double s_creaseAngle;

private:
    
    // Helper methods
    static void meshFace(const TopoDS_Shape& face, TriangleMesh& mesh, const MeshParameters& params);
    static void extractTriangulation(const Handle(Poly_Triangulation)& triangulation, 
                                     const TopLoc_Location& location, 
                                     TriangleMesh& mesh, TopAbs_Orientation orientation = TopAbs_FORWARD);
    
    // Smoothing helper methods
    static void subdivideTriangle(TriangleMesh& mesh, const gp_Pnt& p0, const gp_Pnt& p1, const gp_Pnt& p2, int levels);
    static std::set<std::pair<int, int>> findBoundaryEdges(const TriangleMesh& mesh);
};