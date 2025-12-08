#include "geometry/GeometryRenderer.h"
#include "geometry/FaceDomainTypes.h"  // For struct definitions (FaceDomain, TriangleSegment, BoundaryTriangle)
#include "viewer/ObjectDisplayModeManager.h"
#include "rendering/OpenCASCADEProcessor.h"
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Face.hxx>
#include "geometry/GeometryRenderContext.h"
#include "geometry/VertexExtractor.h"
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
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/actions/SoSearchAction.h>
#include <OpenCASCADE/TopAbs.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Face.hxx>
#include <chrono>
#include <algorithm>
#include <set>
#include <map>
#include <fstream>

GeometryRenderer::GeometryRenderer()
    : m_coinNode(nullptr)
    , m_modeSwitch(nullptr)
    , m_coinNeedsUpdate(true)
    , m_meshRegenerationNeeded(true)
    , m_assemblyLevel(0)
    , useModularEdgeComponent(true)
    , m_shape()
    , m_lastMeshParams{}
{
    // Use only modular edge component
    modularEdgeComponent = std::make_unique<ModularEdgeComponent>();
    
    // Initialize independent vertex extractor for point view
    m_vertexExtractor = std::make_unique<VertexExtractor>();
    
    // Initialize object-level display mode manager
    m_objectDisplayModeManager = std::make_unique<ObjectDisplayModeManager>();

    // Initialize managers
    m_faceDomainManager = std::make_unique<FaceDomainManager>();
    m_triangleMappingManager = std::make_unique<TriangleMappingManager>();
    m_sceneBuilder = std::make_unique<CoinSceneBuilder>();
    m_pointViewRenderer = std::make_unique<PointViewRenderer>();
}

GeometryRenderer::~GeometryRenderer()
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

void GeometryRenderer::setCoinNode(SoSeparator* node)
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

void GeometryRenderer::regenerateMesh(const TopoDS_Shape& shape, const MeshParameters& params)
{
    m_meshRegenerationNeeded = true;
    m_lastMeshParams = params;
    buildCoinRepresentation(shape, params);
}

// Legacy interface implementations
void GeometryRenderer::buildCoinRepresentation(const TopoDS_Shape& shape, const MeshParameters& params)
{
    // Create basic context for legacy interface
    GeometryRenderContext context;
    context.display.visible = true;
    context.display.facesVisible = true;
    buildCoinRepresentation(shape, params, context);
}

void GeometryRenderer::buildCoinRepresentation(const TopoDS_Shape& shape, const MeshParameters& params,
    const Quantity_Color& diffuseColor, const Quantity_Color& ambientColor,
    const Quantity_Color& specularColor, const Quantity_Color& emissiveColor,
    double shininess, double transparency)
{
    // Create context with material parameters
    GeometryRenderContext context;
    context.display.visible = true;
    context.display.facesVisible = true;
    context.material.diffuseColor = diffuseColor;
    context.material.ambientColor = ambientColor;
    context.material.specularColor = specularColor;
    context.material.emissiveColor = emissiveColor;
    context.material.shininess = shininess;
    context.material.transparency = transparency;
    buildCoinRepresentation(shape, params, context);
}

void GeometryRenderer::updateCoinRepresentationIfNeeded(const TopoDS_Shape& shape, const MeshParameters& params)
{
    if (m_meshRegenerationNeeded || m_coinNeedsUpdate) {
        buildCoinRepresentation(shape, params);
    }
}

void GeometryRenderer::forceCoinRepresentationRebuild(const TopoDS_Shape& shape, const MeshParameters& params)
{
    m_meshRegenerationNeeded = true;
    m_coinNeedsUpdate = true;
    buildCoinRepresentation(shape, params);
}

// Edge component methods
void GeometryRenderer::setEdgeDisplayType(EdgeType type, bool show)
{
    if (modularEdgeComponent) {
        modularEdgeComponent->setEdgeDisplayType(type, show);
    }
}

