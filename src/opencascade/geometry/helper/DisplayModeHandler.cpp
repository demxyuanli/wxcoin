#include "geometry/helper/DisplayModeHandler.h"
#include "geometry/helper/RenderNodeBuilder.h"
#include "geometry/helper/WireframeBuilder.h"
#include "logger/Logger.h"
#include "edges/ModularEdgeComponent.h"
#include "config/EdgeSettingsConfig.h"
#include "rendering/RenderingToolkitAPI.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>

DisplayModeHandler::DisplayModeHandler() {
}

DisplayModeHandler::~DisplayModeHandler() {
}

void DisplayModeHandler::updateDisplayMode(SoSeparator* coinNode, RenderingConfig::DisplayMode mode,
                                           ModularEdgeComponent* edgeComponent) {
    if (!coinNode) {
        return;
    }

    if (edgeComponent) {
        SoSeparator* originalEdgeNode = edgeComponent->getEdgeNode(EdgeType::Original);
        SoSeparator* featureEdgeNode = edgeComponent->getEdgeNode(EdgeType::Feature);
        SoSeparator* meshEdgeNode = edgeComponent->getEdgeNode(EdgeType::Mesh);
        SoSeparator* highlightEdgeNode = edgeComponent->getEdgeNode(EdgeType::Highlight);
        SoSeparator* normalLineNode = edgeComponent->getEdgeNode(EdgeType::NormalLine);
        SoSeparator* faceNormalLineNode = edgeComponent->getEdgeNode(EdgeType::FaceNormalLine);
        SoSeparator* silhouetteEdgeNode = edgeComponent->getEdgeNode(EdgeType::Silhouette);
        
        for (int i = coinNode->getNumChildren() - 1; i >= 0; --i) {
            SoNode* child = coinNode->getChild(i);
            if (!child) continue;
            
            if (child == originalEdgeNode || child == featureEdgeNode ||
                child == meshEdgeNode || child == highlightEdgeNode ||
                child == normalLineNode || child == faceNormalLineNode ||
                child == silhouetteEdgeNode) {
                coinNode->removeChild(i);
            }
        }
    }

    SoDrawStyle* drawStyle = nullptr;
    SoMaterial* material = nullptr;

    findDrawStyleAndMaterial(coinNode, drawStyle, material);

    if (drawStyle) {
        switch (mode) {
        case RenderingConfig::DisplayMode::Wireframe:
            drawStyle->style.setValue(SoDrawStyle::LINES);
            break;
        case RenderingConfig::DisplayMode::Points:
            drawStyle->style.setValue(SoDrawStyle::POINTS);
            break;
        case RenderingConfig::DisplayMode::NoShading:
        case RenderingConfig::DisplayMode::Solid:
        case RenderingConfig::DisplayMode::SolidWireframe:
        case RenderingConfig::DisplayMode::HiddenLine:
        default:
            drawStyle->style.setValue(SoDrawStyle::FILLED);
            break;
        }
    }

    if (material && mode == RenderingConfig::DisplayMode::NoShading) {
        material->diffuseColor.setValue(0.8f, 0.8f, 0.8f);
        material->ambientColor.setValue(0.0f, 0.0f, 0.0f);
        material->specularColor.setValue(0.0f, 0.0f, 0.0f);
        material->emissiveColor.setValue(0.0f, 0.0f, 0.0f);
        material->shininess.setValue(0.0f);
    }
    
    coinNode->touch();
}

