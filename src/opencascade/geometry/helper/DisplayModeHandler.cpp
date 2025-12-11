#include "geometry/helper/DisplayModeHandler.h"
#include "geometry/helper/RenderNodeBuilder.h"
#include "geometry/helper/WireframeBuilder.h"
#include "edges/ModularEdgeComponent.h"
#include "config/EdgeSettingsConfig.h"
#include "rendering/RenderingToolkitAPI.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoSwitch.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>

DisplayModeHandler::DisplayModeHandler() 
    : m_modeSwitch(nullptr)
    , m_useSwitchMode(false)
{
}

DisplayModeHandler::~DisplayModeHandler() {
}

void DisplayModeHandler::setModeSwitch(SoSwitch* modeSwitch) {
    m_modeSwitch = modeSwitch;
    m_useSwitchMode = (m_modeSwitch != nullptr);
}

void DisplayModeHandler::updateDisplayMode(SoSeparator* coinNode, RenderingConfig::DisplayMode mode,
                                           ModularEdgeComponent* edgeComponent) {
    if (!coinNode) {
        LOG_INF_S("DisplayModeHandler::updateDisplayMode: coinNode is null");
        return;
    }

    LOG_INF_S("DisplayModeHandler::updateDisplayMode: mode=" + std::to_string(static_cast<int>(mode)) + 
              ", useSwitchMode=" + std::to_string(m_useSwitchMode) + 
              ", modeSwitch=" + std::to_string(m_modeSwitch != nullptr ? 1 : 0));

    if (m_useSwitchMode && m_modeSwitch) {
        int switchIndex = getModeSwitchIndex(mode);
        LOG_INF_S("DisplayModeHandler::updateDisplayMode: Using switch mode, switchIndex=" + 
                  std::to_string(switchIndex) + ", totalChildren=" + std::to_string(m_modeSwitch->getNumChildren()));
        if (switchIndex >= 0 && switchIndex < m_modeSwitch->getNumChildren()) {
            m_modeSwitch->whichChild.setValue(switchIndex);
            LOG_INF_S("DisplayModeHandler::updateDisplayMode: Switch index set to " + std::to_string(switchIndex));
            return;
        } else {
            LOG_WRN_S("DisplayModeHandler::updateDisplayMode: Invalid switch index " + std::to_string(switchIndex));
        }
    }

    if (edgeComponent) {
        SoSeparator* originalEdgeNode = edgeComponent->getEdgeNode(EdgeType::Original);
        SoSeparator* featureEdgeNode = edgeComponent->getEdgeNode(EdgeType::Feature);
        SoSeparator* meshEdgeNode = edgeComponent->getEdgeNode(EdgeType::Mesh);
        SoSeparator* highlightEdgeNode = edgeComponent->getEdgeNode(EdgeType::Highlight);
        SoSeparator* normalLineNode = edgeComponent->getEdgeNode(EdgeType::VerticeNormal);
        SoSeparator* faceNormalLineNode = edgeComponent->getEdgeNode(EdgeType::FaceNormal);
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

    LOG_INF_S("DisplayModeHandler::updateDisplayMode: Found drawStyle=" + std::to_string(drawStyle != nullptr ? 1 : 0) +
              ", material=" + std::to_string(material != nullptr ? 1 : 0));

    if (drawStyle) {
        int oldStyle = drawStyle->style.getValue();
        switch (mode) {
        case RenderingConfig::DisplayMode::NoShading:
			drawStyle->style.setValue(SoDrawStyle::FILLED);
			LOG_INF_S("DisplayModeHandler::updateDisplayMode: Set drawStyle to FILLED for NoShading (was " + std::to_string(oldStyle) + ")");
			break;
        case RenderingConfig::DisplayMode::Points:
            drawStyle->style.setValue(SoDrawStyle::POINTS);
            LOG_INF_S("DisplayModeHandler::updateDisplayMode: Set drawStyle to POINTS (was " + std::to_string(oldStyle) + ")");
            break;
        case RenderingConfig::DisplayMode::Wireframe:
            drawStyle->style.setValue(SoDrawStyle::LINES);
            LOG_INF_S("DisplayModeHandler::updateDisplayMode: Set drawStyle to LINES (was " + std::to_string(oldStyle) + ")");
            break;
        case RenderingConfig::DisplayMode::FlatLines:
			drawStyle->style.setValue(SoDrawStyle::FILLED);
			LOG_INF_S("DisplayModeHandler::updateDisplayMode: Set drawStyle to FILLED for FlatLines (was " + std::to_string(oldStyle) + ")");
			break;
        case RenderingConfig::DisplayMode::Solid:
            drawStyle->style.setValue(SoDrawStyle::FILLED);
			LOG_INF_S("DisplayModeHandler::updateDisplayMode: Set drawStyle to FILLED for Solid (was " + std::to_string(oldStyle) + ")");
            break;
        case RenderingConfig::DisplayMode::Transparent:
            drawStyle->style.setValue(SoDrawStyle::FILLED);
            LOG_INF_S("DisplayModeHandler::updateDisplayMode: Set drawStyle to FILLED for Transparent (was " + std::to_string(oldStyle) + ")");
            break;
        case RenderingConfig::DisplayMode::HiddenLine:
			drawStyle->style.setValue(SoDrawStyle::LINES);
			LOG_INF_S("DisplayModeHandler::updateDisplayMode: Set drawStyle to LINES for HiddenLine (was " + std::to_string(oldStyle) + ")");
			break;
        default:
            drawStyle->style.setValue(SoDrawStyle::FILLED);
            LOG_INF_S("DisplayModeHandler::updateDisplayMode: Set drawStyle to FILLED (was " + std::to_string(oldStyle) + 
                     ", mode=" + std::to_string(static_cast<int>(mode)) + ")");
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
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Invalid parameters - coinNode=" + 
                 std::to_string(coinNode != nullptr ? 1 : 0) + 
                 ", renderBuilder=" + std::to_string(renderBuilder != nullptr ? 1 : 0) +
                 ", wireframeBuilder=" + std::to_string(wireframeBuilder != nullptr ? 1 : 0));
        return;
    }

    const RenderingConfig::DisplayMode displayMode = context.display.displayMode;
    
    LOG_INF_S("DisplayModeHandler::handleDisplayMode: displayMode=" + std::to_string(static_cast<int>(displayMode)) +
              ", wireframeMode=" + std::to_string(context.display.wireframeMode ? 1 : 0) +
              ", facesVisible=" + std::to_string(context.display.facesVisible ? 1 : 0) +
              ", visible=" + std::to_string(context.display.visible ? 1 : 0) +
              ", selected=" + std::to_string(context.display.selected ? 1 : 0) +
              ", useSwitchMode=" + std::to_string(m_useSwitchMode) +
              ", useModularEdgeComponent=" + std::to_string(useModularEdgeComponent ? 1 : 0) +
              ", edgeComponent=" + std::to_string(edgeComponent != nullptr ? 1 : 0));
    
    if (m_useSwitchMode && m_modeSwitch) {
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Building switch mode nodes");
        m_modeSwitch->removeAllChildren();
        
        RenderingConfig::DisplayMode modes[] = {
            RenderingConfig::DisplayMode::NoShading,
            RenderingConfig::DisplayMode::Points,
            RenderingConfig::DisplayMode::Wireframe,
            RenderingConfig::DisplayMode::Solid,
            RenderingConfig::DisplayMode::FlatLines,
            RenderingConfig::DisplayMode::Transparent,
            RenderingConfig::DisplayMode::HiddenLine
        };
        
        for (auto mode : modes) {
            SoSeparator* modeNode = new SoSeparator();
            modeNode->ref();
            GeometryRenderContext modeContext = context;
            modeContext.display.displayMode = mode;
            buildModeNode(modeNode, mode, modeContext, shape, params,
                         edgeComponent, useModularEdgeComponent,
                         renderBuilder, wireframeBuilder);
            m_modeSwitch->addChild(modeNode);
            modeNode->unref();
        }
        
        int switchIndex = getModeSwitchIndex(displayMode);
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Switch index for mode " + 
                 std::to_string(static_cast<int>(displayMode)) + " = " + std::to_string(switchIndex) +
                 ", total children = " + std::to_string(m_modeSwitch->getNumChildren()));
        if (switchIndex >= 0 && switchIndex < m_modeSwitch->getNumChildren()) {
            m_modeSwitch->whichChild.setValue(switchIndex);
            LOG_INF_S("DisplayModeHandler::handleDisplayMode: Switch whichChild set to " + std::to_string(switchIndex));
        } else {
            LOG_WRN_S("DisplayModeHandler::handleDisplayMode: Invalid switch index " + std::to_string(switchIndex));
        }
        
        coinNode->addChild(m_modeSwitch);
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Switch mode setup complete");
        return;
    }
    
    SoPolygonOffset* polygonOffset = nullptr;

    auto appendSurfacePass = [&](const GeometryRenderContext& ctx) {
        LOG_INF_S("DisplayModeHandler::appendSurfacePass: wireframeMode=" + 
                 std::to_string(ctx.display.wireframeMode ? 1 : 0) +
                 ", facesVisible=" + std::to_string(ctx.display.facesVisible ? 1 : 0) +
                 ", displayMode=" + std::to_string(static_cast<int>(ctx.display.displayMode)));
        coinNode->addChild(renderBuilder->createDrawStyleNode(ctx));
        coinNode->addChild(renderBuilder->createMaterialNode(ctx));
        renderBuilder->appendTextureNodes(coinNode, ctx);
        renderBuilder->appendBlendHints(coinNode, ctx);
        
        if (!polygonOffset) {
            polygonOffset = renderBuilder->createPolygonOffsetNode();
            coinNode->addChild(polygonOffset);
        }
        
        renderBuilder->appendSurfaceGeometry(coinNode, shape, params, ctx);
        LOG_INF_S("DisplayModeHandler::appendSurfacePass: Surface geometry appended");
    };

    auto appendWireframePass = [&](const GeometryRenderContext& ctx) {
        LOG_INF_S("DisplayModeHandler::appendWireframePass: wireframeMode=" + 
                 std::to_string(ctx.display.wireframeMode ? 1 : 0) +
                 ", useModularEdgeComponent=" + std::to_string(useModularEdgeComponent ? 1 : 0) +
                 ", edgeComponent=" + std::to_string(edgeComponent != nullptr ? 1 : 0));
        coinNode->addChild(renderBuilder->createDrawStyleNode(ctx));
        coinNode->addChild(renderBuilder->createMaterialNode(ctx));
        
        if (useModularEdgeComponent && edgeComponent) {
            Quantity_Color wireframeColor = ctx.display.wireframeColor;
            double wireframeWidth = ctx.display.wireframeWidth;
            LOG_INF_S("DisplayModeHandler::appendWireframePass: Extracting original edges, color=(" +
                     std::to_string(wireframeColor.Red()) + "," +
                     std::to_string(wireframeColor.Green()) + "," +
                     std::to_string(wireframeColor.Blue()) + "), width=" +
                     std::to_string(wireframeWidth));
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
            LOG_INF_S("DisplayModeHandler::appendWireframePass: Edge display updated");
        } else {
            LOG_INF_S("DisplayModeHandler::appendWireframePass: Using wireframeBuilder");
            wireframeBuilder->createWireframeRepresentation(coinNode, shape, params);
        }
    };

    switch (displayMode) {
    case RenderingConfig::DisplayMode::NoShading: {
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Processing NoShading mode");
        GeometryRenderContext surfaceContext = context;
        surfaceContext.display.wireframeMode = false;
        surfaceContext.display.displayMode = RenderingConfig::DisplayMode::NoShading;
        surfaceContext.display.facesVisible = true;
        surfaceContext.texture.enabled = false;
        surfaceContext.material.ambientColor = Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB);
        surfaceContext.material.diffuseColor = Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB);
        surfaceContext.material.specularColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        surfaceContext.material.emissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        surfaceContext.material.shininess = 0.0;
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Appending surface pass for NoShading");
        appendSurfacePass(surfaceContext);
        break;
    }
    case RenderingConfig::DisplayMode::Points: {
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Processing Points mode");
        if (context.display.showPointView) {
            GeometryRenderContext surfaceContext = context;
            surfaceContext.display.wireframeMode = false;
            surfaceContext.display.facesVisible = context.display.showSolidWithPointView;
            LOG_INF_S("DisplayModeHandler::handleDisplayMode: Appending surface pass for Points (if showSolidWithPointView)");
            if (surfaceContext.display.facesVisible) {
                appendSurfacePass(surfaceContext);
            }
        } else {
            LOG_INF_S("DisplayModeHandler::handleDisplayMode: Points mode but showPointView=false, no geometry added");
        }
        break;
    }
    case RenderingConfig::DisplayMode::Wireframe: {
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Processing Wireframe mode, facesVisible=" + 
                 std::to_string(context.display.facesVisible ? 1 : 0));
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
            LOG_INF_S("DisplayModeHandler::handleDisplayMode: Appending surface pass for Wireframe");
            appendSurfacePass(surfaceContext);
        }

        GeometryRenderContext wireContext = context;
        wireContext.display.wireframeMode = true;
        wireContext.display.facesVisible = false;
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Appending wireframe pass for Wireframe");
        appendWireframePass(wireContext);
        break;
    }
    case RenderingConfig::DisplayMode::FlatLines: {
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Processing SolidWireframe mode");
        GeometryRenderContext surfaceContext = context;
        surfaceContext.display.wireframeMode = false;
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Appending surface pass for SolidWireframe");
        appendSurfacePass(surfaceContext);

        GeometryRenderContext wireContext = context;
        wireContext.display.wireframeMode = true;
        wireContext.display.facesVisible = false;
        wireContext.display.displayMode = RenderingConfig::DisplayMode::Wireframe;
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Appending wireframe pass for SolidWireframe");
        appendWireframePass(wireContext);
        break;
    }
    case RenderingConfig::DisplayMode::Solid: {
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Processing Solid mode");
        GeometryRenderContext surfaceContext = context;
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Appending surface pass for Solid mode");
        appendSurfacePass(surfaceContext);
        break;
    }
    case RenderingConfig::DisplayMode::Transparent: {
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Processing Transparent mode");
        GeometryRenderContext surfaceContext = context;
        surfaceContext.display.wireframeMode = false;
        surfaceContext.display.facesVisible = true;
        if (surfaceContext.material.transparency <= 0.0) {
            surfaceContext.material.transparency = 0.5;
            LOG_INF_S("DisplayModeHandler::handleDisplayMode: Set default transparency to 0.5");
        }
        surfaceContext.blend.blendMode = RenderingConfig::BlendMode::Alpha;
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Appending surface pass for Transparent, transparency=" + 
                 std::to_string(surfaceContext.material.transparency));
        appendSurfacePass(surfaceContext);
        break;
    }
    case RenderingConfig::DisplayMode::HiddenLine: {
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Processing HiddenLine mode");
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
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Appending surface pass for HiddenLine");
        appendSurfacePass(surfaceContext);

        GeometryRenderContext wireContext = context;
        wireContext.display.wireframeMode = true;
        wireContext.display.facesVisible = false;
        wireContext.display.displayMode = RenderingConfig::DisplayMode::Wireframe;
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Appending wireframe pass for HiddenLine");
        appendWireframePass(wireContext);
        break;
    }
    default: {
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Processing default mode " + 
                 std::to_string(static_cast<int>(displayMode)));
        GeometryRenderContext surfaceContext = context;
        LOG_INF_S("DisplayModeHandler::handleDisplayMode: Appending surface pass for default mode");
        appendSurfacePass(surfaceContext);
        break;
    }
    }
    
    LOG_INF_S("DisplayModeHandler::handleDisplayMode: Completed, coinNode children count=" + 
             std::to_string(coinNode->getNumChildren()));
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
    SoSeparator* normalLineNode = edgeComponent->getEdgeNode(EdgeType::VerticeNormal);
    SoSeparator* faceNormalLineNode = edgeComponent->getEdgeNode(EdgeType::FaceNormal);
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

