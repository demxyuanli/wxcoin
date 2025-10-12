#pragma once

#include "BaseEdgeExtractor.h"
#include "rendering/GeometryProcessor.h"
#include <set>
#include <utility>

/**
 * @brief Parameters for mesh edge extraction
 */
struct MeshEdgeParams {
    TriangleMesh mesh;
    bool extractBoundaryOnly = false;  // Only boundary edges
    bool extractInternalEdges = true;  // Include internal edges
    
    MeshEdgeParams() = default;
    MeshEdgeParams(const TriangleMesh& m, bool boundaryOnly = false, bool internal = true)
        : mesh(m), extractBoundaryOnly(boundaryOnly), extractInternalEdges(internal) {}
};

/**
 * @brief Mesh edge extractor
 * 
 * Extracts edges from triangulated mesh data
 */
class MeshEdgeExtractor : public TypedEdgeExtractor<MeshEdgeParams> {
public:
    MeshEdgeExtractor();
    ~MeshEdgeExtractor() = default;
    
    // BaseEdgeExtractor interface
    bool canExtract(const TopoDS_Shape& shape) const override;
    const char* getName() const override { return "MeshEdgeExtractor"; }
    
protected:
    std::vector<gp_Pnt> extractTyped(const TopoDS_Shape& shape, const MeshEdgeParams* params) override;
    
private:
    /**
     * @brief Extract all mesh edges (boundary + internal)
     */
    std::vector<gp_Pnt> extractAllMeshEdges(const TriangleMesh& mesh);
    
    /**
     * @brief Extract only boundary edges
     */
    std::vector<gp_Pnt> extractBoundaryEdges(const TriangleMesh& mesh);
    
    /**
     * @brief Find boundary edges by edge frequency
     */
    void findBoundaryEdges(const TriangleMesh& mesh, std::set<std::pair<int, int>>& boundaryEdges);
};