bool GeometryRenderer::isEdgeDisplayTypeEnabled(EdgeType type) const
{
    return modularEdgeComponent ? modularEdgeComponent->isEdgeDisplayTypeEnabled(type) : false;
}

void GeometryRenderer::updateEdgeDisplay()
{
    if (modularEdgeComponent && m_coinNode) {
        modularEdgeComponent->updateEdgeDisplay(m_coinNode);
    }
}

bool GeometryRenderer::hasOriginalEdges() const
{
    return modularEdgeComponent ? modularEdgeComponent->isEdgeDisplayTypeEnabled(EdgeType::Original) : false;
}

void GeometryRenderer::enableModularEdgeComponent(bool enable)
{
    useModularEdgeComponent = true;
}

// Face domain system - delegated to FaceDomainManager
const std::vector<FaceDomain>& GeometryRenderer::getFaceDomains() const
{
    return m_faceDomainManager->getFaceDomains();
}

const FaceDomain* GeometryRenderer::getFaceDomain(int geometryFaceId) const
{
    return m_faceDomainManager->getFaceDomain(geometryFaceId);
}

bool GeometryRenderer::hasFaceDomainMapping() const
{
    return m_faceDomainManager->hasFaceDomainMapping();
}

// Triangle mapping system - delegated to TriangleMappingManager
const std::vector<TriangleSegment>& GeometryRenderer::getTriangleSegments() const
{
    return m_triangleMappingManager->getTriangleSegments();
}

const std::vector<BoundaryTriangle>& GeometryRenderer::getBoundaryTriangles() const
{
    return m_triangleMappingManager->getBoundaryTriangles();
}

const TriangleSegment* GeometryRenderer::getTriangleSegment(int geometryFaceId) const
{
    return m_triangleMappingManager->getTriangleSegment(geometryFaceId);
}

bool GeometryRenderer::isBoundaryTriangle(int triangleIndex) const
{
    return m_triangleMappingManager->isBoundaryTriangle(triangleIndex);
}

const BoundaryTriangle* GeometryRenderer::getBoundaryTriangle(int triangleIndex) const
{
    return m_triangleMappingManager->getBoundaryTriangle(triangleIndex);
}

int GeometryRenderer::getGeometryFaceIdForTriangle(int triangleIndex) const
{
    return m_triangleMappingManager->getGeometryFaceIdForTriangle(triangleIndex);
}

std::vector<int> GeometryRenderer::getGeometryFaceIdsForTriangle(int triangleIndex) const
{
    return m_triangleMappingManager->getGeometryFaceIdsForTriangle(triangleIndex);
}

std::vector<int> GeometryRenderer::getTrianglesForGeometryFace(int geometryFaceId) const
{
    return m_triangleMappingManager->getTrianglesForGeometryFace(geometryFaceId);
}

// Point view rendering - delegated to PointViewRenderer
void GeometryRenderer::createPointViewRepresentation(const TopoDS_Shape& shape, const MeshParameters& params,
                                                   const DisplaySettings& displaySettings)
{
    if (m_pointViewRenderer && m_coinNode) {
        SoSeparator* pointViewNode = m_pointViewRenderer->createPointViewNode(
            shape, params, displaySettings, m_vertexExtractor.get());
        if (pointViewNode) {
            pointViewNode->ref();
            m_coinNode->addChild(pointViewNode);
        }
    }
}

