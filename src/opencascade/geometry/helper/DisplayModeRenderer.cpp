#include "geometry/helper/DisplayModeRenderer.h"
#include "geometry/helper/DisplayModeStateManager.h"
#include "geometry/helper/DisplayModeNodeManager.h"
#include "rendering/PolygonModeNode.h"
#include "geometry/helper/RenderNodeBuilder.h"
#include "geometry/helper/WireframeBuilder.h"
#include "geometry/helper/PointViewBuilder.h"
#include "edges/ModularEdgeComponent.h"
#include "config/RenderingConfig.h"
#include "rendering/RenderingToolkitAPI.h"
#include "EdgeTypes.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/SoType.h>
#include <OpenCASCADE/Quantity_Color.hxx>
#include <OpenCASCADE/TopoDS_Shape.hxx>

void DisplayModeRenderer::applyRenderState(SoSeparator* coinNode,
                         const DisplayModeRenderState& state,
                         const GeometryRenderContext& context,
                         const TopoDS_Shape& shape,
                         const MeshParameters& params,
                         ModularEdgeComponent* edgeComponent,
                         bool useModularEdgeComponent,
                         RenderNodeBuilder* renderBuilder,
                         WireframeBuilder* wireframeBuilder,
                         PointViewBuilder* pointViewBuilder) {
    if (!coinNode || !renderBuilder || !wireframeBuilder) {
        return;
    }
    
    SoPolygonOffset* polygonOffset = nullptr;

    bool skipNormalSurfacePass = false;
    
    if (state.showSurface && !skipNormalSurfacePass) {
        GeometryRenderContext surfaceContext = context;
        surfaceContext.display.wireframeMode = state.wireframeMode;
        surfaceContext.display.facesVisible = state.showSurface;
        surfaceContext.display.displayMode = state.surfaceDisplayMode;
        surfaceContext.texture.enabled = state.textureEnabled;
        surfaceContext.material.ambientColor = state.surfaceAmbientColor;
        surfaceContext.material.diffuseColor = state.surfaceDiffuseColor;
        surfaceContext.material.specularColor = state.surfaceSpecularColor;
        surfaceContext.material.emissiveColor = state.surfaceEmissiveColor;
        surfaceContext.material.shininess = state.shininess;
        surfaceContext.material.transparency = state.transparency;
        surfaceContext.blend.blendMode = state.blendMode;

        SoLightModel* lightModel = new SoLightModel();
        if (!state.lightingEnabled || state.surfaceDisplayMode == RenderingConfig::DisplayMode::NoShading || 
            state.surfaceDisplayMode == RenderingConfig::DisplayMode::HiddenLine) {
            lightModel->model.setValue(SoLightModel::BASE_COLOR);
        } else {
            lightModel->model.setValue(SoLightModel::PHONG);
        }
        coinNode->addChild(lightModel);
        
        coinNode->addChild(renderBuilder->createDrawStyleNode(surfaceContext));
        coinNode->addChild(renderBuilder->createMaterialNode(surfaceContext));
        renderBuilder->appendTextureNodes(coinNode, surfaceContext);
        renderBuilder->appendBlendHints(coinNode, surfaceContext);
        
        if (!polygonOffset) {
            polygonOffset = renderBuilder->createPolygonOffsetNode();
            if (context.display.displayMode == RenderingConfig::DisplayMode::HiddenLine) {
                polygonOffset->factor.setValue(1.0f);
                polygonOffset->units.setValue(1.0f);
            } else if (context.display.displayMode == RenderingConfig::DisplayMode::Solid) {
                RenderingConfig& config = RenderingConfig::getInstance();
                bool smoothNormalsEnabled = config.getShadingSettings().smoothNormals;
                if (smoothNormalsEnabled) {
                    polygonOffset->factor.setValue(1.0f);
                    polygonOffset->units.setValue(1.0f);
                }
            }
            coinNode->addChild(polygonOffset);
        }
        
        renderBuilder->appendSurfaceGeometry(coinNode, shape, params, surfaceContext);
    }

    if (useModularEdgeComponent && edgeComponent) {
        edgeComponent->setEdgeDisplayType(EdgeType::Original, state.showOriginalEdges);
        edgeComponent->setEdgeDisplayType(EdgeType::Mesh, state.showMeshEdges);
        edgeComponent->setEdgeDisplayType(EdgeType::Feature, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Highlight, false);
        edgeComponent->setEdgeDisplayType(EdgeType::VerticeNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::FaceNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Silhouette, false);
        
        if (context.display.displayMode == RenderingConfig::DisplayMode::HiddenLine) {
            edgeComponent->clearSilhouetteEdgeNode();
            edgeComponent->setEdgeDisplayType(EdgeType::Original, false);
            edgeComponent->setEdgeDisplayType(EdgeType::Feature, false);
            edgeComponent->setEdgeDisplayType(EdgeType::Highlight, false);
            edgeComponent->setEdgeDisplayType(EdgeType::VerticeNormal, false);
            edgeComponent->setEdgeDisplayType(EdgeType::FaceNormal, false);
            edgeComponent->setEdgeDisplayType(EdgeType::Silhouette, false);
            edgeComponent->setEdgeDisplayType(EdgeType::Mesh, true);
        }
    }
    
    if (state.showOriginalEdges && context.display.displayMode != RenderingConfig::DisplayMode::HiddenLine) {
        GeometryRenderContext wireContext = context;
        wireContext.display.wireframeMode = true;
        wireContext.display.facesVisible = false;
        wireContext.display.displayMode = RenderingConfig::DisplayMode::Wireframe;
        wireContext.display.wireframeColor = state.originalEdgeColor;
        wireContext.display.wireframeWidth = state.originalEdgeWidth;

        coinNode->addChild(renderBuilder->createDrawStyleNode(wireContext));
        coinNode->addChild(renderBuilder->createMaterialNode(wireContext));
        
        if (context.display.displayMode == RenderingConfig::DisplayMode::Wireframe && state.showSurface) {
            SoPolygonOffset* edgeOffset = new SoPolygonOffset();
            edgeOffset->factor.setValue(-1.0f);
            edgeOffset->units.setValue(-1.0f);
            coinNode->addChild(edgeOffset);
        }
        
        if (useModularEdgeComponent && edgeComponent) {
            edgeComponent->extractOriginalEdges(
                shape, 
                80.0,
                0.01,
                false,
                state.originalEdgeColor,
                state.originalEdgeWidth,
                false,
                Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB),
                3.0
            );
            edgeComponent->updateEdgeDisplay(coinNode);
            
            if (context.display.displayMode == RenderingConfig::DisplayMode::Solid) {
                RenderingConfig& config = RenderingConfig::getInstance();
                bool smoothNormalsEnabled = config.getShadingSettings().smoothNormals;
                if (smoothNormalsEnabled && state.showOriginalEdges) {
                    SoPolygonOffset* edgeOffset = new SoPolygonOffset();
                    edgeOffset->factor.setValue(-1.0f);
                    edgeOffset->units.setValue(-1.0f);
                    edgeOffset->styles.setValue(SoPolygonOffset::LINES);
                    coinNode->addChild(edgeOffset);
                }
            }
        } else {
            wireframeBuilder->createWireframeRepresentation(coinNode, shape, params);
        }
    } else {
        if (useModularEdgeComponent && edgeComponent) {
            edgeComponent->updateEdgeDisplay(coinNode);
        }
    }

    if (state.showMeshEdges) {
        if (context.display.displayMode == RenderingConfig::DisplayMode::HiddenLine && 
            useModularEdgeComponent && edgeComponent) {
            auto& manager = RenderingToolkitAPI::getManager();
            auto processor = manager.getGeometryProcessor("OpenCASCADE");
            if (processor) {
                TriangleMesh mesh = processor->convertToMesh(shape, params);
                if (!mesh.triangles.empty()) {
                    Quantity_Color effectiveEdgeColor = state.meshEdgeColor;
                    if (state.meshEdgeColor.Red() > 0.4 && state.meshEdgeColor.Green() > 0.4 && state.meshEdgeColor.Blue() > 0.4) {
                        effectiveEdgeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
                    }
                    edgeComponent->extractMeshEdges(mesh, effectiveEdgeColor, state.meshEdgeWidth);
                    edgeComponent->updateEdgeDisplay(coinNode);
                }
            }
        } else {
            if (useModularEdgeComponent && edgeComponent) {
                auto& manager = RenderingToolkitAPI::getManager();
                auto processor = manager.getGeometryProcessor("OpenCASCADE");
                if (processor) {
                    TriangleMesh mesh = processor->convertToMesh(shape, params);
                    if (!mesh.triangles.empty()) {
                        edgeComponent->extractMeshEdges(mesh, state.meshEdgeColor, state.meshEdgeWidth);
                        edgeComponent->updateEdgeDisplay(coinNode);
                        
                        if (context.display.displayMode == RenderingConfig::DisplayMode::Solid) {
                            RenderingConfig& config = RenderingConfig::getInstance();
                            bool smoothNormalsEnabled = config.getShadingSettings().smoothNormals;
                            if (smoothNormalsEnabled) {
                                SoPolygonOffset* edgeOffset = new SoPolygonOffset();
                                edgeOffset->factor.setValue(-1.0f);
                                edgeOffset->units.setValue(-1.0f);
                                edgeOffset->styles.setValue(SoPolygonOffset::LINES);
                                coinNode->addChild(edgeOffset);
                            }
                        }
                    }
                }
            }
        }
    }

    if (state.showPoints && pointViewBuilder) {
        pointViewBuilder->createPointViewRepresentation(coinNode, shape, params, context.display);
    }
    
    if (m_geometryBuiltCallback) {
        m_geometryBuiltCallback(true);
    }
}

