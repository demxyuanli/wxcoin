#include "edges/ModularEdgeComponent.h"
#include "edges/EdgeProcessorFactory.h"
#include "edges/EdgeLODManager.h"
#include "edges/renderers/MeshEdgeRenderer.h"
#include "edges/AsyncEdgeIntersectionComputer.h"
#include "logger/AsyncLogger.h"
#include "logger/Logger.h"
#include <OpenCASCADE/TopExp_Explorer.hxx>
#include <OpenCASCADE/TopoDS.hxx>
#include <OpenCASCADE/TopAbs.hxx>
#include <OpenCASCADE/BRepAdaptor_Curve.hxx>
#include <OpenCASCADE/BRep_Tool.hxx>
#include <OpenCASCADE/Poly_Triangulation.hxx>
#include <OpenCASCADE/Poly_PolygonOnTriangulation.hxx>
#include <OpenCASCADE/TColgp_Array1OfPnt.hxx>
#include <OpenCASCADE/TColStd_Array1OfInteger.hxx>
#include <OpenCASCADE/TopLoc_Location.hxx>
#include <OpenCASCADE/gp_Trsf.hxx>
#include <OpenCASCADE/GCPnts_AbscissaPoint.hxx>
#include <OpenCASCADE/Standard_Failure.hxx>
#include <cmath>
#include <set>
#include <tuple>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoDrawStyle.h>

ModularEdgeComponent::ModularEdgeComponent() {
    m_lodManager = std::make_unique<EdgeLODManager>();

    try {
        auto& factory = EdgeProcessorFactory::getInstance();
        m_originalExtractor = factory.getExtractor(EdgeType::Original);
        m_featureExtractor = factory.getExtractor(EdgeType::Feature);
        m_meshExtractor = factory.getExtractor(EdgeType::Mesh);
        m_silhouetteExtractor = factory.getExtractor(EdgeType::Silhouette);

        m_originalRenderer = factory.getRenderer(EdgeType::Original);
        m_featureRenderer = factory.getRenderer(EdgeType::Feature);
        m_meshRenderer = factory.getRenderer(EdgeType::Mesh);
    } catch (const std::exception& e) {
        LOG_ERR_S_ASYNC("Failed to initialize edge processors: " + std::string(e.what()));
    }
}

ModularEdgeComponent::~ModularEdgeComponent() {
    cleanupEdgeNode(originalEdgeNode);
    cleanupEdgeNode(featureEdgeNode);
    cleanupEdgeNode(meshEdgeNode);
    cleanupEdgeNode(highlightEdgeNode);
    cleanupEdgeNode(normalLineNode);
    cleanupEdgeNode(faceNormalLineNode);
    cleanupEdgeNode(silhouetteEdgeNode);
    cleanupEdgeNode(intersectionNodesNode);
}

void ModularEdgeComponent::extractOriginalEdges(
    const TopoDS_Shape& shape,
    double samplingDensity,
    double minLength,
    bool showLinesOnly,
    const Quantity_Color& color,
    double width,
    bool highlightIntersectionNodes,
    const Quantity_Color& intersectionNodeColor,
    double intersectionNodeSize,
    IntersectionNodeShape intersectionNodeShape) {

    if (!m_originalExtractor || !m_originalRenderer) {
        LOG_WRN_S_ASYNC("Original edge extractor/renderer not available");
        return;
    }

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    OriginalEdgeParams params(samplingDensity, minLength, showLinesOnly, highlightIntersectionNodes);
    std::vector<gp_Pnt> points = m_originalExtractor->extract(shape, &params);

    cleanupEdgeNode(originalEdgeNode);
    originalEdgeNode = m_originalRenderer->generateNode(points, color, width);

    // Handle intersection node highlighting - now handled separately by async computation
    // Do not compute intersections here to avoid premature display

}

void ModularEdgeComponent::addSingleIntersectionNode(const gp_Pnt& point, const Quantity_Color& color, double size) {
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    
    if (!intersectionNodesNode) {
        intersectionNodesNode = new SoSeparator();
        intersectionNodesNode->ref();
    }
    
    // Create sphere node for intersection point
    SoSphere* sphere = new SoSphere();
    sphere->radius = size;
    
    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(color.Red(), color.Green(), color.Blue());
    material->transparency = 0.0;
    
    SoTranslation* translation = new SoTranslation();
    translation->translation.setValue(point.X(), point.Y(), point.Z());
    
    SoSeparator* pointNode = new SoSeparator();
    pointNode->addChild(material);
    pointNode->addChild(translation);
    pointNode->addChild(sphere);
    
    intersectionNodesNode->addChild(pointNode);
}

void ModularEdgeComponent::addBatchIntersectionNodes(const std::vector<gp_Pnt>& points, const Quantity_Color& color, double size) {
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    
    if (points.empty()) return;
    
    if (!intersectionNodesNode) {
        intersectionNodesNode = new SoSeparator();
        intersectionNodesNode->ref();
        
        // Enable intersection nodes display
        edgeFlags.showIntersectionNodes = true;
    }
    
    // Create material once for all points in this batch
    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(color.Red(), color.Green(), color.Blue());
    material->transparency = 0.0;
    intersectionNodesNode->addChild(material);
    
    // Create coordinate node for all points
    SoCoordinate3* coords = new SoCoordinate3();
    for (const auto& point : points) {
        coords->point.set1Value(coords->point.getNum(), point.X(), point.Y(), point.Z());
    }
    intersectionNodesNode->addChild(coords);
    
    // Create point set for all points
    SoPointSet* pointSet = new SoPointSet();
    pointSet->numPoints = points.size();
    intersectionNodesNode->addChild(pointSet);
    
    // Set point size
    SoDrawStyle* drawStyle = new SoDrawStyle();
    drawStyle->pointSize = size;
    intersectionNodesNode->addChild(drawStyle);
}

void ModularEdgeComponent::clearIntersectionNodes() {
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    cleanupEdgeNode(intersectionNodesNode);
}

bool ModularEdgeComponent::hasIntersectionNodes() const {
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    return intersectionNodesNode != nullptr && intersectionNodesNode->getNumChildren() > 0;
}

