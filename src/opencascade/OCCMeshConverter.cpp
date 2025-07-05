#include "OCCMeshConverter.h"
#include "Logger.h"

// OpenCASCADE includes
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Array1OfTriangle.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <gp_Pnt.hxx>
#include <Precision.hxx>

// Coin3D includes
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoShapeHints.h>

// STL includes
#include <fstream>
#include <iostream>
#include <cmath>

OCCMeshConverter::TriangleMesh OCCMeshConverter::convertToMesh(const TopoDS_Shape& shape, 
                                                              const MeshParameters& params)
{
    TriangleMesh mesh;
    
    if (shape.IsNull()) {
        LOG_WRN("Cannot convert null shape to mesh");
        return mesh;
    }
    
    try {
        // Create incremental mesh
        BRepMesh_IncrementalMesh meshGen(shape, params.deflection, params.relative, 
                                         params.angularDeflection, params.inParallel);
        
        if (!meshGen.IsDone()) {
            LOG_ERR("Failed to generate mesh for shape");
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
        
        LOG_INF("Generated mesh with " + std::to_string(mesh.getVertexCount()) + 
                " vertices and " + std::to_string(mesh.getTriangleCount()) + " triangles");
        
    } catch (const std::exception& e) {
        LOG_ERR("Exception in mesh conversion: " + std::string(e.what()));
        mesh.clear();
    }
    
    return mesh;
}

OCCMeshConverter::TriangleMesh OCCMeshConverter::convertToMesh(const TopoDS_Shape& shape, double deflection)
{
    MeshParameters params;
    params.deflection = deflection;
    return convertToMesh(shape, params);
}

SoSeparator* OCCMeshConverter::createCoinNode(const TriangleMesh& mesh)
{
    if (mesh.isEmpty()) {
        LOG_WRN("Cannot create Coin3D node from empty mesh");
        return nullptr;
    }
    
    SoSeparator* root = new SoSeparator;
    root->ref();
    
    // Add shape hints for better rendering
    SoShapeHints* hints = new SoShapeHints;
    hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    hints->shapeType = SoShapeHints::SOLID;
    root->addChild(hints);
    
    // Create coordinate node
    SoCoordinate3* coords = createCoordinateNode(mesh);
    if (coords) {
        root->addChild(coords);
    }
    
    // Create normal node if available
    if (!mesh.normals.empty()) {
        SoNormal* normals = createNormalNode(mesh);
        if (normals) {
            root->addChild(normals);
        }
    }
    
    // Create face set
    SoIndexedFaceSet* faceSet = createFaceSetNode(mesh);
    if (faceSet) {
        root->addChild(faceSet);
    }
    
    root->unrefNoDelete();
    return root;
}

SoSeparator* OCCMeshConverter::createCoinNode(const TopoDS_Shape& shape, double deflection)
{
    TriangleMesh mesh = convertToMesh(shape, deflection);
    return createCoinNode(mesh);
}

void OCCMeshConverter::updateCoinNode(SoSeparator* node, const TriangleMesh& mesh)
{
    if (!node || mesh.isEmpty()) {
        return;
    }
    
    // Remove all children and rebuild
    node->removeAllChildren();
    
    // Add shape hints
    SoShapeHints* hints = new SoShapeHints;
    hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    hints->shapeType = SoShapeHints::SOLID;
    node->addChild(hints);
    
    // Add coordinate node
    SoCoordinate3* coords = createCoordinateNode(mesh);
    if (coords) {
        node->addChild(coords);
    }
    
    // Add normal node if available
    if (!mesh.normals.empty()) {
        SoNormal* normals = createNormalNode(mesh);
        if (normals) {
            node->addChild(normals);
        }
    }
    
    // Add face set
    SoIndexedFaceSet* faceSet = createFaceSetNode(mesh);
    if (faceSet) {
        node->addChild(faceSet);
    }
}

void OCCMeshConverter::calculateNormals(TriangleMesh& mesh)
{
    if (mesh.vertices.empty() || mesh.triangles.empty()) {
        return;
    }
    
    // Initialize normals array
    mesh.normals.resize(mesh.vertices.size());
    for (auto& normal : mesh.normals) {
        normal = gp_Pnt(0, 0, 0);
    }
    
    // Calculate face normals and accumulate at vertices
    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
        int i0 = mesh.triangles[i];
        int i1 = mesh.triangles[i + 1];
        int i2 = mesh.triangles[i + 2];
        
        if (i0 >= 0 && i0 < static_cast<int>(mesh.vertices.size()) &&
            i1 >= 0 && i1 < static_cast<int>(mesh.vertices.size()) &&
            i2 >= 0 && i2 < static_cast<int>(mesh.vertices.size())) {
            
            const gp_Pnt& v0 = mesh.vertices[i0];
            const gp_Pnt& v1 = mesh.vertices[i1];
            const gp_Pnt& v2 = mesh.vertices[i2];
            
            gp_Pnt normal = calculateTriangleNormal(v0, v1, v2);
            
            // Accumulate normal at each vertex
            mesh.normals[i0] = gp_Pnt(
                mesh.normals[i0].X() + normal.X(),
                mesh.normals[i0].Y() + normal.Y(),
                mesh.normals[i0].Z() + normal.Z()
            );
            mesh.normals[i1] = gp_Pnt(
                mesh.normals[i1].X() + normal.X(),
                mesh.normals[i1].Y() + normal.Y(),
                mesh.normals[i1].Z() + normal.Z()
            );
            mesh.normals[i2] = gp_Pnt(
                mesh.normals[i2].X() + normal.X(),
                mesh.normals[i2].Y() + normal.Y(),
                mesh.normals[i2].Z() + normal.Z()
            );
        }
    }
    
    // Normalize the accumulated normals
    for (auto& normal : mesh.normals) {
        double length = std::sqrt(normal.X() * normal.X() + 
                                 normal.Y() * normal.Y() + 
                                 normal.Z() * normal.Z());
        if (length > Precision::Confusion()) {
            normal = gp_Pnt(normal.X() / length, normal.Y() / length, normal.Z() / length);
        }
    }
}

