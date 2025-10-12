#pragma once

#include <vector>
#include <mutex>
#include <OpenCASCADE/gp_Pnt.hxx>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "EdgeTypes.h"

// Forward declarations
class SoSeparator;
class SoMaterial;
class SoDrawStyle;
class SoCoordinate3;
class SoIndexedLineSet;

/**
 * @brief Base class for edge renderers
 * 
 * Defines the common interface for all edge rendering algorithms
 */
class BaseEdgeRenderer {
public:
    BaseEdgeRenderer();
    virtual ~BaseEdgeRenderer() = default;
    
    /**
     * @brief Generate Coin3D node for edge points
     * @param points Edge points
     * @param color Edge color
     * @param width Edge width
     * @param style Line style (solid, dashed, etc.)
     * @return Coin3D separator node
     */
    virtual SoSeparator* generateNode(
        const std::vector<gp_Pnt>& points,
        const Quantity_Color& color = Quantity_Color(1.0, 1.0, 1.0, Quantity_TOC_RGB),
        double width = 1.0,
        int style = 0) = 0;
    
    /**
     * @brief Update existing node appearance
     * @param node Existing Coin3D node
     * @param color New color
     * @param width New width
     * @param style New style
     */
    virtual void updateAppearance(
        SoSeparator* node,
        const Quantity_Color& color,
        double width,
        int style = 0) = 0;
    
    /**
     * @brief Get renderer name for debugging
     * @return Renderer type name
     */
    virtual const char* getName() const = 0;
    
protected:
    /**
     * @brief Create basic line node components
     */
    SoSeparator* createLineNode(
        const std::vector<gp_Pnt>& points,
        const Quantity_Color& color,
        double width,
        int style = 0);
    
    /**
     * @brief Apply material properties
     */
    void applyMaterial(SoMaterial* material, const Quantity_Color& color);
    
    /**
     * @brief Apply line style properties
     */
    void applyLineStyle(SoDrawStyle* drawStyle, double width, int style);
    
    mutable std::mutex m_nodeMutex;
};

