#include "viewer/RenderingController.h"
#include "OCCGeometry.h"
#include "logger/Logger.h"

RenderingController::RenderingController()
    : m_wireframeMode(false)
    , m_showEdges(true)
    , m_antiAliasing(true)
    , m_showNormals(false)
    , m_normalLength(0.5)
    , m_normalConsistencyMode(true)
    , m_normalDebugMode(false)
{
}

void RenderingController::setWireframeMode(bool wireframe)
{
    m_wireframeMode = wireframe;
}

void RenderingController::setShowEdges(bool showEdges)
{
    m_showEdges = showEdges;
}

void RenderingController::setAntiAliasing(bool enabled)
{
    m_antiAliasing = enabled;
}

void RenderingController::setShowNormals(bool showNormals)
{
    m_showNormals = showNormals;
}

void RenderingController::applyRenderingSettings(std::shared_ptr<OCCGeometry> geometry)
{
    if (!geometry) return;
    
    geometry->setWireframeMode(m_wireframeMode);
    geometry->setShowEdges(m_showEdges);
}

void RenderingController::applyRenderingSettingsToAll(const std::vector<std::shared_ptr<OCCGeometry>>& geometries)
{
    for (auto& geometry : geometries) {
        applyRenderingSettings(geometry);
    }
    
}
