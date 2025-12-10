#include "geometry/helper/FaceDomainMapper.h"
#include "geometry/GeomCoinRepresentation.h"
#include "logger/Logger.h"
#include "rendering/RenderingToolkitAPI.h"
#include "rendering/OpenCASCADEProcessor.h"
#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/TopAbs.hxx>
#include <OpenCASCADE/TopoDS.hxx>
#include <OpenCASCADE/TopoDS_Shell.hxx>
#include <OpenCASCADE/TopoDS_Solid.hxx>
#include <OpenCASCADE/TopoDS_Face.hxx>
#include <OpenCASCADE/TopLoc_Location.hxx>
#include <OpenCASCADE/BRep_Tool.hxx>
#include <OpenCASCADE/Poly_Triangulation.hxx>
#include <OpenCASCADE/gp_Trsf.hxx>
#include <map>
#include <algorithm>

FaceDomainMapper::FaceDomainMapper() {
}

FaceDomainMapper::~FaceDomainMapper() {
}

void FaceDomainMapper::buildFaceDomainMapping(const TopoDS_Shape& shape,
                                               const MeshParameters& params,
                                               std::vector<FaceDomain>& faceDomains,
                                               std::vector<TriangleSegment>& triangleSegments,
                                               std::vector<BoundaryTriangle>& boundaryTriangles) {
    if (shape.IsNull()) {
        return;
    }

    try {
        faceDomains.clear();
        triangleSegments.clear();
        boundaryTriangles.clear();

        std::vector<TopoDS_Face> faces;
        extractFaces(shape, faces);

        if (faces.empty()) {
            return;
        }

        auto& manager = RenderingToolkitAPI::getManager();
        auto* baseProcessor = manager.getGeometryProcessor("OpenCASCADE");
        auto* processor = dynamic_cast<OpenCASCADEProcessor*>(baseProcessor);

        if (processor) {
            std::vector<std::pair<int, std::vector<int>>> faceMappings;
            TriangleMesh meshWithMapping = processor->convertToMeshWithFaceMapping(shape, params, faceMappings);

            if (faces.size() != faceMappings.size()) {
                LOG_WRN_S("FaceDomainMapper: Face count mismatch: found " + std::to_string(faces.size()) +
                         " faces but got " + std::to_string(faceMappings.size()) + " mappings");
            }

            buildFaceDomains(shape, faces, params, faceDomains);
            buildTriangleSegments(faceMappings, triangleSegments);
            identifyBoundaryTriangles(faceMappings, boundaryTriangles);
        } else {
            LOG_ERR_S("FaceDomainMapper: OpenCASCADE processor not available");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("FaceDomainMapper: Failed to build face domain mapping: " + std::string(e.what()));
        faceDomains.clear();
        triangleSegments.clear();
        boundaryTriangles.clear();
    }
}

void FaceDomainMapper::extractFaces(const TopoDS_Shape& shape, std::vector<TopoDS_Face>& faces) {
    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
        TopoDS_Face face = TopoDS::Face(exp.Current());
        if (!face.IsNull()) {
            faces.push_back(face);
        }
    }

    if (faces.empty()) {
        for (TopExp_Explorer shellExp(shape, TopAbs_SHELL); shellExp.More(); shellExp.Next()) {
            TopoDS_Shell shell = TopoDS::Shell(shellExp.Current());
            for (TopExp_Explorer faceExp(shell, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
                TopoDS_Face face = TopoDS::Face(faceExp.Current());
                if (!face.IsNull()) {
                    faces.push_back(face);
                }
            }
        }
    }

    if (faces.empty()) {
        for (TopExp_Explorer solidExp(shape, TopAbs_SOLID); solidExp.More(); solidExp.Next()) {
            TopoDS_Solid solid = TopoDS::Solid(solidExp.Current());
            for (TopExp_Explorer faceExp(solid, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
                TopoDS_Face face = TopoDS::Face(faceExp.Current());
                if (!face.IsNull()) {
                    faces.push_back(face);
                }
            }
        }
    }
}

void FaceDomainMapper::buildFaceDomains(const TopoDS_Shape& shape,
                                        const std::vector<TopoDS_Face>& faces,
                                        const MeshParameters& params,
                                        std::vector<FaceDomain>& faceDomains) {
    faceDomains.reserve(faces.size());

    for (std::size_t faceIndex = 0; faceIndex < faces.size(); ++faceIndex) {
        const TopoDS_Face& face = faces[faceIndex];
        FaceDomain domain(static_cast<int>(faceIndex));

        if (triangulateFace(face, domain)) {
            domain.isValid = true;
        } else {
            domain.isValid = false;
        }

        faceDomains.push_back(std::move(domain));
    }
}

bool FaceDomainMapper::triangulateFace(const TopoDS_Face& face, FaceDomain& domain) {
    try {
        TopLoc_Location loc;
        Handle(Poly_Triangulation) hTria = BRep_Tool::Triangulation(face, loc);
        if (hTria.IsNull()) {
            return false;
        }

        gp_Trsf transf;
        bool identity = true;
        if (!loc.IsIdentity()) {
            identity = false;
            transf = loc.Transformation();
        }

        TopAbs_Orientation orient = face.Orientation();

        Standard_Integer nbNodes = hTria->NbNodes();
        Standard_Integer nbTriangles = hTria->NbTriangles();

        domain.points.reserve(nbNodes);
        domain.triangles.reserve(nbTriangles);

        for (int i = 1; i <= nbNodes; i++) {
            gp_Pnt p = hTria->Node(i);

            if (!identity) {
                p.Transform(transf);
            }

            domain.points.push_back(p);
        }

        for (int i = 1; i <= nbTriangles; i++) {
            Standard_Integer n1, n2, n3;
            hTria->Triangle(i).Get(n1, n2, n3);

            --n1; --n2; --n3;

            if (orient != TopAbs_FORWARD) {
                std::swap(n1, n2);
            }

            domain.triangles.emplace_back(n1, n2, n3);
        }

        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}

void FaceDomainMapper::buildTriangleSegments(const std::vector<std::pair<int, std::vector<int>>>& faceMappings,
                                              std::vector<TriangleSegment>& triangleSegments) {
    triangleSegments.clear();
    triangleSegments.reserve(faceMappings.size());

    for (const auto& [faceId, triangleIndices] : faceMappings) {
        TriangleSegment segment(faceId, triangleIndices);
        triangleSegments.push_back(std::move(segment));
    }
}

void FaceDomainMapper::identifyBoundaryTriangles(const std::vector<std::pair<int, std::vector<int>>>& faceMappings,
                                                   std::vector<BoundaryTriangle>& boundaryTriangles) {
    std::map<int, std::vector<int>> triangleToFacesMap;

    for (const auto& [faceId, triangleIndices] : faceMappings) {
        for (int triangleIndex : triangleIndices) {
            triangleToFacesMap[triangleIndex].push_back(faceId);
        }
    }

    for (const auto& [triangleIndex, faceIds] : triangleToFacesMap) {
        if (faceIds.size() > 1) {
            BoundaryTriangle boundaryTri(triangleIndex);
            boundaryTri.faceIds = faceIds;
            boundaryTri.isBoundary = true;
            boundaryTriangles.push_back(boundaryTri);
        }
    }
}

