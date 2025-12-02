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

// Coin3D includes for scene graph management
#include <Inventor/nodes/SoSeparator.h>

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
		LOG_WRN_S("SelectionManager::setGeometryVisible - Geometry not found for visibility change: " + name);
		return;
	}

	LOG_INF_S("SelectionManager::setGeometryVisible - Setting geometry '" + name + "' visibility to " + (visible ? "visible" : "hidden"));

	// Update geometry's internal visibility state
	geometry->setVisible(visible);

	// Manage Coin3D scene graph attachment/detachment
	SoSeparator* coinNode = geometry->getCoinNode();
	if (coinNode && m_sceneManager && m_sceneManager->getObjectRoot()) {
		SoSeparator* root = m_sceneManager->getObjectRoot();
		int idx = root->findChild(coinNode);

		if (visible) {
			// Add to scene graph if not already present
			if (idx < 0) {
				LOG_INF_S("SelectionManager::setGeometryVisible - Adding geometry '" + name + "' to Coin3D scene graph");
				root->addChild(coinNode);
			} else {
				LOG_INF_S("SelectionManager::setGeometryVisible - Geometry '" + name + "' already in scene graph");
			}
		} else {
			// Remove from scene graph if present
			if (idx >= 0) {
				LOG_INF_S("SelectionManager::setGeometryVisible - Removing geometry '" + name + "' from Coin3D scene graph");
				root->removeChild(idx);
			} else {
				LOG_INF_S("SelectionManager::setGeometryVisible - Geometry '" + name + "' not in scene graph");
			}
		}
	} else {
		if (!coinNode) {
			LOG_WRN_S("SelectionManager::setGeometryVisible - Coin3D node is null for geometry '" + name + "', visibility change may not be visible until representation is built");
		}
		if (!m_sceneManager) {
			LOG_WRN_S("SelectionManager::setGeometryVisible - SceneManager is null");
		}
		if (m_sceneManager && !m_sceneManager->getObjectRoot()) {
			LOG_WRN_S("SelectionManager::setGeometryVisible - ObjectRoot is null");
		}
	}

	// Request view refresh with immediate update
	requestRefreshSelectionChanged();
	if (m_sceneManager && m_sceneManager->getCanvas()) {
		// Force immediate repaint for visibility changes
		m_sceneManager->getCanvas()->Refresh(true);
		m_sceneManager->getCanvas()->Update();
	}
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
	
	// Only update tree selection if not currently updating from tree
	if (canvas && canvas->getObjectTreePanel()) {
		auto objectTreePanel = canvas->getObjectTreePanel();
		if (!objectTreePanel->isUpdatingSelection()) {
			objectTreePanel->updateTreeSelectionFromViewer();
		} else {
			LOG_INF_S("SelectionManager::onSelectionChanged - Skipping tree update (tree is currently updating selection)");
		}
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