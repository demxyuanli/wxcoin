#pragma once

#include "../ILayoutStrategy.h"
#include "../UnifiedDockTypes.h"

#include <memory>
#include <string>
#include <map>
#include <functional>

// Layout strategy factory for creating different layout strategies
class LayoutStrategyFactory {
public:
	// Factory method type
	using StrategyCreator = std::function<std::unique_ptr<ILayoutStrategy>()>;

	// Singleton access
	static LayoutStrategyFactory& Instance();

	// Register a new layout strategy
	void RegisterStrategy(LayoutStrategy strategy, const std::string& name, StrategyCreator creator);

	// Create a layout strategy instance
	std::unique_ptr<ILayoutStrategy> CreateStrategy(LayoutStrategy strategy);
	std::unique_ptr<ILayoutStrategy> CreateStrategy(const std::string& name);

	// Get available strategies
	std::vector<LayoutStrategy> GetAvailableStrategies() const;
	std::vector<std::string> GetAvailableStrategyNames() const;

	// Get strategy information
	std::string GetStrategyName(LayoutStrategy strategy) const;
	LayoutStrategy GetStrategyType(const std::string& name) const;

	// Check if strategy is available
	bool IsStrategyAvailable(LayoutStrategy strategy) const;
	bool IsStrategyAvailable(const std::string& name) const;

	// Unregister a strategy
	void UnregisterStrategy(LayoutStrategy strategy);
	void UnregisterStrategy(const std::string& name);

	// Clear all registered strategies
	void ClearStrategies();

	// Get default strategy
	LayoutStrategy GetDefaultStrategy() const;
	void SetDefaultStrategy(LayoutStrategy strategy);

private:
	LayoutStrategyFactory();
	~LayoutStrategyFactory() = default;

	// Disable copy and assignment
	LayoutStrategyFactory(const LayoutStrategyFactory&) = delete;
	LayoutStrategyFactory& operator=(const LayoutStrategyFactory&) = delete;

	// Initialize built-in strategies
	void InitializeBuiltInStrategies();

	// Strategy registry
	std::map<LayoutStrategy, StrategyCreator> m_strategyCreators;
	std::map<LayoutStrategy, std::string> m_strategyNames;
	std::map<std::string, LayoutStrategy> m_nameToStrategy;

	// Default strategy
	LayoutStrategy m_defaultStrategy;
};

// Helper macros for registering strategies
#define REGISTER_LAYOUT_STRATEGY(strategy, name, className) \
    LayoutStrategyFactory::Instance().RegisterStrategy( \
        strategy, name, []() -> std::unique_ptr<ILayoutStrategy> { \
            return std::make_unique<className>(); \
        })

#define REGISTER_LAYOUT_STRATEGY_WITH_FACTORY(strategy, name, factory) \
    LayoutStrategyFactory::Instance().RegisterStrategy(strategy, name, factory)
