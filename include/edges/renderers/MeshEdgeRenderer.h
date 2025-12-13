#pragma once

#include "BaseEdgeRenderer.h"
#include <Inventor/nodes/SoSeparator.h>

// Forward declaration
struct TriangleMesh;

/**
 * @brief Mesh edge renderer
 * 
 * Specialized renderer for mesh-derived edges
 * with GPU acceleration support
 */
class MeshEdgeRenderer : public BaseEdgeRenderer {
public:
    MeshEdgeRenderer();
    ~MeshEdgeRenderer() override;
    
    // BaseEdgeRenderer interface
    SoSeparator* generateNode(
        const std::vector<gp_Pnt>& points,
        const Quantity_Color& color = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB),
        double width = 1.0,
        int style = 0) override;
    
    void updateAppearance(
        SoSeparator* node,
        const Quantity_Color& color,
        double width,
        int style = 0) override;
    
    const char* getName() const override { return "MeshEdgeRenderer"; }
    
    /**
     * @brief Generate normal line node from mesh
     * @param mesh Triangle mesh
     * @param length Normal line length
     * @param color Normal line color
     * @return Coin3D separator node
     */
    SoSeparator* generateNormalLineNode(
        const TriangleMesh& mesh,
        double length,
        const Quantity_Color& color = Quantity_Color(0.0, 1.0, 0.0, Quantity_TOC_RGB));
    
    /**
     * @brief Generate face normal line node from mesh
     * @param mesh Triangle mesh
     * @param length Normal line length
     * @param color Normal line color
     * @return Coin3D separator node
     */
    SoSeparator* generateFaceNormalLineNode(
        const TriangleMesh& mesh,
        double length,
        const Quantity_Color& color = Quantity_Color(1.0, 1.0, 0.0, Quantity_TOC_RGB));
    
    /**
     * @brief Clear mesh edge node (for regeneration)
     */
    void clearMeshEdgeNode();
    
    /**
     * @brief Get current mesh edge node
     */
    SoSeparator* getMeshEdgeNode() const { return meshEdgeNode; }
    
    /**
     * @brief Get current normal line node
     */
    SoSeparator* getNormalLineNode() const { return normalLineNode; }
    
    /**
     * @brief Get current face normal line node
     */
    SoSeparator* getFaceNormalLineNode() const { return faceNormalLineNode; }
    
    /**
     * @brief Generate mesh edge node directly from mesh (with GPU acceleration if available)
     * @param mesh Triangle mesh
     * @param color Edge color
     * @param width Edge width
     * @return Coin3D separator node
     */
    SoSeparator* generateNodeFromMesh(
        const TriangleMesh& mesh,
        const Quantity_Color& color = Quantity_Color(0.0, 0.0, 0.0, Quantity_TOC_RGB),
        double width = 1.0);
    
private:
    SoSeparator* meshEdgeNode;
    SoSeparator* normalLineNode;
    SoSeparator* faceNormalLineNode;
    
    class GPUEdgeRenderer* m_gpuRenderer;
    SoSeparator* m_gpuMeshEdgeNode;
    bool m_gpuAccelerationEnabled;
};

