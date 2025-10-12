#pragma once

#include "BaseEdgeRenderer.h"
#include <Inventor/nodes/SoSeparator.h>

/**
 * @brief Original edge renderer
 * 
 * Specialized renderer for original geometric edges
 * with LOD support and intersection highlighting
 */
class OriginalEdgeRenderer : public BaseEdgeRenderer {
public:
    OriginalEdgeRenderer();
    ~OriginalEdgeRenderer() override;
    
    // BaseEdgeRenderer interface
    SoSeparator* generateNode(
        const std::vector<gp_Pnt>& points,
        const Quantity_Color& color = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB),
        double width = 1.0,
        int style = 0) override;
    
    void updateAppearance(
        SoSeparator* node,
        const Quantity_Color& color,
        double width,
        int style = 0) override;
    
    const char* getName() const override { return "OriginalEdgeRenderer"; }
    
    /**
     * @brief Generate intersection nodes (spheres)
     * @param points Intersection points
     * @param color Node color
     * @param size Node size
     * @return Coin3D separator node
     */
    SoSeparator* generateIntersectionNodes(
        const std::vector<gp_Pnt>& points,
        const Quantity_Color& color,
        double size = 3.0);
    
    /**
     * @brief Update LOD level for current camera distance
     * @param lodManager LOD manager
     * @param color Edge color
     * @param width Edge width
     */
    void updateLODLevel(class EdgeLODManager* lodManager, const Quantity_Color& color, double width);
    
    /**
     * @brief Get current original edge node
     */
    SoSeparator* getOriginalEdgeNode() const { return originalEdgeNode; }
    
    /**
     * @brief Get current intersection nodes
     */
    SoSeparator* getIntersectionNodesNode() const { return intersectionNodesNode; }
    
private:
    SoSeparator* originalEdgeNode;
    SoSeparator* intersectionNodesNode;
};

