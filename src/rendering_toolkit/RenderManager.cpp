#include "rendering/RenderManager.h"
#include "logger/Logger.h"
#include <algorithm>
#include <Inventor/nodes/SoSeparator.h>

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
    } catch (const std::exception& e) {
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
    
    return backend->createSceneNode(mesh, selected);
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