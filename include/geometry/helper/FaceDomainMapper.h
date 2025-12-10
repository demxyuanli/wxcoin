#pragma once

#include <vector>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include <OpenCASCADE/TopoDS_Face.hxx>
#include "rendering/GeometryProcessor.h"

struct MeshParameters;
struct FaceDomain;
struct TriangleSegment;
struct BoundaryTriangle;

class FaceDomainMapper {
public:
    FaceDomainMapper();
    ~FaceDomainMapper();

    void buildFaceDomainMapping(const TopoDS_Shape& shape,
                                const MeshParameters& params,
                                std::vector<FaceDomain>& faceDomains,
                                std::vector<TriangleSegment>& triangleSegments,
                                std::vector<BoundaryTriangle>& boundaryTriangles);

    bool triangulateFace(const TopoDS_Face& face, FaceDomain& domain);

private:
    void extractFaces(const TopoDS_Shape& shape, std::vector<TopoDS_Face>& faces);
    void buildFaceDomains(const TopoDS_Shape& shape,
                         const std::vector<TopoDS_Face>& faces,
                         const MeshParameters& params,
                         std::vector<FaceDomain>& faceDomains);
    void buildTriangleSegments(const std::vector<std::pair<int, std::vector<int>>>& faceMappings,
                               std::vector<TriangleSegment>& triangleSegments);
    void identifyBoundaryTriangles(const std::vector<std::pair<int, std::vector<int>>>& faceMappings,
                                    std::vector<BoundaryTriangle>& boundaryTriangles);
};

