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
}

void RenderModeManager::setWireframeMode(bool wireframe, std::vector<std::shared_ptr<OCCGeometry>>& geometries, 
                                         EdgeDisplayManager* edgeDisplayManager, const MeshParameters& meshParams)
{
    m_wireframeMode = wireframe;
    applyWireframeToAllGeometries(geometries);
    
    // In wireframe mode: enable originalEdges only, disable meshEdges
    // Note: wireFrame = originalEdges (unified naming convention)
    // Wireframe mode shows only geometric original edges, not mesh edges or faces
    if (edgeDisplayManager) {
        if (wireframe) {
            edgeDisplayManager->setShowOriginalEdges(true, meshParams);
            edgeDisplayManager->setShowMeshEdges(false, meshParams);
        }
    }
}

bool RenderModeManager::isWireframeMode() const
{
    return m_wireframeMode;
}

void RenderModeManager::setShadingMode(bool shading)
{
    m_shadingMode = shading;
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

}

bool RenderModeManager::isShowEdges() const
{
    return m_showEdges;
}

void RenderModeManager::setAntiAliasing(bool enabled)
{
    m_antiAliasing = enabled;
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
            // In wireframe mode: hide faces, show only originalEdges (wireFrame = originalEdges)
            // This ensures wireframe mode displays only geometric original edges, not mesh edges or faces
            geometry->setFacesVisible(!m_wireframeMode);
        }
    }
}

void RenderModeManager::applyShadingToAllGeometries(std::vector<std::shared_ptr<OCCGeometry>>& geometries)
{
    // Implementation for shading mode application
    // This might involve updating material properties or shader states
}

void RenderModeManager::updateRenderingToolkitConfiguration(bool showEdges)
{
    // Update edge settings in rendering toolkit
    auto& edgeConfig = EdgeSettingsConfig::getInstance();
    edgeConfig.setGlobalShowEdges(showEdges);
}