// Display mode update - delegated to CoinSceneBuilder
void GeometryRenderer::updateDisplayMode(RenderingConfig::DisplayMode mode)
{
    // CRITICAL FIX: Following FreeCAD's approach - only update whichChild, don't rebuild
    // If no mode switch exists, we need to build the scene graph first
    if (!m_modeSwitch) {
        if (m_shape.IsNull()) {
            LOG_WRN_S("GeometryRenderer::updateDisplayMode: No shape available - cannot build scene graph");
            return;
        }
        LOG_INF_S("GeometryRenderer::updateDisplayMode: No mode switch available - building scene graph first");
        // Build with default context - this will create the mode switch
        GeometryRenderContext context;
        context.display.visible = true;
        context.display.facesVisible = true;
        context.display.displayMode = mode; // Use the requested mode
        MeshParameters params;
        buildCoinRepresentation(m_shape, params, context);
    }

    // Now update the SoSwitch whichChild if we have a mode switch
    if (m_modeSwitch && m_objectDisplayModeManager) {
        m_objectDisplayModeManager->updateDisplayMode(m_modeSwitch, mode);
        LOG_INF_S("GeometryRenderer::updateDisplayMode: Updated whichChild for mode " + std::to_string(static_cast<int>(mode)));
    } else {
        LOG_WRN_S("GeometryRenderer::updateDisplayMode: ObjectDisplayModeManager not available");
    }
}

// Wireframe material update - delegated to CoinSceneBuilder
void GeometryRenderer::updateWireframeMaterial(const Quantity_Color& color)
{
    if (m_sceneBuilder && m_coinNode) {
        m_sceneBuilder->updateWireframeMaterial(m_coinNode, color);
    }
}

// Memory optimization
void GeometryRenderer::releaseTemporaryData()
{
    // Release any temporary mesh generation data
}

void GeometryRenderer::optimizeMemory()
{
    // Optimize memory usage
}

