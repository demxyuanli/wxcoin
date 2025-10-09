#include "geometry/OCCGeometryDisplay.h"
#include "config/RenderingConfig.h"
#include "logger/Logger.h"

OCCGeometryDisplay::OCCGeometryDisplay()
    : m_displayMode(RenderingConfig::DisplayMode::Shaded)
    , m_showEdges(false)
    , m_edgeWidth(1.0)
    , m_edgeColor(0.0, 0.0, 0.0, Quantity_TOC_RGB)
    , m_showVertices(false)
    , m_vertexSize(3.0)
    , m_vertexColor(1.0, 0.0, 0.0, Quantity_TOC_RGB)
    , m_wireframeMode(false)
    , m_wireframeWidth(1.0)
    , m_showWireframe(false)
    , m_facesVisible(true)
    , m_smoothNormals(false)
    , m_pointSize(1.0)
{
}

void OCCGeometryDisplay::setDisplayMode(RenderingConfig::DisplayMode mode)
{
    m_displayMode = mode;
    LOG_INF_S("Display mode changed");
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
    LOG_INF_S(std::string("Wireframe mode: ") + (wireframe ? "ON" : "OFF"));
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

void OCCGeometryDisplay::setPointSize(double size)
{
    if (size < 1.0) size = 1.0;
    if (size > 20.0) size = 20.0;
    m_pointSize = size;
}

void OCCGeometryDisplay::setFaceDisplay(bool enable)
{
    m_facesVisible = enable;
    LOG_INF_S(std::string("Face display: ") + (enable ? "ON" : "OFF"));
}

void OCCGeometryDisplay::setFacesVisible(bool visible)
{
    m_facesVisible = visible;
}

void OCCGeometryDisplay::setWireframeOverlay(bool enable)
{
    // Wireframe overlay means showing wireframe on top of shaded
    if (enable) {
        m_showWireframe = true;
        LOG_INF_S("Wireframe overlay enabled");
    } else {
        m_showWireframe = false;
        LOG_INF_S("Wireframe overlay disabled");
    }
}

void OCCGeometryDisplay::setEdgeDisplay(bool enable)
{
    m_showEdges = enable;
}

void OCCGeometryDisplay::setFeatureEdgeDisplay(bool enable)
{
    // This would be handled by edge component
    LOG_INF_S(std::string("Feature edge display: ") + (enable ? "ON" : "OFF"));
}

void OCCGeometryDisplay::setNormalDisplay(bool enable)
{
    // This would be handled by normal visualization
    LOG_INF_S(std::string("Normal display: ") + (enable ? "ON" : "OFF"));
}
