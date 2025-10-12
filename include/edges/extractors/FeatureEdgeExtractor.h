#pragma once

#include "BaseEdgeExtractor.h"
#include <OpenCASCADE/TopoDS_Edge.hxx>
#include <OpenCASCADE/TopoDS_Face.hxx>
#include <OpenCASCADE/TopTools_ListOfShape.hxx>

/**
 * @brief Parameters for feature edge extraction
 */
struct FeatureEdgeParams {
    double featureAngle = 15.0;        // Feature angle in degrees
    double minLength = 0.005;          // Minimum edge length
    bool onlyConvex = false;           // Only convex edges
    bool onlyConcave = false;          // Only concave edges
    
    FeatureEdgeParams() = default;
    FeatureEdgeParams(double angle, double minLen, bool convex = false, bool concave = false)
        : featureAngle(angle), minLength(minLen), onlyConvex(convex), onlyConcave(concave) {}
};

/**
 * @brief Feature edge extractor
 * 
 * Extracts feature edges based on surface normal angles
 * and edge-face relationships
 */
class FeatureEdgeExtractor : public TypedEdgeExtractor<FeatureEdgeParams> {
public:
    FeatureEdgeExtractor();
    ~FeatureEdgeExtractor() = default;
    
    // BaseEdgeExtractor interface
    bool canExtract(const TopoDS_Shape& shape) const override;
    const char* getName() const override { return "FeatureEdgeExtractor"; }
    
protected:
    std::vector<gp_Pnt> extractTyped(const TopoDS_Shape& shape, const FeatureEdgeParams* params) override;
    
private:
    /**
     * @brief Check if edge is a feature edge based on face normals
     */
    bool isFeatureEdge(
        const TopoDS_Edge& edge,
        const TopTools_ListOfShape& faces,
        double angleThreshold,
        bool onlyConvex,
        bool onlyConcave) const;
    
    /**
     * @brief Calculate angle between two face normals at edge midpoint
     */
    double calculateFaceAngle(
        const TopoDS_Edge& edge,
        const TopoDS_Face& face1,
        const TopoDS_Face& face2) const;
};
