#include "edges/ModularEdgeComponent.h"
#include "edges/EdgeProcessorFactory.h"
#include "edges/EdgeLODManager.h"
#include "edges/renderers/MeshEdgeRenderer.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoSphere.h>

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
        LOG_ERR_S("Failed to initialize edge processors: " + std::string(e.what()));
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
    double intersectionNodeSize) {

    if (!m_originalExtractor || !m_originalRenderer) {
        LOG_WRN_S("Original edge extractor/renderer not available");
        return;
    }

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    OriginalEdgeParams params(samplingDensity, minLength, showLinesOnly, highlightIntersectionNodes);
    std::vector<gp_Pnt> points = m_originalExtractor->extract(shape, &params);

    cleanupEdgeNode(originalEdgeNode);
    originalEdgeNode = m_originalRenderer->generateNode(points, color, width);

    // Handle intersection node highlighting
    if (highlightIntersectionNodes) {
        // Extract intersection points
        std::vector<gp_Pnt> intersectionPoints;
        // Use the extractor's intersection detection method
        static_cast<OriginalEdgeExtractor*>(m_originalExtractor.get())->findEdgeIntersections(shape, intersectionPoints, 0.0); // Use adaptive tolerance

        if (!intersectionPoints.empty()) {
            cleanupEdgeNode(intersectionNodesNode);
            intersectionNodesNode = createIntersectionNodesNode(intersectionPoints, intersectionNodeColor, intersectionNodeSize);
        } else {
            cleanupEdgeNode(intersectionNodesNode);
        }
    } else {
        // Clean up intersection nodes if highlighting is disabled
        cleanupEdgeNode(intersectionNodesNode);
    }

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
        LOG_WRN_S("Feature edge extractor/renderer not available");
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
    if (!m_meshExtractor || !m_meshRenderer) {
        LOG_WRN_S("Mesh edge extractor/renderer not available");
        return;
    }

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    MeshEdgeParams params(mesh);
    std::vector<gp_Pnt> points = m_meshExtractor->extract(TopoDS_Shape(), &params);

    cleanupEdgeNode(meshEdgeNode);
    meshEdgeNode = m_meshRenderer->generateNode(points, color, width);

}

void ModularEdgeComponent::extractSilhouetteEdges(
    const TopoDS_Shape& shape,
    const gp_Pnt& cameraPos) {

    if (!m_silhouetteExtractor) {
        LOG_WRN_S("Silhouette edge extractor not available");
        return;
    }

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    SilhouetteEdgeParams params(cameraPos);
    std::vector<gp_Pnt> points = m_silhouetteExtractor->extract(shape, &params);

    cleanupEdgeNode(silhouetteEdgeNode);
    if (m_originalRenderer) {
        silhouetteEdgeNode = m_originalRenderer->generateNode(points);
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
        case EdgeType::NormalLine: return normalLineNode;
        case EdgeType::FaceNormalLine: return faceNormalLineNode;
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
        case EdgeType::NormalLine: edgeFlags.showNormalLines = show; break;
        case EdgeType::FaceNormalLine: edgeFlags.showFaceNormalLines = show; break;
        case EdgeType::IntersectionNodes: edgeFlags.showIntersectionNodes = show; break;
    }
}

