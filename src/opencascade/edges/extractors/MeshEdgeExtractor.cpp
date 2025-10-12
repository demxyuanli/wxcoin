#include "edges/extractors/MeshEdgeExtractor.h"
#include "logger/Logger.h"
#include <map>
#include <set>

MeshEdgeExtractor::MeshEdgeExtractor() {}

bool MeshEdgeExtractor::canExtract(const TopoDS_Shape& shape) const {
    // Always can extract if mesh is provided via params
    return true;
}

std::vector<gp_Pnt> MeshEdgeExtractor::extractTyped(
    const TopoDS_Shape& shape, 
    const MeshEdgeParams* params) {
    
    if (!params) {
        LOG_WRN_S("MeshEdgeExtractor: No mesh parameters provided");
        return {};
    }
    
    if (params->extractBoundaryOnly) {
        return extractBoundaryEdges(params->mesh);
    } else {
        return extractAllMeshEdges(params->mesh);
    }
}

std::vector<gp_Pnt> MeshEdgeExtractor::extractAllMeshEdges(const TriangleMesh& mesh) {
    std::vector<gp_Pnt> points;
    
    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
        int v1 = mesh.triangles[i];
        int v2 = mesh.triangles[i + 1];
        int v3 = mesh.triangles[i + 2];
        
        if (v1 < static_cast<int>(mesh.vertices.size()) && 
            v2 < static_cast<int>(mesh.vertices.size()) && 
            v3 < static_cast<int>(mesh.vertices.size())) {
            
            // Triangle edges
            points.push_back(mesh.vertices[v1]);
            points.push_back(mesh.vertices[v2]);
            points.push_back(mesh.vertices[v2]);
            points.push_back(mesh.vertices[v3]);
            points.push_back(mesh.vertices[v3]);
            points.push_back(mesh.vertices[v1]);
        }
    }
    
    return points;
}

std::vector<gp_Pnt> MeshEdgeExtractor::extractBoundaryEdges(const TriangleMesh& mesh) {
    std::set<std::pair<int, int>> boundaryEdges;
    findBoundaryEdges(mesh, boundaryEdges);
    
    std::vector<gp_Pnt> points;
    for (const auto& edge : boundaryEdges) {
        if (edge.first < static_cast<int>(mesh.vertices.size()) &&
            edge.second < static_cast<int>(mesh.vertices.size())) {
            points.push_back(mesh.vertices[edge.first]);
            points.push_back(mesh.vertices[edge.second]);
        }
    }
    
    return points;
}

void MeshEdgeExtractor::findBoundaryEdges(
    const TriangleMesh& mesh, 
    std::set<std::pair<int, int>>& boundaryEdges) {
    
    std::map<std::pair<int, int>, int> edgeCount;
    
    // Count edge occurrences
    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
        int v1 = mesh.triangles[i];
        int v2 = mesh.triangles[i + 1];
        int v3 = mesh.triangles[i + 2];
        
        auto makeEdge = [](int a, int b) {
            return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
        };
        
        edgeCount[makeEdge(v1, v2)]++;
        edgeCount[makeEdge(v2, v3)]++;
        edgeCount[makeEdge(v3, v1)]++;
    }
    
    // Boundary edges appear only once
    for (const auto& entry : edgeCount) {
        if (entry.second == 1) {
            boundaryEdges.insert(entry.first);
        }
    }
}


