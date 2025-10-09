#include "EdgeComponent.h"
#include "edges/EdgeExtractor.h"
#include "edges/EdgeRenderer.h"
#include "logger/Logger.h"

EdgeComponent::EdgeComponent() 
    : m_extractor(std::make_unique<EdgeExtractor>())
    , m_renderer(std::make_unique<EdgeRenderer>())
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
    std::vector<gp_Pnt> points = m_extractor->extractOriginalEdges(
        shape, samplingDensity, minLength, showLinesOnly, 
        highlightIntersectionNodes ? &intersectionPoints : nullptr
    );

    m_renderer->generateOriginalEdgeNode(points, color, width);

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
