#include "viewer/modes/FlatLinesMode.h"
#include "edges/ModularEdgeComponent.h"
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

FlatLinesMode::FlatLinesMode() {
}

RenderingConfig::DisplayMode FlatLinesMode::getModeType() const {
    return RenderingConfig::DisplayMode::SolidWireframe;
}

int FlatLinesMode::getSwitchChildIndex() const {
    return 2; // FlatLines mode is child 2
}

bool FlatLinesMode::requiresFaces() const {
    return true;
}

bool FlatLinesMode::requiresEdges() const {
    return true;
}

SoSeparator* FlatLinesMode::buildModeNode(
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
    SoDrawStyle* faceDrawStyle = new SoDrawStyle();
    faceDrawStyle->style.setValue(SoDrawStyle::FILLED);
    modeNode->addChild(faceDrawStyle);

    // Material for faces
    // CRITICAL: Handle HiddenLine mode differently (darker faces for hidden line effect)
    SoMaterial* faceMaterial = new SoMaterial();
    Standard_Real r, g, b;
    
    if (context.display.displayMode == RenderingConfig::DisplayMode::HiddenLine) {
        // HiddenLine mode: darker faces to make edges stand out
        context.material.diffuseColor.Values(r, g, b, Quantity_TOC_RGB);
        faceMaterial->diffuseColor.setValue(static_cast<float>(r * 0.5), static_cast<float>(g * 0.5), static_cast<float>(b * 0.5));
        context.material.ambientColor.Values(r, g, b, Quantity_TOC_RGB);
        faceMaterial->ambientColor.setValue(static_cast<float>(r * 0.8), static_cast<float>(g * 0.8), static_cast<float>(b * 0.8));
    } else {
        // SolidWireframe mode: normal face colors
        context.material.diffuseColor.Values(r, g, b, Quantity_TOC_RGB);
        faceMaterial->diffuseColor.setValue(static_cast<float>(r * 0.8), static_cast<float>(g * 0.8), static_cast<float>(b * 0.8));
        context.material.ambientColor.Values(r, g, b, Quantity_TOC_RGB);
        faceMaterial->ambientColor.setValue(static_cast<float>(r * 1.5), static_cast<float>(g * 1.5), static_cast<float>(b * 1.5));
    }
    faceMaterial->transparency.setValue(static_cast<float>(context.material.transparency));
    modeNode->addChild(faceMaterial);

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

    // Add wireframe overlay
    SoDrawStyle* wireDrawStyle = new SoDrawStyle();
    wireDrawStyle->style.setValue(SoDrawStyle::LINES);
    wireDrawStyle->lineWidth.setValue(static_cast<float>(context.display.wireframeWidth));
    modeNode->addChild(wireDrawStyle);

    SoMaterial* wireMaterial = new SoMaterial();
    context.display.wireframeColor.Values(r, g, b, Quantity_TOC_RGB);
    wireMaterial->diffuseColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
    modeNode->addChild(wireMaterial);

    // Extract and add wireframe edges
    if (modularEdgeComponent) {
        Quantity_Color originalColor(1.0, 1.0, 1.0, Quantity_TOC_RGB);
        modularEdgeComponent->extractOriginalEdges(shape, 80.0, 0.01, false, originalColor, 1.0, false, Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB), 3.0);
        modularEdgeComponent->updateEdgeDisplay(modeNode);
    }

    return modeNode;
}

