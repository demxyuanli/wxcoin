#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif
#include "viewer/ObjectTreeSync.h"

#include "SceneManager.h"
#include "Canvas.h"
#include "ObjectTreePanel.h"
#include "logger/Logger.h"

ObjectTreeSync::ObjectTreeSync(SceneManager* sceneManager,
                               std::vector<std::shared_ptr<OCCGeometry>>* pendingQueue)
    : m_sceneManager(sceneManager), m_pendingQueue(pendingQueue) {}

void ObjectTreeSync::addGeometry(std::shared_ptr<OCCGeometry> geometry, bool batchMode) {
    if (!geometry) return;
    if (!m_sceneManager || !m_sceneManager->getCanvas()) return;
    Canvas* canvas = m_sceneManager->getCanvas();
    if (batchMode) {
        if (m_pendingQueue) m_pendingQueue->push_back(geometry);
        return;
    }
    if (canvas && canvas->getObjectTreePanel()) {
        canvas->getObjectTreePanel()->addOCCGeometry(geometry);
    }
}

void ObjectTreeSync::removeGeometry(std::shared_ptr<OCCGeometry> geometry) {
    if (!geometry) return;
    if (!m_sceneManager || !m_sceneManager->getCanvas()) return;
    Canvas* canvas = m_sceneManager->getCanvas();
    if (canvas && canvas->getObjectTreePanel()) {
        canvas->getObjectTreePanel()->removeOCCGeometry(geometry);
    }
}

void ObjectTreeSync::processDeferred() {
    if (!m_pendingQueue || m_pendingQueue->empty()) return;
    if (!m_sceneManager || !m_sceneManager->getCanvas()) return;
    Canvas* canvas = m_sceneManager->getCanvas();
    if (!canvas || !canvas->getObjectTreePanel()) return;
    for (const auto& g : *m_pendingQueue) {
        if (g) canvas->getObjectTreePanel()->addOCCGeometry(g);
    }
    m_pendingQueue->clear();
}