void ModularEdgeComponent::extractFeatureEdges(
    const TopoDS_Shape& shape,
    double featureAngle,
    double minLength,
    bool onlyConvex,
    bool onlyConcave,
    const Quantity_Color& color,
    double width) {

    if (!m_featureExtractor || !m_featureRenderer) {
        LOG_WRN_S_ASYNC("Feature edge extractor/renderer not available");
        return;
    }

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    FeatureEdgeParams params(featureAngle, minLength, onlyConvex, onlyConcave);
    std::vector<gp_Pnt> points = m_featureExtractor->extract(shape, &params);

    // Note: cleanupEdgeNode sets featureEdgeNode to nullptr, so renderer doesn't need to clean it up again
    cleanupEdgeNode(featureEdgeNode);
    featureEdgeNode = m_featureRenderer->generateNode(points, color, width);

}

void ModularEdgeComponent::extractMeshEdges(
    const TriangleMesh& mesh,
    const Quantity_Color& color,
    double width) {
    LOG_INF_S("ModularEdgeComponent::extractMeshEdges called with " + 
              std::to_string(mesh.vertices.size()) + " vertices, " +
              std::to_string(mesh.triangles.size() / 3) + " triangles");
    
    if (!m_meshExtractor || !m_meshRenderer) {
        LOG_WRN_S_ASYNC("Mesh edge extractor/renderer not available");
        return;
    }

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    cleanupEdgeNode(meshEdgeNode);
    
    // Try GPU-accelerated rendering first (now fixed to use SoIndexedLineSet)
    auto meshRenderer = std::dynamic_pointer_cast<MeshEdgeRenderer>(m_meshRenderer);
    if (meshRenderer) {
        SoSeparator* gpuNode = meshRenderer->generateNodeFromMesh(mesh, color, width);
        if (gpuNode) {
            LOG_INF_S("ModularEdgeComponent: GPU mesh edge node created (using SoIndexedLineSet)");
            meshEdgeNode = gpuNode;
            return;
        }
    }
    
    // CPU fallback: extract edges (now with deduplication) and use SoIndexedLineSet
    LOG_INF_S("ModularEdgeComponent: Using CPU fallback for mesh edges");
    MeshEdgeParams params(mesh);
    std::vector<gp_Pnt> points = m_meshExtractor->extract(TopoDS_Shape(), &params);

    meshEdgeNode = m_meshRenderer->generateNode(points, color, width);
    
    if (meshEdgeNode) {
        LOG_INF_S("ModularEdgeComponent: CPU mesh edge node created with " + 
                  std::to_string(points.size()) + " points");
    } else {
        LOG_WRN_S("ModularEdgeComponent: Failed to create mesh edge node");
    }
}

void ModularEdgeComponent::extractSilhouetteEdges(
    const TopoDS_Shape& shape,
    const gp_Pnt& cameraPos,
    const Quantity_Color& color,
    double width) {

    if (!m_silhouetteExtractor) {
        LOG_WRN_S_ASYNC("Silhouette edge extractor not available");
        return;
    }

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    // Following FreeCAD's approach: extract silhouette edges based on view direction
    SilhouetteEdgeParams params(cameraPos);
    std::vector<gp_Pnt> points = m_silhouetteExtractor->extract(shape, &params);

    cleanupEdgeNode(silhouetteEdgeNode);
    
    // Create silhouette edge node with proper color and width
    if (m_originalRenderer && !points.empty()) {
        silhouetteEdgeNode = m_originalRenderer->generateNode(points, color, width);
        
        // CRITICAL FIX: Disable Display List caching (following FreeCAD's approach)
        if (silhouetteEdgeNode) {
            silhouetteEdgeNode->renderCaching.setValue(SoSeparator::OFF);
            silhouetteEdgeNode->boundingBoxCaching.setValue(SoSeparator::OFF);
            silhouetteEdgeNode->pickCulling.setValue(SoSeparator::OFF);
        }
    }

}

void ModularEdgeComponent::generateAllEdgeNodes() {
    // Compatibility method - edges are generated on-demand
}

SoSeparator* ModularEdgeComponent::getEdgeNode(EdgeType type) {
    std::lock_guard<std::mutex> lock(m_nodeMutex);

    switch (type) {
        case EdgeType::Original: return originalEdgeNode;
        case EdgeType::Feature: return featureEdgeNode;
        case EdgeType::Mesh: return meshEdgeNode;
        case EdgeType::Highlight: return highlightEdgeNode;
        case EdgeType::VerticeNormal: return normalLineNode;
        case EdgeType::FaceNormal: return faceNormalLineNode;
        case EdgeType::Silhouette: return silhouetteEdgeNode;
        case EdgeType::IntersectionNodes: return intersectionNodesNode;
        default: return nullptr;
    }
}

void ModularEdgeComponent::setEdgeDisplayType(EdgeType type, bool show) {
    switch (type) {
        case EdgeType::Original: edgeFlags.showOriginalEdges = show; break;
        case EdgeType::Feature: edgeFlags.showFeatureEdges = show; break;
        case EdgeType::Mesh: edgeFlags.showMeshEdges = show; break;
        case EdgeType::Highlight: edgeFlags.showHighlightEdges = show; break;
        case EdgeType::VerticeNormal: edgeFlags.showVerticeNormals = show; break;
        case EdgeType::FaceNormal: edgeFlags.showFaceNormals = show; break;
        case EdgeType::IntersectionNodes: edgeFlags.showIntersectionNodes = show; break;
    }
}

bool ModularEdgeComponent::isEdgeDisplayTypeEnabled(EdgeType type) const {
    switch (type) {
        case EdgeType::Original: return edgeFlags.showOriginalEdges;
        case EdgeType::Feature: return edgeFlags.showFeatureEdges;
        case EdgeType::Mesh: return edgeFlags.showMeshEdges;
        case EdgeType::Highlight: return edgeFlags.showHighlightEdges;
        case EdgeType::VerticeNormal: return edgeFlags.showVerticeNormals;
        case EdgeType::FaceNormal: return edgeFlags.showFaceNormals;
        case EdgeType::IntersectionNodes: return edgeFlags.showIntersectionNodes;
        default: return false;
    }
}