void DisplayModeRenderer::applyRenderState(SoSeparator* coinNode,
                         const DisplayModeRenderState& state,
                         const GeometryRenderContext& context,
                         const TriangleMesh& mesh,
                         const MeshParameters& params,
                         ModularEdgeComponent* edgeComponent,
                         bool useModularEdgeComponent,
                         RenderNodeBuilder* renderBuilder,
                         WireframeBuilder* wireframeBuilder,
                         PointViewBuilder* pointViewBuilder) {
    if (!coinNode || !renderBuilder || !wireframeBuilder) {
        return;
    }
    
    SoPolygonOffset* polygonOffset = nullptr;

    // CRITICAL: For HiddenLine mode with mesh edges, skip normal surface pass
    // because wireframePass will render both white surface and black lines in one pass
    bool skipNormalSurfacePass = (context.display.displayMode == RenderingConfig::DisplayMode::HiddenLine && 
                                  state.showMeshEdges);
    
    if (state.showSurface && !skipNormalSurfacePass) {
        // CRITICAL: Check if mesh is empty before rendering
        if (mesh.isEmpty() || mesh.triangles.empty()) {
            LOG_WRN_S("Cannot render surface: mesh is empty (vertices: " + 
                     std::to_string(mesh.vertices.size()) + 
                     ", triangles: " + std::to_string(mesh.triangles.size() / 3) + ")");
            // Don't return early - still need to render points/edges if needed
        } else {
            GeometryRenderContext surfaceContext = context;
            surfaceContext.display.wireframeMode = state.wireframeMode;
        surfaceContext.display.facesVisible = state.showSurface;
        surfaceContext.display.displayMode = state.surfaceDisplayMode;
        surfaceContext.texture.enabled = state.textureEnabled;
        surfaceContext.material.ambientColor = state.surfaceAmbientColor;
        surfaceContext.material.diffuseColor = state.surfaceDiffuseColor;
        surfaceContext.material.specularColor = state.surfaceSpecularColor;
        surfaceContext.material.emissiveColor = state.surfaceEmissiveColor;
        surfaceContext.material.shininess = state.shininess;
        surfaceContext.material.transparency = state.transparency;
        surfaceContext.blend.blendMode = state.blendMode;

        SoLightModel* lightModel = new SoLightModel();
        if (!state.lightingEnabled || state.surfaceDisplayMode == RenderingConfig::DisplayMode::NoShading) {
            lightModel->model.setValue(SoLightModel::BASE_COLOR);
        } else {
            lightModel->model.setValue(SoLightModel::PHONG);
        }
        coinNode->addChild(lightModel);
        
        coinNode->addChild(renderBuilder->createDrawStyleNode(surfaceContext));
        coinNode->addChild(renderBuilder->createMaterialNode(surfaceContext));
        renderBuilder->appendTextureNodes(coinNode, surfaceContext);
        renderBuilder->appendBlendHints(coinNode, surfaceContext);
        
        if (!polygonOffset) {
            polygonOffset = renderBuilder->createPolygonOffsetNode();
            if (context.display.displayMode == RenderingConfig::DisplayMode::HiddenLine) {
                polygonOffset->factor.setValue(1.0f);
                polygonOffset->units.setValue(1.0f);
            }
            coinNode->addChild(polygonOffset);
        }
        
        auto& manager = RenderingToolkitAPI::getManager();
        auto backend = manager.getRenderBackend("Coin3D");
        if (backend) {
            auto sceneNode = backend->createSceneNode(mesh, false, 
                surfaceContext.material.diffuseColor, surfaceContext.material.ambientColor,
                surfaceContext.material.specularColor, surfaceContext.material.emissiveColor,
                surfaceContext.material.shininess, surfaceContext.material.transparency);
            if (sceneNode) {
                SoSeparator* meshNode = sceneNode.get();
                if (meshNode) {
                    meshNode->ref();
                    coinNode->addChild(meshNode);
                } else {
                    LOG_WRN_S("createSceneNode returned null meshNode");
                }
            } else {
                LOG_WRN_S("createSceneNode returned null sceneNode");
            }
        } else {
            LOG_ERR_S("Coin3D render backend not found");
        }
        }
    }

    // For pure mesh models, Wireframe mode should always show mesh edges
    // Note: showOriginalEdges -> showMeshEdges conversion should be done in MeshDisplayModeHandler
    bool showMeshEdges = state.showMeshEdges;
    if (context.display.displayMode == RenderingConfig::DisplayMode::Wireframe) {
        // Wireframe mode for mesh: always show mesh edges
        showMeshEdges = true;
    }

    if (useModularEdgeComponent && edgeComponent) {
        // Pure mesh models don't have original edges, only mesh edges
        edgeComponent->setEdgeDisplayType(EdgeType::Original, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Mesh, showMeshEdges);
        edgeComponent->setEdgeDisplayType(EdgeType::Feature, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Highlight, false);
        edgeComponent->setEdgeDisplayType(EdgeType::VerticeNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::FaceNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Silhouette, false);
        
        if (context.display.displayMode == RenderingConfig::DisplayMode::HiddenLine) {
            edgeComponent->clearSilhouetteEdgeNode();
        }
    }
    
    // For Wireframe mode with mesh, extract and display mesh edges
    if (context.display.displayMode == RenderingConfig::DisplayMode::Wireframe) {
        if (useModularEdgeComponent && edgeComponent) {
            if (!mesh.triangles.empty()) {
                edgeComponent->extractMeshEdges(mesh, state.originalEdgeColor, state.originalEdgeWidth);
                edgeComponent->updateEdgeDisplay(coinNode);
            }
        }
        return;  // Wireframe mode only shows edges, no surface
    }
    
    if (useModularEdgeComponent && edgeComponent) {
        edgeComponent->updateEdgeDisplay(coinNode);
    }

    // Extract and display mesh edges if needed (for NoShading, Solid with edges, etc.)
    if (showMeshEdges) {
        if (context.display.displayMode == RenderingConfig::DisplayMode::HiddenLine) {
            if (PolygonModeNode::getClassTypeId() == SoType::badType()) {
                PolygonModeNode::initClass();
            }
            PolygonModeNode* polygonMode = new PolygonModeNode();
            polygonMode->ref();
            polygonMode->mode.setValue(PolygonModeNode::LINE);
            polygonMode->lineWidth.setValue(static_cast<float>(state.meshEdgeWidth));
            polygonMode->disableLighting.setValue(TRUE);
            polygonMode->polygonOffsetFactor.setValue(-1.0f);
            polygonMode->polygonOffsetUnits.setValue(-1.0f);
            
            SoSeparator* hiddenLinePass = new SoSeparator();
            hiddenLinePass->ref();
            
            SoPolygonOffset* surfaceOffset = new SoPolygonOffset();
            surfaceOffset->factor.setValue(1.0f);
            surfaceOffset->units.setValue(1.0f);
            hiddenLinePass->addChild(surfaceOffset);
            
            SoMaterial* whiteMaterial = new SoMaterial();
            whiteMaterial->diffuseColor.setValue(1.0f, 1.0f, 1.0f);
            whiteMaterial->ambientColor.setValue(1.0f, 1.0f, 1.0f);
            whiteMaterial->emissiveColor.setValue(1.0f, 1.0f, 1.0f);
            hiddenLinePass->addChild(whiteMaterial);
            
            SoLightModel* noLightModel = new SoLightModel();
            noLightModel->model.setValue(SoLightModel::BASE_COLOR);
            hiddenLinePass->addChild(noLightModel);
            
            auto& manager = RenderingToolkitAPI::getManager();
            auto backend = manager.getRenderBackend("Coin3D");
            if (backend) {
                auto sceneNode = backend->createSceneNode(mesh, false, 
                    Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB),
                    Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB),
                    Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB),
                    Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB),
                    0.0, 0.0);
                if (sceneNode) {
                    SoSeparator* meshNode = sceneNode.get();
                    meshNode->ref();
                    hiddenLinePass->addChild(meshNode);
                }
            }
            
            SoMaterial* edgeMaterial = new SoMaterial();
            edgeMaterial->diffuseColor.setValue(
                static_cast<float>(state.meshEdgeColor.Red()),
                static_cast<float>(state.meshEdgeColor.Green()),
                static_cast<float>(state.meshEdgeColor.Blue())
            );
            edgeMaterial->emissiveColor.setValue(
                static_cast<float>(state.meshEdgeColor.Red()),
                static_cast<float>(state.meshEdgeColor.Green()),
                static_cast<float>(state.meshEdgeColor.Blue())
            );
            hiddenLinePass->addChild(edgeMaterial);
            
            hiddenLinePass->addChild(polygonMode);
            
            if (backend) {
                auto sceneNode = backend->createSceneNode(mesh, false, 
                    state.meshEdgeColor, state.meshEdgeColor,
                    Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB),
                    state.meshEdgeColor,
                    0.0, 0.0);
                if (sceneNode) {
                    SoSeparator* meshNode = sceneNode.get();
                    meshNode->ref();
                    hiddenLinePass->addChild(meshNode);
                }
            }
            
            coinNode->addChild(hiddenLinePass);
            hiddenLinePass->unref();
            polygonMode->unref();
        } else {
            if (useModularEdgeComponent && edgeComponent) {
                if (!mesh.triangles.empty()) {
                    // Use appropriate edge color based on display mode
                    Quantity_Color edgeColor = state.meshEdgeColor;
                    if (context.display.displayMode == RenderingConfig::DisplayMode::NoShading) {
                        // NoShading mode: use original edge color (typically black) for better contrast
                        edgeColor = state.originalEdgeColor;
                    }
                    edgeComponent->extractMeshEdges(mesh, edgeColor, state.meshEdgeWidth);
                    edgeComponent->updateEdgeDisplay(coinNode);
                }
            }
        }
    }

    if (state.showPoints && pointViewBuilder) {
        pointViewBuilder->createPointViewRepresentation(coinNode, mesh, context.display);
    }
    
    if (m_geometryBuiltCallback) {
        m_geometryBuiltCallback(true);
    }
}

