#include "EdgeComponent.h"
#include "edges/EdgeExtractor.h"
#include "edges/EdgeRenderer.h"
#include "edges/EdgeLODManager.h"
#include "logger/Logger.h"

EdgeComponent::EdgeComponent()
    : m_extractor(std::make_unique<EdgeExtractor>())
    , m_renderer(std::make_unique<EdgeRenderer>())
    , m_lodManager(std::make_unique<EdgeLODManager>())
{
}

EdgeComponent::~EdgeComponent()
{
}

void EdgeComponent::extractOriginalEdges(
    const TopoDS_Shape& shape,
    double samplingDensity,
    double minLength,
    bool showLinesOnly,
    const Quantity_Color& color,
    double width,
    bool highlightIntersectionNodes,
    const Quantity_Color& intersectionNodeColor,
    double intersectionNodeSize)
{
    std::vector<gp_Pnt> intersectionPoints;

    // Check if LOD is enabled and generate LOD levels if needed
    if (m_lodManager && m_lodManager->isLODEnabled()) {
        // Generate LOD levels for this shape (camera position will be updated later)
        generateLODLevels(shape, gp_Pnt(0, 0, 100)); // Default camera position

        // Use LOD manager to update edge display
        m_renderer->updateLODLevel(m_lodManager.get());

        // For LOD mode, we still need to handle intersections if requested
        // Note: This is a simplified implementation. In a full implementation,
        // intersection detection would be LOD-aware.
        if (highlightIntersectionNodes) {
            // Use traditional extraction just for intersection detection
            m_extractor->extractOriginalEdges(
                shape, samplingDensity, minLength, showLinesOnly, &intersectionPoints
            );
        }
    } else {
        // Use traditional edge extraction
        std::vector<gp_Pnt> points = m_extractor->extractOriginalEdges(
            shape, samplingDensity, minLength, showLinesOnly,
            highlightIntersectionNodes ? &intersectionPoints : nullptr
        );

        m_renderer->generateOriginalEdgeNode(points, color, width);
    }

    if (highlightIntersectionNodes && !intersectionPoints.empty()) {
        m_renderer->generateIntersectionNodesNode(
            intersectionPoints, intersectionNodeColor, intersectionNodeSize
        );
    }
}

void EdgeComponent::extractFeatureEdges(
    const TopoDS_Shape& shape, 
    double featureAngle, 
    double minLength, 
    bool onlyConvex, 
    bool onlyConcave)
{
    std::vector<gp_Pnt> points = m_extractor->extractFeatureEdges(
        shape, featureAngle, minLength, onlyConvex, onlyConcave
    );

    Quantity_Color color(1.0, 0.0, 0.0, Quantity_TOC_RGB);
    m_renderer->generateFeatureEdgeNode(points, color, 2.0);
}

void EdgeComponent::extractMeshEdges(const TriangleMesh& mesh)
{
    std::vector<gp_Pnt> points = m_extractor->extractMeshEdges(mesh);

    Quantity_Color color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    m_renderer->generateMeshEdgeNode(points, color, 1.0);
}

void EdgeComponent::generateAllEdgeNodes()
{
}

SoSeparator* EdgeComponent::getEdgeNode(EdgeType type)
{
    return m_renderer->getEdgeNode(type);
}

void EdgeComponent::setEdgeDisplayType(EdgeType type, bool show)
{
    switch (type) {
        case EdgeType::Original:
            edgeFlags.showOriginalEdges = show;
            break;
        case EdgeType::Feature:
            edgeFlags.showFeatureEdges = show;
            break;
        case EdgeType::Mesh:
            edgeFlags.showMeshEdges = show;
            break;
        case EdgeType::Highlight:
            edgeFlags.showHighlightEdges = show;
            break;
        case EdgeType::Silhouette:
            edgeFlags.showSilhouetteEdges = show;
            break;
        case EdgeType::NormalLine:
            edgeFlags.showNormalLines = show;
            break;
        case EdgeType::FaceNormalLine:
            edgeFlags.showFaceNormalLines = show;
            break;
    }
}

