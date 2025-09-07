#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif
#include "viewer/HoverSilhouetteManager.h"

#include "SceneManager.h"
#include "Canvas.h"
#include "viewer/PickingService.h"
#include "DynamicSilhouetteRenderer.h"
#include "OCCGeometry.h"

#include <Inventor/nodes/SoSeparator.h>

HoverSilhouetteManager::HoverSilhouetteManager(SceneManager* sceneManager,
	SoSeparator* occRoot,
	PickingService* pickingService)
	: m_sceneManager(sceneManager), m_occRoot(occRoot), m_pickingService(pickingService) {
}

HoverSilhouetteManager::~HoverSilhouetteManager() = default;

void HoverSilhouetteManager::disableAll() {
	for (auto& kv : m_silhouetteRenderers) {
		kv.second->setEnabled(false);
	}
}

void HoverSilhouetteManager::setHoveredSilhouette(std::shared_ptr<OCCGeometry> geometry) {
	disableAll();
	if (!geometry) return;

	std::string name = geometry->getName();
	if (m_silhouetteRenderers.find(name) == m_silhouetteRenderers.end()) {
		m_silhouetteRenderers[name] = std::make_unique<DynamicSilhouetteRenderer>(m_occRoot);
		m_silhouetteRenderers[name]->setFastMode(true);
		m_silhouetteRenderers[name]->setShape(geometry->getShape());
	}
	else {
		m_silhouetteRenderers[name]->setShape(geometry->getShape());
	}
	if (SoSeparator* geomSep = geometry->getCoinNode()) {
		SoSeparator* silhouetteNode = m_silhouetteRenderers[name]->getSilhouetteNode();
		bool alreadyChild = false;
		for (int i = 0; i < geomSep->getNumChildren(); ++i) {
			if (geomSep->getChild(i) == silhouetteNode) { alreadyChild = true; break; }
		}
		if (!alreadyChild) geomSep->addChild(silhouetteNode);
	}
	m_silhouetteRenderers[name]->setEnabled(true);
}

void HoverSilhouetteManager::updateHoverSilhouetteAt(const wxPoint& screenPos) {
	if (!m_pickingService) return;
	auto g = m_pickingService->pickGeometryAtScreen(screenPos);
	if (g.get() == m_lastHoverGeometry.lock().get()) return;
	m_lastHoverGeometry = g;
	setHoveredSilhouette(g);
	if (m_sceneManager && m_sceneManager->getCanvas()) m_sceneManager->getCanvas()->Refresh(false);
}