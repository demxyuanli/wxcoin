#include "edges/renderers/FeatureEdgeRenderer.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDrawStyle.h>

FeatureEdgeRenderer::FeatureEdgeRenderer()
    : featureEdgeNode(nullptr) {}

FeatureEdgeRenderer::~FeatureEdgeRenderer() {
    if (featureEdgeNode) {
        // Check for invalid pointers
        if (reinterpret_cast<uintptr_t>(featureEdgeNode) == 0xFFFFFFFFFFFFFFFFULL ||
            reinterpret_cast<uintptr_t>(featureEdgeNode) == 0xFFFFFFFFFFFFFE87ULL) {
            LOG_WRN_S("FeatureEdgeRenderer destructor: Invalid featureEdgeNode pointer detected, skipping unref");
        } else {
            try {
                featureEdgeNode->unref();
            } catch (const std::exception& e) {
                LOG_WRN_S("FeatureEdgeRenderer destructor: Exception during unref: " + std::string(e.what()));
            } catch (...) {
                LOG_WRN_S("FeatureEdgeRenderer destructor: Unknown exception during unref");
            }
        }
        featureEdgeNode = nullptr;
    }
}

SoSeparator* FeatureEdgeRenderer::generateNode(
    const std::vector<gp_Pnt>& points,
    const Quantity_Color& color,
    double width,
    int style) {

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    // Note: Node cleanup is handled by ModularEdgeComponent::cleanupEdgeNode()
    // before calling this method, so featureEdgeNode should already be nullptr

    if (points.empty()) return nullptr;

    // Create new node with enhanced styling for feature edges
    featureEdgeNode = createLineNode(points, color, width, style);
    return featureEdgeNode;
}

void FeatureEdgeRenderer::updateAppearance(
    SoSeparator* node,
    const Quantity_Color& color,
    double width,
    int style) {

    if (!node) return;

    // Additional validation - check if node is still valid
    if (reinterpret_cast<uintptr_t>(node) == 0xFFFFFFFFFFFFFFFFULL ||
        reinterpret_cast<uintptr_t>(node) == 0xFFFFFFFFFFFFFE87ULL) {
        LOG_WRN_S("FeatureEdgeRenderer::updateAppearance: Invalid node pointer detected");
        return;
    }

    std::lock_guard<std::mutex> lock(m_nodeMutex);

    try {
        // Update material and draw style with feature edge styling
        for (int i = 0; i < node->getNumChildren(); ++i) {
            SoNode* child = node->getChild(i);
            if (child && child->isOfType(SoMaterial::getClassTypeId())) {
                SoMaterial* material = static_cast<SoMaterial*>(child);
                material->diffuseColor.setValue(
                    static_cast<float>(color.Red()),
                    static_cast<float>(color.Green()),
                    static_cast<float>(color.Blue())
                );
                // Add slight emissive for feature edges
                material->emissiveColor.setValue(
                    static_cast<float>(color.Red() * 0.1f),
                    static_cast<float>(color.Green() * 0.1f),
                    static_cast<float>(color.Blue() * 0.1f)
                );
            } else if (child && child->isOfType(SoDrawStyle::getClassTypeId())) {
                SoDrawStyle* drawStyle = static_cast<SoDrawStyle*>(child);
                drawStyle->lineWidth.setValue(static_cast<float>(width));
            }
        }
    } catch (const std::exception& e) {
        LOG_WRN_S("FeatureEdgeRenderer::updateAppearance: Exception occurred: " + std::string(e.what()));
    } catch (...) {
        LOG_WRN_S("FeatureEdgeRenderer::updateAppearance: Unknown exception occurred");
    }
}

SoSeparator* FeatureEdgeRenderer::generateFeatureNode(
    const std::vector<gp_Pnt>& points,
    const Quantity_Color& color,
    double width,
    const Quantity_Color& convexColor,
    const Quantity_Color& concaveColor) {

    // For now, use the basic implementation
    // TODO: Implement convex/concave color differentiation
    return generateNode(points, color, width);
}