void ModularEdgeComponent::updateOriginalEdgesDisplay(SoSeparator* parentNode) {
    if (!parentNode) return;

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    // Remove existing edge nodes (except intersection nodes)
    for (int i = parentNode->getNumChildren() - 1; i >= 0; --i) {
        SoNode* child = parentNode->getChild(i);
        if (child == originalEdgeNode || child == featureEdgeNode ||
            child == meshEdgeNode || child == highlightEdgeNode ||
            child == normalLineNode || child == faceNormalLineNode ||
            child == silhouetteEdgeNode) {
            parentNode->removeChild(i);
        }
    }

    // Add current edge nodes (except intersection nodes)
    // CRITICAL FEATURE: When silhouette mode is active, show silhouette instead of original edges
    if (silhouetteEdgeNode) {
        // Silhouette edges take priority (fast mode)
        parentNode->addChild(silhouetteEdgeNode);
    } else if (originalEdgeNode && edgeFlags.showOriginalEdges) {
        parentNode->addChild(originalEdgeNode);
    }
    if (featureEdgeNode && edgeFlags.showFeatureEdges) {
        parentNode->addChild(featureEdgeNode);
    }
    if (meshEdgeNode && edgeFlags.showMeshEdges) {
        parentNode->addChild(meshEdgeNode);
    }
    if (highlightEdgeNode && edgeFlags.showHighlightEdges) {
        parentNode->addChild(highlightEdgeNode);
    }
    if (normalLineNode && edgeFlags.showVerticeNormals) {
        parentNode->addChild(normalLineNode);
    } else {
        if (!normalLineNode && edgeFlags.showVerticeNormals) {
            LOG_WRN_S_ASYNC("ModularEdgeComponent::updateOriginalEdgesDisplay - showVerticeNormals=true but normalLineNode is null");
        }
    }
    if (faceNormalLineNode && edgeFlags.showFaceNormals) {
        parentNode->addChild(faceNormalLineNode);
    }
    if (silhouetteEdgeNode) {
        parentNode->addChild(silhouetteEdgeNode);
    }
}

void ModularEdgeComponent::updateIntersectionNodesDisplay(SoSeparator* parentNode) {
    if (!parentNode) return;

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    // Remove existing intersection nodes
    for (int i = parentNode->getNumChildren() - 1; i >= 0; --i) {
        SoNode* child = parentNode->getChild(i);
        if (child == intersectionNodesNode) {
            parentNode->removeChild(i);
        }
    }

    // Add intersection nodes if enabled
    if (intersectionNodesNode && edgeFlags.showIntersectionNodes) {
        parentNode->addChild(intersectionNodesNode);
    }
}

// Helper function to validate edge node pointer
static bool isValidEdgeNode(SoSeparator* node) {
    if (!node) return false;
    
    uintptr_t ptrValue = reinterpret_cast<uintptr_t>(node);
    
    // Check for known invalid pointer patterns
    if (ptrValue == 0 ||
        ptrValue == 0xFFFFFFFFFFFFFFFFULL ||
        ptrValue == 0xFFFFFFFFFFFFFE87ULL ||
        ptrValue == 0xCDCDCDCDCDCDCDCDULL ||
        ptrValue == 0xDDDDDDDDDDDDDDDDULL ||
        ptrValue == 0xFEEEFEEEFEEEFEEEULL ||
        ptrValue == 0xBAADF00DBAADF00DULL ||
        ptrValue == 0xDEADBEEFDEADBEEFULL) {
        return false;
    }
    
    // Check alignment
    if (ptrValue % sizeof(void*) != 0) {
        return false;
    }
    
    // Try to access refCount (safe read)
    try {
        volatile int refCount = node->getRefCount();
        if (refCount < 0) {
            return false;
        }
    } catch (...) {
        return false;
    }
    
    return true;
}

void ModularEdgeComponent::updateEdgeDisplay(SoSeparator* parentNode) {
    if (!parentNode) return;

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    // Remove existing edge nodes
    for (int i = parentNode->getNumChildren() - 1; i >= 0; --i) {
        SoNode* child = parentNode->getChild(i);
        if (child == originalEdgeNode || child == featureEdgeNode ||
            child == meshEdgeNode || child == highlightEdgeNode ||
            child == normalLineNode || child == faceNormalLineNode ||
            child == silhouetteEdgeNode || child == intersectionNodesNode) {
            parentNode->removeChild(i);
        }
    }

    // Add current edge nodes at the END of the parent node
    // This ensures edges are rendered AFTER faces, so they appear on top
    // We use insertChild with a large index to ensure edges are always last
    int insertIndex = parentNode->getNumChildren();
    
    // CRITICAL FEATURE: When silhouette mode is active, show silhouette instead of original edges
    // Following FreeCAD's approach: silhouette edges provide fast preview mode
    // CRITICAL FIX: In HiddenLine mode (when showMeshEdges is true), don't show silhouette edges
    // HiddenLine mode should only show mesh edges, not silhouette edges
    if (silhouetteEdgeNode && isValidEdgeNode(silhouetteEdgeNode) && !edgeFlags.showMeshEdges) {
        // Silhouette edges take priority (fast mode), but not in HiddenLine mode
        parentNode->insertChild(silhouetteEdgeNode, insertIndex++);
    } else if (originalEdgeNode && edgeFlags.showOriginalEdges) {
        if (isValidEdgeNode(originalEdgeNode)) {
            parentNode->insertChild(originalEdgeNode, insertIndex++);
        } else {
            LOG_WRN_S("ModularEdgeComponent::updateEdgeDisplay: Invalid originalEdgeNode pointer detected, cleaning up");
            cleanupEdgeNode(originalEdgeNode);
        }
    }
    if (featureEdgeNode && edgeFlags.showFeatureEdges) {
        if (isValidEdgeNode(featureEdgeNode)) {
            parentNode->insertChild(featureEdgeNode, insertIndex++);
        } else {
            LOG_WRN_S("ModularEdgeComponent::updateEdgeDisplay: Invalid featureEdgeNode pointer detected, cleaning up");
            cleanupEdgeNode(featureEdgeNode);
        }
    }
    if (meshEdgeNode && edgeFlags.showMeshEdges) {
        // CRITICAL: Validate pointer before using it
        if (isValidEdgeNode(meshEdgeNode)) {
            LOG_INF_S("ModularEdgeComponent::updateEdgeDisplay: Adding mesh edge node to scene");
            parentNode->insertChild(meshEdgeNode, insertIndex++);
        } else {
            LOG_WRN_S("ModularEdgeComponent::updateEdgeDisplay: Invalid meshEdgeNode pointer detected, cleaning up");
            cleanupEdgeNode(meshEdgeNode);
        }
    } else if (edgeFlags.showMeshEdges && !meshEdgeNode) {
        LOG_WRN_S("ModularEdgeComponent::updateEdgeDisplay: showMeshEdges=true but meshEdgeNode is null!");
    }
    if (highlightEdgeNode && edgeFlags.showHighlightEdges) {
        if (isValidEdgeNode(highlightEdgeNode)) {
            parentNode->insertChild(highlightEdgeNode, insertIndex++);
        } else {
            LOG_WRN_S("ModularEdgeComponent::updateEdgeDisplay: Invalid highlightEdgeNode pointer detected, cleaning up");
            cleanupEdgeNode(highlightEdgeNode);
        }
    }
    if (normalLineNode && edgeFlags.showVerticeNormals) {
        if (isValidEdgeNode(normalLineNode)) {
            parentNode->insertChild(normalLineNode, insertIndex++);
        } else {
            LOG_WRN_S("ModularEdgeComponent::updateEdgeDisplay: Invalid normalLineNode pointer detected, cleaning up");
            cleanupEdgeNode(normalLineNode);
        }
    } else {
        if (!normalLineNode && edgeFlags.showVerticeNormals) {
            LOG_WRN_S_ASYNC("ModularEdgeComponent::updateEdgeDisplay - showVerticeNormals=true but normalLineNode is null");
        }
    }
    if (faceNormalLineNode && edgeFlags.showFaceNormals) {
        if (isValidEdgeNode(faceNormalLineNode)) {
            parentNode->insertChild(faceNormalLineNode, insertIndex++);
        } else {
            LOG_WRN_S("ModularEdgeComponent::updateEdgeDisplay: Invalid faceNormalLineNode pointer detected, cleaning up");
            cleanupEdgeNode(faceNormalLineNode);
        }
    }
    // Note: silhouetteEdgeNode is already handled above (line 404), no need to add it again here
    if (intersectionNodesNode && edgeFlags.showIntersectionNodes) {
        if (isValidEdgeNode(intersectionNodesNode)) {
            parentNode->insertChild(intersectionNodesNode, insertIndex++);
        } else {
            LOG_WRN_S("ModularEdgeComponent::updateEdgeDisplay: Invalid intersectionNodesNode pointer detected, cleaning up");
            cleanupEdgeNode(intersectionNodesNode);
        }
    }
}