void OCCMeshConverter::meshFace(const TopoDS_Shape& face, TriangleMesh& mesh, const MeshParameters& params)
{
    if (face.ShapeType() != TopAbs_FACE) {
        return;
    }
    
    const TopoDS_Face& topoFace = TopoDS::Face(face);
    TopLoc_Location location;
    Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(topoFace, location);
    
    // If triangulation exists, extract it
    if (!triangulation.IsNull()) {
        extractTriangulation(triangulation, location, mesh);
    } else {
        // If no triangulation exists, create one
        BRepMesh_IncrementalMesh mesher(topoFace, params.deflection, params.relative, params.angularDeflection);
        triangulation = BRep_Tool::Triangulation(topoFace, location);
        if (!triangulation.IsNull()) {
            extractTriangulation(triangulation, location, mesh);
        }
    }
}

void OCCMeshConverter::extractTriangulation(const Handle(Poly_Triangulation)& triangulation, 
                                            const TopLoc_Location& location, 
                                            TriangleMesh& mesh)
{
    if (triangulation.IsNull()) {
        return;
    }
    
    // Get transformation
    gp_Trsf transform = location.Transformation();
    
    // Extract vertices - use new API
    int vertexOffset = static_cast<int>(mesh.vertices.size());
    
    for (int i = 1; i <= triangulation->NbNodes(); i++) {
        gp_Pnt point = triangulation->Node(i);
        point.Transform(transform);
        mesh.vertices.push_back(point);
    }
    
    // Extract triangles
    const Poly_Array1OfTriangle triangles = triangulation->Triangles();
    for (int i = triangles.Lower(); i <= triangles.Upper(); i++) {
        int n1, n2, n3;
        triangles(i).Get(n1, n2, n3);
        
        // Adjust indices to be 0-based and add vertex offset
        mesh.triangles.push_back(vertexOffset + n1 - 1);
        mesh.triangles.push_back(vertexOffset + n2 - 1);
        mesh.triangles.push_back(vertexOffset + n3 - 1);
    }
}

gp_Pnt OCCMeshConverter::calculateTriangleNormal(const gp_Pnt& p1, const gp_Pnt& p2, const gp_Pnt& p3)
{
    gp_Vec v1(p1, p2);
    gp_Vec v2(p1, p3);
    gp_Vec normal = v1.Crossed(v2);
    
    double length = normal.Magnitude();
    if (length > Precision::Confusion()) {
        normal = normal / length;
    }
    
    return gp_Pnt(normal.X(), normal.Y(), normal.Z());
}

SoCoordinate3* OCCMeshConverter::createCoordinateNode(const TriangleMesh& mesh)
{
    if (mesh.vertices.empty()) {
        return nullptr;
    }
    
    SoCoordinate3* coords = new SoCoordinate3;
    coords->point.setNum(static_cast<int>(mesh.vertices.size()));
    
    SbVec3f* points = coords->point.startEditing();
    for (size_t i = 0; i < mesh.vertices.size(); i++) {
        const gp_Pnt& vertex = mesh.vertices[i];
        points[i].setValue(
            static_cast<float>(vertex.X()),
            static_cast<float>(vertex.Y()),
            static_cast<float>(vertex.Z())
        );
    }
    coords->point.finishEditing();
    
    return coords;
}

