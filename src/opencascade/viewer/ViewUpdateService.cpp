#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif
#include "viewer/ViewUpdateService.h"

#include "SceneManager.h"
#include "Canvas.h"
#include "ViewRefreshManager.h"

ViewUpdateService::ViewUpdateService(SceneManager* sceneManager)
	: m_sceneManager(sceneManager) {
}

void ViewUpdateService::updateSceneBounds() const {
	if (m_sceneManager) m_sceneManager->updateSceneBounds();
}

void ViewUpdateService::resetView() const {
	if (m_sceneManager) m_sceneManager->resetView();
}

void ViewUpdateService::requestRefresh(int reasonEnumValue, bool immediate) const {
	if (!m_sceneManager || !m_sceneManager->getCanvas()) return;
	if (auto* refresher = m_sceneManager->getCanvas()->getRefreshManager()) {
		refresher->requestRefresh(static_cast<IViewRefresher::Reason>(reasonEnumValue), immediate);
	}
}

void ViewUpdateService::requestMaterialChanged(bool immediate) const {
	if (!m_sceneManager || !m_sceneManager->getCanvas()) return;
	if (auto* refresher = m_sceneManager->getCanvas()->getRefreshManager()) {
		refresher->requestRefresh(ViewRefreshManager::RefreshReason::MATERIAL_CHANGED, immediate);
	}
}

void ViewUpdateService::requestGeometryChanged(bool immediate) const {
	if (!m_sceneManager || !m_sceneManager->getCanvas()) return;
	if (auto* refresher = m_sceneManager->getCanvas()->getRefreshManager()) {
		refresher->requestRefresh(ViewRefreshManager::RefreshReason::GEOMETRY_CHANGED, immediate);
	}
}

void ViewUpdateService::requestNormalsToggled(bool immediate) const {
	if (!m_sceneManager || !m_sceneManager->getCanvas()) return;
	if (auto* refresher = m_sceneManager->getCanvas()->getRefreshManager()) {
		refresher->requestRefresh(ViewRefreshManager::RefreshReason::NORMALS_TOGGLED, immediate);
	}
}

void ViewUpdateService::requestEdgesToggled(bool immediate) const {
	if (!m_sceneManager || !m_sceneManager->getCanvas()) return;
	if (auto* refresher = m_sceneManager->getCanvas()->getRefreshManager()) {
		refresher->requestRefresh(ViewRefreshManager::RefreshReason::EDGES_TOGGLED, immediate);
	}
}

void ViewUpdateService::requestCameraMoved(bool immediate) const {
	if (!m_sceneManager || !m_sceneManager->getCanvas()) return;
	if (auto* refresher = m_sceneManager->getCanvas()->getRefreshManager()) {
		refresher->requestRefresh(ViewRefreshManager::RefreshReason::CAMERA_MOVED, immediate);
	}
}

void ViewUpdateService::refreshCanvas(bool eraseBackground) const {
	if (m_sceneManager && m_sceneManager->getCanvas()) {
		m_sceneManager->getCanvas()->Refresh(eraseBackground);
	}
}