void DisplayModeRenderer::buildModeStateNode(SoSeparator* parent,
                           RenderingConfig::DisplayMode mode,
                           const DisplayModeRenderState& state,
                           const GeometryRenderContext& context,
                           RenderNodeBuilder* renderBuilder) {
    if (!parent || !renderBuilder) {
        return;
    }
    
    GeometryRenderContext stateContext = context;
    stateContext.display.wireframeMode = state.wireframeMode;
    stateContext.display.facesVisible = state.showSurface;
    stateContext.display.displayMode = state.surfaceDisplayMode;
    stateContext.texture.enabled = state.textureEnabled;
    stateContext.material.ambientColor = state.surfaceAmbientColor;
    stateContext.material.diffuseColor = state.surfaceDiffuseColor;
    stateContext.material.specularColor = state.surfaceSpecularColor;
    stateContext.material.emissiveColor = state.surfaceEmissiveColor;
    stateContext.material.shininess = state.shininess;
    stateContext.material.transparency = state.transparency;
    stateContext.blend.blendMode = state.blendMode;
    
    SoLightModel* lightModel = new SoLightModel();
    if (!state.lightingEnabled || state.surfaceDisplayMode == RenderingConfig::DisplayMode::NoShading) {
        lightModel->model.setValue(SoLightModel::BASE_COLOR);
    } else {
        lightModel->model.setValue(SoLightModel::PHONG);
    }
    parent->addChild(lightModel);
    
    parent->addChild(renderBuilder->createDrawStyleNode(stateContext));
    parent->addChild(renderBuilder->createMaterialNode(stateContext));
    
    renderBuilder->appendTextureNodes(parent, stateContext);
    
    renderBuilder->appendBlendHints(parent, stateContext);
    
    SoPolygonOffset* polygonOffset = renderBuilder->createPolygonOffsetNode();
    if (mode == RenderingConfig::DisplayMode::HiddenLine) {
        polygonOffset->factor.setValue(1.0f);
        polygonOffset->units.setValue(1.0f);
    }
    parent->addChild(polygonOffset);
}

