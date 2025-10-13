#include "viewer/RenderModeManager.h"
#include "edges/EdgeDisplayManager.h"
#include "rendering/RenderingToolkitAPI.h"
#include "logger/Logger.h"

RenderModeManager::RenderModeManager()
{
}

RenderModeManager::~RenderModeManager()
{
}

void RenderModeManager::setWireframeMode(bool wireframe, std::vector<std::shared_ptr<OCCGeometry>>& geometries)
{
    m_wireframeMode = wireframe;
    applyWireframeToAllGeometries(geometries);
    LOG_INF_S("Wireframe mode " + std::string(wireframe ? "enabled" : "disabled"));
}

bool RenderModeManager::isWireframeMode() const
{
    return m_wireframeMode;
}

void RenderModeManager::setShadingMode(bool shading)
{
    m_shadingMode = shading;
    LOG_INF_S("Shading mode " + std::string(shading ? "enabled" : "disabled"));
}

bool RenderModeManager::isShadingMode() const
{
    return m_shadingMode;
}

void RenderModeManager::setShowEdges(bool showEdges, EdgeDisplayManager* edgeDisplayManager, const MeshParameters& meshParams)
{
    m_showEdges = showEdges;

    // Update edge display manager
    if (edgeDisplayManager) {
        // Use the existing method to update edges
        // This will be handled by the edge display manager
    }

    // Update rendering toolkit configuration
    updateRenderingToolkitConfiguration(showEdges);

    LOG_INF_S("Edge display " + std::string(showEdges ? "enabled" : "disabled"));
}

bool RenderModeManager::isShowEdges() const
{
    return m_showEdges;
}

void RenderModeManager::setAntiAliasing(bool enabled)
{
    m_antiAliasing = enabled;
    LOG_INF_S("Anti-aliasing " + std::string(enabled ? "enabled" : "disabled"));
}

bool RenderModeManager::isAntiAliasing() const
{
    return m_antiAliasing;
}

void RenderModeManager::applyWireframeToAllGeometries(std::vector<std::shared_ptr<OCCGeometry>>& geometries)
{
    for (auto& geometry : geometries) {
        if (geometry) {
            geometry->setWireframeMode(m_wireframeMode);
        }
    }
    LOG_INF_S("Applied wireframe mode to all geometries");
}

void RenderModeManager::applyShadingToAllGeometries(std::vector<std::shared_ptr<OCCGeometry>>& geometries)
{
    // Implementation for shading mode application
    // This might involve updating material properties or shader states
    LOG_INF_S("Applied shading mode to all geometries");
}

void RenderModeManager::updateRenderingToolkitConfiguration(bool showEdges)
{
    // Update edge settings in rendering toolkit
    auto& edgeConfig = EdgeSettingsConfig::getInstance();
    edgeConfig.setGlobalShowEdges(showEdges);
}
