#include "rendering/OpenCASCADEProcessor.h"
#include "logger/Logger.h"
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <Precision.hxx>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

OpenCASCADEProcessor::OpenCASCADEProcessor() 
    : m_showEdges(true)
    , m_featureEdgeAngle(45.0)
    , m_smoothingEnabled(true)
    , m_subdivisionEnabled(false)
    , m_subdivisionLevels(2)
    , m_creaseAngle(30.0) {
    LOG_INF_S("OpenCASCADEProcessor created");
}

OpenCASCADEProcessor::~OpenCASCADEProcessor() {
    LOG_INF_S("OpenCASCADEProcessor destroyed");
}

TriangleMesh OpenCASCADEProcessor::convertToMesh(const TopoDS_Shape& shape, 
                                                const MeshParameters& params) {
    TriangleMesh mesh;

    if (shape.IsNull()) {
        LOG_WRN_S("Cannot convert null shape to mesh");
        return mesh;
    }

    try {
        // Create incremental mesh
        BRepMesh_IncrementalMesh meshGen(shape, params.deflection, params.relative,
            params.angularDeflection, params.inParallel);

        if (!meshGen.IsDone()) {
            LOG_ERR_S("Failed to generate mesh for shape");
            return mesh;
        }

        // Extract triangles from all faces
        TopExp_Explorer faceExplorer(shape, TopAbs_FACE);
        for (; faceExplorer.More(); faceExplorer.Next()) {
            const TopoDS_Face& face = TopoDS::Face(faceExplorer.Current());
            meshFace(face, mesh, params);
        }

        // Calculate normals if not already done
        if (mesh.normals.empty() && !mesh.vertices.empty()) {
            calculateNormals(mesh);
        }

        // Apply smoothing if enabled
        if (m_smoothingEnabled) {
            mesh = smoothNormals(mesh, m_creaseAngle, 2);
        }

        // Apply subdivision if enabled
        if (m_subdivisionEnabled) {
            mesh = createSubdivisionSurface(mesh, m_subdivisionLevels);
        }

        LOG_INF_S("Generated mesh with " + std::to_string(mesh.getVertexCount()) +
            " vertices and " + std::to_string(mesh.getTriangleCount()) + " triangles");

    }
    catch (const std::exception& e) {
        LOG_ERR_S("Exception in mesh conversion: " + std::string(e.what()));
        mesh.clear();
    }

    return mesh;
}

void OpenCASCADEProcessor::calculateNormals(TriangleMesh& mesh) {
    // This is a placeholder implementation
    LOG_WRN_S("OpenCASCADEProcessor::calculateNormals not fully implemented yet");
}

TriangleMesh OpenCASCADEProcessor::smoothNormals(const TriangleMesh& mesh, 
                                                double creaseAngle, 
                                                int iterations) {
    // This is a placeholder implementation
    LOG_WRN_S("OpenCASCADEProcessor::smoothNormals not fully implemented yet");
    
    TriangleMesh result = mesh;
    return result;
}

TriangleMesh OpenCASCADEProcessor::createSubdivisionSurface(const TriangleMesh& mesh, int levels) {
    // This is a placeholder implementation
    LOG_WRN_S("OpenCASCADEProcessor::createSubdivisionSurface not fully implemented yet");
    
    TriangleMesh result = mesh;
    return result;
}

void OpenCASCADEProcessor::flipNormals(TriangleMesh& mesh) {
    // This is a placeholder implementation
    LOG_WRN_S("OpenCASCADEProcessor::flipNormals not fully implemented yet");
}

void OpenCASCADEProcessor::setShowEdges(bool show) {
    m_showEdges = show;
}

void OpenCASCADEProcessor::setFeatureEdgeAngle(double angleDegrees) {
    m_featureEdgeAngle = angleDegrees;
}

void OpenCASCADEProcessor::setSmoothingEnabled(bool enabled) {
    m_smoothingEnabled = enabled;
}

void OpenCASCADEProcessor::setSubdivisionEnabled(bool enabled) {
    m_subdivisionEnabled = enabled;
}

void OpenCASCADEProcessor::setSubdivisionLevels(int levels) {
    m_subdivisionLevels = levels;
}

void OpenCASCADEProcessor::setCreaseAngle(double angle) {
    m_creaseAngle = angle;
}

void OpenCASCADEProcessor::meshFace(const TopoDS_Shape& face, TriangleMesh& mesh, const MeshParameters& params) {
    if (face.ShapeType() != TopAbs_FACE) {
        return;
    }

    const TopoDS_Face& topoFace = TopoDS::Face(face);
    TopLoc_Location location;
    Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(topoFace, location);

    // If triangulation exists, extract it
    if (!triangulation.IsNull()) {
        extractTriangulation(&triangulation, &location, mesh, static_cast<int>(topoFace.Orientation()));
    }
    else {
        // If no triangulation exists, create one
        BRepMesh_IncrementalMesh mesher(topoFace, params.deflection, params.relative, params.angularDeflection,
            params.inParallel);
        triangulation = BRep_Tool::Triangulation(topoFace, location);
        if (!triangulation.IsNull()) {
            extractTriangulation(&triangulation, &location, mesh, static_cast<int>(topoFace.Orientation()));
        }
    }
}

void OpenCASCADEProcessor::extractTriangulation(const void* triangulation,
    const void* location,
    TriangleMesh& mesh, int orientation) {
    if (!triangulation) {
        return;
    }

    // Cast to proper types
    const Handle(Poly_Triangulation)& triang = *static_cast<const Handle(Poly_Triangulation)*>(triangulation);
    const TopLoc_Location& loc = *static_cast<const TopLoc_Location*>(location);
    
    if (triang.IsNull()) {
        return;
    }

    // Get transformation
    gp_Trsf transform = loc.Transformation();

    // Extract vertices
    int vertexOffset = static_cast<int>(mesh.vertices.size());

    for (int i = 1; i <= triang->NbNodes(); i++) {
        gp_Pnt point = triang->Node(i);
        point.Transform(transform);
        mesh.vertices.push_back(point);
    }

    // Extract triangles with proper orientation handling
    const Poly_Array1OfTriangle& triangles = triang->Triangles();
    for (int i = triangles.Lower(); i <= triangles.Upper(); i++) {
        int n1, n2, n3;
        triangles.Value(i).Get(n1, n2, n3);

        // Adjust indices to be 0-based and add vertex offset
        int idx1 = vertexOffset + n1 - 1;
        int idx2 = vertexOffset + n2 - 1;
        int idx3 = vertexOffset + n3 - 1;

        // Handle face orientation - reverse triangle winding if face is reversed
        if (orientation == static_cast<int>(TopAbs_REVERSED)) {
            mesh.triangles.push_back(idx1);
            mesh.triangles.push_back(idx3);  // Swap n2 and n3 to reverse winding
            mesh.triangles.push_back(idx2);
        }
        else {
            mesh.triangles.push_back(idx1);
            mesh.triangles.push_back(idx2);
            mesh.triangles.push_back(idx3);
        }
    }
}



gp_Vec OpenCASCADEProcessor::calculateTriangleNormalVec(const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& p3) {
    gp_Vec v1(p1, p2);
    gp_Vec v2(p1, p3);
    gp_Vec normal = v1.Crossed(v2);

    double length = normal.Magnitude();
    if (length > Precision::Confusion()) {
        normal = normal / length;
    }

    return normal;
} 