int DisplayModeHandler::getModeSwitchIndex(RenderingConfig::DisplayMode mode) {
    switch (mode) {
    case RenderingConfig::DisplayMode::NoShading:
        return 0;
    case RenderingConfig::DisplayMode::Points:
        return 1;
    case RenderingConfig::DisplayMode::Wireframe:
        return 2;
    case RenderingConfig::DisplayMode::Solid:
        return 3;
    case RenderingConfig::DisplayMode::FlatLines:
        return 4;
    case RenderingConfig::DisplayMode::Transparent:
        return 5;
    case RenderingConfig::DisplayMode::HiddenLine:
        return 6;
    default:
        return 3;
    }
}

void DisplayModeHandler::buildModeNode(SoSeparator* parent,
                                       RenderingConfig::DisplayMode mode,
                                       const GeometryRenderContext& context,
                                       const TopoDS_Shape& shape,
                                       const MeshParameters& params,
                                       ModularEdgeComponent* edgeComponent,
                                       bool useModularEdgeComponent,
                                       RenderNodeBuilder* renderBuilder,
                                       WireframeBuilder* wireframeBuilder) {
    if (!parent || !renderBuilder || !wireframeBuilder) {
        LOG_INF_S("DisplayModeHandler::buildModeNode: Invalid parameters");
        return;
    }

    LOG_INF_S("DisplayModeHandler::buildModeNode: Building node for mode=" + 
             std::to_string(static_cast<int>(mode)) +
             ", wireframeMode=" + std::to_string(context.display.wireframeMode ? 1 : 0) +
             ", facesVisible=" + std::to_string(context.display.facesVisible ? 1 : 0));

    GeometryRenderContext modeContext = context;
    modeContext.display.displayMode = mode;
    SoPolygonOffset* polygonOffset = nullptr;

    auto appendSurfacePass = [&](const GeometryRenderContext& ctx) {
        parent->addChild(renderBuilder->createDrawStyleNode(ctx));
        parent->addChild(renderBuilder->createMaterialNode(ctx));
        renderBuilder->appendTextureNodes(parent, ctx);
        renderBuilder->appendBlendHints(parent, ctx);
        
        if (!polygonOffset) {
            polygonOffset = renderBuilder->createPolygonOffsetNode();
            parent->addChild(polygonOffset);
        }
        
        renderBuilder->appendSurfaceGeometry(parent, shape, params, ctx);
    };

    auto appendWireframePass = [&](const GeometryRenderContext& ctx) {
        parent->addChild(renderBuilder->createDrawStyleNode(ctx));
        parent->addChild(renderBuilder->createMaterialNode(ctx));
        
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
            edgeComponent->updateEdgeDisplay(parent);
        } else {
            wireframeBuilder->createWireframeRepresentation(parent, shape, params);
        }
    };

    switch (mode) {
    case RenderingConfig::DisplayMode::NoShading: {
        LOG_INF_S("DisplayModeHandler::buildModeNode: Processing NoShading mode");
        GeometryRenderContext surfaceContext = modeContext;
        surfaceContext.display.wireframeMode = false;
        surfaceContext.display.displayMode = RenderingConfig::DisplayMode::NoShading;
        surfaceContext.display.facesVisible = true;
        surfaceContext.texture.enabled = false;
        surfaceContext.material.ambientColor = Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB);
        surfaceContext.material.diffuseColor = Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB);
        surfaceContext.material.specularColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        surfaceContext.material.emissiveColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
        surfaceContext.material.shininess = 0.0;
        appendSurfacePass(surfaceContext);
        break;
    }
    case RenderingConfig::DisplayMode::Points: {
        LOG_INF_S("DisplayModeHandler::buildModeNode: Processing Points mode");
        if (modeContext.display.showPointView) {
            GeometryRenderContext surfaceContext = modeContext;
            surfaceContext.display.wireframeMode = false;
            surfaceContext.display.facesVisible = modeContext.display.showSolidWithPointView;
            if (surfaceContext.display.facesVisible) {
                appendSurfacePass(surfaceContext);
            }
        }
        break;
    }
    case RenderingConfig::DisplayMode::Wireframe: {
        LOG_INF_S("DisplayModeHandler::buildModeNode: Processing Wireframe mode");
        if (modeContext.display.facesVisible) {
            GeometryRenderContext surfaceContext = modeContext;
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

        GeometryRenderContext wireContext = modeContext;
        wireContext.display.wireframeMode = true;
        wireContext.display.facesVisible = false;
        appendWireframePass(wireContext);
        break;
    }
    case RenderingConfig::DisplayMode::FlatLines: {
        LOG_INF_S("DisplayModeHandler::buildModeNode: Processing SolidWireframe mode");
        GeometryRenderContext surfaceContext = modeContext;
        surfaceContext.display.wireframeMode = false;
        appendSurfacePass(surfaceContext);

        GeometryRenderContext wireContext = modeContext;
        wireContext.display.wireframeMode = true;
        wireContext.display.facesVisible = false;
        wireContext.display.displayMode = RenderingConfig::DisplayMode::Wireframe;
        appendWireframePass(wireContext);
        break;
    }
    case RenderingConfig::DisplayMode::Solid: {
        LOG_INF_S("DisplayModeHandler::buildModeNode: Processing Solid mode");
        GeometryRenderContext surfaceContext = modeContext;
        appendSurfacePass(surfaceContext);
        break;
    }
    case RenderingConfig::DisplayMode::Transparent: {
        LOG_INF_S("DisplayModeHandler::buildModeNode: Processing Transparent mode");
        GeometryRenderContext surfaceContext = modeContext;
        surfaceContext.display.wireframeMode = false;
        surfaceContext.display.facesVisible = true;
        if (surfaceContext.material.transparency <= 0.0) {
            surfaceContext.material.transparency = 0.5;
        }
        surfaceContext.blend.blendMode = RenderingConfig::BlendMode::Alpha;
        appendSurfacePass(surfaceContext);
        break;
    }
    case RenderingConfig::DisplayMode::HiddenLine: {
        LOG_INF_S("DisplayModeHandler::buildModeNode: Processing HiddenLine mode");
        GeometryRenderContext surfaceContext = modeContext;
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

        GeometryRenderContext wireContext = modeContext;
        wireContext.display.wireframeMode = true;
        wireContext.display.facesVisible = false;
        wireContext.display.displayMode = RenderingConfig::DisplayMode::Wireframe;
        appendWireframePass(wireContext);
        break;
    }
    default: {
        LOG_INF_S("DisplayModeHandler::buildModeNode: Processing default mode " + 
                 std::to_string(static_cast<int>(mode)));
        GeometryRenderContext surfaceContext = modeContext;
        appendSurfacePass(surfaceContext);
        break;
    }
    }
}
