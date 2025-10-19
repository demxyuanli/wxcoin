#include "geometry/OCCGeometryDisplay.h"
#include "config/RenderingConfig.h"
#include "logger/Logger.h"

OCCGeometryDisplay::OCCGeometryDisplay()
    : m_displayMode(RenderingConfig::DisplayMode::Solid)
    , m_showEdges(false)
    , m_edgeWidth(1.0)
    , m_edgeColor(0.0, 0.0, 0.0, Quantity_TOC_RGB)
    , m_showVertices(false)
    , m_vertexSize(3.0)
    , m_vertexColor(1.0, 0.0, 0.0, Quantity_TOC_RGB)
    , m_wireframeMode(false)
    , m_wireframeWidth(1.0)
    , m_wireframeColor(0.0, 0.0, 0.0, Quantity_TOC_RGB)
    , m_showWireframe(false)
    , m_facesVisible(true)
    , m_smoothNormals(false)
    , m_pointSize(1.0)
{
}

void OCCGeometryDisplay::setDisplayMode(RenderingConfig::DisplayMode mode)
{
    m_displayMode = mode;
}

void OCCGeometryDisplay::setShowEdges(bool enabled)
{
    m_showEdges = enabled;
}

void OCCGeometryDisplay::setEdgeWidth(double width)
{
    if (width < 0.1) width = 0.1;
    if (width > 10.0) width = 10.0;
    m_edgeWidth = width;
}

void OCCGeometryDisplay::setEdgeColor(const Quantity_Color& color)
{
    m_edgeColor = color;
}

void OCCGeometryDisplay::setShowVertices(bool enabled)
{
    m_showVertices = enabled;
}

void OCCGeometryDisplay::setVertexSize(double size)
{
    if (size < 1.0) size = 1.0;
    if (size > 20.0) size = 20.0;
    m_vertexSize = size;
}

void OCCGeometryDisplay::setVertexColor(const Quantity_Color& color)
{
    m_vertexColor = color;
}

void OCCGeometryDisplay::setWireframeMode(bool wireframe)
{
    m_wireframeMode = wireframe;
}

void OCCGeometryDisplay::setShowWireframe(bool enabled)
{
    m_showWireframe = enabled;
}

void OCCGeometryDisplay::setSmoothNormals(bool enabled)
{
    m_smoothNormals = enabled;
}

void OCCGeometryDisplay::setWireframeWidth(double width)
{
    if (width < 0.1) width = 0.1;
    if (width > 10.0) width = 10.0;
    m_wireframeWidth = width;
}

void OCCGeometryDisplay::setWireframeColor(const Quantity_Color& color)
{
    m_wireframeColor = color;
}

void OCCGeometryDisplay::setPointSize(double size)
{
    if (size < 1.0) size = 1.0;
    if (size > 20.0) size = 20.0;
    m_pointSize = size;
}

void OCCGeometryDisplay::setFaceDisplay(bool enable)
{
    m_facesVisible = enable;
}

void OCCGeometryDisplay::setFacesVisible(bool visible)
{
    if (m_facesVisible != visible) {
        m_facesVisible = visible;
        // Trigger re-render when faces visibility changes
        // This will be handled by the geometry's buildCoinRepresentation
    }
}

void OCCGeometryDisplay::setWireframeOverlay(bool enable)
{
    // Wireframe overlay means showing wireframe on top of shaded
    if (enable) {
        m_showWireframe = true;
    } else {
        m_showWireframe = false;
    }
}

void OCCGeometryDisplay::setEdgeDisplay(bool enable)
{
    m_showEdges = enable;
}

void OCCGeometryDisplay::setFeatureEdgeDisplay(bool enable)
{
    // This would be handled by edge component
}

void OCCGeometryDisplay::setNormalDisplay(bool enable)
{
    // This would be handled by normal visualization
}
