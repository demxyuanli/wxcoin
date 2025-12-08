#include "geometry/CoinSceneBuilder.h"
#include "viewer/ObjectDisplayModeManager.h"
#include "edges/ModularEdgeComponent.h"
#include "geometry/VertexExtractor.h"
#include "geometry/FaceDomainManager.h"
#include "geometry/TriangleMappingManager.h"
#include "geometry/GeometryRenderContext.h"
#include "geometry/FaceDomainTypes.h"  // For FaceDomain, TriangleSegment, BoundaryTriangle structs
#include "config/EdgeSettingsConfig.h"
#include "rendering/RenderingToolkitAPI.h"
#include "rendering/OpenCASCADEProcessor.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoMaterial.h>
#include <OpenCASCADE/TopAbs.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>

CoinSceneBuilder::CoinSceneBuilder()
{
}

CoinSceneBuilder::~CoinSceneBuilder()
{
}

SoSeparator* CoinSceneBuilder::createRootNode()
{
    SoSeparator* root = new SoSeparator();
    if (root) {
        root->renderCaching.setValue(SoSeparator::OFF);
        root->boundingBoxCaching.setValue(SoSeparator::OFF);
        root->pickCulling.setValue(SoSeparator::OFF);
        root->ref();
    }
    return root;
}

void CoinSceneBuilder::addTransformAndHints(SoSeparator* root, const GeometryRenderContext& context)
{
    if (!root) return;

    // Add transform
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
    root->addChild(transform);

    // Add shape hints
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
    root->addChild(hints);
}

SoSeparator* CoinSceneBuilder::buildSceneGraph(
    const TopoDS_Shape& shape,
    const MeshParameters& params,
    const GeometryRenderContext& context,
    ObjectDisplayModeManager* modeManager,
    ModularEdgeComponent* edgeComponent,
    VertexExtractor* vertexExtractor,
    FaceDomainManager* faceDomainManager,
    TriangleMappingManager* triangleMappingManager)
{
    if (shape.IsNull()) {
        return nullptr;
    }

    // Create root node
    SoSeparator* root = createRootNode();
    if (!root) {
        LOG_ERR_S("CoinSceneBuilder::buildSceneGraph: Failed to create root node");
        return nullptr;
    }

    // Add transform and hints
    addTransformAndHints(root, context);

    // Create SoSwitch for mode switching
    SoSwitch* modeSwitch = new SoSwitch();
    modeSwitch->ref();
    root->addChild(modeSwitch);

    // Build mode nodes using ObjectDisplayModeManager
    if (modeManager) {
        SoSwitch* builtSwitch = modeManager->buildModeSwitch(
            shape, params, context,
            edgeComponent,
            vertexExtractor
        );

        if (builtSwitch && builtSwitch != modeSwitch) {
            // Replace mode switch
            root->removeChild(root->getNumChildren() - 1); // Remove old switch
            modeSwitch->unref();
            modeSwitch = builtSwitch;
            root->addChild(modeSwitch);
        }
    } else {
        LOG_ERR_S("CoinSceneBuilder::buildSceneGraph: ObjectDisplayModeManager not provided");
    }

    // Setup edge display for non-wireframe modes
    if (!context.display.wireframeMode && edgeComponent) {
        setupEdgeDisplay(modeSwitch, shape, params, context, edgeComponent);
    }

    // Set visibility
    root->renderCulling = context.display.visible ? SoSeparator::OFF : SoSeparator::ON;

    return root;
}

