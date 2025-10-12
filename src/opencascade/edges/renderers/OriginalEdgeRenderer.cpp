#include "edges/renderers/OriginalEdgeRenderer.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoTranslation.h>

OriginalEdgeRenderer::OriginalEdgeRenderer()
    : originalEdgeNode(nullptr)
    , intersectionNodesNode(nullptr) {}

OriginalEdgeRenderer::~OriginalEdgeRenderer() {
    if (originalEdgeNode) {
        originalEdgeNode->unref();
        originalEdgeNode = nullptr;
    }
    if (intersectionNodesNode) {
        intersectionNodesNode->unref();
        intersectionNodesNode = nullptr;
    }
}

SoSeparator* OriginalEdgeRenderer::generateNode(
    const std::vector<gp_Pnt>& points,
    const Quantity_Color& color,
    double width,
    int style) {

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    // Note: Node cleanup is handled by ModularEdgeComponent::cleanupEdgeNode()
    // before calling this method, so originalEdgeNode should already be nullptr

    if (points.empty()) return nullptr;

    // Create new node
    originalEdgeNode = createLineNode(points, color, width, style);
    return originalEdgeNode;
}

void OriginalEdgeRenderer::updateAppearance(
    SoSeparator* node,
    const Quantity_Color& color,
    double width,
    int style) {

    if (!node) return;

    // Additional validation - check if node is still valid
    if (reinterpret_cast<uintptr_t>(node) == 0xFFFFFFFFFFFFFFFFULL ||
        reinterpret_cast<uintptr_t>(node) == 0xFFFFFFFFFFFFFE87ULL) {
        LOG_WRN_S("OriginalEdgeRenderer::updateAppearance: Invalid node pointer detected");
        return;
    }

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    try {
        // Update material and draw style
        for (int i = 0; i < node->getNumChildren(); ++i) {
            SoNode* child = node->getChild(i);
            if (child && child->isOfType(SoMaterial::getClassTypeId())) {
                SoMaterial* material = static_cast<SoMaterial*>(child);
                material->diffuseColor.setValue(
                    static_cast<float>(color.Red()),
                    static_cast<float>(color.Green()),
                    static_cast<float>(color.Blue())
                );
            } else if (child && child->isOfType(SoDrawStyle::getClassTypeId())) {
                SoDrawStyle* drawStyle = static_cast<SoDrawStyle*>(child);
                applyLineStyle(drawStyle, width, style);
            }
        }
    } catch (const std::exception& e) {
        LOG_WRN_S("OriginalEdgeRenderer::updateAppearance: Exception occurred: " + std::string(e.what()));
    } catch (...) {
        LOG_WRN_S("OriginalEdgeRenderer::updateAppearance: Unknown exception occurred");
    }
}

SoSeparator* OriginalEdgeRenderer::generateIntersectionNodes(
    const std::vector<gp_Pnt>& points,
    const Quantity_Color& color,
    double size) {

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    // Clean up existing node
    if (intersectionNodesNode) {
        intersectionNodesNode->unref();
        intersectionNodesNode = nullptr;
    }

    if (points.empty()) return nullptr;

    // Create separator for intersection nodes
    intersectionNodesNode = new SoSeparator();
    intersectionNodesNode->ref();

    // Material for intersection nodes
    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(
        static_cast<float>(color.Red()),
        static_cast<float>(color.Green()),
        static_cast<float>(color.Blue())
    );
    intersectionNodesNode->addChild(material);

    // Create sphere for each intersection point
    for (const auto& point : points) {
        SoSeparator* sphereSep = new SoSeparator();

        // Translation to position
        SoTranslation* translation = new SoTranslation();
        translation->translation.setValue(
            static_cast<float>(point.X()),
            static_cast<float>(point.Y()),
            static_cast<float>(point.Z())
        );
        sphereSep->addChild(translation);

        // Sphere
        SoSphere* sphere = new SoSphere();
        sphere->radius.setValue(static_cast<float>(size));
        sphereSep->addChild(sphere);

        intersectionNodesNode->addChild(sphereSep);
    }

    return intersectionNodesNode;
}

void OriginalEdgeRenderer::updateLODLevel(class EdgeLODManager* lodManager, const Quantity_Color& color, double width) {
    // TODO: Implement LOD level updating for original edges
}
