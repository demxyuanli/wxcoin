#include "geometry/OCCGeometryMesh.h"
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
    , m_lastMeshParams{}  // 初始化为默认值，避免随机值导致的问题
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
    }
    m_coinNode = node;
    if (m_coinNode) {
        m_coinNode->ref();
    }
}

void OCCGeometryMesh::regenerateMesh(const TopoDS_Shape& shape, const MeshParameters& params)
{
    LOG_INF_S("Regenerating mesh");
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
    LOG_INF_S("Coin representation built in " + std::to_string(buildDuration.count()) + "ms");
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
    LOG_INF_S("Coin representation with materials built in " + std::to_string(buildDuration.count()) + "ms");
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

int OCCGeometryMesh::getGeometryFaceIdForTriangle(int triangleIndex) const
{
    if (!hasFaceIndexMapping()) {
        return -1;
    }

    for (const auto& mapping : m_faceIndexMappings) {
        auto it = std::find(mapping.triangleIndices.begin(), mapping.triangleIndices.end(), triangleIndex);
        if (it != mapping.triangleIndices.end()) {
            return mapping.geometryFaceId;
        }
    }

    return -1;
}

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

        LOG_INF_S("Building face index mapping for " + std::to_string(faces.size()) + " faces");

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
                
                LOG_INF_S("Face index mapping built successfully: " + std::to_string(m_faceIndexMappings.size()) + " faces mapped");
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
    LOG_INF_S("Releasing temporary mesh data");
}

void OCCGeometryMesh::optimizeMemory()
{
    // Optimize memory usage
    m_faceIndexMappings.shrink_to_fit();
    LOG_INF_S("Memory optimized");
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

    LOG_INF_S("Creating wireframe with " + std::to_string(mesh.vertices.size()) + " vertices and " + 
              std::to_string(mesh.triangles.size() / 3) + " triangles");

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
    
    LOG_INF_S("Wireframe representation created successfully");
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

    // Create or clear coin node
    if (!m_coinNode) {
        m_coinNode = new SoSeparator();
        m_coinNode->ref();
    } else {
        m_coinNode->removeAllChildren();
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
            LOG_INF_S("Mesh parameters changed, cleared modular mesh edge nodes and normal nodes");
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

    // ===== Draw style =====
    SoDrawStyle* drawStyle = new SoDrawStyle();
    drawStyle->style = context.display.wireframeMode ? SoDrawStyle::LINES : SoDrawStyle::FILLED;
    drawStyle->lineWidth = context.display.wireframeMode ? static_cast<float>(context.display.wireframeWidth) : 0.0f;
    m_coinNode->addChild(drawStyle);

    // ===== Material =====
    SoMaterial* material = new SoMaterial();
    if (context.display.wireframeMode) {
        // Wireframe mode: use configured wireframe color
        const Quantity_Color& wColor = context.display.wireframeColor;
        material->diffuseColor.setValue(
            static_cast<float>(wColor.Red()),
            static_cast<float>(wColor.Green()),
            static_cast<float>(wColor.Blue())
        );
        material->transparency.setValue(static_cast<float>(context.material.transparency));
    }
    else {
        // Solid mode: use material colors with enhancement
        Standard_Real r, g, b;
        
        // Enhanced ambient color (increase by 50%)
        context.material.ambientColor.Values(r, g, b, Quantity_TOC_RGB);
        material->ambientColor.setValue(
            static_cast<float>(r * 1.5),
            static_cast<float>(g * 1.5),
            static_cast<float>(b * 1.5)
        );
        
        // Slightly reduce diffuse (reduce by 20%)
        context.material.diffuseColor.Values(r, g, b, Quantity_TOC_RGB);
        material->diffuseColor.setValue(
            static_cast<float>(r * 0.8),
            static_cast<float>(g * 0.8),
            static_cast<float>(b * 0.8)
        );
        
        // Keep specular unchanged
        context.material.specularColor.Values(r, g, b, Quantity_TOC_RGB);
        material->specularColor.setValue(
            static_cast<float>(r),
            static_cast<float>(g),
            static_cast<float>(b)
        );
        
        material->shininess.setValue(static_cast<float>(context.material.shininess / 100.0));
        
        // Hide faces when requested
        double appliedTransparency = context.display.facesVisible ? context.material.transparency : 1.0;
        material->transparency.setValue(static_cast<float>(appliedTransparency));
        
        // Emissive color
        context.material.emissiveColor.Values(r, g, b, Quantity_TOC_RGB);
        material->emissiveColor.setValue(
            static_cast<float>(r),
            static_cast<float>(g),
            static_cast<float>(b)
        );
    }
    m_coinNode->addChild(material);

    // ===== Texture support =====
    if (context.texture.enabled && !context.texture.imagePath.empty()) {
        std::ifstream fileCheck(context.texture.imagePath);
        if (!fileCheck.good()) {
            LOG_WRN_S("Texture file not found: " + context.texture.imagePath);
        } else {
            fileCheck.close();
            try {
                SoTexture2* texture = new SoTexture2();
                texture->filename.setValue(context.texture.imagePath.c_str());
                
                // Set texture mode
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
                
                m_coinNode->addChild(texture);
                m_coinNode->addChild(new SoTextureCoordinate2());
            }
            catch (const std::exception& e) {
                LOG_ERR_S("Exception loading texture: " + std::string(e.what()));
            }
        }
    }

    // ===== Blend settings =====
    if (context.blend.blendMode != RenderingConfig::BlendMode::None && context.material.transparency > 0.0) {
        SoShapeHints* blendHints = new SoShapeHints();
        blendHints->faceType = SoShapeHints::UNKNOWN_FACE_TYPE;
        blendHints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
        m_coinNode->addChild(blendHints);
    }

    // ===== Add shape geometry =====
    if (context.display.wireframeMode) {
        createWireframeRepresentation(shape, params);
    } else {
        auto& manager = RenderingToolkitAPI::getManager();
        auto backend = manager.getRenderBackend("Coin3D");
        if (backend) {
            auto sceneNode = backend->createSceneNode(shape, params, context.display.selected,
                context.material.diffuseColor, context.material.ambientColor,
                context.material.specularColor, context.material.emissiveColor,
                context.material.shininess, context.material.transparency);
            if (sceneNode && context.display.facesVisible) {
                SoSeparator* meshNode = sceneNode.get();
                meshNode->ref();
                m_coinNode->addChild(meshNode);
            }
        }
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
    LOG_INF_S("Coin3D scene built (modular) in " + std::to_string(buildDuration.count()) + "ms");
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
