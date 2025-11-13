#include "geometry/OCCGeometryMesh.h"
#include "geometry/GeometryRenderContext.h"
#include "edges/ModularEdgeComponent.h"
#include "logger/Logger.h"
#include "rendering/RenderingToolkitAPI.h"
#include "rendering/OpenCASCADEProcessor.h"
#include "config/EdgeSettingsConfig.h"
#include "OCCMeshConverter.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoScale.h>
#include <OpenCASCADE/TopAbs.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <chrono>
#include <algorithm>
#include <fstream>

OCCGeometryMesh::OCCGeometryMesh()
    : m_coinNode(nullptr)
    , m_coinNeedsUpdate(true)
    , m_meshRegenerationNeeded(true)
    , m_assemblyLevel(0)
    , useModularEdgeComponent(true)
    , m_lastMeshParams{}  // Initialize to default values to avoid issues caused by random values
{
    // Use only modular edge component - migration completed
    modularEdgeComponent = std::make_unique<ModularEdgeComponent>();
}

OCCGeometryMesh::~OCCGeometryMesh()
{
    if (m_coinNode) {
        m_coinNode->unref();
        m_coinNode = nullptr;
    }
}

void OCCGeometryMesh::setCoinNode(SoSeparator* node)
{
    if (m_coinNode) {
        m_coinNode->unref();
        m_coinNode = nullptr;
    }
    m_coinNode = node;
    if (m_coinNode) {
        m_coinNode->ref();
    }
}

void OCCGeometryMesh::regenerateMesh(const TopoDS_Shape& shape, const MeshParameters& params)
{
    m_meshRegenerationNeeded = true;
    m_lastMeshParams = params;
    buildCoinRepresentation(shape, params);
}

void OCCGeometryMesh::buildCoinRepresentation(const TopoDS_Shape& shape, const MeshParameters& params)
{
    auto buildStartTime = std::chrono::high_resolution_clock::now();
    
    if (shape.IsNull()) {
        LOG_WRN_S("Cannot build coin representation for null shape");
        return;
    }
    
    // Create or clear coin node
    if (!m_coinNode) {
        m_coinNode = new SoSeparator();
        m_coinNode->ref();
    } else {
        m_coinNode->removeAllChildren();
    }
    
    // Clean up any existing texture nodes to prevent memory issues
    for (int i = m_coinNode->getNumChildren() - 1; i >= 0; --i) {
        SoNode* child = m_coinNode->getChild(i);
        if (child && (child->isOfType(SoTexture2::getClassTypeId()) ||
            child->isOfType(SoTextureCoordinate2::getClassTypeId()))) {
            m_coinNode->removeChild(i);
        }
    }

    // NOTE: Transform, material, and style nodes need to be added by the caller
    // This method focuses on mesh generation. For full implementation, see the
    // material-aware version buildCoinRepresentation() below.

    // Use rendering toolkit to create scene node for solid/filled mode
    auto& manager = RenderingToolkitAPI::getManager();
    auto backend = manager.getRenderBackend("Coin3D");
    if (backend) {
        auto sceneNode = backend->createSceneNode(shape, params);
        if (sceneNode) {
            SoSeparator* meshNode = sceneNode.get();
            meshNode->ref();
            m_coinNode->addChild(meshNode);
        }
    }

    // Set update flags
    m_coinNeedsUpdate = false;
    m_meshRegenerationNeeded = false;
    m_lastMeshParams = params;
    
    auto buildEndTime = std::chrono::high_resolution_clock::now();
    auto buildDuration = std::chrono::duration_cast<std::chrono::milliseconds>(buildEndTime - buildStartTime);
}

