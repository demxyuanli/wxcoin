#include "rendering/RenderPlugin.h"
#include "logger/Logger.h"
#include <filesystem>

RenderPluginManager& RenderPluginManager::getInstance() {
	static RenderPluginManager instance;
	return instance;
}

bool RenderPluginManager::loadPlugin(const std::string& filename) {
	// This is a placeholder implementation
	// In a real implementation, this would load dynamic libraries
	LOG_WRN_S("Plugin loading not implemented yet: " + filename);
	return false;
}

int RenderPluginManager::loadPluginsFromDirectory(const std::string& directory) {
	// This is a placeholder implementation
	// In a real implementation, this would scan directory for plugin files
	LOG_WRN_S("Plugin directory loading not implemented yet: " + directory);
	return 0;
}

void RenderPluginManager::registerPlugin(const std::string& name, std::unique_ptr<RenderPlugin> plugin) {
	if (plugin) {
		m_plugins[name] = std::move(plugin);
		LOG_INF_S("Registered plugin: " + name);
	}
}

RenderPlugin* RenderPluginManager::getPlugin(const std::string& name) {
	auto it = m_plugins.find(name);
	if (it != m_plugins.end()) {
		return it->second.get();
	}
	return nullptr;
}

GeometryProcessorPlugin* RenderPluginManager::getGeometryProcessorPlugin(const std::string& name) {
	auto plugin = getPlugin(name);
	if (plugin) {
		return dynamic_cast<GeometryProcessorPlugin*>(plugin);
	}
	return nullptr;
}

RenderBackendPlugin* RenderPluginManager::getRenderBackendPlugin(const std::string& name) {
	auto plugin = getPlugin(name);
	if (plugin) {
		return dynamic_cast<RenderBackendPlugin*>(plugin);
	}
	return nullptr;
}

std::vector<std::string> RenderPluginManager::getAvailablePlugins() const {
	std::vector<std::string> names;
	names.reserve(m_plugins.size());
	for (const auto& pair : m_plugins) {
		names.push_back(pair.first);
	}
	return names;
}

void RenderPluginManager::unloadPlugin(const std::string& name) {
	auto it = m_plugins.find(name);
	if (it != m_plugins.end()) {
		m_plugins.erase(it);
		LOG_INF_S("Unloaded plugin: " + name);
	}
}

void RenderPluginManager::unloadAllPlugins() {
	m_plugins.clear();
	LOG_INF_S("Unloaded all plugins");
}