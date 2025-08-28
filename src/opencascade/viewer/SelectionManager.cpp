#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif
#include "viewer/SelectionManager.h"

#include "OCCGeometry.h"
#include "SceneManager.h"
#include "Canvas.h"
#include "ObjectTreePanel.h"
#include "ViewRefreshManager.h"
#include "logger/Logger.h"

SelectionManager::SelectionManager(SceneManager* sceneManager,
	std::vector<std::shared_ptr<OCCGeometry>>* allGeometries,
	std::vector<std::shared_ptr<OCCGeometry>>* selectedGeometries)
	: m_sceneManager(sceneManager),
	m_allGeometries(allGeometries),
	m_selectedGeometries(selectedGeometries) {
}

std::shared_ptr<OCCGeometry> SelectionManager::findGeometry(const std::string& name) {
	if (!m_allGeometries) return nullptr;
	for (auto& g : *m_allGeometries) {
		if (g && g->getName() == name) return g;
	}
	return nullptr;
}

void SelectionManager::setGeometryVisible(const std::string& name, bool visible) {
	auto geometry = findGeometry(name);
	if (!geometry) {
		LOG_WRN_S("Geometry not found for visibility change: " + name);
		return;
	}
	geometry->setVisible(visible);
	requestRefreshSelectionChanged();
}

void SelectionManager::setGeometrySelected(const std::string& name, bool selected) {
	auto geometry = findGeometry(name);
	if (!geometry || !m_selectedGeometries) return;

	geometry->setSelected(selected);
	if (selected) {
		if (std::find(m_selectedGeometries->begin(), m_selectedGeometries->end(), geometry) == m_selectedGeometries->end()) {
			m_selectedGeometries->push_back(geometry);
		}
	}
	else {
		auto it = std::remove(m_selectedGeometries->begin(), m_selectedGeometries->end(), geometry);
		if (it != m_selectedGeometries->end()) {
			m_selectedGeometries->erase(it, m_selectedGeometries->end());
		}
	}
	onSelectionChanged();
}

void SelectionManager::setGeometryColor(const std::string& name, const Quantity_Color& color) {
	if (auto g = findGeometry(name)) g->setColor(color);
}

void SelectionManager::setGeometryTransparency(const std::string& name, double transparency) {
	if (auto g = findGeometry(name)) {
		g->setTransparency(transparency);
		requestRefreshMaterialChanged();
	}
}

void SelectionManager::hideAll() {
	if (!m_allGeometries) return;
	for (auto& g : *m_allGeometries) if (g) g->setVisible(false);
	requestRefreshSelectionChanged();
}

void SelectionManager::showAll() {
	if (!m_allGeometries) return;
	for (auto& g : *m_allGeometries) if (g) g->setVisible(true);
	requestRefreshSelectionChanged();
}

void SelectionManager::selectAll() {
	if (!m_allGeometries || !m_selectedGeometries) return;
	m_selectedGeometries->clear();
	for (auto& g : *m_allGeometries) {
		if (!g) continue;
		g->setSelected(true);
		m_selectedGeometries->push_back(g);
	}
	onSelectionChanged();
}

void SelectionManager::deselectAll() {
	if (!m_selectedGeometries) return;
	for (auto& g : *m_selectedGeometries) if (g) g->setSelected(false);
	m_selectedGeometries->clear();
	onSelectionChanged();
}

void SelectionManager::onSelectionChanged() {
	if (!m_sceneManager || !m_sceneManager->getCanvas()) return;
	Canvas* canvas = m_sceneManager->getCanvas();
	if (canvas && canvas->getObjectTreePanel()) {
		canvas->getObjectTreePanel()->updateTreeSelectionFromViewer();
	}
	if (canvas && canvas->getRefreshManager()) {
		canvas->getRefreshManager()->requestRefresh(IViewRefresher::Reason::SELECTION_CHANGED, true);
	}
}

void SelectionManager::requestRefreshSelectionChanged() {
	if (!m_sceneManager || !m_sceneManager->getCanvas()) return;
	if (auto* refresher = m_sceneManager->getCanvas()->getRefreshManager()) {
		refresher->requestRefresh(IViewRefresher::Reason::GEOMETRY_CHANGED, true);
	}
	// Ensure repaint
	m_sceneManager->getCanvas()->Refresh(false);
}

void SelectionManager::requestRefreshMaterialChanged() {
	if (!m_sceneManager || !m_sceneManager->getCanvas()) return;
	if (auto* refresher = m_sceneManager->getCanvas()->getRefreshManager()) {
		refresher->requestRefresh(ViewRefreshManager::RefreshReason::MATERIAL_CHANGED, true);
	}
	// Ensure repaint
	m_sceneManager->getCanvas()->Refresh(false);
}