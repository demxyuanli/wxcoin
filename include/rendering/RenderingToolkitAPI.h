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

	// Culling system methods
	/**
	 * @brief Update culling systems with current camera
	 * @param camera Current camera
	 */
	void updateCulling(const void* camera);

	/**
	 * @brief Check if shape should be rendered (with culling)
	 * @param shape Shape to test
	 * @return true if should be rendered
	 */
	bool shouldRenderShape(const TopoDS_Shape& shape);

	/**
	 * @brief Add shape as occluder for occlusion culling
	 * @param shape Shape to add as occluder
	 * @param sceneNode Associated scene node
	 */
	void addOccluder(const TopoDS_Shape& shape, void* sceneNode);

	/**
	 * @brief Remove shape from occluders
	 * @param shape Shape to remove
	 */
	void removeOccluder(const TopoDS_Shape& shape);

	/**
	 * @brief Enable/disable frustum culling
	 * @param enabled Culling state
	 */
	void setFrustumCullingEnabled(bool enabled);

	/**
	 * @brief Enable/disable occlusion culling
	 * @param enabled Culling state
	 */
	void setOcclusionCullingEnabled(bool enabled);

	/**
	 * @brief Get culling statistics
	 * @return Statistics string
	 */
	std::string getCullingStats();

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