// Main rendering method - uses CoinSceneBuilder and managers
void GeometryRenderer::buildCoinRepresentation(
    const TopoDS_Shape& shape,
    const MeshParameters& params,
    const GeometryRenderContext& context)
{
    auto buildStartTime = std::chrono::high_resolution_clock::now();

    if (shape.IsNull()) {
        return;
    }

    // Check if mesh parameters changed
    bool meshParamsChanged = (m_lastMeshParams.deflection != params.deflection ||
                               m_lastMeshParams.angularDeflection != params.angularDeflection);

    if (meshParamsChanged) {
        if (modularEdgeComponent) {
            modularEdgeComponent->clearMeshEdgeNode();
            modularEdgeComponent->clearEdgeNode(EdgeType::NormalLine);
            modularEdgeComponent->clearEdgeNode(EdgeType::FaceNormalLine);
        }
    }

    // Build face domain mapping if needed
    if (!m_faceDomainManager->hasFaceDomainMapping() || meshParamsChanged) {
        try {
            if (shape.IsNull()) {
                return;
            }

            // Extract all faces from the shape
            std::vector<TopoDS_Face> faces;
            for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next()) {
                TopoDS_Face face = TopoDS::Face(exp.Current());
                if (!face.IsNull()) {
                    faces.push_back(face);
                }
            }

            // If no faces found directly, try extracting from shells
            if (faces.empty()) {
                for (TopExp_Explorer shellExp(shape, TopAbs_SHELL); shellExp.More(); shellExp.Next()) {
                    TopoDS_Shell shell = TopoDS::Shell(shellExp.Current());
                    for (TopExp_Explorer faceExp(shell, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
                        TopoDS_Face face = TopoDS::Face(faceExp.Current());
                        if (!face.IsNull()) {
                            faces.push_back(face);
                        }
                    }
                }
            }

            // If still no faces, try extracting from solids
            if (faces.empty()) {
                for (TopExp_Explorer solidExp(shape, TopAbs_SOLID); solidExp.More(); solidExp.Next()) {
                    TopoDS_Solid solid = TopoDS::Solid(solidExp.Current());
                    for (TopExp_Explorer faceExp(solid, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
                        TopoDS_Face face = TopoDS::Face(faceExp.Current());
                        if (!face.IsNull()) {
                            faces.push_back(face);
                        }
                    }
                }
            }

            if (!faces.empty()) {
                // Build face domains
                m_faceDomainManager->buildFaceDomains(shape, faces, params);

                // Build triangle segments and boundary triangles
                auto& manager = RenderingToolkitAPI::getManager();
                auto* baseProcessor = manager.getGeometryProcessor("OpenCASCADE");
                auto* processor = dynamic_cast<OpenCASCADEProcessor*>(baseProcessor);

                if (processor) {
                    std::vector<std::pair<int, std::vector<int>>> faceMappings;
                    TriangleMesh meshWithMapping = processor->convertToMeshWithFaceMapping(shape, params, faceMappings);

                    m_triangleMappingManager->buildTriangleSegments(faceMappings);
                    m_triangleMappingManager->identifyBoundaryTriangles(faceMappings);
                }
            }
        }
        catch (const std::exception& e) {
            LOG_ERR_S("Failed to build face domain mapping: " + std::string(e.what()));
        }
    }

    // Build scene graph using CoinSceneBuilder
    SoSeparator* builtRoot = m_sceneBuilder->buildSceneGraph(
        shape, params, context,
        m_objectDisplayModeManager.get(),
        modularEdgeComponent.get(),
        m_vertexExtractor.get(),
        m_faceDomainManager.get(),
        m_triangleMappingManager.get()
    );

    if (builtRoot) {
        // Replace existing coin node if different
        if (builtRoot != m_coinNode) {
            if (m_coinNode) {
                m_coinNode->unref();
            }
            m_coinNode = builtRoot;
        }
        
        // CRITICAL FIX: Use SoSearchAction to reliably find SoSwitch
        // This is more robust than assuming it's the last child
        SoSearchAction searchAction;
        searchAction.setType(SoSwitch::getClassTypeId());
        searchAction.setInterest(SoSearchAction::FIRST);
        if (m_coinNode) {
            searchAction.apply(m_coinNode);
            SoPath* path = searchAction.getPath();
            if (path && path->getLength() > 0) {
                SoNode* foundNode = path->getNode(path->getLength() - 1);
                if (foundNode && foundNode->isOfType(SoSwitch::getClassTypeId())) {
                    SoSwitch* foundSwitch = static_cast<SoSwitch*>(foundNode);
                    if (m_modeSwitch != foundSwitch) {
                        if (m_modeSwitch) {
                            m_modeSwitch->unref();
                        }
                        m_modeSwitch = foundSwitch;
                        // Only ref if not already referenced
                        if (m_modeSwitch->getRefCount() == 0) {
                            m_modeSwitch->ref();
                        }
                    }
                }
            }
        }
        
        // Fallback: Try last child if search failed
        if (!m_modeSwitch && m_coinNode && m_coinNode->getNumChildren() > 0) {
            SoNode* lastChild = m_coinNode->getChild(m_coinNode->getNumChildren() - 1);
            if (lastChild && lastChild->isOfType(SoSwitch::getClassTypeId())) {
                if (m_modeSwitch) {
                    m_modeSwitch->unref();
                }
                m_modeSwitch = static_cast<SoSwitch*>(lastChild);
                m_modeSwitch->ref();
            }
        }
    }

    // Extract and cache vertices
    if (m_vertexExtractor) {
        try {
            m_vertexExtractor->extractAndCache(shape);
        } catch (const std::exception& e) {
            LOG_ERR_S("GeometryRenderer: Failed to cache vertices: " + std::string(e.what()));
        }
    }

    // Update flags
    m_coinNeedsUpdate = false;
    m_meshRegenerationNeeded = false;
    m_lastMeshParams = params;

    auto buildEndTime = std::chrono::high_resolution_clock::now();
    auto buildDuration = std::chrono::duration_cast<std::chrono::milliseconds>(buildEndTime - buildStartTime);
}

// Legacy helper - will be removed after migration
void GeometryRenderer::createWireframeRepresentation(const TopoDS_Shape& shape, const MeshParameters& params)
{
    // This is now handled by WireframeMode in ObjectDisplayModeManager
    // Keeping for backward compatibility
}

