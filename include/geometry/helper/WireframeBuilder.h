#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include "rendering/GeometryProcessor.h"

class SoSeparator;
struct MeshParameters;

class WireframeBuilder {
public:
    WireframeBuilder();
    ~WireframeBuilder();

    void createWireframeRepresentation(SoSeparator* coinNode, 
                                      const TopoDS_Shape& shape, 
                                      const MeshParameters& params);

    // Overload for direct mesh creation (for STL/OBJ mesh-only geometries)
    void createWireframeRepresentation(SoSeparator* coinNode, 
                                      const TriangleMesh& mesh);
};

