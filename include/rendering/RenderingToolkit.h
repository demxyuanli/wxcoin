#pragma once

#include <memory>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <functional>

/**
 * @brief Core rendering toolkit interface
 *
 * Provides a unified interface for different rendering backends and techniques
 */
class RenderingToolkit {
public:
	virtual ~RenderingToolkit() = default;

	/**
	 * @brief Initialize the rendering toolkit
	 * @param config Configuration parameters
	 * @return true if initialization successful
	 */
	virtual bool initialize(const std::string& config = "") = 0;

	/**
	 * @brief Shutdown the rendering toolkit
	 */
	virtual void shutdown() = 0;

	/**
	 * @brief Get toolkit name
	 * @return Toolkit identifier
	 */
	virtual std::string getName() const = 0;

	/**
	 * @brief Get toolkit version
	 * @return Version string
	 */
	virtual std::string getVersion() const = 0;

	/**
	 * @brief Check if toolkit is available
	 * @return true if toolkit can be used
	 */
	virtual bool isAvailable() const = 0;
};

/**
 * @brief Rendering toolkit factory
 */
class RenderingToolkitFactory {
public:
	using ToolkitCreator = std::function<std::unique_ptr<RenderingToolkit>()>;

	/**
	 * @brief Register a toolkit creator
	 * @param name Toolkit name
	 * @param creator Factory function
	 */
	static void registerToolkit(const std::string& name, ToolkitCreator creator);

	/**
	 * @brief Create toolkit by name
	 * @param name Toolkit name
	 * @return Toolkit instance or nullptr if not found
	 */
	static std::unique_ptr<RenderingToolkit> createToolkit(const std::string& name);

	/**
	 * @brief Get available toolkit names
	 * @return List of registered toolkit names
	 */
	static std::vector<std::string> getAvailableToolkits();

private:
	static std::map<std::string, ToolkitCreator>& getRegistry();
};