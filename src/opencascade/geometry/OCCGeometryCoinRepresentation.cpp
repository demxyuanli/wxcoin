#include "geometry/OCCGeometryCoinRepresentation.h"
#include "geometry/GeometryRenderContext.h"
#include "geometry/VertexExtractor.h"
#include "edges/ModularEdgeComponent.h"
#include "logger/Logger.h"
#include "rendering/RenderingToolkitAPI.h"
#include "rendering/OpenCASCADEProcessor.h"
#include "config/EdgeSettingsConfig.h"
#include "OCCMeshConverter.h"
#include "geometry/helper/CoinNodeManager.h"
#include "geometry/helper/RenderNodeBuilder.h"
#include "geometry/helper/DisplayModeHandler.h"
#include "geometry/helper/WireframeBuilder.h"
#include "geometry/helper/PointViewBuilder.h"
#include "geometry/helper/FaceDomainMapper.h"
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
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoScale.h>
#include <OpenCASCADE/TopAbs.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Face.hxx>
#include <TopLoc_Location.hxx>
#include <BRep_Tool.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Triangle.hxx>
#include <gp_Trsf.hxx>
#include <chrono>
#include <algorithm>
#include <set>
#include <map>
#include <fstream>

OCCGeometryCoinRepresentation::OCCGeometryCoinRepresentation()
    : m_coinNode(nullptr)
    , m_modeSwitch(nullptr)
    , m_coinNeedsUpdate(true)
    , m_meshRegenerationNeeded(true)
    , m_assemblyLevel(0)
    , useModularEdgeComponent(true)
    , m_lastMeshParams{}  // Initialize to default values to avoid issues caused by random values
{
    // Use only modular edge component - migration completed
    modularEdgeComponent = std::make_unique<ModularEdgeComponent>();
    
    // Initialize independent vertex extractor for point view
    m_vertexExtractor = std::make_unique<VertexExtractor>();
    
    // Initialize helper classes
    m_nodeManager = std::make_unique<helper::CoinNodeManager>();
    m_renderBuilder = std::make_unique<helper::RenderNodeBuilder>();
    m_displayHandler = std::make_unique<helper::DisplayModeHandler>();
    m_wireframeBuilder = std::make_unique<helper::WireframeBuilder>();
    m_pointViewBuilder = std::make_unique<helper::PointViewBuilder>();
    m_faceMapper = std::make_unique<helper::FaceDomainMapper>();
}

OCCGeometryCoinRepresentation::~OCCGeometryCoinRepresentation()
{
    if (m_coinNode) {
        m_coinNode->unref();
        m_coinNode = nullptr;
    }
    if (m_modeSwitch) {
        m_modeSwitch->unref();
        m_modeSwitch = nullptr;
    }
}

void OCCGeometryCoinRepresentation::setCoinNode(SoSeparator* node)
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

void OCCGeometryCoinRepresentation::regenerateMesh(const TopoDS_Shape& shape, const MeshParameters& params)
{
    m_meshRegenerationNeeded = true;
    m_lastMeshParams = params;
    buildCoinRepresentation(shape, params);
}