void DisplayModeRenderer::buildModeNode(SoSeparator* parent,
                      RenderingConfig::DisplayMode mode,
                      const GeometryRenderContext& context,
                      const TopoDS_Shape& shape,
                      const MeshParameters& params,
                      ModularEdgeComponent* edgeComponent,
                      bool useModularEdgeComponent,
                      RenderNodeBuilder* renderBuilder,
                      WireframeBuilder* wireframeBuilder,
                      PointViewBuilder* pointViewBuilder) {
    if (!parent || !renderBuilder || !wireframeBuilder) {
        return;
    }

    DisplayModeNodeManager nodeManager;
    nodeManager.resetAllRenderStates(parent, edgeComponent);
    
    DisplayModeRenderState state;
    state.surfaceAmbientColor = context.material.ambientColor;
    state.surfaceDiffuseColor = context.material.diffuseColor;
    state.surfaceSpecularColor = context.material.specularColor;
    state.surfaceEmissiveColor = context.material.emissiveColor;
    state.shininess = context.material.shininess;
    state.transparency = context.material.transparency;
    state.originalEdgeColor = context.display.wireframeColor;
    state.meshEdgeColor = context.material.diffuseColor;
    state.originalEdgeWidth = context.display.wireframeWidth;
    state.meshEdgeWidth = context.display.wireframeWidth;
    state.textureEnabled = context.texture.enabled;
    state.blendMode = context.blend.blendMode;
    state.showPoints = context.display.showPointView;
    state.showSolidWithPoints = context.display.showSolidWithPointView;
    state.surfaceDisplayMode = mode;

    DisplayModeStateManager stateManager;
    stateManager.setRenderStateForMode(state, mode, context);

    applyRenderState(parent, state, context, shape, params, edgeComponent, 
                     useModularEdgeComponent, renderBuilder, wireframeBuilder, pointViewBuilder);
}