void ModularEdgeComponent::applyAppearanceToEdgeNode(
    EdgeType type,
    const Quantity_Color& color,
    double width,
    int style) {

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    switch (type) {
        case EdgeType::Original:
            if (m_originalRenderer && originalEdgeNode) {
                // Additional validation: check if node is still valid
                if (reinterpret_cast<uintptr_t>(originalEdgeNode) != 0xFFFFFFFFFFFFFFFFULL &&
                    reinterpret_cast<uintptr_t>(originalEdgeNode) != 0xFFFFFFFFFFFFFE87ULL) {
                    try {
                        m_originalRenderer->updateAppearance(originalEdgeNode, color, width, style);
                    } catch (...) {
                        LOG_WRN_S_ASYNC("ModularEdgeComponent: Exception in updateAppearance for original edges");
                        // Node might be corrupted, clean it up
                        cleanupEdgeNode(originalEdgeNode);
                    }
                } else {
                    LOG_WRN_S_ASYNC("ModularEdgeComponent: Invalid originalEdgeNode pointer detected");
                    cleanupEdgeNode(originalEdgeNode);
                }
            }
            break;
        case EdgeType::Feature:
            if (m_featureRenderer && featureEdgeNode) {
                if (reinterpret_cast<uintptr_t>(featureEdgeNode) != 0xFFFFFFFFFFFFFFFFULL &&
                    reinterpret_cast<uintptr_t>(featureEdgeNode) != 0xFFFFFFFFFFFFFE87ULL) {
                    try {
                        m_featureRenderer->updateAppearance(featureEdgeNode, color, width, style);
                    } catch (...) {
                        LOG_WRN_S_ASYNC("ModularEdgeComponent: Exception in updateAppearance for feature edges");
                        cleanupEdgeNode(featureEdgeNode);
                    }
                } else {
                    LOG_WRN_S_ASYNC("ModularEdgeComponent: Invalid featureEdgeNode pointer detected");
                    cleanupEdgeNode(featureEdgeNode);
                }
            }
            break;
        case EdgeType::Mesh:
            if (m_meshRenderer && meshEdgeNode) {
                if (reinterpret_cast<uintptr_t>(meshEdgeNode) != 0xFFFFFFFFFFFFFFFFULL &&
                    reinterpret_cast<uintptr_t>(meshEdgeNode) != 0xFFFFFFFFFFFFFE87ULL) {
                    try {
                        m_meshRenderer->updateAppearance(meshEdgeNode, color, width, style);
                    } catch (...) {
                        LOG_WRN_S_ASYNC("ModularEdgeComponent: Exception in updateAppearance for mesh edges");
                        cleanupEdgeNode(meshEdgeNode);
                    }
                } else {
                    LOG_WRN_S_ASYNC("ModularEdgeComponent: Invalid meshEdgeNode pointer detected");
                    cleanupEdgeNode(meshEdgeNode);
                }
            }
            break;
        case EdgeType::IntersectionNodes:
            // For intersection nodes, we need to update the appearance directly
            // since they don't have a dedicated renderer
            if (intersectionNodesNode) {
                if (reinterpret_cast<uintptr_t>(intersectionNodesNode) != 0xFFFFFFFFFFFFFFFFULL &&
                    reinterpret_cast<uintptr_t>(intersectionNodesNode) != 0xFFFFFFFFFFFFFE87ULL) {
                    try {
                        updateIntersectionNodesAppearance(intersectionNodesNode, color, width);
                    } catch (...) {
                        LOG_WRN_S_ASYNC("ModularEdgeComponent: Exception in updateAppearance for intersection nodes");
                        cleanupEdgeNode(intersectionNodesNode);
                    }
                } else {
                    LOG_WRN_S_ASYNC("ModularEdgeComponent: Invalid intersectionNodesNode pointer detected");
                    cleanupEdgeNode(intersectionNodesNode);
                }
            }
            break;
        default:
            break;
    }
}