bool EdgeComponent::isEdgeDisplayTypeEnabled(EdgeType type) const
{
    switch (type) {
        case EdgeType::Original:
            return edgeFlags.showOriginalEdges;
        case EdgeType::Feature:
            return edgeFlags.showFeatureEdges;
        case EdgeType::Mesh:
            return edgeFlags.showMeshEdges;
        case EdgeType::Highlight:
            return edgeFlags.showHighlightEdges;
        case EdgeType::Silhouette:
            return edgeFlags.showSilhouetteEdges;
        case EdgeType::NormalLine:
            return edgeFlags.showNormalLines;
        case EdgeType::FaceNormalLine:
            return edgeFlags.showFaceNormalLines;
        default:
            return false;
    }
}

void EdgeComponent::updateEdgeDisplay(SoSeparator* parentNode)
{
    m_renderer->updateEdgeDisplay(parentNode, edgeFlags);
}

void EdgeComponent::applyAppearanceToEdgeNode(
    EdgeType type, 
    const Quantity_Color& color, 
    double width, 
    int style)
{
    m_renderer->applyAppearanceToEdgeNode(type, color, width, style);
}

void EdgeComponent::generateHighlightEdgeNode()
{
    m_renderer->generateHighlightEdgeNode();
}

void EdgeComponent::generateNormalLineNode(const TriangleMesh& mesh, double length)
{
    m_renderer->generateNormalLineNode(mesh, length);
}

void EdgeComponent::generateFaceNormalLineNode(const TriangleMesh& mesh, double length)
{
    m_renderer->generateFaceNormalLineNode(mesh, length);
}

void EdgeComponent::generateSilhouetteEdgeNode(const TopoDS_Shape& shape, const gp_Pnt& cameraPos)
{
    std::vector<gp_Pnt> points = m_extractor->extractSilhouetteEdges(shape, cameraPos);

    Quantity_Color color(0.0, 0.0, 0.0, Quantity_TOC_RGB);
    m_renderer->generateSilhouetteEdgeNode(points, color, 2.0);
}

void EdgeComponent::clearSilhouetteEdgeNode()
{
    m_renderer->clearSilhouetteEdgeNode();
}

void EdgeComponent::generateIntersectionNodesNode(
    const std::vector<gp_Pnt>& intersectionPoints,
    const Quantity_Color& color,
    double size)
{
    m_renderer->generateIntersectionNodesNode(intersectionPoints, color, size);
}

// LOD (Level of Detail) management methods
void EdgeComponent::setLODEnabled(bool enabled)
{
    if (m_lodManager) {
        m_lodManager->setLODEnabled(enabled);
        LOG_INF_S("Edge LOD " + std::string(enabled ? "enabled" : "disabled"));
    }
}

bool EdgeComponent::isLODEnabled() const
{
    return m_lodManager ? m_lodManager->isLODEnabled() : false;
}

void EdgeComponent::updateLODLevel(const gp_Pnt& cameraPos)
{
    if (!m_lodManager || !m_lodManager->isLODEnabled()) {
        return;
    }

    bool lodChanged = m_lodManager->updateLODLevel(cameraPos);
    if (lodChanged) {
        LOG_INF_S("Edge LOD level updated based on camera position");
        // TODO: Update edge rendering based on new LOD level
    }
}

void EdgeComponent::generateLODLevels(const TopoDS_Shape& shape, const gp_Pnt& cameraPos)
{
    if (!m_lodManager) {
        LOG_WRN_S("LOD manager not initialized");
        return;
    }

    EdgeLODManager::LODStats stats;
    m_lodManager->generateLODLevels(shape, cameraPos, &stats);

    LOG_INF_S("Generated edge LOD levels:");
    LOG_INF_S("  Total edges: " + std::to_string(stats.totalEdges));
    LOG_INF_S("  Minimal LOD: " + std::to_string(stats.minimalEdges) + " edges");
    LOG_INF_S("  Low LOD: " + std::to_string(stats.lowEdges) + " edges");
    LOG_INF_S("  Medium LOD: " + std::to_string(stats.mediumEdges) + " edges");
    LOG_INF_S("  High LOD: " + std::to_string(stats.highEdges) + " edges");
    LOG_INF_S("  Maximum LOD: " + std::to_string(stats.maximumEdges) + " edges");
    LOG_INF_S("  Memory usage: " + std::to_string(stats.memoryUsageMB) + " MB");
}
