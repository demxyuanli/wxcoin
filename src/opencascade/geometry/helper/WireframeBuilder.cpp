#include "geometry/helper/WireframeBuilder.h"
#include "rendering/RenderingToolkitAPI.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <OpenCASCADE/TopoDS_Shape.hxx>

WireframeBuilder::WireframeBuilder() {
}

WireframeBuilder::~WireframeBuilder() {
}

void WireframeBuilder::createWireframeRepresentation(SoSeparator* coinNode, 
                                                      const TopoDS_Shape& shape, 
                                                      const MeshParameters& params) {
    if (shape.IsNull() || !coinNode) {
        return;
    }

    auto& manager = RenderingToolkitAPI::getManager();
    auto processor = manager.getGeometryProcessor("OpenCASCADE");
    if (!processor) {
        return;
    }

    TriangleMesh mesh = processor->convertToMesh(shape, params);
    if (mesh.isEmpty()) {
        return;
    }

    SoCoordinate3* coords = new SoCoordinate3();
    std::vector<float> vertices;
    vertices.reserve(mesh.vertices.size() * 3);
    for (const auto& vertex : mesh.vertices) {
        vertices.push_back(static_cast<float>(vertex.X()));
        vertices.push_back(static_cast<float>(vertex.Y()));
        vertices.push_back(static_cast<float>(vertex.Z()));
    }
    coords->point.setValues(0, static_cast<int>(mesh.vertices.size()),
        reinterpret_cast<const SbVec3f*>(vertices.data()));
    coinNode->addChild(coords);

    SoIndexedLineSet* lineSet = new SoIndexedLineSet();
    std::vector<int32_t> indices;
    indices.reserve(mesh.triangles.size() * 4);

    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
        int v0 = mesh.triangles[i];
        int v1 = mesh.triangles[i + 1];
        int v2 = mesh.triangles[i + 2];

        indices.push_back(v0);
        indices.push_back(v1);
        indices.push_back(SO_END_LINE_INDEX);

        indices.push_back(v1);
        indices.push_back(v2);
        indices.push_back(SO_END_LINE_INDEX);

        indices.push_back(v2);
        indices.push_back(v0);
        indices.push_back(SO_END_LINE_INDEX);
    }

    lineSet->coordIndex.setValues(0, static_cast<int>(indices.size()), indices.data());
    coinNode->addChild(lineSet);
}

