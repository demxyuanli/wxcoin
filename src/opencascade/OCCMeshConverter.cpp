#include "OCCMeshConverter.h"
#include "logger/Logger.h"

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
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTexture2.h>

// STL includes
#include <fstream>
#include <iostream>
#include <cmath>
#include <set>
#include <vector>
#include <map>

// Forward declaration
static SoIndexedLineSet* createEdgeSetNode(const OCCMeshConverter::TriangleMesh& mesh);

bool OCCMeshConverter::s_showEdges = true;
double OCCMeshConverter::s_featureEdgeAngle = 30.0;

void OCCMeshConverter::setShowEdges(bool show)
{
    s_showEdges = show;
}

void OCCMeshConverter::setFeatureEdgeAngle(double angleDegrees)
{
    s_featureEdgeAngle = angleDegrees;
}

OCCMeshConverter::TriangleMesh OCCMeshConverter::convertToMesh(const TopoDS_Shape& shape, 
                                                              const MeshParameters& params)
{
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
        
        LOG_INF_S("Generated mesh with " + std::to_string(mesh.getVertexCount()) + 
                " vertices and " + std::to_string(mesh.getTriangleCount()) + " triangles");
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception in mesh conversion: " + std::string(e.what()));
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
        LOG_WRN_S("Cannot create Coin3D node from empty mesh");
        return nullptr;
    }
    
    SoSeparator* root = new SoSeparator;
    root->ref();
    
    // Add shape hints for better rendering of imported models
    SoShapeHints* hints = new SoShapeHints;
    hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    hints->shapeType = SoShapeHints::SOLID;
    hints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
    hints->creaseAngle = 0.8f;  // Increase crease angle from 0.5 to 0.8 for smoother shading
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
    
    // Create edge set if showEdges is true
    if (s_showEdges) {
        SoSeparator* edgeGroup = new SoSeparator;

        // Disable texture for this subgraph so edges are not textured
        SoTexture2* disableTexture = new SoTexture2;
        edgeGroup->addChild(disableTexture);

        // Set a black material for the edges
        SoMaterial* edgeMaterial = new SoMaterial;
        edgeMaterial->diffuseColor.setValue(0.0f, 0.0f, 0.0f);
        edgeGroup->addChild(edgeMaterial);

        // Create and add the line geometry
        SoIndexedLineSet* edgeSet = createEdgeSetNode(mesh);
        if (edgeSet) {
            edgeGroup->addChild(edgeSet);
        }

        root->addChild(edgeGroup);
    }
    
    root->unrefNoDelete();
    return root;
}

SoSeparator* OCCMeshConverter::createCoinNode(const TopoDS_Shape& shape, const MeshParameters& params)
{
    TriangleMesh mesh = convertToMesh(shape, params);
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
    
    // Create edge set if showEdges is true
    if (s_showEdges) {
        SoSeparator* edgeGroup = new SoSeparator;

        // Disable texture for this subgraph so edges are not textured
        SoTexture2* disableTexture = new SoTexture2;
        edgeGroup->addChild(disableTexture);

        // Set a black material for the edges
        SoMaterial* edgeMaterial = new SoMaterial;
        edgeMaterial->diffuseColor.setValue(0.0f, 0.0f, 0.0f);
        edgeGroup->addChild(edgeMaterial);

        // Create and add the line geometry
        SoIndexedLineSet* edgeSet = createEdgeSetNode(mesh);
        if (edgeSet) {
            edgeGroup->addChild(edgeSet);
        }

        node->addChild(edgeGroup);
    }
}

