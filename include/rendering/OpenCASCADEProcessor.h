#pragma once

#include "GeometryProcessor.h"
#include <memory>
#include <set>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/gp_Vec.hxx>

/**
 * @brief OpenCASCADE-based geometry processor
 * 
 * Implements geometry processing using OpenCASCADE library
 */
class OpenCASCADEProcessor : public GeometryProcessor {
public:
    OpenCASCADEProcessor();
    virtual ~OpenCASCADEProcessor();

    // GeometryProcessor interface
    TriangleMesh convertToMesh(const TopoDS_Shape& shape, 
                              const MeshParameters& params = MeshParameters()) override;
    
    void calculateNormals(TriangleMesh& mesh) override;
    
    TriangleMesh smoothNormals(const TriangleMesh& mesh, 
                              double creaseAngle = 30.0, 
                              int iterations = 2) override;
    
    TriangleMesh createSubdivisionSurface(const TriangleMesh& mesh, int levels = 2) override;
    
    void flipNormals(TriangleMesh& mesh) override;
    
    std::string getName() const override { return "OpenCASCADE"; }

    // Configuration methods
    void setShowEdges(bool show);
    void setFeatureEdgeAngle(double angleDegrees);
    void setSmoothingEnabled(bool enabled);
    void setSubdivisionEnabled(bool enabled);
    void setSubdivisionLevels(int levels);
    void setCreaseAngle(double angle);

private:
    // Helper methods
    void meshFace(const TopoDS_Shape& face, TriangleMesh& mesh, const MeshParameters& params);
    void extractTriangulation(const void* triangulation, const void* location, 
                             TriangleMesh& mesh, int orientation);
    void subdivideTriangle(TriangleMesh& mesh, const gp_Pnt& p0, const gp_Pnt& p1, 
                          const gp_Pnt& p2, int levels);
    std::set<std::pair<int, int>> findBoundaryEdges(const TriangleMesh& mesh);
    gp_Vec calculateTriangleNormalVec(const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& p3);

    // Configuration
    bool m_showEdges;
    double m_featureEdgeAngle;
    bool m_smoothingEnabled;
    bool m_subdivisionEnabled;
    int m_subdivisionLevels;
    double m_creaseAngle;
}; 