// ========== New Data-Driven Rendering Methods ==========

void DisplayModeRenderer::buildStateNodeFromConfig(SoSeparator* parent,
                                                    const DisplayModeConfig& config,
                                                    const GeometryRenderContext& context,
                                                    RenderNodeBuilder* renderBuilder) {
    if (!parent || !renderBuilder) {
        return;
    }
    
    // Build LightModel node
    SoLightModel* lightModel = new SoLightModel();
    if (config.rendering.lightModel == DisplayModeConfig::RenderingProperties::LightModel::BASE_COLOR) {
        lightModel->model.setValue(SoLightModel::BASE_COLOR);
    } else {
        lightModel->model.setValue(SoLightModel::PHONG);
    }
    parent->addChild(lightModel);
    
    // Build GeometryRenderContext from config
    GeometryRenderContext stateContext = context;
    // Wireframe mode is determined by requireSurface=false, not by wireframeMode flag
    stateContext.display.facesVisible = config.nodes.requireSurface;
    stateContext.texture.enabled = config.rendering.textureEnabled;
    stateContext.blend.blendMode = config.rendering.blendMode;
    
    // Apply material override if enabled
    if (config.rendering.materialOverride.enabled) {
        stateContext.material.ambientColor = config.rendering.materialOverride.ambientColor;
        stateContext.material.diffuseColor = config.rendering.materialOverride.diffuseColor;
        stateContext.material.specularColor = config.rendering.materialOverride.specularColor;
        stateContext.material.emissiveColor = config.rendering.materialOverride.emissiveColor;
        stateContext.material.shininess = config.rendering.materialOverride.shininess;
        stateContext.material.transparency = config.rendering.materialOverride.transparency;
    }
    
    // Build DrawStyle and Material nodes
    parent->addChild(renderBuilder->createDrawStyleNode(stateContext));
    parent->addChild(renderBuilder->createMaterialNode(stateContext));
    
    // Add texture nodes if enabled
    renderBuilder->appendTextureNodes(parent, stateContext);
    
    // Add blend hints if blend mode is active
    renderBuilder->appendBlendHints(parent, stateContext);
    
    // Add polygon offset if enabled
    SoPolygonOffset* polygonOffset = renderBuilder->createPolygonOffsetNode();
    if (config.postProcessing.polygonOffset.enabled) {
        polygonOffset->factor.setValue(config.postProcessing.polygonOffset.factor);
        polygonOffset->units.setValue(config.postProcessing.polygonOffset.units);
    }
    parent->addChild(polygonOffset);
}

