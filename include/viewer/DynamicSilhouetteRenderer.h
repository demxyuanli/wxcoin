#pragma once

#include <OpenCASCADE/TopoDS_Shape.hxx>

// Forward declarations
class SoSeparator;

/**
 * @brief Dynamic silhouette renderer for individual shapes
 * 
 * Renders silhouette edges for OpenCASCADE shapes using Coin3D line rendering.
 * Provides fast outline rendering for individual geometries with optional
 * fast mode for real-time hover effects.
 */
class DynamicSilhouetteRenderer {
public:
    /**
     * @brief Constructor
     * @param parent Parent node to attach silhouette rendering
     */
    explicit DynamicSilhouetteRenderer(SoSeparator* parent);
    
    /**
     * @brief Destructor
     */
    ~DynamicSilhouetteRenderer();

    /**
     * @brief Set the shape to render silhouette for
     * @param shape OpenCASCADE shape to process
     */
    void setShape(const TopoDS_Shape& shape);

    /**
     * @brief Enable or disable silhouette rendering
     * @param enabled True to enable rendering
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Check if silhouette rendering is enabled
     * @return True if enabled
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief Enable fast rendering mode for real-time effects
     * @param fastMode True to enable fast mode (lower quality, higher performance)
     */
    void setFastMode(bool fastMode);
    
    /**
     * @brief Check if fast mode is enabled
     * @return True if fast mode is enabled
     */
    bool isFastMode() const { return m_fastMode; }

    /**
     * @brief Get the silhouette scene node
     * @return Coin3D separator containing silhouette geometry
     */
    SoSeparator* getSilhouetteNode() const { return m_silhouetteNode; }

    /**
     * @brief Set silhouette line color
     * @param r Red component (0.0-1.0)
     * @param g Green component (0.0-1.0)
     * @param b Blue component (0.0-1.0)
     */
    void setColor(float r, float g, float b);

    /**
     * @brief Set silhouette line width
     * @param width Line width in pixels
     */
    void setLineWidth(float width);

private:
    void buildSilhouette();
    void clearSilhouette();
    void extractSilhouetteEdges(const TopoDS_Shape& shape);

    SoSeparator* m_parent;
    SoSeparator* m_silhouetteNode;
    TopoDS_Shape m_shape;
    
    bool m_enabled = false;
    bool m_fastMode = false;
    float m_lineWidth = 2.0f;
    float m_colorR = 1.0f;
    float m_colorG = 0.5f;
    float m_colorB = 0.0f;
};