void OCCGeometryMesh::buildCoinRepresentation(
    const TopoDS_Shape& shape,
    const MeshParameters& params,
    const Quantity_Color& diffuseColor,
    const Quantity_Color& ambientColor,
    const Quantity_Color& specularColor,
    const Quantity_Color& emissiveColor,
    double shininess,
    double transparency)
{
    auto buildStartTime = std::chrono::high_resolution_clock::now();

    if (shape.IsNull()) {
        LOG_WRN_S("Cannot build coin representation for null shape");
        return;
    }

    // Create or clear coin node
    if (!m_coinNode) {
        m_coinNode = new SoSeparator();
        m_coinNode->ref();
    } else {
        m_coinNode->removeAllChildren();
    }

    // Clean up any existing texture nodes
    for (int i = m_coinNode->getNumChildren() - 1; i >= 0; --i) {
        SoNode* child = m_coinNode->getChild(i);
        if (child && (child->isOfType(SoTexture2::getClassTypeId()) ||
            child->isOfType(SoTextureCoordinate2::getClassTypeId()))) {
            m_coinNode->removeChild(i);
        }
    }

    // Use rendering toolkit with explicit material parameters
    auto& manager = RenderingToolkitAPI::getManager();
    auto backend = manager.getRenderBackend("Coin3D");
    if (backend) {
        // Use the material-aware version to preserve custom material settings
        auto sceneNode = backend->createSceneNode(shape, params, false,
            diffuseColor, ambientColor, specularColor, emissiveColor,
            shininess, transparency);
        if (sceneNode) {
            SoSeparator* meshNode = sceneNode.get();
            meshNode->ref();
            m_coinNode->addChild(meshNode);
        }
    }

    // Set update flags
    m_coinNeedsUpdate = false;
    m_meshRegenerationNeeded = false;
    m_lastMeshParams = params;

    auto buildEndTime = std::chrono::high_resolution_clock::now();
    auto buildDuration = std::chrono::duration_cast<std::chrono::milliseconds>(buildEndTime - buildStartTime);
}

void OCCGeometryMesh::updateCoinRepresentationIfNeeded(const TopoDS_Shape& shape, const MeshParameters& params)
{
    if (m_meshRegenerationNeeded || m_coinNeedsUpdate) {
        buildCoinRepresentation(shape, params);
    }
}

void OCCGeometryMesh::forceCoinRepresentationRebuild(const TopoDS_Shape& shape, const MeshParameters& params)
{
    m_meshRegenerationNeeded = true;
    m_coinNeedsUpdate = true;
    buildCoinRepresentation(shape, params);
}

void OCCGeometryMesh::setEdgeDisplayType(EdgeType type, bool show)
{
    if (modularEdgeComponent) {
        modularEdgeComponent->setEdgeDisplayType(type, show);
    }
}

bool OCCGeometryMesh::isEdgeDisplayTypeEnabled(EdgeType type) const
{
    return modularEdgeComponent ? modularEdgeComponent->isEdgeDisplayTypeEnabled(type) : false;
}

void OCCGeometryMesh::updateEdgeDisplay()
{
    if (modularEdgeComponent && m_coinNode) {
        modularEdgeComponent->updateEdgeDisplay(m_coinNode);
    }
}

bool OCCGeometryMesh::hasOriginalEdges() const
{
    return modularEdgeComponent ? modularEdgeComponent->isEdgeDisplayTypeEnabled(EdgeType::Original) : false;
}

void OCCGeometryMesh::enableModularEdgeComponent(bool enable)
{
    // Migration completed - always use modular edge component
    if (!enable) {
        LOG_WRN_S("Legacy edge component no longer supported - using modular component");
    }
    useModularEdgeComponent = true;
}

// Original getGeometryFaceIdForTriangle implementation moved to bottom of file
// to be replaced with optimized version

std::vector<int> OCCGeometryMesh::getTrianglesForGeometryFace(int geometryFaceId) const
{
    if (!hasFaceIndexMapping()) {
        return {};
    }

    for (const auto& mapping : m_faceIndexMappings) {
        if (mapping.geometryFaceId == geometryFaceId) {
            return mapping.triangleIndices;
        }
    }

    return {};
}

