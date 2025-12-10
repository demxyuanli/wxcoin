#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include "geometry/GeometryRenderContext.h"
#include "rendering/GeometryProcessor.h"

class SoSeparator;
struct MeshParameters;
struct DisplaySettings;

class PointViewBuilder {
public:
    PointViewBuilder();
    ~PointViewBuilder();

    void createPointViewRepresentation(SoSeparator* coinNode,
                                       const TopoDS_Shape& shape,
                                       const MeshParameters& params,
                                       const DisplaySettings& displaySettings);
};

