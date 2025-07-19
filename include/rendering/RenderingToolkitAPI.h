#pragma once

// Core toolkit interface
#include "RenderingToolkit.h"

// Geometry processing
#include "GeometryProcessor.h"
#include "OpenCASCADEProcessor.h"

// Rendering backends
#include "RenderBackend.h"
#include "Coin3DBackend.h"

// Configuration and management
#include "RenderConfig.h"
#include "RenderManager.h"

// Plugin system
#include "RenderPlugin.h"

/**
 * @brief Rendering Toolkit API
 * 
 * Main entry point for the rendering toolkit
 */
namespace RenderingToolkitAPI {
    
    /**
     * @brief Initialize the rendering toolkit
     * @param config Configuration parameters
     * @return true if initialization successful
     */
    bool initialize(const std::string& config = "");

    /**
     * @brief Shutdown the rendering toolkit
     */
    void shutdown();

    /**
     * @brief Get rendering manager
     * @return Manager instance
     */
    RenderManager& getManager();

    /**
     * @brief Get configuration manager
     * @return Configuration instance
     */
    RenderConfig& getConfig();

    /**
     * @brief Get plugin manager
     * @return Plugin manager instance
     */
    RenderPluginManager& getPluginManager();

    /**
     * @brief Create scene node from mesh
     * @param mesh Input mesh
     * @param selected Selection state
     * @param backendName Backend name (empty for default)
     * @return Scene node
     */
    SoSeparatorPtr createSceneNode(const TriangleMesh& mesh, 
                                  bool selected = false,
                                  const std::string& backendName = "");

    /**
     * @brief Create scene node from shape
     * @param shape Input shape
     * @param params Meshing parameters
     * @param selected Selection state
     * @param processorName Processor name (empty for default)
     * @param backendName Backend name (empty for default)
     * @return Scene node
     */
    SoSeparatorPtr createSceneNode(const TopoDS_Shape& shape,
                                  const MeshParameters& params = MeshParameters(),
                                  bool selected = false,
                                  const std::string& processorName = "",
                                  const std::string& backendName = "");

    /**
     * @brief Load plugins from directory
     * @param directory Plugin directory path
     * @return Number of plugins loaded
     */
    int loadPlugins(const std::string& directory);

    /**
     * @brief Get available geometry processors
     * @return List of processor names
     */
    std::vector<std::string> getAvailableGeometryProcessors();

    /**
     * @brief Get available rendering backends
     * @return List of backend names
     */
    std::vector<std::string> getAvailableRenderBackends();

    /**
     * @brief Get toolkit version
     * @return Version string
     */
    std::string getVersion();

    /**
     * @brief Check if toolkit is initialized
     * @return true if initialized
     */
    bool isInitialized();
} 