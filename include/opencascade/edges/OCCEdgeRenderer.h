#pragma once

#include <memory>

// Forward declarations
class SoSeparator;
class Quantity_Color;
struct TriangleMesh;

/**
 * @brief OCCEdgeRenderer - Edge renderer
 *
 * Responsible for rendering extracted edges as Coin3D nodes
 */
class OCCEdgeRenderer
{
public:
    OCCEdgeRenderer();
    virtual ~OCCEdgeRenderer() = default;

    // Node creation
    virtual SoSeparator* createOriginalEdgeNode(const std::vector<TopoDS_Edge>& edges,
        const Quantity_Color& color, double width);
    virtual SoSeparator* createFeatureEdgeNode(const std::vector<TopoDS_Edge>& edges,
        const Quantity_Color& color, double width, int style = 0);
    virtual SoSeparator* createMeshEdgeNode(const std::vector<std::pair<gp_Pnt, gp_Pnt>>& edges,
        const Quantity_Color& color, double width);
    virtual SoSeparator* createNormalLineNode(const std::vector<std::pair<gp_Pnt, gp_Vec>>& normals,
        double length, const Quantity_Color& color);
    virtual SoSeparator* createIntersectionNode(const std::vector<gp_Pnt>& points,
        const Quantity_Color& color, double size);

    // Advanced rendering
    virtual SoSeparator* createHighlightEdgeNode();
    virtual void applyAppearanceToEdgeNode(SoSeparator* edgeNode,
        const Quantity_Color& color, double width, int style = 0);

    // Batch rendering
    virtual void renderAllEdges(SoSeparator* parentNode);

protected:
    // Rendering helper methods
    virtual void setupMaterial(SoSeparator* node, const Quantity_Color& color, double transparency = 0.0);
    virtual void setupDrawStyle(SoSeparator* node, double width, int style = 0);

    // Cached nodes
    SoSeparator* m_originalEdgeNode;
    SoSeparator* m_featureEdgeNode;
    SoSeparator* m_meshEdgeNode;
    SoSeparator* m_normalLineNode;
    SoSeparator* m_intersectionNode;
    SoSeparator* m_highlightEdgeNode;
};