void ModularEdgeComponent::updateIntersectionNodesAppearance(
    SoSeparator* node,
    const Quantity_Color& color,
    double size) {

    if (!node) return;

    // Node structure:
    // node (SoSeparator)
    //   ├─ material (SoMaterial) - first child
    //   └─ pointSep (SoSeparator) - for each intersection point
    //       ├─ trans (SoTranslation)
    //       └─ sphere (SoSphere)

    // Update the top-level material color
    for (int i = 0; i < node->getNumChildren(); ++i) {
        SoNode* child = node->getChild(i);
        if (child->isOfType(SoMaterial::getClassTypeId())) {
            SoMaterial* material = static_cast<SoMaterial*>(child);
            material->diffuseColor.setValue(
                static_cast<float>(color.Red()),
                static_cast<float>(color.Green()),
                static_cast<float>(color.Blue())
            );
        } else if (child->isOfType(SoSeparator::getClassTypeId())) {
            // This is a point separator containing translation and sphere
            SoSeparator* sphereSep = static_cast<SoSeparator*>(child);
            for (int j = 0; j < sphereSep->getNumChildren(); ++j) {
                SoNode* subChild = sphereSep->getChild(j);
                if (subChild->isOfType(SoSphere::getClassTypeId())) {
                    SoSphere* sphere = static_cast<SoSphere*>(subChild);
                    sphere->radius.setValue(static_cast<float>(size * 0.01));
                }
            }
        }
    }
}

void ModularEdgeComponent::generateHighlightEdgeNode() {
}

void ModularEdgeComponent::generateNormalLineNode(const TriangleMesh& mesh, double length) {
    if (!m_meshRenderer) {
        LOG_WRN_S_ASYNC("ModularEdgeComponent::generateNormalLineNode - m_meshRenderer is null");
        return;
    }

    std::lock_guard<std::mutex> lock(m_nodeMutex);
    cleanupEdgeNode(normalLineNode);

    auto meshRenderer = std::static_pointer_cast<MeshEdgeRenderer>(m_meshRenderer);
    // Use red color for vertex normals (matching old implementation)
    Quantity_Color normalColor(1.0, 0.0, 0.0, Quantity_TOC_RGB);
    normalLineNode = meshRenderer->generateNormalLineNode(mesh, length, normalColor);
    
    if (normalLineNode) {
    } else {
        LOG_WRN_S_ASYNC("ModularEdgeComponent::generateNormalLineNode - Normal line node is null after generation");
    }
}

SoSeparator* ModularEdgeComponent::createIntersectionNodesNode(
    const std::vector<gp_Pnt>& intersectionPoints,
    const Quantity_Color& color,
    double size,
    IntersectionNodeShape shape) {

    if (intersectionPoints.empty()) return nullptr;

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    // Clean up existing intersection nodes
    cleanupEdgeNode(intersectionNodesNode);

    SoSeparator* node = new SoSeparator();
    node->ref();

    // CRITICAL FIX: Disable Display List caching to prevent GL context crashes
    // Following FreeCAD's approach: disable all three caches (triple safety)
    node->renderCaching.setValue(SoSeparator::OFF);
    node->boundingBoxCaching.setValue(SoSeparator::OFF);
    node->pickCulling.setValue(SoSeparator::OFF);

    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(
        static_cast<float>(color.Red()),
        static_cast<float>(color.Green()),
        static_cast<float>(color.Blue())
    );
    node->addChild(material);

    switch (shape) {
        case IntersectionNodeShape::Point: {
            // Most efficient: single point set for all points with adjustable size
            SoCoordinate3* coords = new SoCoordinate3();
            coords->point.setNum(intersectionPoints.size());

            SbVec3f* points = coords->point.startEditing();
            for (size_t i = 0; i < intersectionPoints.size(); ++i) {
                const auto& pt = intersectionPoints[i];
                points[i].setValue(
                    static_cast<float>(pt.X()),
                    static_cast<float>(pt.Y()),
                    static_cast<float>(pt.Z())
                );
            }
            coords->point.finishEditing();

            // Use SoDrawStyle to control point size
            SoDrawStyle* drawStyle = new SoDrawStyle();
            drawStyle->pointSize.setValue(static_cast<float>(size));

            SoPointSet* pointSet = new SoPointSet();
            pointSet->numPoints.setValue(intersectionPoints.size());

            node->addChild(drawStyle);
            node->addChild(coords);
            node->addChild(pointSet);
            break;
        }

        case IntersectionNodeShape::Cross: {
            // Balanced performance: cross made of lines
            float crossSize = static_cast<float>(size * 0.005f); // Adjust size for cross

            for (const auto& pt : intersectionPoints) {
                SoSeparator* pointSep = new SoSeparator();

                SoTranslation* trans = new SoTranslation();
                trans->translation.setValue(
                    static_cast<float>(pt.X()),
                    static_cast<float>(pt.Y()),
                    static_cast<float>(pt.Z())
                );
                pointSep->addChild(trans);

                // Create cross using two lines
                SoCoordinate3* coords = new SoCoordinate3();
                coords->point.setNum(4);
                coords->point.set1Value(0, -crossSize, 0, 0);
                coords->point.set1Value(1, crossSize, 0, 0);
                coords->point.set1Value(2, 0, -crossSize, 0);
                coords->point.set1Value(3, 0, crossSize, 0);

                SoLineSet* lineSet = new SoLineSet();
                int32_t indices[] = {0, 1, -1, 2, 3, -1}; // Two lines with separator
                lineSet->numVertices.setValues(0, 6, indices);

                pointSep->addChild(coords);
                pointSep->addChild(lineSet);

                node->addChild(pointSep);
            }
            break;
        }

        case IntersectionNodeShape::Cube: {
            // Good balance: simple cube
            float cubeSize = static_cast<float>(size * 0.003f); // Adjust size for cube

            for (const auto& pt : intersectionPoints) {
                SoSeparator* pointSep = new SoSeparator();

                SoTranslation* trans = new SoTranslation();
                trans->translation.setValue(
                    static_cast<float>(pt.X()),
                    static_cast<float>(pt.Y()),
                    static_cast<float>(pt.Z())
                );
                pointSep->addChild(trans);

                SoCube* cube = new SoCube();
                cube->width.setValue(cubeSize);
                cube->height.setValue(cubeSize);
                cube->depth.setValue(cubeSize);

                pointSep->addChild(cube);

                node->addChild(pointSep);
            }
            break;
        }

        case IntersectionNodeShape::Sphere:
        default: {
            // Traditional sphere (higher quality, slower)
            for (const auto& pt : intersectionPoints) {
                SoSeparator* pointSep = new SoSeparator();

                SoTranslation* trans = new SoTranslation();
                trans->translation.setValue(
                    static_cast<float>(pt.X()),
                    static_cast<float>(pt.Y()),
                    static_cast<float>(pt.Z())
                );
                pointSep->addChild(trans);

                SoSphere* sphere = new SoSphere();
                sphere->radius.setValue(static_cast<float>(size * 0.01));
                pointSep->addChild(sphere);

                node->addChild(pointSep);
            }
            break;
        }
    }

    // Save to member variable and enable display
    intersectionNodesNode = node;
    edgeFlags.showIntersectionNodes = true;

    return node;
}

