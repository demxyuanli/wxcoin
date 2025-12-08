#include "viewer/modes/ShadedMode.h"
#include "rendering/RenderingToolkitAPI.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include <fstream>

ShadedMode::ShadedMode() {
}

RenderingConfig::DisplayMode ShadedMode::getModeType() const {
    return RenderingConfig::DisplayMode::Solid;
}

int ShadedMode::getSwitchChildIndex() const {
    return 3; // Shaded mode is child 3
}

bool ShadedMode::requiresFaces() const {
    return true;
}

bool ShadedMode::requiresEdges() const {
    return false;
}

SoSeparator* ShadedMode::buildModeNode(
    const TopoDS_Shape& shape,
    const MeshParameters& params,
    const GeometryRenderContext& context,
    ModularEdgeComponent* modularEdgeComponent,
    VertexExtractor* vertexExtractor) {
    
    if (shape.IsNull()) {
        return nullptr;
    }

    SoSeparator* modeNode = new SoSeparator();
    modeNode->ref();
    modeNode->renderCaching.setValue(SoSeparator::OFF);
    modeNode->boundingBoxCaching.setValue(SoSeparator::OFF);
    modeNode->pickCulling.setValue(SoSeparator::OFF);

    // DrawStyle for faces
    SoDrawStyle* drawStyle = new SoDrawStyle();
    drawStyle->style.setValue(SoDrawStyle::FILLED);
    modeNode->addChild(drawStyle);

    // Material for faces
    SoMaterial* material = new SoMaterial();
    Standard_Real r, g, b;
    
    // Handle NoShading mode
    if (context.display.displayMode == RenderingConfig::DisplayMode::NoShading) {
        material->diffuseColor.setValue(0.8f, 0.8f, 0.8f);
        material->ambientColor.setValue(0.0f, 0.0f, 0.0f);
        material->specularColor.setValue(0.0f, 0.0f, 0.0f);
        material->emissiveColor.setValue(0.0f, 0.0f, 0.0f);
        material->shininess.setValue(0.0f);
        material->transparency.setValue(static_cast<float>(context.material.transparency));
    } else {
        context.material.ambientColor.Values(r, g, b, Quantity_TOC_RGB);
        material->ambientColor.setValue(static_cast<float>(r * 1.5), static_cast<float>(g * 1.5), static_cast<float>(b * 1.5));
        
        context.material.diffuseColor.Values(r, g, b, Quantity_TOC_RGB);
        material->diffuseColor.setValue(static_cast<float>(r * 0.8), static_cast<float>(g * 0.8), static_cast<float>(b * 0.8));
        
        context.material.specularColor.Values(r, g, b, Quantity_TOC_RGB);
        material->specularColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
        
        material->shininess.setValue(static_cast<float>(context.material.shininess / 100.0));
        double appliedTransparency = context.display.facesVisible ? context.material.transparency : 1.0;
        material->transparency.setValue(static_cast<float>(appliedTransparency));
        
        context.material.emissiveColor.Values(r, g, b, Quantity_TOC_RGB);
        material->emissiveColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    }
    modeNode->addChild(material);

    // Texture if enabled
    if (context.texture.enabled && !context.texture.imagePath.empty()) {
        std::ifstream fileCheck(context.texture.imagePath);
        if (fileCheck.good()) {
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
                modeNode->addChild(texture);
                modeNode->addChild(new SoTextureCoordinate2());
            } catch (const std::exception& e) {
                LOG_ERR_S("Exception loading texture: " + std::string(e.what()));
            }
        }
    }

    // Blend hints if needed
    if (context.blend.blendMode != RenderingConfig::BlendMode::None && context.material.transparency > 0.0) {
        SoShapeHints* blendHints = new SoShapeHints();
        blendHints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
        blendHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
        modeNode->addChild(blendHints);
    }

    // PolygonOffset for faces (FreeCAD order: Points -> PolygonOffset -> Faces -> Edges)
    SoPolygonOffset* polygonOffset = new SoPolygonOffset();
    modeNode->addChild(polygonOffset);

    // Add face geometry
    auto& manager = RenderingToolkitAPI::getManager();
    auto backend = manager.getRenderBackend("Coin3D");
    if (backend && context.display.facesVisible) {
        bool shouldShowFaces = context.display.facesVisible;
        if (context.display.showPointView) {
            shouldShowFaces = shouldShowFaces && context.display.showSolidWithPointView;
        }
        if (shouldShowFaces) {
            auto sceneNode = backend->createSceneNode(shape, params, context.display.selected,
                context.material.diffuseColor, context.material.ambientColor,
                context.material.specularColor, context.material.emissiveColor,
                context.material.shininess, context.material.transparency);
            if (sceneNode) {
                SoSeparator* meshNode = sceneNode.get();
                meshNode->ref();
                modeNode->addChild(meshNode);
            }
        }
    }

    return modeNode;
}

