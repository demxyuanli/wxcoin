#include "geometry/helper/RenderNodeBuilder.h"
#include "rendering/RenderingToolkitAPI.h"
#include "config/RenderingConfig.h"
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoSeparator.h>
#include <OpenCASCADE/TopAbs.hxx>
#include <fstream>

RenderNodeBuilder::RenderNodeBuilder() {
}

RenderNodeBuilder::~RenderNodeBuilder() {
}

SoTransform* RenderNodeBuilder::createTransformNode(const GeometryRenderContext& context) {
    SoTransform* transform = new SoTransform();
    transform->translation.setValue(
        static_cast<float>(context.transform.position.X()),
        static_cast<float>(context.transform.position.Y()),
        static_cast<float>(context.transform.position.Z())
    );
    
    if (context.transform.rotationAngle != 0.0) {
        SbVec3f axis(
            static_cast<float>(context.transform.rotationAxis.X()),
            static_cast<float>(context.transform.rotationAxis.Y()),
            static_cast<float>(context.transform.rotationAxis.Z())
        );
        transform->rotation.setValue(axis, static_cast<float>(context.transform.rotationAngle));
    }
    
    transform->scaleFactor.setValue(
        static_cast<float>(context.transform.scale),
        static_cast<float>(context.transform.scale),
        static_cast<float>(context.transform.scale)
    );
    
    return transform;
}

SoShapeHints* RenderNodeBuilder::createShapeHintsNode(const GeometryRenderContext& context) {
    SoShapeHints* hints = new SoShapeHints();
    
    bool isShellModel = (context.display.shapeType == TopAbs_SHELL) || !context.display.cullFace;
    if (isShellModel) {
        hints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
        hints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
        hints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
    } else {
        hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
        hints->shapeType = SoShapeHints::SOLID;
        hints->faceType = SoShapeHints::CONVEX;
    }
    
    return hints;
}

SoDrawStyle* RenderNodeBuilder::createDrawStyleNode(const GeometryRenderContext& context) {
    SoDrawStyle* node = new SoDrawStyle();
    node->style = context.display.wireframeMode ? SoDrawStyle::LINES : SoDrawStyle::FILLED;
    node->lineWidth = context.display.wireframeMode ? static_cast<float>(context.display.wireframeWidth) : 0.0f;
    return node;
}

SoMaterial* RenderNodeBuilder::createMaterialNode(const GeometryRenderContext& context) {
    SoMaterial* node = new SoMaterial();
    if (context.display.wireframeMode) {
        const Quantity_Color& wColor = context.display.wireframeColor;
        node->diffuseColor.setValue(
            static_cast<float>(wColor.Red()),
            static_cast<float>(wColor.Green()),
            static_cast<float>(wColor.Blue())
        );
        node->transparency.setValue(static_cast<float>(context.material.transparency));
    }
    else if (context.display.displayMode == RenderingConfig::DisplayMode::NoShading) {
        // No shading mode: use original geometry color without lighting effects
        Standard_Real r, g, b;
        context.material.diffuseColor.Values(r, g, b, Quantity_TOC_RGB);
        node->diffuseColor.setValue(
            static_cast<float>(r),
            static_cast<float>(g),
            static_cast<float>(b)
        );
        // Disable lighting effects: no ambient, specular, emissive, or shininess
        node->ambientColor.setValue(0.0f, 0.0f, 0.0f);
        node->specularColor.setValue(0.0f, 0.0f, 0.0f);
        node->emissiveColor.setValue(0.0f, 0.0f, 0.0f);
        node->shininess.setValue(0.0f);
        node->transparency.setValue(static_cast<float>(context.material.transparency));
    }
    else {
        Standard_Real r, g, b;
        context.material.ambientColor.Values(r, g, b, Quantity_TOC_RGB);
        node->ambientColor.setValue(
            static_cast<float>(r * 1.5),
            static_cast<float>(g * 1.5),
            static_cast<float>(b * 1.5)
        );

        context.material.diffuseColor.Values(r, g, b, Quantity_TOC_RGB);
        node->diffuseColor.setValue(
            static_cast<float>(r * 0.8),
            static_cast<float>(g * 0.8),
            static_cast<float>(b * 0.8)
        );

        context.material.specularColor.Values(r, g, b, Quantity_TOC_RGB);
        node->specularColor.setValue(
            static_cast<float>(r),
            static_cast<float>(g),
            static_cast<float>(b)
        );

        node->shininess.setValue(static_cast<float>(context.material.shininess / 100.0));
        double appliedTransparency = context.display.facesVisible ? context.material.transparency : 1.0;
        node->transparency.setValue(static_cast<float>(appliedTransparency));

        context.material.emissiveColor.Values(r, g, b, Quantity_TOC_RGB);
        node->emissiveColor.setValue(
            static_cast<float>(r),
            static_cast<float>(g),
            static_cast<float>(b)
        );
    }
    return node;
}

void RenderNodeBuilder::appendTextureNodes(SoSeparator* parent, const GeometryRenderContext& context) {
    if (!parent || !context.texture.enabled || context.texture.imagePath.empty()) {
        return;
    }

    std::ifstream fileCheck(context.texture.imagePath);
    if (!fileCheck.good()) {
        return;
    }

    fileCheck.close();
    try {
        SoTexture2* texture = new SoTexture2();
        texture->filename.setValue(context.texture.imagePath.c_str());

        switch (context.texture.mode) {
            case RenderingConfig::TextureMode::Replace:
                texture->model.setValue(SoTexture2::DECAL);
                break;
            case RenderingConfig::TextureMode::Modulate:
                texture->model.setValue(SoTexture2::MODULATE);
                break;
            case RenderingConfig::TextureMode::Blend:
                texture->model.setValue(SoTexture2::BLEND);
                break;
            default:
                texture->model.setValue(SoTexture2::DECAL);
                break;
        }

        parent->addChild(texture);
        parent->addChild(new SoTextureCoordinate2());
    }
    catch (const std::exception& e) {
    }
}

void RenderNodeBuilder::appendBlendHints(SoSeparator* parent, const GeometryRenderContext& context) {
    if (!parent) {
        return;
    }
    
    // Add SoShapeHints if transparency > 0, regardless of blendMode
    // Coin3D needs SoShapeHints for proper transparency rendering order
    if (context.material.transparency > 0.0) {
        SoShapeHints* blendHints = new SoShapeHints();
        blendHints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
        blendHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
        parent->addChild(blendHints);
    }
}

void RenderNodeBuilder::appendSurfaceGeometry(SoSeparator* parent, const TopoDS_Shape& shape, 
                                              const MeshParameters& params, const GeometryRenderContext& context) {
    if (!parent) {
        return;
    }
    
    auto& manager = RenderingToolkitAPI::getManager();
    auto backend = manager.getRenderBackend("Coin3D");
    if (!backend) {
        return;
    }

    bool shouldShowFaces = context.display.facesVisible;
    if (context.display.showPointView) {
        shouldShowFaces = shouldShowFaces && context.display.showSolidWithPointView;
    }

    auto sceneNode = backend->createSceneNode(shape, params, context.display.selected,
        context.material.diffuseColor, context.material.ambientColor,
        context.material.specularColor, context.material.emissiveColor,
        context.material.shininess, context.material.transparency);
    if (sceneNode && shouldShowFaces) {
        SoSeparator* meshNode = sceneNode.get();
        meshNode->ref();
        parent->addChild(meshNode);
    }
}

SoPolygonOffset* RenderNodeBuilder::createPolygonOffsetNode() {
    SoPolygonOffset* polygonOffset = new SoPolygonOffset();
    return polygonOffset;
}