void ModularEdgeComponent::generateFaceNormalLineNode(const TriangleMesh& mesh, double length) {
    if (!m_meshRenderer) return;

    std::lock_guard<std::mutex> lock(m_nodeMutex);
    cleanupEdgeNode(faceNormalLineNode);

    auto meshRenderer = std::static_pointer_cast<MeshEdgeRenderer>(m_meshRenderer);
    // Use blue color for face normals (matching old implementation)
    Quantity_Color faceNormalColor(0.0, 0.0, 1.0, Quantity_TOC_RGB);
    faceNormalLineNode = meshRenderer->generateFaceNormalLineNode(mesh, length, faceNormalColor);
}

void ModularEdgeComponent::clearMeshEdgeNode() {
    std::lock_guard<std::mutex> lock(m_nodeMutex);

    if (m_meshRenderer) {
        auto renderer = std::static_pointer_cast<MeshEdgeRenderer>(m_meshRenderer);
        renderer->clearMeshEdgeNode();
    }

    cleanupEdgeNode(meshEdgeNode);
}

void ModularEdgeComponent::clearSilhouetteEdgeNode() {
    std::lock_guard<std::mutex> lock(m_nodeMutex);
    cleanupEdgeNode(silhouetteEdgeNode);
}

void ModularEdgeComponent::setLODEnabled(bool enabled) {
    if (m_lodManager) {
        m_lodManager->setLODEnabled(enabled);
    }
}

bool ModularEdgeComponent::isLODEnabled() const {
    return m_lodManager && m_lodManager->isLODEnabled();
}

void ModularEdgeComponent::updateLODLevel(const gp_Pnt& cameraPos) {
    if (m_lodManager) {
        m_lodManager->updateLODLevel(cameraPos);
    }
}

void ModularEdgeComponent::generateLODLevels(const TopoDS_Shape& shape, const gp_Pnt& cameraPos) {
    if (m_lodManager) {
        m_lodManager->generateLODLevels(shape, cameraPos);
    }
}

void ModularEdgeComponent::cleanupEdgeNode(SoSeparator*& node) {
    if (node) {
        // Validate pointer before dereferencing
        uintptr_t ptrValue = reinterpret_cast<uintptr_t>(node);

        // Check for known invalid pointer patterns (64-bit) - MUST BE FIRST
        if (ptrValue == 0 ||
            ptrValue == 0xFFFFFFFFFFFFFFFFULL ||
            ptrValue == 0xFFFFFFFFFFFFFE87ULL ||
            ptrValue == 0xCDCDCDCDCDCDCDCDULL ||  // Uninitialized heap memory (MSVC debug)
            ptrValue == 0xDDDDDDDDDDDDDDDDULL ||  // Freed memory (MSVC debug)
            ptrValue == 0xFEEEFEEEFEEEFEEEULL ||  // Other debug patterns
            ptrValue == 0xBAADF00DBAADF00DULL ||
            ptrValue == 0xDEADBEEFDEADBEEFULL) {
            LOG_WRN_S_ASYNC("ModularEdgeComponent: Invalid pointer pattern detected in cleanupEdgeNode (0x" +
                     std::to_string(ptrValue) + "), skipping unref");
            node = nullptr;
            return;
        }

        // Additional check: pointer should be aligned (SoSeparator objects are typically 8-byte aligned)
        if (ptrValue % sizeof(void*) != 0) {
            LOG_WRN_S_ASYNC("ModularEdgeComponent: Unaligned pointer detected in cleanupEdgeNode (0x" +
                     std::to_string(ptrValue) + "), skipping unref");
            node = nullptr;
            return;
        }

        // Use Windows API to validate memory region (safer than direct access)
#ifdef _WIN32
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(node, &mbi, sizeof(mbi)) == 0) {
            // VirtualQuery failed - invalid memory
            LOG_WRN_S_ASYNC("ModularEdgeComponent: VirtualQuery failed for pointer in cleanupEdgeNode (0x" +
                     std::to_string(ptrValue) + "), skipping unref");
            node = nullptr;
            return;
        }

        // Check if memory is committed and accessible
        if (mbi.State != MEM_COMMIT ||
            (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
            LOG_WRN_S_ASYNC("ModularEdgeComponent: Invalid memory protection for pointer in cleanupEdgeNode (0x" +
                     std::to_string(ptrValue) + "), skipping unref");
            node = nullptr;
            return;
        }
#endif

        // Final safety check: try to call a simple method that doesn't modify state
        try {
            // Check if the object has a valid vtable by calling a non-virtual method
            // This is safer than accessing vtable directly
            volatile int refCount = node->getRefCount(); // This should not crash if object is valid
            if (refCount < 0) {
                LOG_WRN_S_ASYNC("ModularEdgeComponent: Invalid refCount detected in cleanupEdgeNode (" +
                         std::to_string(refCount) + "), skipping unref");
                node = nullptr;
                return;
            }
        } catch (...) {
            LOG_WRN_S_ASYNC("ModularEdgeComponent: Exception during refCount check in cleanupEdgeNode (0x" +
                     std::to_string(ptrValue) + "), skipping unref");
            node = nullptr;
            return;
        }

        // If we reach here, the pointer should be valid
        try {
            node->unref();
        } catch (const std::exception& e) {
            LOG_ERR_S_ASYNC("ModularEdgeComponent: Exception during node->unref(): " + std::string(e.what()));
        } catch (...) {
            LOG_ERR_S_ASYNC("ModularEdgeComponent: Unknown exception during node->unref()");
        }
        node = nullptr;
    }
}