void DisplayModeRenderer::applyRenderFromConfig(SoSeparator* coinNode,
                                                 const DisplayModeConfig& config,
                                                 const GeometryRenderContext& context,
                                                 const TopoDS_Shape& shape,
                                                 const MeshParameters& params,
                                                 ModularEdgeComponent* edgeComponent,
                                                 bool useModularEdgeComponent,
                                                 RenderNodeBuilder* renderBuilder,
                                                 WireframeBuilder* wireframeBuilder,
                                                 PointViewBuilder* pointViewBuilder) {
    if (!coinNode || !renderBuilder || !wireframeBuilder) {
        return;
    }
    
    // Apply surface rendering if required
    if (config.nodes.requireSurface) {
        GeometryRenderContext surfaceContext = context;
        surfaceContext.display.facesVisible = true;
        // Wireframe mode is determined by requireSurface=false, not by wireframeMode flag
        // When requireSurface=true, surface is always rendered as filled (not wireframe)
        surfaceContext.texture.enabled = config.rendering.textureEnabled;
        surfaceContext.blend.blendMode = config.rendering.blendMode;
        
        // Apply material override if enabled
        if (config.rendering.materialOverride.enabled) {
            surfaceContext.material.ambientColor = config.rendering.materialOverride.ambientColor;
            surfaceContext.material.diffuseColor = config.rendering.materialOverride.diffuseColor;
            surfaceContext.material.specularColor = config.rendering.materialOverride.specularColor;
            surfaceContext.material.emissiveColor = config.rendering.materialOverride.emissiveColor;
            surfaceContext.material.shininess = config.rendering.materialOverride.shininess;
            surfaceContext.material.transparency = config.rendering.materialOverride.transparency;
        }
        
        // Build lighting model
        SoLightModel* lightModel = new SoLightModel();
        if (config.rendering.lightModel == DisplayModeConfig::RenderingProperties::LightModel::BASE_COLOR) {
            lightModel->model.setValue(SoLightModel::BASE_COLOR);
        } else {
            lightModel->model.setValue(SoLightModel::PHONG);
        }
        coinNode->addChild(lightModel);
        
        coinNode->addChild(renderBuilder->createDrawStyleNode(surfaceContext));
        coinNode->addChild(renderBuilder->createMaterialNode(surfaceContext));
        renderBuilder->appendTextureNodes(coinNode, surfaceContext);
        renderBuilder->appendBlendHints(coinNode, surfaceContext);
        
        // Add polygon offset if enabled
        if (config.postProcessing.polygonOffset.enabled) {
            SoPolygonOffset* polygonOffset = renderBuilder->createPolygonOffsetNode();
            polygonOffset->factor.setValue(config.postProcessing.polygonOffset.factor);
            polygonOffset->units.setValue(config.postProcessing.polygonOffset.units);
            coinNode->addChild(polygonOffset);
        }
        
        // Add surface geometry
        renderBuilder->appendSurfaceGeometry(coinNode, shape, params, surfaceContext);
    }
    
    // Apply edge rendering based on config
    if (useModularEdgeComponent && edgeComponent) {
        // Set edge display flags based on config
        bool showOriginalEdges = config.nodes.requireOriginalEdges && config.edges.originalEdge.enabled;
        bool showMeshEdges = config.nodes.requireMeshEdges && config.edges.meshEdge.enabled;
        
        edgeComponent->setEdgeDisplayType(EdgeType::Original, showOriginalEdges);
        edgeComponent->setEdgeDisplayType(EdgeType::Mesh, showMeshEdges);
        edgeComponent->setEdgeDisplayType(EdgeType::Feature, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Highlight, false);
        edgeComponent->setEdgeDisplayType(EdgeType::VerticeNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::FaceNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Silhouette, false);
        
        // Extract edges if needed (only if not already extracted)
        if (showOriginalEdges && !edgeComponent->getEdgeNode(EdgeType::Original)) {
            edgeComponent->extractOriginalEdges(shape,
                                               80.0, 0.01, false,
                                               config.edges.originalEdge.color,
                                               config.edges.originalEdge.width,
                                               false,
                                               Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB),
                                               3.0);
        }
        
        // Apply appearance (color and width) to edges even if they already exist
        // This ensures that configuration changes are reflected in the display
        if (showOriginalEdges && edgeComponent->getEdgeNode(EdgeType::Original)) {
            edgeComponent->applyAppearanceToEdgeNode(EdgeType::Original,
                                                     config.edges.originalEdge.color,
                                                     config.edges.originalEdge.width,
                                                     0);
        }
        if (showMeshEdges && edgeComponent->getEdgeNode(EdgeType::Mesh)) {
            Quantity_Color edgeColor = config.edges.meshEdge.color;
            // Handle effective color for HiddenLine mode
            if (config.edges.meshEdge.useEffectiveColor) {
                if (edgeColor.Red() > 0.4 && edgeColor.Green() > 0.4 && edgeColor.Blue() > 0.4) {
                    edgeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
                }
            }
            edgeComponent->applyAppearanceToEdgeNode(EdgeType::Mesh,
                                                     edgeColor,
                                                     config.edges.meshEdge.width,
                                                     0);
        }
        
        // Add polygon offset for edges if enabled and edges are shown
        // This allows edges to appear on top of surfaces by adjusting depth
        if ((showOriginalEdges || showMeshEdges) && config.postProcessing.polygonOffset.enabled) {
            SoPolygonOffset* edgeOffset = renderBuilder->createPolygonOffsetNode();
            edgeOffset->factor.setValue(config.postProcessing.polygonOffset.factor);
            edgeOffset->units.setValue(config.postProcessing.polygonOffset.units);
            edgeOffset->styles.setValue(SoPolygonOffset::LINES);
            coinNode->addChild(edgeOffset);
        }
        
        edgeComponent->updateEdgeDisplay(coinNode);
    }
    
    // Apply point view if required
    if (config.nodes.requirePoints && pointViewBuilder) {
        pointViewBuilder->createPointViewRepresentation(coinNode, shape, params, context.display);
    }
}

