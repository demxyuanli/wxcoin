#pragma once

#include "BaseEdgeRenderer.h"
#include <Inventor/nodes/SoSeparator.h>

/**
 * @brief Feature edge renderer
 * 
 * Specialized renderer for feature edges with
 * enhanced visual properties for CAD visualization
 */
class FeatureEdgeRenderer : public BaseEdgeRenderer {
public:
    FeatureEdgeRenderer();
    ~FeatureEdgeRenderer() override;
    
    // BaseEdgeRenderer interface
    SoSeparator* generateNode(
        const std::vector<gp_Pnt>& points,
        const Quantity_Color& color = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB),
        double width = 2.0,
        int style = 0) override;
    
    void updateAppearance(
        SoSeparator* node,
        const Quantity_Color& color,
        double width,
        int style = 0) override;
    
    const char* getName() const override { return "FeatureEdgeRenderer"; }
    
    /**
     * @brief Generate node with enhanced feature edge styling
     * @param points Edge points
     * @param color Edge color
     * @param width Edge width
     * @param convexColor Color for convex edges
     * @param concaveColor Color for concave edges
     * @return Coin3D separator node
     */
    SoSeparator* generateFeatureNode(
        const std::vector<gp_Pnt>& points,
        const Quantity_Color& color,
        double width,
        const Quantity_Color& convexColor = Quantity_Color(1.0, 0.0, 0.0, Quantity_TOC_RGB),
        const Quantity_Color& concaveColor = Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB));
    
    /**
     * @brief Get current feature edge node
     */
    SoSeparator* getFeatureEdgeNode() const { return featureEdgeNode; }
    
private:
    SoSeparator* featureEdgeNode;
};