void CoinSceneBuilder::setupEdgeDisplay(
    SoSwitch* modeSwitch,
    const TopoDS_Shape& shape,
    const MeshParameters& params,
    const GeometryRenderContext& context,
    ModularEdgeComponent* edgeComponent)
{
    if (!modeSwitch || !edgeComponent) {
        return;
    }

    // Check if any edge display is requested
    bool anyEdgeDisplayRequested = false;
    EdgeDisplayFlags currentFlags;

    if (edgeComponent) {
        currentFlags = edgeComponent->edgeFlags;
        anyEdgeDisplayRequested = currentFlags.showOriginalEdges || currentFlags.showFeatureEdges ||
            currentFlags.showMeshEdges || currentFlags.showHighlightEdges ||
            currentFlags.showNormalLines || currentFlags.showFaceNormalLines;
    }

    // Check EdgeSettingsConfig for global/selected/hover edge settings
    const EdgeSettingsConfig& edgeCfg = EdgeSettingsConfig::getInstance();
    anyEdgeDisplayRequested = anyEdgeDisplayRequested ||
        edgeCfg.getGlobalSettings().showEdges ||
        edgeCfg.getSelectedSettings().showEdges ||
        edgeCfg.getHoverSettings().showEdges;

    if (!anyEdgeDisplayRequested) {
        return;
    }

    // Get mesh data if needed for mesh edges or normal lines
    auto& manager = RenderingToolkitAPI::getManager();
    auto processor = manager.getGeometryProcessor("OpenCASCADE");
    TriangleMesh mesh;

    if ((currentFlags.showMeshEdges || currentFlags.showNormalLines || currentFlags.showFaceNormalLines) && processor) {
        mesh = processor->convertToMesh(shape, params);
    }

    // Extract edges using modular edge component
    // CRITICAL FIX: Only show feature edges if explicitly requested
    // By default, only show original edges to avoid confusion
    if (edgeComponent) {
        if (currentFlags.showOriginalEdges) {
            Quantity_Color originalColor(1.0, 1.0, 1.0, Quantity_TOC_RGB);
            edgeComponent->extractOriginalEdges(shape, 80.0, 0.01, false, originalColor, 1.0, false, Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB), 3.0);
        }
        // CRITICAL FIX: Only extract feature edges if explicitly enabled
        // Feature edges should not be shown by default during initialization
        // They should only be shown when user explicitly enables them through UI
        // In Wireframe mode, feature edges are handled by WireframeMode class
        if (currentFlags.showFeatureEdges) {
            // Use context color if available, otherwise use default gray (not red)
            Quantity_Color featureColor(0.8, 0.8, 0.8, Quantity_TOC_RGB);
            edgeComponent->extractFeatureEdges(shape, 15.0, 0.005, false, false, featureColor, 1.0);
        }
        if (currentFlags.showMeshEdges && !mesh.triangles.empty()) {
            Quantity_Color meshColor(0.0, 0.0, 0.0, Quantity_TOC_RGB);
            edgeComponent->extractMeshEdges(mesh, meshColor, 1.0);
        }
        if (currentFlags.showNormalLines && !mesh.triangles.empty()) {
            edgeComponent->generateNormalLineNode(mesh, 0.5);
        }
        if (currentFlags.showFaceNormalLines && !mesh.triangles.empty()) {
            edgeComponent->generateFaceNormalLineNode(mesh, 0.5);
        }
        if (currentFlags.showHighlightEdges) {
            edgeComponent->generateHighlightEdgeNode();
        }

        // Add edges to FlatLines mode (child 2) and Shaded mode (child 3)
        if (modeSwitch->getNumChildren() >= 4) {
            // Child 2: FlatLines mode
            SoNode* flatLinesNode = modeSwitch->getChild(2);
            if (flatLinesNode && flatLinesNode->isOfType(SoSeparator::getClassTypeId())) {
                edgeComponent->updateEdgeDisplay(static_cast<SoSeparator*>(flatLinesNode));
            }
            // Child 3: Shaded mode
            SoNode* shadedNode = modeSwitch->getChild(3);
            if (shadedNode && shadedNode->isOfType(SoSeparator::getClassTypeId())) {
                edgeComponent->updateEdgeDisplay(static_cast<SoSeparator*>(shadedNode));
            }
        }
    }
}

