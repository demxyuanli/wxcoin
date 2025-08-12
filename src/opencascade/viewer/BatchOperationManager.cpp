#include "viewer/BatchOperationManager.h"

#include "SceneManager.h"
#include "viewer/ObjectTreeSync.h"
#include "viewer/ViewUpdateService.h"

BatchOperationManager::BatchOperationManager(SceneManager* sceneManager,
                                             ObjectTreeSync* objectTree,
                                             ViewUpdateService* viewUpdater)
    : m_sceneManager(sceneManager), m_objectTree(objectTree), m_viewUpdater(viewUpdater) {}

void BatchOperationManager::begin() {
    m_active = true;
    m_needsViewRefresh = false;
}

void BatchOperationManager::end() {
    m_active = false;
    if (m_objectTree) m_objectTree->processDeferred();
    if (m_needsViewRefresh && m_viewUpdater) {
        m_viewUpdater->updateSceneBounds();
        m_viewUpdater->resetView();
        m_viewUpdater->requestGeometryChanged(true);
        m_viewUpdater->refreshCanvas(false);
    }
    m_needsViewRefresh = false;
}