void OCCGeometryMesh::buildFaceIndexMapping(const TopoDS_Shape& shape, const MeshParameters& params)
{
    try {
        if (shape.IsNull()) {
            LOG_WRN_S("Cannot build face index mapping for null shape");
            return;
        }

        m_faceIndexMappings.clear();

        // Extract all faces from the shape
        std::vector<TopoDS_Face> faces;
        for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
            faces.push_back(TopoDS::Face(exp.Current()));
        }

        if (faces.empty()) {
            LOG_WRN_S("No faces found in shape for index mapping");
            return;
        }


        // Use the processor to generate mesh with face mapping
        auto& manager = RenderingToolkitAPI::getManager();
        auto* baseProcessor = manager.getGeometryProcessor("OpenCASCADE");
        auto* processor = dynamic_cast<OpenCASCADEProcessor*>(baseProcessor);
        
        if (processor) {
            std::vector<std::pair<int, std::vector<int>>> faceMappings;
            TriangleMesh meshWithMapping = processor->convertToMeshWithFaceMapping(shape, params, faceMappings);
            
            if (!faceMappings.empty()) {
                // Build face index mappings
                m_faceIndexMappings.reserve(faceMappings.size());
                
                for (const auto& [faceId, triangleIndices] : faceMappings) {
                    FaceIndexMapping mapping(faceId);
                    mapping.triangleIndices = triangleIndices;
                    m_faceIndexMappings.push_back(mapping);
                }
                
            } else {
                LOG_WRN_S("No face mappings generated");
            }
        } else {
            LOG_ERR_S("OpenCASCADE processor not available for face mapping");
        }
    }
    catch (const std::exception& e) {
        LOG_ERR_S("Failed to build face index mapping: " + std::string(e.what()));
        m_faceIndexMappings.clear();
    }
}

void OCCGeometryMesh::releaseTemporaryData()
{
    // Release any temporary mesh generation data
}

void OCCGeometryMesh::optimizeMemory()
{
    // Optimize memory usage
    m_faceIndexMappings.shrink_to_fit();
}

void OCCGeometryMesh::createWireframeRepresentation(const TopoDS_Shape& shape, const MeshParameters& params)
{
    if (shape.IsNull()) {
        LOG_WRN_S("Cannot create wireframe for null shape");
        return;
    }

    if (!m_coinNode) {
        LOG_ERR_S("Coin node not initialized for wireframe");
        return;
    }

    // Convert shape to mesh for wireframe generation
    auto& manager = RenderingToolkitAPI::getManager();
    auto processor = manager.getGeometryProcessor("OpenCASCADE");
    if (!processor) {
        LOG_ERR_S("OpenCASCADE processor not found for wireframe generation");
        return;
    }

    TriangleMesh mesh = processor->convertToMesh(shape, params);
    if (mesh.isEmpty()) {
        LOG_WRN_S("Empty mesh generated for wireframe representation");
        return;
    }

    // Create coordinate node
    SoCoordinate3* coords = new SoCoordinate3();
    std::vector<float> vertices;
    vertices.reserve(mesh.vertices.size() * 3);
    for (const auto& vertex : mesh.vertices) {
        vertices.push_back(static_cast<float>(vertex.X()));
        vertices.push_back(static_cast<float>(vertex.Y()));
        vertices.push_back(static_cast<float>(vertex.Z()));
    }
    coords->point.setValues(0, static_cast<int>(mesh.vertices.size()),
        reinterpret_cast<const SbVec3f*>(vertices.data()));
    m_coinNode->addChild(coords);

    // Create wireframe line set
    SoIndexedLineSet* lineSet = new SoIndexedLineSet();
    std::vector<int32_t> indices;
    indices.reserve(mesh.triangles.size() * 4); // Each triangle = 3 edges * (2 vertices + 1 end marker)

    // Create wireframe from triangle edges
    for (size_t i = 0; i < mesh.triangles.size(); i += 3) {
        int v0 = mesh.triangles[i];
        int v1 = mesh.triangles[i + 1];
        int v2 = mesh.triangles[i + 2];

        // Add three edges of the triangle
        indices.push_back(v0);
        indices.push_back(v1);
        indices.push_back(SO_END_LINE_INDEX);

        indices.push_back(v1);
        indices.push_back(v2);
        indices.push_back(SO_END_LINE_INDEX);

        indices.push_back(v2);
        indices.push_back(v0);
        indices.push_back(SO_END_LINE_INDEX);
    }

    lineSet->coordIndex.setValues(0, static_cast<int>(indices.size()), indices.data());
    m_coinNode->addChild(lineSet);
    
}

// ========== NEW MODULAR INTERFACE ==========
// This is the truly modular implementation that doesn't depend on other modules

