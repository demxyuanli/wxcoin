#include "viewer/SceneAttachmentService.h"
#include "OCCGeometry.h"
#include "logger/Logger.h"

#include <Inventor/nodes/SoSeparator.h>

SceneAttachmentService::SceneAttachmentService(
	SoSeparator* occRoot,
	std::unordered_map<SoSeparator*, std::shared_ptr<OCCGeometry>>* nodeToGeom)
	: m_occRoot(occRoot), m_nodeToGeom(nodeToGeom) {
}

void SceneAttachmentService::attach(std::shared_ptr<OCCGeometry> geometry) {
	if (!m_occRoot || !geometry) return;
	SoSeparator* coin = geometry->getCoinNode();
	if (!coin) return;
	
	int existingIndex = m_occRoot->findChild(coin);
	if (existingIndex < 0) {
		m_occRoot->addChild(coin);
		LOG_DBG_S("SceneAttachmentService: attached geometry '" + geometry->getName() + 
				 "' to scene (children: " + std::to_string(m_occRoot->getNumChildren()) + ")");
	} else {
		LOG_DBG_S("SceneAttachmentService: geometry '" + geometry->getName() + 
				 "' already attached at index " + std::to_string(existingIndex));
	}
	
	if (m_nodeToGeom) (*m_nodeToGeom)[coin] = geometry;
}

void SceneAttachmentService::detach(std::shared_ptr<OCCGeometry> geometry) {
	if (!m_occRoot || !geometry) return;
	SoSeparator* coin = geometry->getCoinNode();
	if (!coin) return;
	int idx = m_occRoot->findChild(coin);
	if (idx >= 0) m_occRoot->removeChild(idx);
	if (m_nodeToGeom) m_nodeToGeom->erase(coin);
}

void SceneAttachmentService::detachAll() {
	if (!m_occRoot) return;
	m_occRoot->removeAllChildren();
	if (m_nodeToGeom) m_nodeToGeom->clear();
}