void ModularEdgeComponent::clearEdgeNode(EdgeType type) {
    switch (type) {
        case EdgeType::Original:
            cleanupEdgeNode(originalEdgeNode);
            break;
        case EdgeType::Feature:
            cleanupEdgeNode(featureEdgeNode);
            break;
        case EdgeType::Mesh:
            clearMeshEdgeNode(); // Use specialized method that also clears renderer
            break;
        case EdgeType::VerticeNormal:
            cleanupEdgeNode(normalLineNode);
            break;
        case EdgeType::FaceNormal:
            cleanupEdgeNode(faceNormalLineNode);
            break;
        case EdgeType::Silhouette:
            clearSilhouetteEdgeNode(); // Use specialized method
            break;
        case EdgeType::Highlight:
            cleanupEdgeNode(highlightEdgeNode);
            break;
        default:
            break;
    }
}

void ModularEdgeComponent::computeIntersectionsAsync(
    const TopoDS_Shape& shape,
    double tolerance,
    class IAsyncEngine* engine,
    std::function<void(const std::vector<gp_Pnt>&, bool, const std::string&)> onComplete,
    std::function<void(int, const std::string&)> onProgress)
{
    if (!engine) {
        LOG_ERR_S_ASYNC("ModularEdgeComponent: AsyncEngineIntegration is null");
        if (onComplete) {
            onComplete({}, false, "AsyncEngineIntegration is null");
        }
        return;
    }

    if (m_computingIntersections.load()) {
        LOG_WRN_S_ASYNC("ModularEdgeComponent: Intersection computation already in progress");
        return;
    }

    // Count edges for diagnostic and decision making
    size_t edgeCount = 0;
    for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
        edgeCount++;
    }

    if (!m_asyncIntersectionComputer) {
        m_asyncIntersectionComputer = std::make_unique<async::AsyncEdgeIntersectionComputer>(engine);
    }

    m_computingIntersections.store(true);
    
    // Use progressive intersection detection directly
    if (m_originalExtractor) {
        auto originalExtractor = static_cast<OriginalEdgeExtractor*>(m_originalExtractor.get());
        
        std::vector<gp_Pnt> allIntersections;
        
        // Batch completion callback - display intersections as they're found
        auto onBatchComplete = [this, &allIntersections](const std::vector<gp_Pnt>& batchPoints) {
            if (!batchPoints.empty()) {
                // Only show simple intersection count in status bar
                LOG_INF_S_ASYNC("Found " + std::to_string(batchPoints.size()) + " intersections");

                // Create intersection nodes for this batch
                createIntersectionNodesNode(
                    batchPoints,
                    Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB), // Red color
                    3.0, // Size
                    IntersectionNodeShape::Point
                );

                // Request UI refresh to display the new intersection nodes
                // Note: We can't directly update the scene graph here because we don't have the parent node context
                // The intersection nodes will be displayed when the geometry's display is updated
                LOG_INF_S_ASYNC("Intersection nodes created, will be displayed on next refresh");
            }
        };
        
        // Progress callback
        auto onProgressCallback = [onProgress](int progress, const std::string& message) {
            if (onProgress) {
                onProgress(progress, message);
            }
        };
        
        // Use progressive intersection detection
        originalExtractor->findEdgeIntersectionsProgressive(
            shape,
            allIntersections,
            tolerance,
            onBatchComplete,
            onProgressCallback
        );
        
        m_computingIntersections.store(false);
        
        if (onComplete) {
            onComplete(allIntersections, true, "");
        }
    } else {
        LOG_ERR_S_ASYNC("ModularEdgeComponent: OriginalEdgeExtractor not available");
        m_computingIntersections.store(false);
        if (onComplete) {
            onComplete({}, false, "OriginalEdgeExtractor not available");
        }
    }
}

void ModularEdgeComponent::cancelIntersectionComputation() {
    if (m_asyncIntersectionComputer) {
        m_asyncIntersectionComputer->cancelComputation();
    }
    m_computingIntersections.store(false);
}

// ===== NEW IMPLEMENTATION: Cache-based edge rendering =====

