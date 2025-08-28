#pragma once

#include "RenderingToolkit.h"
#include "GeometryProcessor.h"
#include "RenderBackend.h"
#include "RenderConfig.h"
#include "FrustumCuller.h"
#include "OcclusionCuller.h"
#include <memory>
#include <string>
#include <map>

/**
 * @brief Rendering manager
 *
 * Manages rendering components and provides unified interface
 */
class RenderManager {
public:
	/**
	 * @brief Get singleton instance
	 * @return Manager instance
	 */
	static RenderManager& getInstance();

	/**
	 * @brief Initialize rendering system
	 * @param config Configuration parameters
	 * @return true if initialization successful
	 */
	bool initialize(const std::string& config = "");

	/**
	 * @brief Shutdown rendering system
	 */
	void shutdown();

	/**
	 * @brief Register geometry processor
	 * @param name Processor name
	 * @param processor Processor instance
	 */
	void registerGeometryProcessor(const std::string& name,
		std::unique_ptr<GeometryProcessor> processor);

	/**
	 * @brief Register rendering backend
	 * @param name Backend name
	 * @param backend Backend instance
	 */
	void registerRenderBackend(const std::string& name,
		std::unique_ptr<RenderBackend> backend);

	/**
	 * @brief Get geometry processor
	 * @param name Processor name (empty for default)
	 * @return Processor instance or nullptr
	 */
	GeometryProcessor* getGeometryProcessor(const std::string& name = "");

	/**
	 * @brief Get rendering backend
	 * @param name Backend name (empty for default)
	 * @return Backend instance or nullptr
	 */
	RenderBackend* getRenderBackend(const std::string& name = "");

	/**
	 * @brief Set default geometry processor
	 * @param name Processor name
	 */
	void setDefaultGeometryProcessor(const std::string& name);

	/**
	 * @brief Set default rendering backend
	 * @param name Backend name
	 */
	void setDefaultRenderBackend(const std::string& name);

	/**
	 * @brief Get configuration manager
	 * @return Configuration reference
	 */
	RenderConfig& getConfig() { return m_config; }
	const RenderConfig& getConfig() const { return m_config; }

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
	 * @brief Get available geometry processors
	 * @return List of processor names
	 */
	std::vector<std::string> getAvailableGeometryProcessors() const;

	/**
	 * @brief Get available rendering backends
	 * @return List of backend names
	 */
	std::vector<std::string> getAvailableRenderBackends() const;

	/**
	 * @brief Check if system is initialized
	 * @return true if initialized
	 */
	bool isInitialized() const { return m_initialized; }

	// Culling system methods
	/**
	 * @brief Update culling systems with current camera
	 * @param camera Current camera
	 */
	void updateCulling(const void* camera);

	/**
	 * @brief Check if shape should be rendered (frustum + occlusion culling)
	 * @param shape Shape to test
	 * @return true if should be rendered
	 */
	bool shouldRenderShape(const TopoDS_Shape& shape);

	/**
	 * @brief Add shape as occluder for occlusion culling
	 * @param shape Shape to add as occluder
	 * @param sceneNode Associated scene node
	 */
	void addOccluder(const TopoDS_Shape& shape, SoSeparator* sceneNode);

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
	 * @brief Get frustum culler
	 * @return Frustum culler reference
	 */
	FrustumCuller& getFrustumCuller() { return m_frustumCuller; }

	/**
	 * @brief Get occlusion culler
	 * @return Occlusion culler reference
	 */
	OcclusionCuller& getOcclusionCuller() { return m_occlusionCuller; }

	/**
	 * @brief Get culling statistics
	 * @return Statistics string
	 */
	std::string getCullingStats() const;

private:
	RenderManager();
	RenderManager(const RenderManager&) = delete;
	RenderManager& operator=(const RenderManager&) = delete;

	bool m_initialized;
	RenderConfig& m_config;

	std::string m_defaultProcessor;
	std::string m_defaultBackend;

	std::map<std::string, std::unique_ptr<GeometryProcessor>> m_geometryProcessors;
	std::map<std::string, std::unique_ptr<RenderBackend>> m_renderBackends;

	// Culling systems
	FrustumCuller m_frustumCuller;
	OcclusionCuller m_occlusionCuller;
};