void DisplayModeHandler::handleDisplayMode(SoSeparator* coinNode, 
                                            const GeometryRenderContext& context,
                                            const TopoDS_Shape& shape,
                                            const MeshParameters& params,
                                            ModularEdgeComponent* edgeComponent,
                                            bool useModularEdgeComponent,
                                            RenderNodeBuilder* renderBuilder,
                                            WireframeBuilder* wireframeBuilder) {
    if (!coinNode || !renderBuilder || !wireframeBuilder) {
        return;
    }

    const RenderingConfig::DisplayMode displayMode = context.display.displayMode;
    SoPolygonOffset* polygonOffset = nullptr;

    auto appendSurfacePass = [&](const GeometryRenderContext& ctx) {
        coinNode->addChild(renderBuilder->createDrawStyleNode(ctx));
        coinNode->addChild(renderBuilder->createMaterialNode(ctx));
        renderBuilder->appendTextureNodes(coinNode, ctx);
        renderBuilder->appendBlendHints(coinNode, ctx);
        
        if (!polygonOffset) {
            polygonOffset = renderBuilder->createPolygonOffsetNode();
            coinNode->addChild(polygonOffset);
        }
        
        renderBuilder->appendSurfaceGeometry(coinNode, shape, params, ctx);
    };

    auto appendWireframePass = [&](const GeometryRenderContext& ctx) {
        coinNode->addChild(renderBuilder->createDrawStyleNode(ctx));
        coinNode->addChild(renderBuilder->createMaterialNode(ctx));
        
        if (useModularEdgeComponent && edgeComponent) {
            Quantity_Color wireframeColor = ctx.display.wireframeColor;
            double wireframeWidth = ctx.display.wireframeWidth;
            edgeComponent->extractOriginalEdges(
                shape, 
                80.0,
                0.01,
                false,
                wireframeColor,
                wireframeWidth,
                false,
                Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB),
                3.0
            );
            edgeComponent->updateEdgeDisplay(coinNode);
        } else {
            wireframeBuilder->createWireframeRepresentation(coinNode, shape, params);
        }
    };

    switch (displayMode) {
    case RenderingConfig::DisplayMode::Wireframe: {
        if (context.display.facesVisible) {
            GeometryRenderContext surfaceContext = context;
            surfaceContext.display.wireframeMode = false;
            surfaceContext.display.displayMode = RenderingConfig::DisplayMode::NoShading;
            surfaceContext.display.facesVisible = true;
            surfaceContext.texture.enabled = false;
            surfaceContext.material.ambientColor = Quantity_Color(0.1, 0.1, 0.1, Quantity_TOC_RGB);
            surfaceContext.material.diffuseColor = Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB);
            surfaceContext.material.specularColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
            surfaceContext.material.emissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
            surfaceContext.material.shininess = 0.0;
            appendSurfacePass(surfaceContext);
        }

        GeometryRenderContext wireContext = context;
        wireContext.display.wireframeMode = true;
        wireContext.display.facesVisible = false;
        appendWireframePass(wireContext);
        break;
    }
    case RenderingConfig::DisplayMode::SolidWireframe: {
        GeometryRenderContext surfaceContext = context;
        surfaceContext.display.wireframeMode = false;
        appendSurfacePass(surfaceContext);

        GeometryRenderContext wireContext = context;
        wireContext.display.wireframeMode = true;
        wireContext.display.facesVisible = false;
        wireContext.display.displayMode = RenderingConfig::DisplayMode::Wireframe;
        appendWireframePass(wireContext);
        break;
    }
    case RenderingConfig::DisplayMode::HiddenLine: {
        GeometryRenderContext surfaceContext = context;
        surfaceContext.display.wireframeMode = false;
        surfaceContext.display.displayMode = RenderingConfig::DisplayMode::NoShading;
        surfaceContext.display.facesVisible = true;
        surfaceContext.texture.enabled = false;
        surfaceContext.material.ambientColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
        surfaceContext.material.diffuseColor = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB);
        surfaceContext.material.specularColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        surfaceContext.material.emissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        surfaceContext.material.shininess = 0.0;
        appendSurfacePass(surfaceContext);

        GeometryRenderContext wireContext = context;
        wireContext.display.wireframeMode = true;
        wireContext.display.facesVisible = false;
        wireContext.display.displayMode = RenderingConfig::DisplayMode::Wireframe;
        appendWireframePass(wireContext);
        break;
    }
    default: {
        GeometryRenderContext surfaceContext = context;
        appendSurfacePass(surfaceContext);
        break;
    }
    }
}

void DisplayModeHandler::findDrawStyleAndMaterial(SoNode* node, SoDrawStyle*& drawStyle, SoMaterial*& material) {
    if (!node) return;
    
    if (node->isOfType(SoDrawStyle::getClassTypeId())) {
        if (!drawStyle) {
            drawStyle = static_cast<SoDrawStyle*>(node);
        }
    } else if (node->isOfType(SoMaterial::getClassTypeId())) {
        if (!material) {
            material = static_cast<SoMaterial*>(node);
        }
    } else if (node->isOfType(SoSeparator::getClassTypeId())) {
        SoSeparator* sep = static_cast<SoSeparator*>(node);
        for (int i = 0; i < sep->getNumChildren(); ++i) {
            findDrawStyleAndMaterial(sep->getChild(i), drawStyle, material);
        }
    }
}

void DisplayModeHandler::cleanupEdgeNodes(SoSeparator* coinNode, ModularEdgeComponent* edgeComponent) {
    if (!coinNode || !edgeComponent) {
        return;
    }

    SoSeparator* originalEdgeNode = edgeComponent->getEdgeNode(EdgeType::Original);
    SoSeparator* featureEdgeNode = edgeComponent->getEdgeNode(EdgeType::Feature);
    SoSeparator* meshEdgeNode = edgeComponent->getEdgeNode(EdgeType::Mesh);
    SoSeparator* highlightEdgeNode = edgeComponent->getEdgeNode(EdgeType::Highlight);
    SoSeparator* normalLineNode = edgeComponent->getEdgeNode(EdgeType::NormalLine);
    SoSeparator* faceNormalLineNode = edgeComponent->getEdgeNode(EdgeType::FaceNormalLine);
    SoSeparator* silhouetteEdgeNode = edgeComponent->getEdgeNode(EdgeType::Silhouette);
    
    for (int i = coinNode->getNumChildren() - 1; i >= 0; --i) {
        SoNode* child = coinNode->getChild(i);
        if (!child) continue;
        
        if (child == originalEdgeNode || child == featureEdgeNode ||
            child == meshEdgeNode || child == highlightEdgeNode ||
            child == normalLineNode || child == faceNormalLineNode ||
            child == silhouetteEdgeNode) {
            coinNode->removeChild(i);
        }
    }
}

