#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef min
#undef max
#include "viewer/ViewportController.h"
#include "SceneManager.h"
#include "Canvas.h"
#include "logger/Logger.h"
#include "ViewRefreshManager.h"
#include <Inventor/nodes/SoCamera.h>

ViewportController::ViewportController(SceneManager* sceneManager)
    : m_sceneManager(sceneManager)
    , m_preserveViewOnAdd(true)
{
}

void ViewportController::fitAll()
{
    if (!m_sceneManager || !m_sceneManager->getCanvas()) {
        LOG_WRN_S("Cannot fitAll: SceneManager or Canvas not available");
        return;
    }
    
    m_sceneManager->getCanvas()->resetView();
    LOG_INF_S("Viewport: fitAll executed");
}

void ViewportController::fitGeometry(const std::string& name)
{
    // TODO: Implement fit specific geometry
    // This requires bounding box calculation and camera adjustment
    LOG_INF_S("Viewport: fitGeometry for " + name + " (framework)");
    
    // For now, just fit all
    fitAll();
}

void ViewportController::requestViewRefresh()
{
    if (!m_sceneManager || !m_sceneManager->getCanvas()) {
        return;
    }
    
    auto* canvas = m_sceneManager->getCanvas();
    if (canvas && canvas->getRefreshManager()) {
        canvas->getRefreshManager()->requestRefresh();
    }
    LOG_INF_S("View refresh requested");
}

gp_Pnt ViewportController::getCameraPosition() const
{
    SoCamera* camera = getCamera();
    if (!camera) {
        return gp_Pnt(0, 0, 0);
    }
    
    SbVec3f pos = camera->position.getValue();
    return gp_Pnt(pos[0], pos[1], pos[2]);
}

SoCamera* ViewportController::getCamera() const
{
    if (!m_sceneManager || !m_sceneManager->getCanvas()) {
        return nullptr;
    }
    
    return m_sceneManager->getCanvas()->getCamera();
}
