#include "geometry/FaceDomainTypes.h"
#include <Inventor/SbVec3f.h>
#include <algorithm>

void FaceDomain::toCoin3DFormat(std::vector<SbVec3f>& vertices, std::vector<int>& indices) const
{
    if (isEmpty()) {
        return;
    }

    // Reserve space
    vertices.reserve(vertices.size() + points.size());
    indices.reserve(indices.size() + triangles.size() * 3);

    // Add vertices
    size_t vertexOffset = vertices.size();
    for (const auto& point : points) {
        // Explicit cast from Standard_Real (double) to float to avoid warning
        vertices.emplace_back(
            static_cast<float>(point.X()),
            static_cast<float>(point.Y()),
            static_cast<float>(point.Z())
        );
    }

    // Add triangles (convert from MeshTriangle to indices)
    for (const auto& triangle : triangles) {
        indices.push_back(static_cast<int>(vertexOffset + triangle.I1));
        indices.push_back(static_cast<int>(vertexOffset + triangle.I2));
        indices.push_back(static_cast<int>(vertexOffset + triangle.I3));
        indices.push_back(-1); // Triangle separator for Coin3D
    }
}

