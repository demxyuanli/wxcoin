#include "LayoutStrategyFactory.h"
#include "IDELayoutStrategy.h"
#include "FlexibleLayoutStrategy.h"
#include "HybridLayoutStrategy.h"

LayoutStrategyFactory& LayoutStrategyFactory::Instance()
{
	static LayoutStrategyFactory instance;
	return instance;
}

LayoutStrategyFactory::LayoutStrategyFactory()
	: m_defaultStrategy(LayoutStrategy::IDE)
{
	InitializeBuiltInStrategies();
}

void LayoutStrategyFactory::InitializeBuiltInStrategies()
{
	// Register IDE layout strategy
	RegisterStrategy(LayoutStrategy::IDE, "IDE Layout",
		[]() -> std::unique_ptr<ILayoutStrategy> {
			return std::make_unique<IDELayoutStrategy>();
		});

	// Register Fixed layout strategy (placeholder)
	RegisterStrategy(LayoutStrategy::Fixed, "Fixed Layout",
		[]() -> std::unique_ptr<ILayoutStrategy> {
			// For now, return IDE layout as a fallback
			return std::make_unique<IDELayoutStrategy>();
		});

	// Register Flexible layout strategy
	RegisterStrategy(LayoutStrategy::Flexible, "Flexible Layout",
		[]() -> std::unique_ptr<ILayoutStrategy> {
			return std::make_unique<FlexibleLayoutStrategy>();
		});

	// Register Hybrid layout strategy
	RegisterStrategy(LayoutStrategy::Hybrid, "Hybrid Layout",
		[]() -> std::unique_ptr<ILayoutStrategy> {
			return std::make_unique<HybridLayoutStrategy>();
		});
}

void LayoutStrategyFactory::RegisterStrategy(LayoutStrategy strategy, const std::string& name, StrategyCreator creator)
{
	if (creator) {
		m_strategyCreators[strategy] = creator;
		m_strategyNames[strategy] = name;
		m_nameToStrategy[name] = strategy;
	}
}

std::unique_ptr<ILayoutStrategy> LayoutStrategyFactory::CreateStrategy(LayoutStrategy strategy)
{
	auto it = m_strategyCreators.find(strategy);
	if (it != m_strategyCreators.end()) {
		return it->second();
	}

	// Fallback to default strategy
	if (strategy != m_defaultStrategy) {
		return CreateStrategy(m_defaultStrategy);
	}

	return nullptr;
}

std::unique_ptr<ILayoutStrategy> LayoutStrategyFactory::CreateStrategy(const std::string& name)
{
	auto it = m_nameToStrategy.find(name);
	if (it != m_nameToStrategy.end()) {
		return CreateStrategy(it->second);
	}

	// Fallback to default strategy
	return CreateStrategy(m_defaultStrategy);
}

std::vector<LayoutStrategy> LayoutStrategyFactory::GetAvailableStrategies() const
{
	std::vector<LayoutStrategy> strategies;
	for (const auto& pair : m_strategyCreators) {
		strategies.push_back(pair.first);
	}
	return strategies;
}

std::vector<std::string> LayoutStrategyFactory::GetAvailableStrategyNames() const
{
	std::vector<std::string> names;
	for (const auto& pair : m_strategyNames) {
		names.push_back(pair.second);
	}
	return names;
}

std::string LayoutStrategyFactory::GetStrategyName(LayoutStrategy strategy) const
{
	auto it = m_strategyNames.find(strategy);
	if (it != m_strategyNames.end()) {
		return it->second;
	}
	return "Unknown Strategy";
}

LayoutStrategy LayoutStrategyFactory::GetStrategyType(const std::string& name) const
{
	auto it = m_nameToStrategy.find(name);
	if (it != m_nameToStrategy.end()) {
		return it->second;
	}
	return m_defaultStrategy;
}

bool LayoutStrategyFactory::IsStrategyAvailable(LayoutStrategy strategy) const
{
	return m_strategyCreators.find(strategy) != m_strategyCreators.end();
}

bool LayoutStrategyFactory::IsStrategyAvailable(const std::string& name) const
{
	return m_nameToStrategy.find(name) != m_nameToStrategy.end();
}

void LayoutStrategyFactory::UnregisterStrategy(LayoutStrategy strategy)
{
	auto nameIt = m_strategyNames.find(strategy);
	if (nameIt != m_strategyNames.end()) {
		std::string name = nameIt->second;
		m_nameToStrategy.erase(name);
		m_strategyNames.erase(nameIt);
	}

	m_strategyCreators.erase(strategy);
}

void LayoutStrategyFactory::UnregisterStrategy(const std::string& name)
{
	auto strategyIt = m_nameToStrategy.find(name);
	if (strategyIt != m_nameToStrategy.end()) {
		LayoutStrategy strategy = strategyIt->second;
		m_nameToStrategy.erase(strategyIt);
		m_strategyNames.erase(strategy);
		m_strategyCreators.erase(strategy);
	}
}

void LayoutStrategyFactory::ClearStrategies()
{
	m_strategyCreators.clear();
	m_strategyNames.clear();
	m_nameToStrategy.clear();
}

LayoutStrategy LayoutStrategyFactory::GetDefaultStrategy() const
{
	return m_defaultStrategy;
}

void LayoutStrategyFactory::SetDefaultStrategy(LayoutStrategy strategy)
{
	if (IsStrategyAvailable(strategy)) {
		m_defaultStrategy = strategy;
	}
}