SoIndexedFaceSet* OCCMeshConverter::createFaceSetNode(const TriangleMesh& mesh)
{
    if (mesh.triangles.empty()) {
        return nullptr;
    }
    
    SoIndexedFaceSet* faceSet = new SoIndexedFaceSet;
    
    // Set up coordinate indices
    int numIndices = static_cast<int>(mesh.triangles.size()) + mesh.getTriangleCount(); // +1 for each triangle separator
    faceSet->coordIndex.setNum(numIndices);
    
    int32_t* indices = faceSet->coordIndex.startEditing();
    int indexPos = 0;
    
    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
        indices[indexPos++] = mesh.triangles[i];
        indices[indexPos++] = mesh.triangles[i + 1];
        indices[indexPos++] = mesh.triangles[i + 2];
        indices[indexPos++] = -1; // Triangle separator
    }
    
    faceSet->coordIndex.finishEditing();
    
    return faceSet;
}

SoNormal* OCCMeshConverter::createNormalNode(const TriangleMesh& mesh)
{
    if (mesh.normals.empty()) {
        return nullptr;
    }
    
    SoNormal* normals = new SoNormal;
    normals->vector.setNum(static_cast<int>(mesh.normals.size()));
    
    SbVec3f* normalVecs = normals->vector.startEditing();
    for (size_t i = 0; i < mesh.normals.size(); i++) {
        const gp_Pnt& normal = mesh.normals[i];
        normalVecs[i].setValue(
            static_cast<float>(normal.X()),
            static_cast<float>(normal.Y()),
            static_cast<float>(normal.Z())
        );
    }
    normals->vector.finishEditing();
    
    return normals;
}

bool OCCMeshConverter::exportToSTL(const TriangleMesh& mesh, const std::string& filename, bool binary)
{
    if (mesh.isEmpty()) {
        LOG_ERR("Cannot export empty mesh to STL");
        return false;
    }
    
    try {
        std::ofstream file(filename, binary ? std::ios::binary : std::ios::out);
        if (!file.is_open()) {
            LOG_ERR("Cannot open file for writing: " + filename);
            return false;
        }
        
        if (binary) {
            // Binary STL format
            char header[80] = {0};
            snprintf(header, sizeof(header), "Binary STL generated by OCCMeshConverter");
            file.write(header, 80);
            
            uint32_t numTriangles = static_cast<uint32_t>(mesh.getTriangleCount());
            file.write(reinterpret_cast<const char*>(&numTriangles), sizeof(uint32_t));
            
            for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
                // Calculate normal
                const gp_Pnt& v0 = mesh.vertices[mesh.triangles[i]];
                const gp_Pnt& v1 = mesh.vertices[mesh.triangles[i + 1]];
                const gp_Pnt& v2 = mesh.vertices[mesh.triangles[i + 2]];
                gp_Pnt normal = calculateTriangleNormal(v0, v1, v2);
                
                // Write normal
                float nx = static_cast<float>(normal.X());
                float ny = static_cast<float>(normal.Y());
                float nz = static_cast<float>(normal.Z());
                file.write(reinterpret_cast<const char*>(&nx), sizeof(float));
                file.write(reinterpret_cast<const char*>(&ny), sizeof(float));
                file.write(reinterpret_cast<const char*>(&nz), sizeof(float));
                
                // Write vertices
                for (int j = 0; j < 3; j++) {
                    const gp_Pnt& vertex = mesh.vertices[mesh.triangles[i + j]];
                    float x = static_cast<float>(vertex.X());
                    float y = static_cast<float>(vertex.Y());
                    float z = static_cast<float>(vertex.Z());
                    file.write(reinterpret_cast<const char*>(&x), sizeof(float));
                    file.write(reinterpret_cast<const char*>(&y), sizeof(float));
                    file.write(reinterpret_cast<const char*>(&z), sizeof(float));
                }
                
                // Write attribute byte count (always 0)
                uint16_t attributeCount = 0;
                file.write(reinterpret_cast<const char*>(&attributeCount), sizeof(uint16_t));
            }
        } else {
            // ASCII STL format
            file << "solid OCCMesh\n";
            
            for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
                const gp_Pnt& v0 = mesh.vertices[mesh.triangles[i]];
                const gp_Pnt& v1 = mesh.vertices[mesh.triangles[i + 1]];
                const gp_Pnt& v2 = mesh.vertices[mesh.triangles[i + 2]];
                gp_Pnt normal = calculateTriangleNormal(v0, v1, v2);
                
                file << "  facet normal " << normal.X() << " " << normal.Y() << " " << normal.Z() << "\n";
                file << "    outer loop\n";
                file << "      vertex " << v0.X() << " " << v0.Y() << " " << v0.Z() << "\n";
                file << "      vertex " << v1.X() << " " << v1.Y() << " " << v1.Z() << "\n";
                file << "      vertex " << v2.X() << " " << v2.Y() << " " << v2.Z() << "\n";
                file << "    endloop\n";
                file << "  endfacet\n";
            }
            
            file << "endsolid OCCMesh\n";
        }
        
        file.close();
        LOG_INF("Successfully exported mesh to STL: " + filename);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERR("Exception while exporting STL: " + std::string(e.what()));
        return false;
    }
}