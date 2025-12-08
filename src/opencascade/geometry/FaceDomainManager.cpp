#include "geometry/FaceDomainManager.h"
#include "geometry/FaceDomainTypes.h"  // For FaceDomain struct definition
#include "logger/Logger.h"
#include <OpenCASCADE/BRep_Tool.hxx>
#include <OpenCASCADE/TopLoc_Location.hxx>
#include <OpenCASCADE/Poly_Triangulation.hxx>
#include <OpenCASCADE/gp_Trsf.hxx>
#include <OpenCASCADE/TopAbs.hxx>

FaceDomainManager::FaceDomainManager()
{
}

FaceDomainManager::~FaceDomainManager()
{
}

void FaceDomainManager::buildFaceDomains(
    const TopoDS_Shape& shape,
    const std::vector<TopoDS_Face>& faces,
    const MeshParameters& params)
{
    m_faceDomains.clear();
    m_faceDomains.reserve(faces.size());

    // Create domains for all faces (similar to FreeCAD's getDomains approach)
    for (std::size_t faceIndex = 0; faceIndex < faces.size(); ++faceIndex) {
        const TopoDS_Face& face = faces[faceIndex];
        FaceDomain domain(static_cast<int>(faceIndex));

        // Try to triangulate this face using OpenCASCADE directly
        if (triangulateFace(face, domain)) {
            domain.isValid = true;
        } else {
            // Face could not be triangulated - create empty domain
            // This maintains the 1:1 mapping between faces and domains
            domain.isValid = false;
        }

        m_faceDomains.push_back(std::move(domain));
    }
}

bool FaceDomainManager::triangulateFace(const TopoDS_Face& face, FaceDomain& domain)
{
    try {
        TopLoc_Location loc;
        Handle(Poly_Triangulation) hTria = BRep_Tool::Triangulation(face, loc);
        if (hTria.IsNull()) {
            return false;
        }

        // Getting the transformation of the face
        gp_Trsf transf;
        bool identity = true;
        if (!loc.IsIdentity()) {
            identity = false;
            transf = loc.Transformation();
        }

        // Check orientation
        TopAbs_Orientation orient = face.Orientation();

        Standard_Integer nbNodes = hTria->NbNodes();
        Standard_Integer nbTriangles = hTria->NbTriangles();

        // Reserve space for points and triangles
        domain.points.reserve(nbNodes);
        domain.triangles.reserve(nbTriangles);

        // Cycling through the poly mesh
        for (int i = 1; i <= nbNodes; i++) {
            gp_Pnt p = hTria->Node(i);

            // Transform the vertices to the location of the face
            if (!identity) {
                p.Transform(transf);
            }

            domain.points.push_back(p);
        }

        for (int i = 1; i <= nbTriangles; i++) {
            // Get the triangle
            Standard_Integer n1, n2, n3;
            hTria->Triangle(i).Get(n1, n2, n3);

            // Adjust for 0-based indexing
            --n1; --n2; --n3;

            // Change orientation of the triangles
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

const FaceDomain* FaceDomainManager::getFaceDomain(int geometryFaceId) const
{
    for (const auto& domain : m_faceDomains) {
        if (domain.geometryFaceId == geometryFaceId) {
            return &domain;
        }
    }
    return nullptr;
}

void FaceDomainManager::clear()
{
    m_faceDomains.clear();
}

