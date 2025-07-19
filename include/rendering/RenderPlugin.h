#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>

/**
 * @brief Rendering plugin interface
 * 
 * Base interface for rendering plugins
 */
class RenderPlugin {
public:
    virtual ~RenderPlugin() = default;

    /**
     * @brief Get plugin name
     * @return Plugin identifier
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Get plugin version
     * @return Version string
     */
    virtual std::string getVersion() const = 0;

    /**
     * @brief Get plugin description
     * @return Description string
     */
    virtual std::string getDescription() const = 0;

    /**
     * @brief Initialize plugin
     * @param config Configuration parameters
     * @return true if initialization successful
     */
    virtual bool initialize(const std::string& config = "") = 0;

    /**
     * @brief Shutdown plugin
     */
    virtual void shutdown() = 0;

    /**
     * @brief Check if plugin is available
     * @return true if plugin can be used
     */
    virtual bool isAvailable() const = 0;
};

/**
 * @brief Geometry processor plugin interface
 */
class GeometryProcessorPlugin : public RenderPlugin {
public:
    /**
     * @brief Create geometry processor instance
     * @return Processor instance
     */
    virtual std::unique_ptr<class GeometryProcessor> createProcessor() = 0;
};

/**
 * @brief Rendering backend plugin interface
 */
class RenderBackendPlugin : public RenderPlugin {
public:
    /**
     * @brief Create rendering backend instance
     * @return Backend instance
     */
    virtual std::unique_ptr<class RenderBackend> createBackend() = 0;
};

/**
 * @brief Plugin manager
 */
class RenderPluginManager {
public:
    /**
     * @brief Get singleton instance
     * @return Manager instance
     */
    static RenderPluginManager& getInstance();

    /**
     * @brief Load plugin from file
     * @param filename Plugin file path
     * @return true if loading successful
     */
    bool loadPlugin(const std::string& filename);

    /**
     * @brief Load plugins from directory
     * @param directory Plugin directory path
     * @return Number of plugins loaded
     */
    int loadPluginsFromDirectory(const std::string& directory);

    /**
     * @brief Register plugin manually
     * @param name Plugin name
     * @param plugin Plugin instance
     */
    void registerPlugin(const std::string& name, std::unique_ptr<RenderPlugin> plugin);

    /**
     * @brief Get plugin by name
     * @param name Plugin name
     * @return Plugin instance or nullptr
     */
    RenderPlugin* getPlugin(const std::string& name);

    /**
     * @brief Get geometry processor plugin
     * @param name Plugin name
     * @return Processor plugin or nullptr
     */
    GeometryProcessorPlugin* getGeometryProcessorPlugin(const std::string& name);

    /**
     * @brief Get rendering backend plugin
     * @param name Plugin name
     * @return Backend plugin or nullptr
     */
    RenderBackendPlugin* getRenderBackendPlugin(const std::string& name);

    /**
     * @brief Get available plugin names
     * @return List of plugin names
     */
    std::vector<std::string> getAvailablePlugins() const;

    /**
     * @brief Unload plugin
     * @param name Plugin name
     */
    void unloadPlugin(const std::string& name);

    /**
     * @brief Unload all plugins
     */
    void unloadAllPlugins();

private:
    RenderPluginManager() = default;
    RenderPluginManager(const RenderPluginManager&) = delete;
    RenderPluginManager& operator=(const RenderPluginManager&) = delete;

    std::map<std::string, std::unique_ptr<RenderPlugin>> m_plugins;
}; 