void CoinSceneBuilder::updateDisplayMode(
    SoSwitch* modeSwitch,
    RenderingConfig::DisplayMode mode,
    ObjectDisplayModeManager* modeManager)
{
    if (!modeSwitch || !modeManager) {
        return;
    }

    // CRITICAL FIX: Update material in mode nodes for modes that share the same index
    // FlatLines, ShadedWireframe, and HiddenLine all use index 2, but need different materials
    // Shaded and NoShading both use index 3, but need different materials
    
    // First, update SoSwitch whichChild for fast switching
    modeManager->updateDisplayMode(modeSwitch, mode);
    
    // Then, update material in the active mode node if needed
    int modeIndex = -1;
    switch (mode) {
    case RenderingConfig::DisplayMode::Points:
        modeIndex = 0;
        break;
    case RenderingConfig::DisplayMode::Wireframe:
        modeIndex = 1;
        break;
    case RenderingConfig::DisplayMode::SolidWireframe:
    case RenderingConfig::DisplayMode::HiddenLine:
        modeIndex = 2;
        // Update material in FlatLines mode node for HiddenLine
        if (mode == RenderingConfig::DisplayMode::HiddenLine && modeSwitch->getNumChildren() > modeIndex) {
            SoNode* childNode = modeSwitch->getChild(modeIndex);
            if (childNode && childNode->isOfType(SoSeparator::getClassTypeId())) {
                SoSeparator* modeNode = static_cast<SoSeparator*>(childNode);
                updateMaterialInModeNode(modeNode, mode);
            }
        }
        break;
    case RenderingConfig::DisplayMode::Solid:
    case RenderingConfig::DisplayMode::NoShading:
        modeIndex = 3;
        // Update material in Shaded mode node for NoShading
        if (mode == RenderingConfig::DisplayMode::NoShading && modeSwitch->getNumChildren() > modeIndex) {
            SoNode* childNode = modeSwitch->getChild(modeIndex);
            if (childNode && childNode->isOfType(SoSeparator::getClassTypeId())) {
                SoSeparator* modeNode = static_cast<SoSeparator*>(childNode);
                updateMaterialInModeNode(modeNode, mode);
            }
        }
        break;
    default:
        break;
    }
}

void CoinSceneBuilder::updateMaterialInModeNode(SoSeparator* modeNode, RenderingConfig::DisplayMode mode)
{
    if (!modeNode) return;
    
    // Find SoMaterial node in the mode node
    for (int i = 0; i < modeNode->getNumChildren(); ++i) {
        SoNode* child = modeNode->getChild(i);
        if (child && child->isOfType(SoMaterial::getClassTypeId())) {
            SoMaterial* material = static_cast<SoMaterial*>(child);
            
            if (mode == RenderingConfig::DisplayMode::NoShading) {
                // NoShading: uniform color, no lighting
                material->diffuseColor.setValue(0.8f, 0.8f, 0.8f);
                material->ambientColor.setValue(0.0f, 0.0f, 0.0f);
                material->specularColor.setValue(0.0f, 0.0f, 0.0f);
                material->emissiveColor.setValue(0.0f, 0.0f, 0.0f);
                material->shininess.setValue(0.0f);
            } else if (mode == RenderingConfig::DisplayMode::HiddenLine) {
                // HiddenLine: darker faces to make edges stand out
                // Get current diffuse color and darken it
                SbColor currentDiffuse = material->diffuseColor[0];
                material->diffuseColor.setValue(
                    currentDiffuse[0] * 0.5f,
                    currentDiffuse[1] * 0.5f,
                    currentDiffuse[2] * 0.5f
                );
                SbColor currentAmbient = material->ambientColor[0];
                material->ambientColor.setValue(
                    currentAmbient[0] * 0.8f,
                    currentAmbient[1] * 0.8f,
                    currentAmbient[2] * 0.8f
                );
            }
            // For other modes, material is already correct from build time
            break;
        }
    }
}

void CoinSceneBuilder::updateWireframeMaterial(SoSeparator* coinNode, const Quantity_Color& color)
{
    if (!coinNode) {
        return;
    }

    // Find the material node in the Coin scene graph
    SoMaterial* material = nullptr;

    for (int i = 0; i < coinNode->getNumChildren(); ++i) {
        SoNode* child = coinNode->getChild(i);
        if (child->isOfType(SoMaterial::getClassTypeId())) {
            material = static_cast<SoMaterial*>(child);
            break;
        }
    }

    if (material) {
        material->diffuseColor.setValue(
            static_cast<float>(color.Red()),
            static_cast<float>(color.Green()),
            static_cast<float>(color.Blue())
        );
    }
}

