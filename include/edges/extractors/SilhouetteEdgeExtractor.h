#pragma once

#include "BaseEdgeExtractor.h"
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/TopoDS_Edge.hxx>
#include <OpenCASCADE/TopoDS_Face.hxx>

/**
 * @brief Parameters for silhouette edge extraction
 */
struct SilhouetteEdgeParams {
    gp_Pnt cameraPosition;
    double tolerance = 1e-6;           // Angle tolerance for silhouette detection
    
    SilhouetteEdgeParams() = default;
    SilhouetteEdgeParams(const gp_Pnt& camPos, double tol = 1e-6)
        : cameraPosition(camPos), tolerance(tol) {}
};

/**
 * @brief Silhouette edge extractor
 * 
 * Extracts silhouette edges based on camera viewpoint
 * and face normal visibility
 */
class SilhouetteEdgeExtractor : public TypedEdgeExtractor<SilhouetteEdgeParams> {
public:
    SilhouetteEdgeExtractor();
    ~SilhouetteEdgeExtractor() = default;
    
    // BaseEdgeExtractor interface
    bool canExtract(const TopoDS_Shape& shape) const override;
    const char* getName() const override { return "SilhouetteEdgeExtractor"; }
    
protected:
    std::vector<gp_Pnt> extractTyped(const TopoDS_Shape& shape, const SilhouetteEdgeParams* params) override;
    
private:
    /**
     * @brief Check if edge is silhouette from camera viewpoint
     */
    bool isSilhouetteEdge(
        const TopoDS_Edge& edge,
        const TopoDS_Face& face1,
        const TopoDS_Face& face2,
        const gp_Pnt& cameraPos,
        double tolerance);
    
    /**
     * @brief Calculate face normal at edge midpoint
     */
    gp_Vec calculateFaceNormal(const TopoDS_Face& face, const TopoDS_Edge& edge) const;
};
