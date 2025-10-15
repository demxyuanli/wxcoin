#pragma once

#include <OpenCASCADE/Quantity_Color.hxx>
#include "config/RenderingConfig.h"
#include "EdgeTypes.h"

// Forward declarations
class EdgeComponent;

/**
 * @brief Geometry display settings
 * 
 * Manages display modes, edges, vertices, wireframe, and face visibility
 */
class OCCGeometryDisplay {
public:
    OCCGeometryDisplay();
    virtual ~OCCGeometryDisplay() = default;

    // Display mode
    RenderingConfig::DisplayMode getDisplayMode() const { return m_displayMode; }
    virtual void setDisplayMode(RenderingConfig::DisplayMode mode);

    // Edge display
    bool isShowEdgesEnabled() const { return m_showEdges; }
    virtual void setShowEdges(bool enabled);

    double getEdgeWidth() const { return m_edgeWidth; }
    virtual void setEdgeWidth(double width);

    Quantity_Color getEdgeColor() const { return m_edgeColor; }
    virtual void setEdgeColor(const Quantity_Color& color);

    // Vertex display
    bool isShowVerticesEnabled() const { return m_showVertices; }
    virtual void setShowVertices(bool enabled);

    double getVertexSize() const { return m_vertexSize; }
    virtual void setVertexSize(double size);

    Quantity_Color getVertexColor() const { return m_vertexColor; }
    virtual void setVertexColor(const Quantity_Color& color);

    // Wireframe mode
    bool isWireframeMode() const { return m_wireframeMode; }
    virtual void setWireframeMode(bool wireframe);

    double getWireframeWidth() const { return m_wireframeWidth; }
    virtual void setWireframeWidth(double width);

    Quantity_Color getWireframeColor() const { return m_wireframeColor; }
    virtual void setWireframeColor(const Quantity_Color& color);

    bool isShowWireframe() const { return m_showWireframe; }
    virtual void setShowWireframe(bool enabled);

    // Face visibility
    void setFaceDisplay(bool enable);
    void setFacesVisible(bool visible);
    bool isFacesVisible() const { return m_facesVisible; }

    // Shading settings
    bool isSmoothNormalsEnabled() const { return m_smoothNormals; }
    virtual void setSmoothNormals(bool enabled);

    double getPointSize() const { return m_pointSize; }
    virtual void setPointSize(double size);

    // Display control helpers
    void setWireframeOverlay(bool enable);
    void setEdgeDisplay(bool enable);
    void setFeatureEdgeDisplay(bool enable);
    void setNormalDisplay(bool enable);

protected:
    RenderingConfig::DisplayMode m_displayMode;
    
    // Edge settings
    bool m_showEdges;
    double m_edgeWidth;
    Quantity_Color m_edgeColor;

    // Vertex settings
    bool m_showVertices;
    double m_vertexSize;
    Quantity_Color m_vertexColor;

    // Wireframe settings
    bool m_wireframeMode;
    double m_wireframeWidth;
    Quantity_Color m_wireframeColor;
    bool m_showWireframe;

    // Face visibility
    bool m_facesVisible;

    // Shading settings
    bool m_smoothNormals;
    double m_pointSize;
};
