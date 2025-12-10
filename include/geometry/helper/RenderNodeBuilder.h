#pragma once

#include "geometry/GeometryRenderContext.h"
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoSeparator.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include <OpenCASCADE/TopAbs.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>
#include "rendering/GeometryProcessor.h"

class SoSeparator;
struct MeshParameters;
struct TextureData;

class RenderNodeBuilder {
public:
    RenderNodeBuilder();
    ~RenderNodeBuilder();

    SoTransform* createTransformNode(const GeometryRenderContext& context);
    SoShapeHints* createShapeHintsNode(const GeometryRenderContext& context);
    SoDrawStyle* createDrawStyleNode(const GeometryRenderContext& context);
    SoMaterial* createMaterialNode(const GeometryRenderContext& context);
    void appendTextureNodes(SoSeparator* parent, const GeometryRenderContext& context);
    void appendBlendHints(SoSeparator* parent, const GeometryRenderContext& context);
    void appendSurfaceGeometry(SoSeparator* parent, const TopoDS_Shape& shape, 
                              const MeshParameters& params, const GeometryRenderContext& context);
    SoPolygonOffset* createPolygonOffsetNode();

private:
    void setupTextureNode(SoSeparator* parent, const TextureData& texture);
};