void OCCGeometryMesh::buildCoinRepresentation(
    const TopoDS_Shape& shape,
    const MeshParameters& params,
    const GeometryRenderContext& context)
{
    auto buildStartTime = std::chrono::high_resolution_clock::now();

    if (shape.IsNull()) {
        LOG_WRN_S("Cannot build coin representation for null shape");
        return;
    }

    // Create or clear coin node with error checking
    if (!m_coinNode) {
        try {
            m_coinNode = new SoSeparator();
            if (m_coinNode) {
                m_coinNode->ref();
            } else {
                LOG_ERR_S("OCCGeometryMesh::buildCoinRepresentation: Failed to create SoSeparator");
                return;
            }
        } catch (const std::exception& e) {
            LOG_ERR_S("OCCGeometryMesh::buildCoinRepresentation: Exception creating SoSeparator: " + std::string(e.what()));
            return;
        }
    } else {
        try {
            m_coinNode->removeAllChildren();
        } catch (const std::exception& e) {
            LOG_ERR_S("OCCGeometryMesh::buildCoinRepresentation: Exception removing children: " + std::string(e.what()));
            // Try to recover by creating a new node
            m_coinNode->unref();
            m_coinNode = new SoSeparator();
            m_coinNode->ref();
        }
    }

    // Check if mesh parameters changed - if so, clear mesh-dependent edge nodes
    // This ensures that when edges are re-enabled, they will be regenerated with new mesh quality
    bool meshParamsChanged = (m_lastMeshParams.deflection != params.deflection ||
                               m_lastMeshParams.angularDeflection != params.angularDeflection);

    if (meshParamsChanged) {
        if (modularEdgeComponent) {
            // Clear mesh-dependent edge nodes for modular component
            modularEdgeComponent->clearMeshEdgeNode();
            // Also clear normal-related nodes since they depend on mesh quality
            modularEdgeComponent->clearEdgeNode(EdgeType::NormalLine);
            modularEdgeComponent->clearEdgeNode(EdgeType::FaceNormalLine);
        }
    }

    // Clean up any existing texture nodes
    for (int i = m_coinNode->getNumChildren() - 1; i >= 0; --i) {
        SoNode* child = m_coinNode->getChild(i);
        if (child && (child->isOfType(SoTexture2::getClassTypeId()) ||
            child->isOfType(SoTextureCoordinate2::getClassTypeId()))) {
            m_coinNode->removeChild(i);
        }
    }

    // ===== Transform setup =====
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
    m_coinNode->addChild(transform);

    // ===== Shape hints =====
    SoShapeHints* hints = new SoShapeHints();
    
    bool isShellModel = (context.display.shapeType == TopAbs_SHELL) || !context.display.cullFace;
    if (isShellModel) {
        // For shell models (pipes, thin-wall parts), disable backface culling completely
        // Use unknown ordering so Coin does not assume a specific front face
        hints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
        hints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;  // Not a closed solid
        hints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;    // Disable front/back distinction
    } else {
        // For solid models, use standard settings with backface culling
        hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
        hints->shapeType = SoShapeHints::SOLID;
        hints->faceType = SoShapeHints::CONVEX;
    }
    m_coinNode->addChild(hints);

    auto createDrawStyleNode = [&](const GeometryRenderContext& ctx) {
        SoDrawStyle* node = new SoDrawStyle();
        node->style = ctx.display.wireframeMode ? SoDrawStyle::LINES : SoDrawStyle::FILLED;
        node->lineWidth = ctx.display.wireframeMode ? static_cast<float>(ctx.display.wireframeWidth) : 0.0f;
        return node;
    };

    auto createMaterialNode = [&](const GeometryRenderContext& ctx) {
        SoMaterial* node = new SoMaterial();
        if (ctx.display.wireframeMode) {
            const Quantity_Color& wColor = ctx.display.wireframeColor;
            node->diffuseColor.setValue(
                static_cast<float>(wColor.Red()),
                static_cast<float>(wColor.Green()),
                static_cast<float>(wColor.Blue())
            );
            node->transparency.setValue(static_cast<float>(ctx.material.transparency));
        }
        else if (ctx.display.displayMode == RenderingConfig::DisplayMode::NoShading) {
            node->diffuseColor.setValue(0.8f, 0.8f, 0.8f);
            node->ambientColor.setValue(0.0f, 0.0f, 0.0f);
            node->specularColor.setValue(0.0f, 0.0f, 0.0f);
            node->emissiveColor.setValue(0.0f, 0.0f, 0.0f);
            node->shininess.setValue(0.0f);
            node->transparency.setValue(static_cast<float>(ctx.material.transparency));
        }
        else {
            Standard_Real r, g, b;
            ctx.material.ambientColor.Values(r, g, b, Quantity_TOC_RGB);
            node->ambientColor.setValue(
                static_cast<float>(r * 1.5),
                static_cast<float>(g * 1.5),
                static_cast<float>(b * 1.5)
            );

            ctx.material.diffuseColor.Values(r, g, b, Quantity_TOC_RGB);
            node->diffuseColor.setValue(
                static_cast<float>(r * 0.8),
                static_cast<float>(g * 0.8),
                static_cast<float>(b * 0.8)
            );

            ctx.material.specularColor.Values(r, g, b, Quantity_TOC_RGB);
            node->specularColor.setValue(
                static_cast<float>(r),
                static_cast<float>(g),
                static_cast<float>(b)
            );

            node->shininess.setValue(static_cast<float>(ctx.material.shininess / 100.0));
            double appliedTransparency = ctx.display.facesVisible ? ctx.material.transparency : 1.0;
            node->transparency.setValue(static_cast<float>(appliedTransparency));

            ctx.material.emissiveColor.Values(r, g, b, Quantity_TOC_RGB);
            node->emissiveColor.setValue(
                static_cast<float>(r),
                static_cast<float>(g),
                static_cast<float>(b)
            );
        }
        return node;
    };

    auto appendTextureNodes = [&](const GeometryRenderContext& ctx) {
        if (!ctx.texture.enabled || ctx.texture.imagePath.empty()) {
            return;
        }

        std::ifstream fileCheck(ctx.texture.imagePath);
        if (!fileCheck.good()) {
            LOG_WRN_S("Texture file not found: " + ctx.texture.imagePath);
            return;
        }

        fileCheck.close();
        try {
            SoTexture2* texture = new SoTexture2();
            texture->filename.setValue(ctx.texture.imagePath.c_str());

            switch (ctx.texture.mode) {
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

            m_coinNode->addChild(texture);
            m_coinNode->addChild(new SoTextureCoordinate2());
        }
        catch (const std::exception& e) {
            LOG_ERR_S("Exception loading texture: " + std::string(e.what()));
        }
    };

    auto appendBlendHints = [&](const GeometryRenderContext& ctx) {
        if (ctx.blend.blendMode == RenderingConfig::BlendMode::None || ctx.material.transparency <= 0.0) {
            return;
        }

        SoShapeHints* blendHints = new SoShapeHints();
        blendHints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
        blendHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
        m_coinNode->addChild(blendHints);
    };

    auto appendSurfaceGeometry = [&](const GeometryRenderContext& ctx) {
        auto& manager = RenderingToolkitAPI::getManager();
        auto backend = manager.getRenderBackend("Coin3D");
        if (!backend) {
            return;
        }

        bool shouldShowFaces = ctx.display.facesVisible;
        if (ctx.display.showPointView) {
            shouldShowFaces = shouldShowFaces && ctx.display.showSolidWithPointView;
        }

        auto sceneNode = backend->createSceneNode(shape, params, ctx.display.selected,
            ctx.material.diffuseColor, ctx.material.ambientColor,
            ctx.material.specularColor, ctx.material.emissiveColor,
            ctx.material.shininess, ctx.material.transparency);
        if (sceneNode && shouldShowFaces) {
            SoSeparator* meshNode = sceneNode.get();
            meshNode->ref();
            m_coinNode->addChild(meshNode);
        }
    };

    auto appendSurfacePass = [&](const GeometryRenderContext& ctx) {
        m_coinNode->addChild(createDrawStyleNode(ctx));
        m_coinNode->addChild(createMaterialNode(ctx));
        appendTextureNodes(ctx);
        appendBlendHints(ctx);
        appendSurfaceGeometry(ctx);
    };

    auto appendWireframePass = [&](const GeometryRenderContext& ctx) {
        m_coinNode->addChild(createDrawStyleNode(ctx));
        m_coinNode->addChild(createMaterialNode(ctx));
        createWireframeRepresentation(shape, params);
    };

    const RenderingConfig::DisplayMode displayMode = context.display.displayMode;

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

    // ===== Point view rendering =====
    if (context.display.showPointView) {
        createPointViewRepresentation(shape, params, context.display);
    }

    // ===== Set visibility =====
    m_coinNode->renderCulling = context.display.visible ? SoSeparator::OFF : SoSeparator::ON;

    // ===== Edge component handling =====
    // NOTE: Edge component only processes when NOT in wireframe mode
    // Wireframe mode already shows all edges, edge overlay is for solid mode
    bool anyEdgeDisplayRequested = false;
    EdgeDisplayFlags currentFlags;

    if (useModularEdgeComponent && modularEdgeComponent && !context.display.wireframeMode) {
        currentFlags = modularEdgeComponent->edgeFlags;
        anyEdgeDisplayRequested = currentFlags.showOriginalEdges || currentFlags.showFeatureEdges ||
            currentFlags.showMeshEdges || currentFlags.showHighlightEdges ||
            currentFlags.showNormalLines || currentFlags.showFaceNormalLines;
    }
    
    // Check EdgeSettingsConfig for global/selected/hover edge settings
    if (!context.display.wireframeMode) {
        const EdgeSettingsConfig& edgeCfg = EdgeSettingsConfig::getInstance();
        anyEdgeDisplayRequested = anyEdgeDisplayRequested || 
            edgeCfg.getGlobalSettings().showEdges ||
            edgeCfg.getSelectedSettings().showEdges ||
            edgeCfg.getHoverSettings().showEdges;
    }
    
    if (anyEdgeDisplayRequested && !context.display.wireframeMode) {
        auto& manager = RenderingToolkitAPI::getManager();
        auto processor = manager.getGeometryProcessor("OpenCASCADE");
        TriangleMesh mesh;

        // Get mesh data if needed for mesh edges or normal lines
        if ((currentFlags.showMeshEdges || currentFlags.showNormalLines || currentFlags.showFaceNormalLines) && processor) {
            mesh = processor->convertToMesh(shape, params);
        }

        // Use modular edge component (migration completed)
        if (modularEdgeComponent) {
            if (currentFlags.showOriginalEdges) {
                Quantity_Color originalColor(1.0, 1.0, 1.0, Quantity_TOC_RGB);
                modularEdgeComponent->extractOriginalEdges(shape, 80.0, 0.01, false, originalColor, 1.0, false, Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB), 3.0);
            }
            if (currentFlags.showFeatureEdges) {
                Quantity_Color featureColor(1.0, 0.0, 0.0, Quantity_TOC_RGB);
                modularEdgeComponent->extractFeatureEdges(shape, 15.0, 0.005, false, false, featureColor, 2.0);
            }
            if (currentFlags.showMeshEdges && !mesh.triangles.empty()) {
                Quantity_Color meshColor(0.0, 0.0, 0.0, Quantity_TOC_RGB);
                modularEdgeComponent->extractMeshEdges(mesh, meshColor, 1.0);
            }
            if (currentFlags.showNormalLines && !mesh.triangles.empty()) {
                modularEdgeComponent->generateNormalLineNode(mesh, 0.5);
            }
            if (currentFlags.showFaceNormalLines && !mesh.triangles.empty()) {
                modularEdgeComponent->generateFaceNormalLineNode(mesh, 0.5);
            }
            if (currentFlags.showHighlightEdges) {
                modularEdgeComponent->generateHighlightEdgeNode();
            }

            // Update edge display to attach new edge nodes to the scene graph
            modularEdgeComponent->updateEdgeDisplay(m_coinNode);
        }
    }

    // ===== Build face index mapping =====
    if (m_faceIndexMappings.empty()) {
        buildFaceIndexMapping(shape, params);
    }

    // Update flags
    m_coinNeedsUpdate = false;
    m_meshRegenerationNeeded = false;
    m_lastMeshParams = params;

    auto buildEndTime = std::chrono::high_resolution_clock::now();
    auto buildDuration = std::chrono::duration_cast<std::chrono::milliseconds>(buildEndTime - buildStartTime);
}

