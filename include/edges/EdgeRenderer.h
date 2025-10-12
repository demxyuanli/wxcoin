#pragma once

#include <vector>
#include <mutex>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "EdgeTypes.h"

// Forward declarations
class SoSeparator;
struct TriangleMesh;
class EdgeLODManager;
class GPUEdgeRenderer;

/**
 * @brief Edge visualization and rendering
 * 
 * Handles creation and management of Coin3D nodes for edge display:
 * - Original edges
 * - Feature edges
 * - Mesh edges
 * - Highlight edges
 * - Normal lines
 * - Silhouette edges
 */
class EdgeRenderer {
public:
    EdgeRenderer();
    ~EdgeRenderer();

    /**
     * @brief Generate Coin3D node for original edges
     * @param points Sampled edge points
     * @param color Edge color
     * @param width Edge line width
     */
    void generateOriginalEdgeNode(
        const std::vector<gp_Pnt>& points,
        const Quantity_Color& color = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB),
        double width = 1.0);

    /**
     * @brief Generate Coin3D node for feature edges
     * @param points Sampled edge points
     * @param color Edge color
     * @param width Edge line width
     */
    void generateFeatureEdgeNode(
        const std::vector<gp_Pnt>& points,
        const Quantity_Color& color = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB),
        double width = 2.0);

    /**
     * @brief Generate Coin3D node for mesh edges
     * @param points Edge endpoints
     * @param color Edge color
     * @param width Edge line width
     */
    void generateMeshEdgeNode(
        const std::vector<gp_Pnt>& points,
        const Quantity_Color& color = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB),
        double width = 1.0);

    /**
     * @brief Generate highlight edge node
     */
    void generateHighlightEdgeNode();

    /**
     * @brief Generate normal line node from mesh
     * @param mesh Triangle mesh
     * @param length Normal line length
     */
    void generateNormalLineNode(const TriangleMesh& mesh, double length);

    /**
     * @brief Generate face normal line node from mesh
     * @param mesh Triangle mesh
     * @param length Normal line length
     */
    void generateFaceNormalLineNode(const TriangleMesh& mesh, double length);

    /**
     * @brief Generate silhouette edge node
     * @param points Silhouette edge points
     * @param color Edge color
     * @param width Edge line width
     */
    void generateSilhouetteEdgeNode(
        const std::vector<gp_Pnt>& points,
        const Quantity_Color& color = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB),
        double width = 2.0);

    /**
     * @brief Clear silhouette edge node
     */
    void clearSilhouetteEdgeNode();

    /**
     * @brief Clear mesh edge node
     */
    void clearMeshEdgeNode();

    /**
     * @brief Generate intersection nodes visualization
     * @param intersectionPoints Points to visualize
     * @param color Point color
     * @param size Point size
     */
    void generateIntersectionNodesNode(
        const std::vector<gp_Pnt>& intersectionPoints,
        const Quantity_Color& color,
        double size);

    /**
     * @brief Generate LOD-based edge nodes
     * @param lodManager LOD manager containing edge data for different levels
     * @param color Edge color
     * @param width Edge line width
     */
    void generateLODEdgeNodes(
        EdgeLODManager* lodManager,
        const Quantity_Color& color = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB),
        double width = 1.0);

    /**
     * @brief Update edge display based on current LOD level
     * @param lodManager LOD manager
     * @param color Edge color
     * @param width Edge width
     */
    void updateLODLevel(EdgeLODManager* lodManager, const Quantity_Color& color = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB), double width = 1.0);

    /**
     * @brief Enable/disable GPU-accelerated edge rendering
     * @param enabled True to use GPU acceleration
     */
    void setGPUAccelerationEnabled(bool enabled);

    /**
     * @brief Check if GPU acceleration is enabled
     */
    bool isGPUAccelerationEnabled() const { return m_gpuAccelerationEnabled; }

    /**
     * @brief Generate GPU-accelerated mesh edge node
     * @param mesh Triangle mesh
     * @param color Edge color
     * @param width Edge line width
     */
    void generateGPUMeshEdgeNode(
        const TriangleMesh& mesh,
        const Quantity_Color& color = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB),
        double width = 1.0);

    /**
     * @brief Get edge node by type
     * @param type Edge type
     * @return Coin3D separator node for the edge type
     */
    SoSeparator* getEdgeNode(EdgeType type);

    /**
     * @brief Apply appearance (color, width, style) to edge node
     * @param type Edge type
     * @param color Edge color
     * @param width Edge line width
     * @param style Line style (0=solid, 1=dashed, etc.)
     */
    void applyAppearanceToEdgeNode(
        EdgeType type,
        const Quantity_Color& color,
        double width,
        int style = 0);

    /**
     * @brief Update edge display in parent node
     * @param parentNode Parent separator to attach/detach edge nodes
     */
    void updateEdgeDisplay(
        SoSeparator* parentNode,
        const EdgeDisplayFlags& edgeFlags);

private:
    // Coin3D nodes for different edge types
    SoSeparator* originalEdgeNode;
    SoSeparator* featureEdgeNode;
    SoSeparator* meshEdgeNode;
    SoSeparator* highlightEdgeNode;
    SoSeparator* normalLineNode;
    SoSeparator* faceNormalLineNode;
    SoSeparator* silhouetteEdgeNode;
    SoSeparator* intersectionNodesNode;

    // GPU-accelerated rendering
    GPUEdgeRenderer* m_gpuRenderer;
    bool m_gpuAccelerationEnabled;
    SoSeparator* m_gpuMeshEdgeNode;

    mutable std::mutex m_nodeMutex;

    // Helper methods
    SoSeparator* createLineNode(
        const std::vector<gp_Pnt>& points,
        const Quantity_Color& color,
        double width);
};
