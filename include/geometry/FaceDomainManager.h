#pragma once

#include <vector>
#include <OpenCASCADE/TopoDS_Face.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include "rendering/GeometryProcessor.h"

struct FaceDomain;
struct MeshParameters;

/**
 * @brief Manages face domain system - independent mesh containers per geometry face
 * 
 * This class is responsible for building, querying, and maintaining face domains.
 * Each face domain contains its own triangulation data, allowing for precise
 * face-level operations like highlighting and selection.
 */
class FaceDomainManager {
public:
    FaceDomainManager();
    ~FaceDomainManager();

    /**
     * @brief Build face domains for all faces in the shape
     * @param shape The OpenCASCADE shape to process
     * @param faces Vector of faces extracted from the shape
     * @param params Mesh generation parameters
     */
    void buildFaceDomains(
        const TopoDS_Shape& shape,
        const std::vector<TopoDS_Face>& faces,
        const MeshParameters& params);

    /**
     * @brief Triangulate a single face and populate the domain
     * @param face The face to triangulate
     * @param domain The domain to populate
     * @return true if triangulation succeeded
     */
    bool triangulateFace(const TopoDS_Face& face, FaceDomain& domain);

    /**
     * @brief Get face domain by geometry face ID
     * @param geometryFaceId The face ID in the original geometry
     * @return Pointer to domain or nullptr if not found
     */
    const FaceDomain* getFaceDomain(int geometryFaceId) const;

    /**
     * @brief Get all face domains
     * @return Reference to the face domains vector
     */
    const std::vector<FaceDomain>& getFaceDomains() const { return m_faceDomains; }

    /**
     * @brief Check if face domain mapping exists
     * @return true if domains have been built
     */
    bool hasFaceDomainMapping() const { return !m_faceDomains.empty(); }

    /**
     * @brief Clear all face domains
     */
    void clear();

    /**
     * @brief Get the number of face domains
     */
    std::size_t getDomainCount() const { return m_faceDomains.size(); }

private:
    std::vector<FaceDomain> m_faceDomains;
};