void OCCGeometryMesh::updateWireframeMaterial(const Quantity_Color& color)
{
    if (!m_coinNode) {
        return;
    }

    // Find the material node in the Coin scene graph
    // The structure is: Separator -> DrawStyle -> Material -> Shape
    SoMaterial* material = nullptr;

    for (int i = 0; i < m_coinNode->getNumChildren(); ++i) {
        SoNode* child = m_coinNode->getChild(i);
        if (child->isOfType(SoMaterial::getClassTypeId())) {
            material = static_cast<SoMaterial*>(child);
            break;
        }
    }

    if (material) {
        // Update the diffuse color
        material->diffuseColor.setValue(
            static_cast<float>(color.Red()),
            static_cast<float>(color.Green()),
            static_cast<float>(color.Blue())
        );
    }
}

// Performance optimization: reverse mapping for O(1) triangle-to-face lookup
void OCCGeometryMesh::setFaceIndexMappings(const std::vector<FaceIndexMapping>& mappings) {
    m_faceIndexMappings = mappings;

    // Automatically build reverse mapping for fast lookup
    buildReverseMapping();
}

void OCCGeometryMesh::buildReverseMapping() {
    m_triangleToFaceMap.clear();

    if (m_faceIndexMappings.empty()) {
        m_hasReverseMapping = false;
        return;
    }

    // Pre-allocate space for better performance
    size_t totalTriangles = 0;
    for (const auto& mapping : m_faceIndexMappings) {
        totalTriangles += mapping.triangleIndices.size();
    }
    m_triangleToFaceMap.reserve(totalTriangles);

    // Build the reverse mapping: triangle index -> geometry face ID
    for (const auto& mapping : m_faceIndexMappings) {
        for (int triangleIndex : mapping.triangleIndices) {
            m_triangleToFaceMap[triangleIndex] = mapping.geometryFaceId;
        }
    }

    m_hasReverseMapping = true;

    LOG_INF_S("OCCGeometryMesh: Built reverse mapping for " +
              std::to_string(m_faceIndexMappings.size()) + " faces, " +
              std::to_string(totalTriangles) + " triangles");
}