void OCCMeshConverter::updateCoinNode(SoSeparator* node, const TopoDS_Shape& shape, const MeshParameters& params)
{
    if (!node) {
        return;
    }
    TriangleMesh mesh = convertToMesh(shape, params);
    updateCoinNode(node, mesh);
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
    
    // Normalize accumulated normals
    for (auto& normal : mesh.normals) {
        double length = sqrt(normal.X() * normal.X() + normal.Y() * normal.Y() + normal.Z() * normal.Z());
        if (length > 1e-6) {
            normal.SetX(normal.X() / length);
            normal.SetY(normal.Y() / length);
            normal.SetZ(normal.Z() / length);
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
        extractTriangulation(triangulation, location, mesh, topoFace.Orientation());
    } else {
        // If no triangulation exists, create one
        BRepMesh_IncrementalMesh mesher(topoFace, params.deflection, params.relative, params.angularDeflection);
        triangulation = BRep_Tool::Triangulation(topoFace, location);
        if (!triangulation.IsNull()) {
            extractTriangulation(triangulation, location, mesh, topoFace.Orientation());
        }
    }
}

void OCCMeshConverter::extractTriangulation(const Handle(Poly_Triangulation)& triangulation, 
                                            const TopLoc_Location& location, 
                                            TriangleMesh& mesh, TopAbs_Orientation orientation)
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
    
    // Extract triangles with proper orientation handling
    const Poly_Array1OfTriangle triangles = triangulation->Triangles();
    for (int i = triangles.Lower(); i <= triangles.Upper(); i++) {
        int n1, n2, n3;
        triangles(i).Get(n1, n2, n3);
        
        // Adjust indices to be 0-based and add vertex offset
        int idx1 = vertexOffset + n1 - 1;
        int idx2 = vertexOffset + n2 - 1;
        int idx3 = vertexOffset + n3 - 1;
        
        // Handle face orientation - reverse triangle winding if face is reversed
        if (orientation == TopAbs_REVERSED) {
            mesh.triangles.push_back(idx1);
            mesh.triangles.push_back(idx3);  // Swap n2 and n3 to reverse winding
            mesh.triangles.push_back(idx2);
        } else {
            mesh.triangles.push_back(idx1);
            mesh.triangles.push_back(idx2);
            mesh.triangles.push_back(idx3);
        }
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

static SoIndexedLineSet* createEdgeSetNode(const OCCMeshConverter::TriangleMesh& mesh)
{
    if (mesh.triangles.empty()) return nullptr;

    std::map<std::pair<int,int>, std::vector<int>> edgeFaces;
    for (size_t i = 0; i + 2 < mesh.triangles.size(); i += 3) {
        int verts[3] = { mesh.triangles[i], mesh.triangles[i+1], mesh.triangles[i+2] };
        auto processEdge = [&](int v1, int v2) {
            if (v1 > v2) std::swap(v1, v2);
            edgeFaces[{v1, v2}].push_back(static_cast<int>(i));
        };
        processEdge(verts[0], verts[1]);
        processEdge(verts[1], verts[2]);
        processEdge(verts[2], verts[0]);
    }

    std::vector<int32_t> indices;
    double threshold = OCCMeshConverter::s_featureEdgeAngle;
    for (auto& kv : edgeFaces) {
        const auto& edge = kv.first;
        const auto& faces = kv.second;
        bool includeEdge = false;
        if (threshold <= 0.0) {
            includeEdge = true;
        } else if (faces.size() == 1) {
            includeEdge = true;
        } else if (faces.size() == 2) {
            int f0 = faces[0], f1 = faces[1];
            int a0 = mesh.triangles[f0], b0 = mesh.triangles[f0+1], c0 = mesh.triangles[f0+2];
            int a1 = mesh.triangles[f1], b1 = mesh.triangles[f1+1], c1 = mesh.triangles[f1+2];
            gp_Pnt n0p = OCCMeshConverter::calculateTriangleNormal(mesh.vertices[a0], mesh.vertices[b0], mesh.vertices[c0]);
            gp_Pnt n1p = OCCMeshConverter::calculateTriangleNormal(mesh.vertices[a1], mesh.vertices[b1], mesh.vertices[c1]);
            gp_Vec n0(n0p.X(), n0p.Y(), n0p.Z());
            gp_Vec n1(n1p.X(), n1p.Y(), n1p.Z());
            double angle = n0.Angle(n1) * 180.0 / acos(-1.0);
            if (angle >= threshold) {
                includeEdge = true;
            }
        } else {
            includeEdge = true;
        }
        if (includeEdge) {
            indices.push_back(edge.first);
            indices.push_back(edge.second);
            indices.push_back(SO_END_LINE_INDEX);
        }
    }

    if (indices.empty()) return nullptr;
    SoIndexedLineSet* lineSet = new SoIndexedLineSet;
    lineSet->coordIndex.setValues(0, static_cast<int>(indices.size()), indices.data());
    return lineSet;
}

bool OCCMeshConverter::exportToSTL(const TriangleMesh& mesh, const std::string& filename, bool binary)
{
    if (mesh.isEmpty()) {
        LOG_ERR_S("Cannot export empty mesh to STL");
        return false;
    }
    
    try {
        std::ofstream file(filename, binary ? std::ios::binary : std::ios::out);
        if (!file.is_open()) {
            LOG_ERR_S("Cannot open file for writing: " + filename);
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
        LOG_INF_S("Successfully exported mesh to STL: " + filename);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("Exception while exporting STL: " + std::string(e.what()));
        return false;
    }
}

void OCCMeshConverter::flipNormals(TriangleMesh& mesh)
{
    if (mesh.isEmpty()) {
        return;
    }
    
    // Flip triangle winding order (reverse vertex order for each triangle)
    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
        if (i + 2 < mesh.triangles.size()) {
            // Swap second and third vertices to flip the triangle
            std::swap(mesh.triangles[i + 1], mesh.triangles[i + 2]);
        }
    }
    
    // Flip normals if they exist
    for (auto& normal : mesh.normals) {
        normal.SetX(-normal.X());
        normal.SetY(-normal.Y());
        normal.SetZ(-normal.Z());
    }
    
    LOG_INF_S("Flipped normals for mesh with " + std::to_string(mesh.getTriangleCount()) + " triangles");
}
