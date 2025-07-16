#include "CommandDispatcher.h"
#include "CommandListener.h"
#include "logger/Logger.h"
#include <algorithm>
#include <mutex>
#include "CommandType.h"

CommandDispatcher::CommandDispatcher()
{
    LOG_INF_S("CommandDispatcher initialized");
}

CommandDispatcher::~CommandDispatcher()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners.clear();
    
    // Avoid logging during shutdown to prevent crashes
    // The Logger singleton might be destroyed before this destructor
    try {
        // Only log if we can safely access the logger
        static bool isShuttingDown = false;
        if (!isShuttingDown) {
            isShuttingDown = true;
            // Use a simple output instead of logger to avoid crashes
            std::cout << "CommandDispatcher destroyed" << std::endl;
        }
    } catch (...) {
        // Ignore any exceptions during shutdown
    }
}

void CommandDispatcher::registerListener(const std::string& commandType, std::shared_ptr<CommandListener> listener)
{
    if (!listener) {
        LOG_ERR_S("Attempted to register null listener for command: " + commandType);
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners[commandType].push_back(listener);
    LOG_INF_S("Registered listener '" + listener->getListenerName() + "' for command: " + commandType);
}

void CommandDispatcher::unregisterListener(const std::string& commandType, std::shared_ptr<CommandListener> listener)
{
    if (!listener) {
        return;
    }
    
    try {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_listeners.find(commandType);
        if (it != m_listeners.end()) {
            auto& listeners = it->second;
            listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
            
            if (listeners.empty()) {
                m_listeners.erase(it);
            }
            
            try {
                LOG_INF_S("Unregistered listener '" + listener->getListenerName() + "' for command: " + commandType);
            } catch (...) {
                // Avoid logging during shutdown
                std::cout << "Unregistered listener for command: " + commandType << std::endl;
            }
        }
    } catch (...) {
        // Ignore exceptions during shutdown - static objects might be destroyed
        std::cout << "Exception during listener unregistration (ignored)" << std::endl;
    }
}

CommandResult CommandDispatcher::dispatchCommand(const std::string& commandType, 
                                               const std::unordered_map<std::string, std::string>& parameters)
{
    LOG_INF_S("Dispatching command: " + commandType);
    
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_listeners.find(commandType);
    if (it == m_listeners.end() || it->second.empty()) {
        std::string errorMsg = "No listeners registered for command: " + commandType;
        LOG_ERR_S(errorMsg);
        CommandResult result(false, errorMsg, commandType);
        
        if (m_uiFeedbackHandler) {
            m_uiFeedbackHandler(result);
        }
        
        return result;
    }
    
    // Execute command with the first available listener
    // In a more complex system, you might want to execute with all listeners
    auto& listeners = it->second;
    for (auto& listener : listeners) {
        if (listener && listener->canHandleCommand(commandType)) {
            try {
                CommandResult result = listener->executeCommand(commandType, parameters);
                result.commandId = commandType;
                
                LOG_INF_S("Command '" + commandType + "' executed by '" + listener->getListenerName() + 
                       "' with result: " + (result.success ? "SUCCESS" : "FAILURE"));
                
                if (m_uiFeedbackHandler) {
                    m_uiFeedbackHandler(result);
                }
                
                return result;
            }
            catch (const std::exception& e) {
                std::string errorMsg = "Exception in command execution: " + std::string(e.what());
                LOG_ERR_S(errorMsg);
                CommandResult result(false, errorMsg, commandType);
                
                if (m_uiFeedbackHandler) {
                    m_uiFeedbackHandler(result);
                }
                
                return result;
            }
        }
    }
    
    std::string errorMsg = "No capable listener found for command: " + commandType;
    LOG_ERR_S(errorMsg);
    CommandResult result(false, errorMsg, commandType);
    
    if (m_uiFeedbackHandler) {
        m_uiFeedbackHandler(result);
    }
    
    return result;
}

void CommandDispatcher::setUIFeedbackHandler(std::function<void(const CommandResult&)> handler)
{
    m_uiFeedbackHandler = handler;
    LOG_INF_S("UI feedback handler registered");
}

bool CommandDispatcher::hasHandler(const std::string& commandType) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_listeners.find(commandType);
    return it != m_listeners.end() && !it->second.empty();
}

void CommandDispatcher::registerListener(cmd::CommandType commandType, std::shared_ptr<CommandListener> listener)
{
    try {
        registerListener(cmd::to_string(commandType), listener);
    } catch (...) {
        // Handle potential static map access issues during shutdown
        std::cout << "Exception during CommandType registration (ignored)" << std::endl;
    }
}

void CommandDispatcher::unregisterListener(cmd::CommandType commandType, std::shared_ptr<CommandListener> listener)
{
    try {
        unregisterListener(cmd::to_string(commandType), listener);
    } catch (...) {
        // Handle potential static map access issues during shutdown
        std::cout << "Exception during CommandType unregistration (ignored)" << std::endl;
    }
}

CommandResult CommandDispatcher::dispatchCommand(cmd::CommandType commandType,
                                                  const std::unordered_map<std::string, std::string>& parameters)
{
    try {
        return dispatchCommand(cmd::to_string(commandType), parameters);
    } catch (...) {
        // Handle potential static map access issues during shutdown
        return CommandResult(false, "Static map access error during shutdown", "UNKNOWN");
    }
}

bool CommandDispatcher::hasHandler(cmd::CommandType commandType) const
{
    try {
        return hasHandler(cmd::to_string(commandType));
    } catch (...) {
        // Handle potential static map access issues during shutdown
        return false;
    }
}