void DisplayModeRenderer::applyRenderFromConfig(SoSeparator* coinNode,
                                                 const DisplayModeConfig& config,
                                                 const GeometryRenderContext& context,
                                                 const TriangleMesh& mesh,
                                                 const MeshParameters& params,
                                                 ModularEdgeComponent* edgeComponent,
                                                 bool useModularEdgeComponent,
                                                 RenderNodeBuilder* renderBuilder,
                                                 WireframeBuilder* wireframeBuilder,
                                                 PointViewBuilder* pointViewBuilder) {
    if (!coinNode || !renderBuilder || !wireframeBuilder) {
        return;
    }
    
    // Apply surface rendering if required
    if (config.nodes.requireSurface && !mesh.isEmpty() && !mesh.triangles.empty()) {
        auto& manager = RenderingToolkitAPI::getManager();
        auto backend = manager.getRenderBackend("Coin3D");
        if (backend) {
            // Prepare material colors
            Quantity_Color ambient = context.material.ambientColor;
            Quantity_Color diffuse = context.material.diffuseColor;
            Quantity_Color specular = context.material.specularColor;
            Quantity_Color emissive = context.material.emissiveColor;
            double shininess = context.material.shininess;
            double transparency = context.material.transparency;
            
            // Apply material override if enabled
            if (config.rendering.materialOverride.enabled) {
                ambient = config.rendering.materialOverride.ambientColor;
                diffuse = config.rendering.materialOverride.diffuseColor;
                specular = config.rendering.materialOverride.specularColor;
                emissive = config.rendering.materialOverride.emissiveColor;
                shininess = config.rendering.materialOverride.shininess;
                transparency = config.rendering.materialOverride.transparency;
            }
            
            auto sceneNode = backend->createSceneNode(mesh, false,
                diffuse, ambient, specular, emissive, shininess, transparency);
            if (sceneNode) {
                SoSeparator* meshNode = sceneNode.get();
                meshNode->ref();
                
                // Build lighting model
                SoLightModel* lightModel = new SoLightModel();
                if (config.rendering.lightModel == DisplayModeConfig::RenderingProperties::LightModel::BASE_COLOR) {
                    lightModel->model.setValue(SoLightModel::BASE_COLOR);
                } else {
                    lightModel->model.setValue(SoLightModel::PHONG);
                }
                coinNode->addChild(lightModel);
                
                coinNode->addChild(meshNode);
            }
        }
    }
    
    // Apply edge rendering based on config
    // For mesh models, convert requireOriginalEdges to requireMeshEdges
    bool showMeshEdges = config.nodes.requireMeshEdges || config.nodes.requireOriginalEdges;
    
    if (useModularEdgeComponent && edgeComponent && showMeshEdges) {
        edgeComponent->setEdgeDisplayType(EdgeType::Original, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Mesh, true);
        edgeComponent->setEdgeDisplayType(EdgeType::Feature, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Highlight, false);
        edgeComponent->setEdgeDisplayType(EdgeType::VerticeNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::FaceNormal, false);
        edgeComponent->setEdgeDisplayType(EdgeType::Silhouette, false);
        
        // Extract mesh edges if needed (only if not already extracted)
        if (!edgeComponent->getEdgeNode(EdgeType::Mesh) && !mesh.triangles.empty()) {
            Quantity_Color edgeColor = config.edges.meshEdge.color;
            
            // Handle effective color for HiddenLine mode
            if (config.edges.meshEdge.useEffectiveColor) {
                if (edgeColor.Red() > 0.4 && edgeColor.Green() > 0.4 && edgeColor.Blue() > 0.4) {
                    edgeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
                }
            }
            
            edgeComponent->extractMeshEdges(mesh, edgeColor, config.edges.meshEdge.width);
        }
        
        // Apply appearance (color and width) to mesh edges even if they already exist
        // This ensures that configuration changes are reflected in the display
        if (edgeComponent->getEdgeNode(EdgeType::Mesh)) {
            Quantity_Color edgeColor = config.edges.meshEdge.color;
            // Handle effective color for HiddenLine mode
            if (config.edges.meshEdge.useEffectiveColor) {
                if (edgeColor.Red() > 0.4 && edgeColor.Green() > 0.4 && edgeColor.Blue() > 0.4) {
                    edgeColor = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
                }
            }
            edgeComponent->applyAppearanceToEdgeNode(EdgeType::Mesh,
                                                     edgeColor,
                                                     config.edges.meshEdge.width,
                                                     0);
        }
        
        // Add polygon offset for edges if enabled
        // This allows edges to appear on top of surfaces by adjusting depth
        if (config.postProcessing.polygonOffset.enabled) {
            SoPolygonOffset* edgeOffset = renderBuilder->createPolygonOffsetNode();
            edgeOffset->factor.setValue(config.postProcessing.polygonOffset.factor);
            edgeOffset->units.setValue(config.postProcessing.polygonOffset.units);
            edgeOffset->styles.setValue(SoPolygonOffset::LINES);
            coinNode->addChild(edgeOffset);
        }
        
        edgeComponent->updateEdgeDisplay(coinNode);
    }
    
    // Apply point view if required
    if (config.nodes.requirePoints && pointViewBuilder) {
        pointViewBuilder->createPointViewRepresentation(coinNode, mesh, context.display);
    }
}


