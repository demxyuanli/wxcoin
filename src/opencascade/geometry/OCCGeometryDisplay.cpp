#include "geometry/OCCGeometryDisplay.h"
#include "config/RenderingConfig.h"

OCCGeometryDisplay::OCCGeometryDisplay()
    : m_displayMode(RenderingConfig::DisplayMode::Solid)
    , m_showEdges(false)
    , m_edgeWidth(1.0)
    , m_edgeColor(0.0, 0.0, 0.0, Quantity_TOC_RGB)
    , m_showVertices(false)
    , m_vertexSize(3.0)
    , m_vertexColor(1.0, 0.0, 0.0, Quantity_TOC_RGB)
    , m_showPointView(false)
    , m_showSolidWithPointView(true)
    , m_pointViewSize(3.0)
    , m_pointViewColor(1.0, 0.0, 0.0, Quantity_TOC_RGB)
    , m_pointViewShape(0)
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
    // Check if display mode actually changed to avoid unnecessary updates
    if (m_displayMode == mode) {
        return;
    }
    
    m_displayMode = mode;
    
    // Apply the display mode settings
    switch (mode) {
    case RenderingConfig::DisplayMode::NoShading:
        m_wireframeMode = true;
        m_facesVisible = true;
        m_showPointView = false;
        // NoShading mode will be handled by material settings
        break;
    case RenderingConfig::DisplayMode::Points:
        m_wireframeMode = false;
        m_facesVisible = false;
        m_showPointView = true;
        m_showSolidWithPointView = false; // Only show points
        break;
    case RenderingConfig::DisplayMode::Wireframe:
        m_wireframeMode = true;
        m_facesVisible = true;
        m_showPointView = false;
        break;
    case RenderingConfig::DisplayMode::FlatLines:
        m_wireframeMode = true;
        m_facesVisible = true;
        m_showPointView = false;
        // Note: m_showEdges will be set by setShowEdges() call from DisplaySettings
        break;
    case RenderingConfig::DisplayMode::Solid:
        m_wireframeMode = false;
        m_facesVisible = true;
        m_showPointView = false;
        break;
    case RenderingConfig::DisplayMode::Transparent:
        m_wireframeMode = false;
        m_facesVisible = true;
        m_showPointView = false;
        // Transparency will be handled by material settings
        break;
    case RenderingConfig::DisplayMode::HiddenLine:
        m_wireframeMode = true;
        m_facesVisible = true;
        m_showPointView = false;
        // Hidden line mode will be handled by rendering backend
        break;
    default:
        m_wireframeMode = false;
        m_facesVisible = true;
        m_showPointView = false;
        break;
    }
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

void OCCGeometryDisplay::setShowPointView(bool enabled)
{
    m_showPointView = enabled;
}

void OCCGeometryDisplay::setPointViewSize(double size)
{
    if (size < 0.1) size = 0.1;
    if (size > 20.0) size = 20.0;
    m_pointViewSize = size;
}

void OCCGeometryDisplay::setPointViewColor(const Quantity_Color& color)
{
    m_pointViewColor = color;
}

void OCCGeometryDisplay::setShowSolidWithPointView(bool enabled)
{
    m_showSolidWithPointView = enabled;
}

void OCCGeometryDisplay::setPointViewShape(int shape)
{
    if (shape < 0) shape = 0;
    if (shape > 2) shape = 2; // 0 = square, 1 = circle, 2 = triangle
    m_pointViewShape = shape;
}
