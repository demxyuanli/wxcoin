#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <future>
#include <queue>
#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include "CommandType.h"
#include "CommandListener.h"
#include "CommandDispatcher.h"

/**
 * @brief Optimized command dispatcher with caching and performance monitoring
 */
class OptimizedCommandDispatcher {
public:
    // Type aliases for better readability
    using CommandParameters = std::unordered_map<std::string, std::string>;


    // Constructor and destructor
    OptimizedCommandDispatcher();
    ~OptimizedCommandDispatcher() = default;

    // Listener management
    void registerListener(uint32_t commandId, std::shared_ptr<CommandListener> listener);
    void registerListener(cmd::CommandType commandType, std::shared_ptr<CommandListener> listener);
    void unregisterListener(uint32_t commandId, std::shared_ptr<CommandListener> listener);

    // Command dispatching
    CommandResult dispatchCommand(uint32_t commandId, const CommandParameters& parameters);
    CommandResult dispatchCommand(cmd::CommandType commandType, const CommandParameters& parameters);

    // Batch command dispatching
    std::vector<CommandResult> dispatchCommands(
        const std::vector<std::pair<uint32_t, CommandParameters>>& commands);

    // Helper method to convert command ID to string
    std::string commandIdToString(uint32_t commandId) const;

    // UI feedback handler
    void setUIFeedbackHandler(std::function<void(const CommandResult&)> handler);

    // Query methods
    bool hasHandler(uint32_t commandId) const;
    uint32_t getCommandId(const std::string& commandString) const;

    // Precompilation for performance
    std::vector<uint32_t> precompileCommands(const std::vector<std::string>& commandStrings) const;

    // Performance monitoring
    std::string getPerformanceStats() const;

private:
    // Thread-safe data structures
    mutable std::shared_mutex m_mutex;
    std::unordered_map<uint32_t, std::vector<std::shared_ptr<CommandListener>>> m_listeners;

    // String to ID caching for performance
    mutable std::unordered_map<std::string, uint32_t> m_stringToIdCache;

    // Performance tracking
    mutable std::atomic<uint64_t> m_dispatchCount{0};
    mutable std::atomic<uint64_t> m_cacheHits{0};
    mutable std::atomic<uint64_t> m_cacheMisses{0};

    // UI feedback handler
    std::function<void(const CommandResult&)> m_uiFeedbackHandler;

    // Internal helper methods
    uint32_t stringToCommandId(const std::string& commandString) const;
    void updatePerformanceStats(bool cacheHit) const;
};
