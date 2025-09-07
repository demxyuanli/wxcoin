#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "viewer/SelectionOutlineManager.h"

#include "SceneManager.h"
#include "Canvas.h"
#include "OCCGeometry.h"
#include "DynamicSilhouetteRenderer.h"
#include "logger/Logger.h"

#include <Inventor/nodes/SoSeparator.h>

SelectionOutlineManager::SelectionOutlineManager(SceneManager* sceneManager,
	SoSeparator* occRoot,
	std::vector<std::shared_ptr<OCCGeometry>>* selectedGeometries)
	: m_sceneManager(sceneManager), m_occRoot(occRoot), m_selectedGeometries(selectedGeometries) {
}

SelectionOutlineManager::~SelectionOutlineManager() = default;

void SelectionOutlineManager::setEnabled(bool enabled) {
	if (m_enabled == enabled) return;
	m_enabled = enabled;
	LOG_INF((std::string("SelectionOutlineManager setEnabled ") + (enabled ? "true" : "false")).c_str(), "SelectionOutline");
	if (!m_enabled) {
		clearAll();
	}
	else {
		syncToSelection();
	}
}

void SelectionOutlineManager::setStyle(const SelectionOutlineStyle& style) {
	m_style = style;
	for (auto& kv : m_renderersByName) {
		kv.second->setLineWidth(m_style.lineWidth);
		kv.second->setLineColor(m_style.r, m_style.g, m_style.b);
	}
}

void SelectionOutlineManager::syncToSelection() {
	if (!m_enabled) return;
	if (!m_selectedGeometries) return;

	// Disable all first; re-enable for selected ones
	for (auto& kv : m_renderersByName) kv.second->setEnabled(false);

	for (auto& g : *m_selectedGeometries) {
		if (!g) continue;
		const std::string name = g->getName();
		auto it = m_renderersByName.find(name);
		if (it == m_renderersByName.end()) {
			auto renderer = std::make_unique<DynamicSilhouetteRenderer>(m_occRoot);
			renderer->setFastMode(true);
			renderer->setShape(g->getShape());
			renderer->setLineWidth(m_style.lineWidth);
			renderer->setLineColor(m_style.r, m_style.g, m_style.b);
			if (SoSeparator* geomSep = g->getCoinNode()) {
				SoSeparator* node = renderer->getSilhouetteNode();
				if (geomSep->findChild(node) < 0) geomSep->addChild(node);
			}
			renderer->setEnabled(true);
			m_renderersByName.emplace(name, std::move(renderer));
		}
		else {
			it->second->setShape(g->getShape());
			it->second->setLineWidth(m_style.lineWidth);
			it->second->setLineColor(m_style.r, m_style.g, m_style.b);
			it->second->setEnabled(true);
		}
	}

	if (m_sceneManager && m_sceneManager->getCanvas()) m_sceneManager->getCanvas()->Refresh(false);
}

void SelectionOutlineManager::clearAll() {
	for (auto& kv : m_renderersByName) kv.second->setEnabled(false);
}