void ModularEdgeComponent::extractAndCacheOriginalEdges(const TopoDS_Shape& shape, double samplingDensity, double minLength, const MeshParameters& meshParams) {
    std::lock_guard<std::mutex> lock(m_cachedEdgesMutex);
    
    // Clear previous cache
    m_cachedOriginalEdges.clear();
    
    if (shape.IsNull()) {
        LOG_WRN_S("ModularEdgeComponent::extractAndCacheOriginalEdges: Shape is null");
        return;
    }
    
    try {
        auto addSegmentsFromPoints = [&](const std::vector<gp_Pnt>& points) {
            if (points.size() < 2) {
                return;
            }
            std::vector<int> edgeVertexIndices;
            edgeVertexIndices.reserve(points.size());
            for (const auto& pt : points) {
                int vertexIndex = static_cast<int>(m_cachedOriginalEdges.vertices.size());
                m_cachedOriginalEdges.vertices.push_back(pt);
                edgeVertexIndices.push_back(vertexIndex);
            }
            for (size_t i = 1; i < edgeVertexIndices.size(); ++i) {
                m_cachedOriginalEdges.segments.push_back({ edgeVertexIndices[i - 1], edgeVertexIndices[i] });
            }
        };

        // Extract all edges from the shape using OpenCASCADE API directly
        for (TopExp_Explorer exp(shape, TopAbs_EDGE); exp.More(); exp.Next()) {
            const TopoDS_Edge& edge = TopoDS::Edge(exp.Current());
            
            if (edge.IsNull()) continue;
            
            bool sampledFromMesh = false;
            std::vector<gp_Pnt> edgePoints;

            // Prefer using existing face triangulation so edge vertices align with mesh nodes
            for (TopExp_Explorer faceExp(shape, TopAbs_FACE); faceExp.More(); faceExp.Next()) {
                const TopoDS_Face& face = TopoDS::Face(faceExp.Current());
                if (face.IsNull()) continue;

                TopLoc_Location loc;
                Handle(Poly_Triangulation) triangulation = BRep_Tool::Triangulation(face, loc);
                if (triangulation.IsNull()) continue;

                Handle(Poly_PolygonOnTriangulation) poly = BRep_Tool::PolygonOnTriangulation(edge, triangulation, loc);
                if (poly.IsNull()) continue;

                const TColStd_Array1OfInteger& nodes = poly->Nodes();
                if (nodes.Length() < 2) continue;

                gp_Trsf trsf = loc.Transformation();
                edgePoints.reserve(static_cast<size_t>(nodes.Length()));
                for (int i = nodes.Lower(); i <= nodes.Upper(); ++i) {
                    int nodeIndex = nodes(i);
                    if (nodeIndex < 1 || nodeIndex > triangulation->NbNodes()) continue;
                    gp_Pnt pt = triangulation->Node(nodeIndex);
                    if (!loc.IsIdentity()) {
                        pt.Transform(trsf);
                    }
                    edgePoints.push_back(pt);
                }

                if (edgePoints.size() >= 2) {
                    sampledFromMesh = true;
                }
                break; // Use first available triangulation to avoid duplicates
            }

            if (sampledFromMesh && edgePoints.size() >= 2) {
                addSegmentsFromPoints(edgePoints);
                continue;
            }

            // Get the curve adaptor
            BRepAdaptor_Curve curve(edge);
            double first = curve.FirstParameter();
            double last = curve.LastParameter();
            
            // Skip very short edges
            if (last - first < minLength * 0.1) continue;
            
            // Fallback: sample along curve; align spacing with mesh deflection when possible
            double length = 0.0;
            try {
                length = GCPnts_AbscissaPoint::Length(curve, first, last);
            } catch (const Standard_Failure&) {
                length = std::max(last - first, minLength);
            }

            double targetSpacing = std::max(minLength, meshParams.deflection);
            if (targetSpacing <= 0.0) {
                targetSpacing = minLength;
            }

            int numSamples = std::max(2, static_cast<int>(std::ceil(length / targetSpacing)) + 1);
            double step = (last - first) / static_cast<double>(numSamples - 1);

            edgePoints.clear();
            edgePoints.reserve(static_cast<size_t>(numSamples));
            for (int i = 0; i < numSamples; ++i) {
                double param = first + i * step;
                gp_Pnt pt = curve.Value(param);
                edgePoints.push_back(pt);
            }

            addSegmentsFromPoints(edgePoints);
        }
        
        m_cachedOriginalEdges.isValid = true;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("ModularEdgeComponent::extractAndCacheOriginalEdges: Exception: " + std::string(e.what()));
        m_cachedOriginalEdges.clear();
    }
}

SoSeparator* ModularEdgeComponent::createNodeFromCachedEdges(const Quantity_Color& color, double width) {
    std::lock_guard<std::mutex> lock(m_cachedEdgesMutex);
    
    if (!m_cachedOriginalEdges.isValid || m_cachedOriginalEdges.vertices.empty()) {
        LOG_WRN_S("ModularEdgeComponent::createNodeFromCachedEdges: No cached edge data available");
        return nullptr;
    }
    
    try {
        std::lock_guard<std::mutex> nodeLock(m_nodeMutex);
        
        // Clean up existing node
        cleanupEdgeNode(originalEdgeNode);
        
        // Create new node
        originalEdgeNode = new SoSeparator();
        originalEdgeNode->ref();
        
        // CRITICAL FIX: Disable Display List caching to prevent GL context crashes
        // Following FreeCAD's approach: disable all three caches (triple safety)
        // This prevents Coin3D from trying to create OpenGL Display Lists in invalid contexts
        originalEdgeNode->renderCaching.setValue(SoSeparator::OFF);
        originalEdgeNode->boundingBoxCaching.setValue(SoSeparator::OFF);
        originalEdgeNode->pickCulling.setValue(SoSeparator::OFF);
        
        // Add material
        SoMaterial* material = new SoMaterial();
        material->diffuseColor.setValue(
            static_cast<float>(color.Red()),
            static_cast<float>(color.Green()),
            static_cast<float>(color.Blue())
        );
        originalEdgeNode->addChild(material);
        
        // Add draw style
        SoDrawStyle* drawStyle = new SoDrawStyle();
        drawStyle->lineWidth.setValue(static_cast<float>(width));
        originalEdgeNode->addChild(drawStyle);
        
        // Add coordinates
        SoCoordinate3* coords = new SoCoordinate3();
        coords->point.setNum(m_cachedOriginalEdges.vertices.size());
        for (size_t i = 0; i < m_cachedOriginalEdges.vertices.size(); ++i) {
            const auto& pt = m_cachedOriginalEdges.vertices[i];
            coords->point.set1Value(i, 
                static_cast<float>(pt.X()),
                static_cast<float>(pt.Y()),
                static_cast<float>(pt.Z())
            );
        }
        originalEdgeNode->addChild(coords);
        
        // Add indexed line segments (CRITICAL FIX: use SoIndexedLineSet, not SoLineSet)
        // segments contains index pairs, we need to build coordIndex array with separators
        SoIndexedLineSet* lineSet = new SoIndexedLineSet();
        
        // Build coordIndex: for each segment (v1, v2), add: v1, v2, -1
        int coordIndexSize = m_cachedOriginalEdges.segments.size() * 3; // 3 indices per segment (v1, v2, -1)
        lineSet->coordIndex.setNum(coordIndexSize);
        
        int indexPos = 0;
        for (const auto& segment : m_cachedOriginalEdges.segments) {
            lineSet->coordIndex.set1Value(indexPos++, segment.first);   // Start vertex
            lineSet->coordIndex.set1Value(indexPos++, segment.second);  // End vertex
            lineSet->coordIndex.set1Value(indexPos++, -1);              // Separator
        }
        
        originalEdgeNode->addChild(lineSet);
        
        return originalEdgeNode;
        
    } catch (const std::exception& e) {
        LOG_ERR_S("ModularEdgeComponent::createNodeFromCachedEdges: Exception: " + std::string(e.what()));
        return nullptr;
    }
}