int OCCGeometryMesh::getGeometryFaceIdForTriangle(int triangleIndex) const {
    // Use optimized O(1) lookup if reverse mapping is available
    if (m_hasReverseMapping) {
        auto it = m_triangleToFaceMap.find(triangleIndex);
        if (it != m_triangleToFaceMap.end()) {
            return it->second;
        }
        return -1; // Triangle not found in any face
    }

    // Fallback to O(n) linear search for backward compatibility
    if (!hasFaceIndexMapping()) {
        return -1;
    }

    for (const auto& mapping : m_faceIndexMappings) {
        auto it = std::find(mapping.triangleIndices.begin(),
                           mapping.triangleIndices.end(),
                           triangleIndex);
        if (it != mapping.triangleIndices.end()) {
            return mapping.geometryFaceId;
        }
    }

    return -1;
}

void OCCGeometryMesh::createPointViewRepresentation(const TopoDS_Shape& shape, const MeshParameters& params,
                                                   const ::DisplaySettings& displaySettings)
{
    try {
        // Convert MeshParameters to OCCMeshConverter::MeshParameters
        OCCMeshConverter::MeshParameters occParams;
        occParams.deflection = params.deflection;
        occParams.angularDeflection = params.angularDeflection;
        occParams.relative = params.relative;
        occParams.inParallel = params.inParallel;

        // Generate mesh to extract vertices
        TriangleMesh mesh = OCCMeshConverter::convertToMesh(shape, occParams);

        if (mesh.vertices.empty()) {
            LOG_WRN_S("No vertices found for point view");
            return;
        }

        // Create point view separator
        SoSeparator* pointViewSep = new SoSeparator();

        // Create material for points
        SoMaterial* pointMaterial = new SoMaterial();
        Standard_Real r, g, b;
        displaySettings.pointViewColor.Values(r, g, b, Quantity_TOC_RGB);
        pointMaterial->diffuseColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
        pointMaterial->emissiveColor.setValue(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b)); // Make points more visible
        pointViewSep->addChild(pointMaterial);

        // Create draw style for point size
        SoDrawStyle* pointStyle = new SoDrawStyle();
        pointStyle->pointSize.setValue(static_cast<float>(displaySettings.pointViewSize));
        pointViewSep->addChild(pointStyle);

        // Create coordinates
        SoCoordinate3* coords = new SoCoordinate3();
        coords->point.setNum(static_cast<int>(mesh.vertices.size()));

        // Set vertex coordinates
        SbVec3f* points = new SbVec3f[mesh.vertices.size()];
        for (size_t i = 0; i < mesh.vertices.size(); ++i) {
            const gp_Pnt& vertex = mesh.vertices[i];
            points[i].setValue(
                static_cast<float>(vertex.X()),
                static_cast<float>(vertex.Y()),
                static_cast<float>(vertex.Z())
            );
        }
        coords->point.setValues(0, static_cast<int>(mesh.vertices.size()), points);
        delete[] points;

        pointViewSep->addChild(coords);

        // Create point set with shape-specific rendering
        SoPointSet* pointSet = new SoPointSet();
        pointSet->numPoints.setValue(static_cast<int>(mesh.vertices.size()));

        // Add shape-specific rendering based on pointShape
        if (displaySettings.pointViewShape == 1) { // Circle
            // For circles, we can use SoSphere nodes at each point
            // This is more expensive but gives true circular points
            SoSeparator* circleSep = new SoSeparator();
            circleSep->addChild(pointMaterial);
            
            for (size_t i = 0; i < mesh.vertices.size(); ++i) {
                const gp_Pnt& vertex = mesh.vertices[i];
                
                SoSeparator* sphereSep = new SoSeparator();
                
                // Translation to vertex position
                SoTranslation* translation = new SoTranslation();
                translation->translation.setValue(
                    static_cast<float>(vertex.X()),
                    static_cast<float>(vertex.Y()),
                    static_cast<float>(vertex.Z())
                );
                sphereSep->addChild(translation);
                
                // Scale for point size
                SoScale* scale = new SoScale();
                float scaleFactor = static_cast<float>(displaySettings.pointViewSize) / 10.0f;
                scale->scaleFactor.setValue(scaleFactor, scaleFactor, scaleFactor);
                sphereSep->addChild(scale);
                
                // Create sphere
                SoSphere* sphere = new SoSphere();
                sphereSep->addChild(sphere);
                
                circleSep->addChild(sphereSep);
            }
            
            pointViewSep->addChild(circleSep);
        } else if (displaySettings.pointViewShape == 2) { // Triangle
            // For triangles, we can use SoCone nodes (triangular cross-section)
            SoSeparator* triangleSep = new SoSeparator();
            triangleSep->addChild(pointMaterial);
            
            for (size_t i = 0; i < mesh.vertices.size(); ++i) {
                const gp_Pnt& vertex = mesh.vertices[i];
                
                SoSeparator* coneSep = new SoSeparator();
                
                // Translation to vertex position
                SoTranslation* translation = new SoTranslation();
                translation->translation.setValue(
                    static_cast<float>(vertex.X()),
                    static_cast<float>(vertex.Y()),
                    static_cast<float>(vertex.Z())
                );
                coneSep->addChild(translation);
                
                // Scale for point size
                SoScale* scale = new SoScale();
                float scaleFactor = static_cast<float>(displaySettings.pointViewSize) / 10.0f;
                scale->scaleFactor.setValue(scaleFactor, scaleFactor, scaleFactor);
                coneSep->addChild(scale);
                
                // Create cone (triangular cross-section)
                SoCone* cone = new SoCone();
                coneSep->addChild(cone);
                
                triangleSep->addChild(coneSep);
            }
            
            pointViewSep->addChild(triangleSep);
        } else {
            // Default: Square points (SoPointSet)
            pointViewSep->addChild(pointSet);
        }

        // Add to main coin node
        pointViewSep->ref();
        m_coinNode->addChild(pointViewSep);

        LOG_INF_S("Created point view with " + std::to_string(mesh.vertices.size()) + " points, shape: " + std::to_string(displaySettings.pointViewShape));

    } catch (const std::exception& e) {
        LOG_ERR_S("Exception in createPointViewRepresentation: " + std::string(e.what()));
    }
}
