#ifdef _WIN32
#define NOMINMAX
#define _WINSOCKAPI_
#include <windows.h>
#endif

#include "viewer/OutlineDisplayManager.h"

#include "SceneManager.h"
#include "Canvas.h"
#include "DynamicSilhouetteRenderer.h"
#include "viewer/ImageOutlinePass.h"
#include "OCCGeometry.h"

#include <Inventor/nodes/SoSeparator.h>

OutlineDisplayManager::OutlineDisplayManager(SceneManager* sceneManager,
	SoSeparator* occRoot,
	std::vector<std::shared_ptr<OCCGeometry>>* geometries)
	: m_sceneManager(sceneManager), m_occRoot(occRoot), m_geometries(geometries) {
}

OutlineDisplayManager::~OutlineDisplayManager() = default;

void OutlineDisplayManager::setEnabled(bool enabled) {
	if (m_enabled == enabled) return;
	m_enabled = enabled;
	if (!m_imagePass) m_imagePass = std::make_unique<ImageOutlinePass>(m_sceneManager, m_occRoot);
	m_imagePass->setEnabled(m_enabled);
	// Keep legacy per-geometry renderers off to avoid conflicts
}

void OutlineDisplayManager::onGeometryAdded(const std::shared_ptr<OCCGeometry>& geometry) {
	if (!geometry) return;
	if (!m_enabled) return;
	ensureForGeometry(geometry);
}

void OutlineDisplayManager::updateAll() {
	if (!m_geometries) return;
	if (!m_enabled) return;
	for (auto& g : *m_geometries) {
		ensureForGeometry(g);
	}
}

void OutlineDisplayManager::clearAll() {
	m_outlineByName.clear();
}

void OutlineDisplayManager::ensureForGeometry(const std::shared_ptr<OCCGeometry>& geometry) {
	if (!geometry) return;
	const std::string name = geometry->getName();
	auto it = m_outlineByName.find(name);
	if (it == m_outlineByName.end()) {
		auto renderer = std::make_unique<DynamicSilhouetteRenderer>(m_occRoot);
		renderer->setFastMode(true);
		renderer->setShape(geometry->getShape());
		if (SoSeparator* geomSep = geometry->getCoinNode()) {
			SoSeparator* silhouetteNode = renderer->getSilhouetteNode();
			bool alreadyChild = false;
			for (int i = 0; i < geomSep->getNumChildren(); ++i) {
				if (geomSep->getChild(i) == silhouetteNode) { alreadyChild = true; break; }
			}
			if (!alreadyChild) geomSep->addChild(silhouetteNode);
		}
		renderer->setEnabled(true);
		m_outlineByName.emplace(name, std::move(renderer));
	}
	else {
		it->second->setShape(geometry->getShape());
		it->second->setEnabled(true);
	}
}

void OutlineDisplayManager::setParams(const ImageOutlineParams& params) {
	if (!m_imagePass) m_imagePass = std::make_unique<ImageOutlinePass>(m_sceneManager, m_occRoot);
	m_imagePass->setParams(params);
}

ImageOutlineParams OutlineDisplayManager::getParams() const {
	return m_imagePass ? m_imagePass->getParams() : ImageOutlineParams{};
}

void OutlineDisplayManager::refreshOutlineAll() {
	if (m_imagePass) {
		m_imagePass->refresh();
	}
}