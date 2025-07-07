#include "CommandDispatcher.h"
#include "CommandListener.h"
#include "Logger.h"
#include <algorithm>
#include <mutex>

CommandDispatcher::CommandDispatcher()
{
    LOG_INF("CommandDispatcher initialized");
}

CommandDispatcher::~CommandDispatcher()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners.clear();
    LOG_INF("CommandDispatcher destroyed");
}

void CommandDispatcher::registerListener(const std::string& commandType, std::shared_ptr<CommandListener> listener)
{
    if (!listener) {
        LOG_ERR("Attempted to register null listener for command: " + commandType);
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_listeners[commandType].push_back(listener);
    LOG_INF("Registered listener '" + listener->getListenerName() + "' for command: " + commandType);
}

void CommandDispatcher::unregisterListener(const std::string& commandType, std::shared_ptr<CommandListener> listener)
{
    if (!listener) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_listeners.find(commandType);
    if (it != m_listeners.end()) {
        auto& listeners = it->second;
        listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
        
        if (listeners.empty()) {
            m_listeners.erase(it);
        }
        
        LOG_INF("Unregistered listener '" + listener->getListenerName() + "' for command: " + commandType);
    }
}

CommandResult CommandDispatcher::dispatchCommand(const std::string& commandType, 
                                               const std::unordered_map<std::string, std::string>& parameters)
{
    LOG_INF("Dispatching command: " + commandType);
    
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_listeners.find(commandType);
    if (it == m_listeners.end() || it->second.empty()) {
        std::string errorMsg = "No listeners registered for command: " + commandType;
        LOG_ERR(errorMsg);
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
                
                LOG_INF("Command '" + commandType + "' executed by '" + listener->getListenerName() + 
                       "' with result: " + (result.success ? "SUCCESS" : "FAILURE"));
                
                if (m_uiFeedbackHandler) {
                    m_uiFeedbackHandler(result);
                }
                
                return result;
            }
            catch (const std::exception& e) {
                std::string errorMsg = "Exception in command execution: " + std::string(e.what());
                LOG_ERR(errorMsg);
                CommandResult result(false, errorMsg, commandType);
                
                if (m_uiFeedbackHandler) {
                    m_uiFeedbackHandler(result);
                }
                
                return result;
            }
        }
    }
    
    std::string errorMsg = "No capable listener found for command: " + commandType;
    LOG_ERR(errorMsg);
    CommandResult result(false, errorMsg, commandType);
    
    if (m_uiFeedbackHandler) {
        m_uiFeedbackHandler(result);
    }
    
    return result;
}

void CommandDispatcher::setUIFeedbackHandler(std::function<void(const CommandResult&)> handler)
{
    m_uiFeedbackHandler = handler;
    LOG_INF("UI feedback handler registered");
}

bool CommandDispatcher::hasHandler(const std::string& commandType) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_listeners.find(commandType);
    return it != m_listeners.end() && !it->second.empty();
}