#include "rendering/RenderManager.h"
#include "logger/Logger.h"
#include <algorithm>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCamera.h>
#include <sstream>

RenderManager& RenderManager::getInstance() {
	static RenderManager instance;
	return instance;
}

RenderManager::RenderManager()
	: m_initialized(false)
	, m_config(RenderConfig::getInstance()) {
}

bool RenderManager::initialize(const std::string& config) {
	if (m_initialized) {
		LOG_WRN_S("RenderManager already initialized");
		return true;
	}

	try {
		m_initialized = true;
		LOG_INF_S("RenderManager initialized successfully");
		return true;
	}
	catch (const std::exception& e) {
		LOG_ERR_S("Failed to initialize RenderManager: " + std::string(e.what()));
		return false;
	}
}

void RenderManager::shutdown() {
	m_geometryProcessors.clear();
	m_renderBackends.clear();
	m_initialized = false;
	LOG_INF_S("RenderManager shutdown complete");
}

void RenderManager::registerGeometryProcessor(const std::string& name,
	std::unique_ptr<GeometryProcessor> processor) {
	if (processor) {
		m_geometryProcessors[name] = std::move(processor);
		if (m_defaultProcessor.empty()) {
			m_defaultProcessor = name;
		}
		LOG_INF_S("Registered geometry processor: " + name);
	}
}

void RenderManager::registerRenderBackend(const std::string& name,
	std::unique_ptr<RenderBackend> backend) {
	if (backend) {
		m_renderBackends[name] = std::move(backend);
		if (m_defaultBackend.empty()) {
			m_defaultBackend = name;
		}
		LOG_INF_S("Registered render backend: " + name);
	}
}

GeometryProcessor* RenderManager::getGeometryProcessor(const std::string& name) {
	std::string processorName = name.empty() ? m_defaultProcessor : name;
	auto it = m_geometryProcessors.find(processorName);
	if (it != m_geometryProcessors.end()) {
		return it->second.get();
	}
	return nullptr;
}

RenderBackend* RenderManager::getRenderBackend(const std::string& name) {
	std::string backendName = name.empty() ? m_defaultBackend : name;
	auto it = m_renderBackends.find(backendName);
	if (it != m_renderBackends.end()) {
		return it->second.get();
	}
	return nullptr;
}

void RenderManager::setDefaultGeometryProcessor(const std::string& name) {
	if (m_geometryProcessors.find(name) != m_geometryProcessors.end()) {
		m_defaultProcessor = name;
		LOG_INF_S("Set default geometry processor: " + name);
	}
}

void RenderManager::setDefaultRenderBackend(const std::string& name) {
	if (m_renderBackends.find(name) != m_renderBackends.end()) {
		m_defaultBackend = name;
		LOG_INF_S("Set default render backend: " + name);
	}
}

SoSeparatorPtr RenderManager::createSceneNode(const TriangleMesh& mesh,
	bool selected,
	const std::string& backendName) {
	auto backend = getRenderBackend(backendName);
	if (!backend) {
		LOG_ERR_S("Render backend not found: " + backendName);
		return nullptr;
	}

	// Use default material properties when no custom material is specified
	Quantity_Color defaultDiffuse(0.8, 0.8, 0.8, Quantity_TOC_RGB);
	Quantity_Color defaultAmbient(0.2, 0.2, 0.2, Quantity_TOC_RGB);
	Quantity_Color defaultSpecular(1.0, 1.0, 1.0, Quantity_TOC_RGB);
	Quantity_Color defaultEmissive(0.0, 0.0, 0.0, Quantity_TOC_RGB);
	return backend->createSceneNode(mesh, selected, defaultDiffuse, defaultAmbient, defaultSpecular, defaultEmissive, 0.5, 0.0, false);
}

SoSeparatorPtr RenderManager::createSceneNode(const TopoDS_Shape& shape,
	const MeshParameters& params,
	bool selected,
	const std::string& processorName,
	const std::string& backendName) {
	auto backend = getRenderBackend(backendName);
	if (!backend) {
		LOG_ERR_S("Render backend not found: " + backendName);
		return nullptr;
	}

	return backend->createSceneNode(shape, params, selected);
}

std::vector<std::string> RenderManager::getAvailableGeometryProcessors() const {
	std::vector<std::string> names;
	names.reserve(m_geometryProcessors.size());
	for (const auto& pair : m_geometryProcessors) {
		names.push_back(pair.first);
	}
	return names;
}

std::vector<std::string> RenderManager::getAvailableRenderBackends() const {
	std::vector<std::string> names;
	names.reserve(m_renderBackends.size());
	for (const auto& pair : m_renderBackends) {
		names.push_back(pair.first);
	}
	return names;
}

void RenderManager::updateCulling(const void* camera) {
	if (!m_initialized) {
		return;
	}

	const SoCamera* coinCamera = static_cast<const SoCamera*>(camera);
	if (!coinCamera) {
		return;
	}

	// Update frustum culling
	m_frustumCuller.updateFrustum(coinCamera);

	// Update occlusion culling
	m_occlusionCuller.updateOcclusion(coinCamera, &m_frustumCuller);
}

bool RenderManager::shouldRenderShape(const TopoDS_Shape& shape) {
	if (!m_initialized || shape.IsNull()) {
		return true;
	}

	// First check frustum culling
	if (!m_frustumCuller.isShapeVisible(shape)) {
		return false;
	}

	// Then check occlusion culling
	if (!m_occlusionCuller.isShapeVisible(shape)) {
		return false;
	}

	return true;
}

void RenderManager::addOccluder(const TopoDS_Shape& shape, SoSeparator* sceneNode) {
	if (!m_initialized) {
		return;
	}

	m_occlusionCuller.addOccluder(shape, sceneNode);
}

void RenderManager::removeOccluder(const TopoDS_Shape& shape) {
	if (!m_initialized) {
		return;
	}

	m_occlusionCuller.removeOccluder(shape);
}

void RenderManager::setFrustumCullingEnabled(bool enabled) {
	m_frustumCuller.setEnabled(enabled);
	LOG_INF_S("Frustum culling " + std::string(enabled ? "enabled" : "disabled"));
}

void RenderManager::setOcclusionCullingEnabled(bool enabled) {
	m_occlusionCuller.setEnabled(enabled);
	LOG_INF_S("Occlusion culling " + std::string(enabled ? "enabled" : "disabled"));
}

std::string RenderManager::getCullingStats() const {
	std::ostringstream stats;
	stats << "Frustum culling: " << (m_frustumCuller.isEnabled() ? "ON" : "OFF");
	stats << " (culled: " << m_frustumCuller.getCulledCount() << ")";
	stats << " | Occlusion culling: " << (m_occlusionCuller.isEnabled() ? "ON" : "OFF");
	stats << " (culled: " << m_occlusionCuller.getOccludedCount() << ")";
	stats << " | Active occluders: " << m_occlusionCuller.getOccluderCount();
	return stats.str();
}