void OCCGeometryCoinRepresentation::buildCoinRepresentation(const TopoDS_Shape& shape, const MeshParameters& params)
{
    auto buildStartTime = std::chrono::high_resolution_clock::now();
    
    if (shape.IsNull()) {
        return;
    }
    
    // Create or clear coin node
    // CRITICAL FIX: Following FreeCAD's approach - disable render caching
    if (!m_coinNode) {
        m_coinNode = new SoSeparator();
        m_coinNode->renderCaching.setValue(SoSeparator::OFF);
        m_coinNode->boundingBoxCaching.setValue(SoSeparator::OFF);
        m_coinNode->pickCulling.setValue(SoSeparator::OFF);
        m_coinNode->ref();
    } else {
        m_coinNode->removeAllChildren();
        m_coinNode->renderCaching.setValue(SoSeparator::OFF);
        m_coinNode->boundingBoxCaching.setValue(SoSeparator::OFF);
        m_coinNode->pickCulling.setValue(SoSeparator::OFF);
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

void OCCGeometryCoinRepresentation::buildCoinRepresentation(
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
        return;
    }

    // Create or clear coin node
    // CRITICAL FIX: Following FreeCAD's approach - disable render caching
    if (!m_coinNode) {
        m_coinNode = new SoSeparator();
        m_coinNode->renderCaching.setValue(SoSeparator::OFF);
        m_coinNode->boundingBoxCaching.setValue(SoSeparator::OFF);
        m_coinNode->pickCulling.setValue(SoSeparator::OFF);
        m_coinNode->ref();
    } else {
        m_coinNode->removeAllChildren();
        m_coinNode->renderCaching.setValue(SoSeparator::OFF);
        m_coinNode->boundingBoxCaching.setValue(SoSeparator::OFF);
        m_coinNode->pickCulling.setValue(SoSeparator::OFF);
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

void OCCGeometryCoinRepresentation::updateCoinRepresentationIfNeeded(const TopoDS_Shape& shape, const MeshParameters& params)
{
    if (m_meshRegenerationNeeded || m_coinNeedsUpdate) {
        buildCoinRepresentation(shape, params);
    }
}

void OCCGeometryCoinRepresentation::forceCoinRepresentationRebuild(const TopoDS_Shape& shape, const MeshParameters& params)
{
    m_meshRegenerationNeeded = true;
    m_coinNeedsUpdate = true;
    buildCoinRepresentation(shape, params);
}

void OCCGeometryCoinRepresentation::setEdgeDisplayType(EdgeType type, bool show)
{
    if (modularEdgeComponent) {
        modularEdgeComponent->setEdgeDisplayType(type, show);
    }
}

bool OCCGeometryCoinRepresentation::isEdgeDisplayTypeEnabled(EdgeType type) const
{
    return modularEdgeComponent ? modularEdgeComponent->isEdgeDisplayTypeEnabled(type) : false;
}

void OCCGeometryCoinRepresentation::updateEdgeDisplay()
{
    if (modularEdgeComponent && m_coinNode) {
        modularEdgeComponent->updateEdgeDisplay(m_coinNode);
    }
}

bool OCCGeometryCoinRepresentation::hasOriginalEdges() const
{
    return modularEdgeComponent ? modularEdgeComponent->isEdgeDisplayTypeEnabled(EdgeType::Original) : false;
}

void OCCGeometryCoinRepresentation::enableModularEdgeComponent(bool enable)
{
    // Migration completed - always use modular edge component
    if (!enable) {
    }
    useModularEdgeComponent = true;
}

// Original getGeometryFaceIdForTriangle implementation moved to bottom of file

void OCCGeometryCoinRepresentation::releaseTemporaryData()
{
    // Release any temporary mesh generation data
}

void OCCGeometryCoinRepresentation::optimizeMemory()
{
    // Optimize memory usage
    // FaceDomains can be large, but we keep them for face highlighting
    // TriangleSegments are lightweight, keep them
}

void FaceDomain::toCoin3DFormat(std::vector<SbVec3f>& vertices, std::vector<int>& indices) const
{
    if (isEmpty()) {
            return;
        }

    // Reserve space
    vertices.reserve(vertices.size() + points.size());
    indices.reserve(indices.size() + triangles.size() * 3);

    // Add vertices
    size_t vertexOffset = vertices.size();
    for (const auto& point : points) {
        vertices.emplace_back(point.X(), point.Y(), point.Z());
    }

    // Add triangles (convert from MeshTriangle to indices)
    for (const auto& triangle : triangles) {
        indices.push_back(static_cast<int>(vertexOffset + triangle.I1));
        indices.push_back(static_cast<int>(vertexOffset + triangle.I2));
        indices.push_back(static_cast<int>(vertexOffset + triangle.I3));
        indices.push_back(-1); // Triangle separator for Coin3D
    }
}

void OCCGeometryCoinRepresentation::createWireframeRepresentation(const TopoDS_Shape& shape, const MeshParameters& params)
{
    if (shape.IsNull() || !m_coinNode) {
        return;
    }
    m_wireframeBuilder->createWireframeRepresentation(m_coinNode, shape, params);
}

// ========== NEW MODULAR INTERFACE ==========
// This is the truly modular implementation that doesn't depend on other modules

void OCCGeometryCoinRepresentation::buildCoinRepresentation(
    const TopoDS_Shape& shape,
    const MeshParameters& params,
    const GeometryRenderContext& context)
{
    auto buildStartTime = std::chrono::high_resolution_clock::now();

    if (shape.IsNull()) {
        return;
    }

    // Create or clear coin node using helper
    m_coinNode = m_nodeManager->createOrClearNode(m_coinNode);
    if (!m_coinNode) {
                return;
    }

    // Initialize mode switch for fast display mode switching
    if (!m_modeSwitch) {
        m_modeSwitch = new SoSwitch();
        m_modeSwitch->ref();
        m_modeSwitch->whichChild.setValue(SO_SWITCH_NONE);
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
            modularEdgeComponent->clearEdgeNode(EdgeType::VerticeNormal);
            modularEdgeComponent->clearEdgeNode(EdgeType::FaceNormal);
        }
        // Clear reverse mapping when mesh parameters change
    }
    
    // Build face domain mapping using helper
    if (!hasFaceDomainMapping() || meshParamsChanged) {
        m_faceMapper->buildFaceDomainMapping(shape, params, m_faceDomains, m_triangleSegments, m_boundaryTriangles);
        if (!hasFaceDomainMapping()) {
            LOG_WRN_S("Face domain mapping is empty - face highlighting may not work");
        }
    }

    // Clean up texture nodes using helper
    m_nodeManager->cleanupTextureNodes(m_coinNode);

    // Build render nodes using helper
    m_coinNode->addChild(m_renderBuilder->createTransformNode(context));
    m_coinNode->addChild(m_renderBuilder->createShapeHintsNode(context));

    // Handle display mode using helper
    m_displayHandler->setModeSwitch(m_modeSwitch);
    m_displayHandler->handleDisplayMode(m_coinNode, context, shape, params,
                                        modularEdgeComponent.get(), useModularEdgeComponent,
                                        m_renderBuilder.get(), m_wireframeBuilder.get());

    // Point view rendering using helper
    if (context.display.showPointView) {
        m_pointViewBuilder->createPointViewRepresentation(m_coinNode, shape, params, context.display);
    }

    // ===== Set visibility =====
    m_coinNode->renderCulling = context.display.visible ? SoSeparator::OFF : SoSeparator::ON;

    // ===== CRITICAL: Extract and cache original edges and vertices at import time =====
    // CRITICAL FIX: Following FreeCAD's CoinThread approach - extract in background thread
    // This prevents UI blocking and GL context crashes for large models
    // Edge extraction will be done asynchronously via EdgeDisplayManager::startAsyncOriginalEdgeExtraction
    // We don't extract here to avoid blocking the import process
    // The extraction will be triggered when user enables original edges display
    
    // Extract and cache vertices using independent VertexExtractor
    if (m_vertexExtractor) {
        try {
            m_vertexExtractor->extractAndCache(shape);
        } catch (const std::exception& e) {
            LOG_ERR_S("OCCGeometryCoinRepresentation: Failed to cache vertices: " + std::string(e.what()));
        }
    }

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


    // Update flags
    m_coinNeedsUpdate = false;
    m_meshRegenerationNeeded = false;
    m_lastMeshParams = params;

    auto buildEndTime = std::chrono::high_resolution_clock::now();
    auto buildDuration = std::chrono::duration_cast<std::chrono::milliseconds>(buildEndTime - buildStartTime);
}

void OCCGeometryCoinRepresentation::updateWireframeMaterial(const Quantity_Color& color)
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


void OCCGeometryCoinRepresentation::updateDisplayMode(RenderingConfig::DisplayMode mode)
{
    if (!m_coinNode) {
        return;
    }
    m_displayHandler->setModeSwitch(m_modeSwitch);
    m_displayHandler->updateDisplayMode(m_coinNode, mode, modularEdgeComponent.get());
}



int OCCGeometryCoinRepresentation::getGeometryFaceIdForTriangle(int triangleIndex) const {
    // Use domain system to find which face contains this triangle
    if (!hasFaceDomainMapping()) {
        return -1;
    }

    // Search through triangle segments to find which face contains this triangle
    for (const auto& segment : m_triangleSegments) {
        if (segment.contains(triangleIndex)) {
            return segment.geometryFaceId;
        }
    }

    return -1;
}

void OCCGeometryCoinRepresentation::createPointViewRepresentation(const TopoDS_Shape& shape, const MeshParameters& params,
                                                   const ::DisplaySettings& displaySettings)
{
    if (!m_coinNode) {
        return;
    }
    m_pointViewBuilder->createPointViewRepresentation(m_coinNode, shape, params, displaySettings);
}


// ===== New Domain-based Implementation =====

void OCCGeometryCoinRepresentation::buildFaceDomains(const TopoDS_Shape& shape,
                                      const std::vector<TopoDS_Face>& faces,
                                      const MeshParameters& params)
{
    std::vector<FaceDomain> tempDomains;
    tempDomains.reserve(faces.size());
    for (std::size_t faceIndex = 0; faceIndex < faces.size(); ++faceIndex) {
        const TopoDS_Face& face = faces[faceIndex];
        FaceDomain domain(static_cast<int>(faceIndex));
        if (m_faceMapper->triangulateFace(face, domain)) {
            domain.isValid = true;
        } else {
            domain.isValid = false;
        }
        tempDomains.push_back(std::move(domain));
    }
    m_faceDomains = std::move(tempDomains);
}

bool OCCGeometryCoinRepresentation::triangulateFace(const TopoDS_Face& face, FaceDomain& domain)
{
    return m_faceMapper->triangulateFace(face, domain);
}

void OCCGeometryCoinRepresentation::buildTriangleSegments(const std::vector<std::pair<int, std::vector<int>>>& faceMappings)
{
    m_triangleSegments.clear();
    m_triangleSegments.reserve(faceMappings.size());
    for (const auto& [faceId, triangleIndices] : faceMappings) {
        TriangleSegment segment(faceId, triangleIndices);
        m_triangleSegments.push_back(std::move(segment));
    }
}

void OCCGeometryCoinRepresentation::identifyBoundaryTriangles(const std::vector<std::pair<int, std::vector<int>>>& faceMappings)
{
    std::map<int, std::vector<int>> triangleToFacesMap;
    for (const auto& [faceId, triangleIndices] : faceMappings) {
        for (int triangleIndex : triangleIndices) {
            triangleToFacesMap[triangleIndex].push_back(faceId);
        }
    }
    for (const auto& [triangleIndex, faceIds] : triangleToFacesMap) {
        if (faceIds.size() > 1) {
            BoundaryTriangle boundaryTri(triangleIndex);
            boundaryTri.faceIds = faceIds;
            boundaryTri.isBoundary = true;
            m_boundaryTriangles.push_back(boundaryTri);
        }
    }
}

// ===== Query Methods for New Domain System =====

const FaceDomain* OCCGeometryCoinRepresentation::getFaceDomain(int geometryFaceId) const
{
    for (const auto& domain : m_faceDomains) {
        if (domain.geometryFaceId == geometryFaceId) {
            return &domain;
        }
    }
    return nullptr;
}

const TriangleSegment* OCCGeometryCoinRepresentation::getTriangleSegment(int geometryFaceId) const
{
    for (const auto& segment : m_triangleSegments) {
        if (segment.geometryFaceId == geometryFaceId) {
            return &segment;
        }
    }
    return nullptr;
}

bool OCCGeometryCoinRepresentation::isBoundaryTriangle(int triangleIndex) const
{
    for (const auto& boundaryTri : m_boundaryTriangles) {
        if (boundaryTri.triangleIndex == triangleIndex) {
            return boundaryTri.isBoundary;
        }
    }
    return false;
}

const BoundaryTriangle* OCCGeometryCoinRepresentation::getBoundaryTriangle(int triangleIndex) const
{
    for (const auto& boundaryTri : m_boundaryTriangles) {
        if (boundaryTri.triangleIndex == triangleIndex) {
            return &boundaryTri;
        }
    }
    return nullptr;
}

std::vector<int> OCCGeometryCoinRepresentation::getGeometryFaceIdsForTriangle(int triangleIndex) const
{
    // Domain system doesn't support multiple faces per triangle efficiently
    // Return single face if found
    int faceId = getGeometryFaceIdForTriangle(triangleIndex);
    if (faceId >= 0) {
        return {faceId};
    }
    return {};
}

std::vector<int> OCCGeometryCoinRepresentation::getTrianglesForGeometryFace(int geometryFaceId) const
{
    // Deprecated: Now we use FaceDomain directly instead of triangle indices
    // This method is kept for backward compatibility but should not be used
    LOG_WRN_S("getTrianglesForGeometryFace is deprecated - use getFaceDomain instead for face " + std::to_string(geometryFaceId));
    return {};
}
