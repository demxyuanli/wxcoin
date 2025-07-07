#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <mutex>

class Command;
class CommandListener;

/**
 * @brief Result of command execution
 */
struct CommandResult {
    bool success;           // Whether command executed successfully
    std::string message;    // Result message or error description
    std::string commandId;  // Unique command identifier
    
    CommandResult(bool s = true, const std::string& msg = "", const std::string& id = "")
        : success(s), message(msg), commandId(id) {}
};

/**
 * @brief Command dispatcher acts as the central command broadcast slot
 * Manages command listeners and dispatches commands to appropriate handlers
 */
class CommandDispatcher {
public:
    CommandDispatcher();
    ~CommandDispatcher();
    
    /**
     * @brief Register a command listener for specific command types
     * @param commandType Type of command to listen for
     * @param listener Shared pointer to the listener
     */
    void registerListener(const std::string& commandType, std::shared_ptr<CommandListener> listener);
    
    /**
     * @brief Unregister a command listener
     * @param commandType Type of command
     * @param listener Listener to remove
     */
    void unregisterListener(const std::string& commandType, std::shared_ptr<CommandListener> listener);
    
    /**
     * @brief Dispatch command to registered listeners
     * @param commandType Type of command to execute
     * @param parameters Command parameters
     * @return CommandResult with execution status
     */
    CommandResult dispatchCommand(const std::string& commandType, 
                                const std::unordered_map<std::string, std::string>& parameters = {});
    
    /**
     * @brief Register UI feedback handler for command results
     * @param handler Function to handle command feedback
     */
    void setUIFeedbackHandler(std::function<void(const CommandResult&)> handler);
    
    /**
     * @brief Check if any listener can handle the command type
     * @param commandType Command type to check
     * @return True if handler exists
     */
    bool hasHandler(const std::string& commandType) const;
    
private:
    std::unordered_map<std::string, std::vector<std::shared_ptr<CommandListener>>> m_listeners;
    std::function<void(const CommandResult&)> m_uiFeedbackHandler;
    mutable std::mutex m_mutex;  // Thread safety for listener management
};