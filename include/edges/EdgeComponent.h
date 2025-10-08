#pragma once

#include "edges/EdgeExtractor.h"
#include "edges/EdgeRenderer.h"
#include "EdgeTypes.h"
#include <memory>

/**
 * @brief High-level edge component - combines extraction and rendering
 * 
 * This class provides a simplified interface that combines
 * EdgeExtractor and EdgeRenderer functionality
 */
class EdgeComponent {
public:
    EdgeDisplayFlags edgeFlags;

    EdgeComponent();
    ~EdgeComponent() = default;

    /**
     * @brief Extract and visualize original edges from shape
     */
    void extractOriginalEdges(
        const TopoDS_Shape& shape, 
        double samplingDensity = 80.0, 
        double minLength = 0.01, 
        bool showLinesOnly = false, 
        const Quantity_Color& color = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB), 
        double width = 1.0,
        bool highlightIntersectionNodes = false, 
        const Quantity_Color& intersectionNodeColor = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB), 
        double intersectionNodeSize = 3.0);

    /**
     * @brief Extract and visualize feature edges from shape
     */
    void extractFeatureEdges(
        const TopoDS_Shape& shape, 
        double featureAngle, 
        double minLength, 
        bool onlyConvex, 
        bool onlyConcave);

    /**
     * @brief Extract and visualize mesh edges
     */
    void extractMeshEdges(const TriangleMesh& mesh);

    /**
     * @brief Generate all edge nodes
     */
    void generateAllEdgeNodes();

    /**
     * @brief Get edge node by type
     */
    SoSeparator* getEdgeNode(EdgeType type);

    /**
     * @brief Set edge display type visibility
     */
    void setEdgeDisplayType(EdgeType type, bool show);

    /**
     * @brief Check if edge display type is enabled
     */
    bool isEdgeDisplayTypeEnabled(EdgeType type) const;

    /**
     * @brief Update edge display in parent node
     */
    void updateEdgeDisplay(SoSeparator* parentNode);

    /**
     * @brief Apply appearance to edge node
     */
    void applyAppearanceToEdgeNode(
        EdgeType type, 
        const Quantity_Color& color, 
        double width, 
        int style = 0);

    /**
     * @brief Generate highlight edge node
     */
    void generateHighlightEdgeNode();

    /**
     * @brief Generate normal line visualization
     */
    void generateNormalLineNode(const TriangleMesh& mesh, double length);

    /**
     * @brief Generate face normal line visualization
     */
    void generateFaceNormalLineNode(const TriangleMesh& mesh, double length);

    /**
     * @brief Generate silhouette edges for camera position
     */
    void generateSilhouetteEdgeNode(const TopoDS_Shape& shape, const gp_Pnt& cameraPos);

    /**
     * @brief Clear silhouette edge node
     */
    void clearSilhouetteEdgeNode();

    /**
     * @brief Generate intersection nodes visualization
     */
    void generateIntersectionNodesNode(
        const std::vector<gp_Pnt>& intersectionPoints, 
        const Quantity_Color& color, 
        double size);

    // Allow OCCViewer to access internal nodes if needed
    friend class OCCViewer;

private:
    std::unique_ptr<EdgeExtractor> m_extractor;
    std::unique_ptr<EdgeRenderer> m_renderer;
};