bool ModularEdgeComponent::isEdgeDisplayTypeEnabled(EdgeType type) const {
    switch (type) {
        case EdgeType::Original: return edgeFlags.showOriginalEdges;
        case EdgeType::Feature: return edgeFlags.showFeatureEdges;
        case EdgeType::Mesh: return edgeFlags.showMeshEdges;
        case EdgeType::Highlight: return edgeFlags.showHighlightEdges;
        case EdgeType::NormalLine: return edgeFlags.showNormalLines;
        case EdgeType::FaceNormalLine: return edgeFlags.showFaceNormalLines;
        case EdgeType::IntersectionNodes: return edgeFlags.showIntersectionNodes;
        default: return false;
    }
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

    // Add current edge nodes
    if (originalEdgeNode && edgeFlags.showOriginalEdges) {
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
    if (normalLineNode && edgeFlags.showNormalLines) {
        parentNode->addChild(normalLineNode);
        LOG_INF_S("ModularEdgeComponent::updateEdgeDisplay - Added normal line node to scene");
    } else {
        if (!normalLineNode && edgeFlags.showNormalLines) {
            LOG_WRN_S("ModularEdgeComponent::updateEdgeDisplay - showNormalLines=true but normalLineNode is null");
        }
    }
    if (faceNormalLineNode && edgeFlags.showFaceNormalLines) {
        parentNode->addChild(faceNormalLineNode);
    }
    if (silhouetteEdgeNode) {
        parentNode->addChild(silhouetteEdgeNode);
    }
    if (intersectionNodesNode && edgeFlags.showIntersectionNodes) {
        parentNode->addChild(intersectionNodesNode);
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
                        LOG_WRN_S("ModularEdgeComponent: Exception in updateAppearance for original edges");
                        // Node might be corrupted, clean it up
                        cleanupEdgeNode(originalEdgeNode);
                    }
                } else {
                    LOG_WRN_S("ModularEdgeComponent: Invalid originalEdgeNode pointer detected");
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
                        LOG_WRN_S("ModularEdgeComponent: Exception in updateAppearance for feature edges");
                        cleanupEdgeNode(featureEdgeNode);
                    }
                } else {
                    LOG_WRN_S("ModularEdgeComponent: Invalid featureEdgeNode pointer detected");
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
                        LOG_WRN_S("ModularEdgeComponent: Exception in updateAppearance for mesh edges");
                        cleanupEdgeNode(meshEdgeNode);
                    }
                } else {
                    LOG_WRN_S("ModularEdgeComponent: Invalid meshEdgeNode pointer detected");
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
                        LOG_WRN_S("ModularEdgeComponent: Exception in updateAppearance for intersection nodes");
                        cleanupEdgeNode(intersectionNodesNode);
                    }
                } else {
                    LOG_WRN_S("ModularEdgeComponent: Invalid intersectionNodesNode pointer detected");
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
        LOG_WRN_S("ModularEdgeComponent::generateNormalLineNode - m_meshRenderer is null");
        return;
    }

    std::lock_guard<std::mutex> lock(m_nodeMutex);
    cleanupEdgeNode(normalLineNode);

    auto meshRenderer = std::static_pointer_cast<MeshEdgeRenderer>(m_meshRenderer);
    // Use red color for vertex normals (matching old implementation)
    Quantity_Color normalColor(1.0, 0.0, 0.0, Quantity_TOC_RGB);
    normalLineNode = meshRenderer->generateNormalLineNode(mesh, length, normalColor);
    
    if (normalLineNode) {
        LOG_INF_S("ModularEdgeComponent::generateNormalLineNode - Normal line node created successfully");
    } else {
        LOG_WRN_S("ModularEdgeComponent::generateNormalLineNode - Normal line node is null after generation");
    }
}

SoSeparator* ModularEdgeComponent::createIntersectionNodesNode(
    const std::vector<gp_Pnt>& intersectionPoints,
    const Quantity_Color& color,
    double size) {

    if (intersectionPoints.empty()) return nullptr;

    SoSeparator* node = new SoSeparator();
    node->ref();

    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(
        static_cast<float>(color.Red()),
        static_cast<float>(color.Green()),
        static_cast<float>(color.Blue())
    );
    node->addChild(material);

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
            LOG_WRN_S("ModularEdgeComponent: Invalid pointer pattern detected in cleanupEdgeNode (0x" +
                     std::to_string(ptrValue) + "), skipping unref");
            node = nullptr;
            return;
        }

        // Additional check: pointer should be aligned (SoSeparator objects are typically 8-byte aligned)
        if (ptrValue % sizeof(void*) != 0) {
            LOG_WRN_S("ModularEdgeComponent: Unaligned pointer detected in cleanupEdgeNode (0x" +
                     std::to_string(ptrValue) + "), skipping unref");
            node = nullptr;
            return;
        }

        // Use Windows API to validate memory region (safer than direct access)
#ifdef _WIN32
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(node, &mbi, sizeof(mbi)) == 0) {
            // VirtualQuery failed - invalid memory
            LOG_WRN_S("ModularEdgeComponent: VirtualQuery failed for pointer in cleanupEdgeNode (0x" +
                     std::to_string(ptrValue) + "), skipping unref");
            node = nullptr;
            return;
        }

        // Check if memory is committed and accessible
        if (mbi.State != MEM_COMMIT ||
            (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
            LOG_WRN_S("ModularEdgeComponent: Invalid memory protection for pointer in cleanupEdgeNode (0x" +
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
                LOG_WRN_S("ModularEdgeComponent: Invalid refCount detected in cleanupEdgeNode (" +
                         std::to_string(refCount) + "), skipping unref");
                node = nullptr;
                return;
            }
        } catch (...) {
            LOG_WRN_S("ModularEdgeComponent: Exception during refCount check in cleanupEdgeNode (0x" +
                     std::to_string(ptrValue) + "), skipping unref");
            node = nullptr;
            return;
        }

        // If we reach here, the pointer should be valid
        try {
            node->unref();
            LOG_DBG_S("ModularEdgeComponent: Successfully unref'd node");
        } catch (const std::exception& e) {
            LOG_ERR_S("ModularEdgeComponent: Exception during node->unref(): " + std::string(e.what()));
        } catch (...) {
            LOG_ERR_S("ModularEdgeComponent: Unknown exception during node->unref()");
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
        case EdgeType::NormalLine:
            cleanupEdgeNode(normalLineNode);
            break;
        case EdgeType::FaceNormalLine:
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
