#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "viewer/ViewOperationsService.h"
#include "SceneManager.h"
#include "viewer/SelectionManager.h"
#include "viewer/ViewUpdateService.h"
#include "viewer/ObjectTreeSync.h"
#include "OCCGeometry.h"
#include "Canvas.h"
#include "ViewRefreshManager.h"
#include "logger/Logger.h"

ViewOperationsService::ViewOperationsService()
    : m_batchMode(false)
{
}

ViewOperationsService::~ViewOperationsService()
{
}

void ViewOperationsService::fitAll(SceneManager* sceneManager, ViewUpdateService* viewUpdater)
{
    if (!sceneManager) {
        LOG_ERR_S("SceneManager is null, cannot perform fitAll");
        return;
    }

    // Update scene bounds first
    updateSceneBounds(sceneManager);

    // Reset view to fit all geometries
    resetView(sceneManager, viewUpdater);

    // Request view refresh
    if (sceneManager->getCanvas()) {
        auto* canvas = sceneManager->getCanvas();
        if (canvas) {
            refreshCanvas(sceneManager);
        }
    }

}

void ViewOperationsService::hideAll(SelectionManager* selectionManager)
{
    if (selectionManager) {
        selectionManager->hideAll();
    }
}

void ViewOperationsService::showAll(SelectionManager* selectionManager)
{
    if (selectionManager) {
        selectionManager->showAll();
    }
}

void ViewOperationsService::selectAll(SelectionManager* selectionManager)
{
    if (selectionManager) {
        selectionManager->selectAll();
    }
}

void ViewOperationsService::deselectAll(SelectionManager* selectionManager)
{
    if (selectionManager) {
        selectionManager->deselectAll();
    }
}

void ViewOperationsService::setAllColor(const Quantity_Color& color, const std::vector<std::shared_ptr<OCCGeometry>>& geometries)
{
    for (auto& geometry : geometries) {
        if (geometry) {
            geometry->setColor(color);
        }
    }
}

void ViewOperationsService::requestViewRefresh(SceneManager* sceneManager, ViewUpdateService* viewUpdater)
{
    if (sceneManager && sceneManager->getCanvas()) {
        auto* canvas = sceneManager->getCanvas();
        if (canvas && canvas->getRefreshManager()) {
            canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::MATERIAL_CHANGED, true);
        }
    }
}

void ViewOperationsService::updateObjectTreeDeferred(ObjectTreeSync* objectTreeSync)
{
    if (!objectTreeSync) {
        return;
    }

    objectTreeSync->processDeferred();
}

void ViewOperationsService::updateSceneBounds(SceneManager* sceneManager)
{
    if (sceneManager) {
        sceneManager->updateSceneBounds();
    }
}

void ViewOperationsService::resetView(SceneManager* sceneManager, ViewUpdateService* viewUpdater)
{
    if (sceneManager) {
        sceneManager->resetView();
    }

    if (viewUpdater) {
        viewUpdater->requestRefresh(static_cast<int>(IViewRefresher::Reason::CAMERA_MOVED), true);
    }
}

void ViewOperationsService::refreshCanvas(SceneManager* sceneManager)
{
    if (sceneManager && sceneManager->getCanvas()) {
        sceneManager->getCanvas()->Refresh();
    }
}
