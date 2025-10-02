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
	: m_sceneManager(sceneManager), m_pendingQueue(pendingQueue) {
}

void ObjectTreeSync::addGeometry(std::shared_ptr<OCCGeometry> geometry, bool batchMode) {
	if (!geometry) return;
	if (!m_sceneManager || !m_sceneManager->getCanvas()) return;
	Canvas* canvas = m_sceneManager->getCanvas();
	if (batchMode) {
		if (m_pendingQueue) {
			LOG_INF_S("ObjectTreeSync: Queuing geometry '" + geometry->getName() + "' for batch processing");
			m_pendingQueue->push_back(geometry);
		}
		return;
	}
	if (canvas && canvas->getObjectTreePanel()) {
		// Use filename-based organization if filename is available
		std::string fileName = geometry->getFileName();
		LOG_INF_S("ObjectTreeSync: Adding geometry '" + geometry->getName() + "' with filename '" + fileName + "'");
		if (!fileName.empty()) {
			canvas->getObjectTreePanel()->addOCCGeometryFromFile(fileName, geometry);
		} else {
			// Fallback to old method for geometries without filename
			LOG_WRN_S("ObjectTreeSync: Geometry '" + geometry->getName() + "' has no filename, using old method");
			canvas->getObjectTreePanel()->addOCCGeometry(geometry);
		}
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
	if (!m_pendingQueue || m_pendingQueue->empty()) {
		LOG_INF_S("ObjectTreeSync: No pending geometries to process");
		return;
	}
	if (!m_sceneManager || !m_sceneManager->getCanvas()) return;
	Canvas* canvas = m_sceneManager->getCanvas();
	if (!canvas || !canvas->getObjectTreePanel()) return;

	LOG_INF_S("ObjectTreeSync: Processing " + std::to_string(m_pendingQueue->size()) + " deferred geometries");

	// First pass: Add all geometries to tree data without refreshing display
	for (const auto& g : *m_pendingQueue) {
		if (g) {
			// Use filename-based organization if filename is available
			std::string fileName = g->getFileName();
			LOG_INF_S("ObjectTreeSync: Processing deferred geometry '" + g->getName() + "' with filename '" + fileName + "'");
			if (!fileName.empty()) {
				// Add to tree data without immediate refresh (false = batch mode)
				canvas->getObjectTreePanel()->addOCCGeometryFromFile(fileName, g, false);
			} else {
				// For geometries without filename, we need to handle them differently
				// since addOCCGeometry doesn't have a batch mode parameter
				// We'll add them to tree data directly
				LOG_INF_S("ObjectTreeSync: Adding geometry '" + g->getName() + "' without filename to tree data");
				canvas->getObjectTreePanel()->addOCCGeometry(g);
			}
		}
	}

	// Clear the pending queue
	m_pendingQueue->clear();

	// Now refresh the tree display once for all geometries
	canvas->getObjectTreePanel()->refreshTreeDisplay();
	LOG_INF_S("ObjectTreeSync: Completed processing deferred geometries");
}