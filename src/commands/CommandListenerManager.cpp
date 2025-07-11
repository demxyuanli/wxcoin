#include "CommandListenerManager.h"

void CommandListenerManager::registerListener(cmd::CommandType type, std::shared_ptr<CommandListener> listener) {
    m_listeners[type] = listener;
}

CommandResult CommandListenerManager::dispatch(cmd::CommandType type, const std::unordered_map<std::string, std::string>& params) {
    auto it = m_listeners.find(type);
    if (it == m_listeners.end() || !it->second) {
        return CommandResult(false, "No listener registered for command", cmd::to_string(type));
    }
    return it->second->executeCommand(type, params);
}

bool CommandListenerManager::hasListener(cmd::CommandType type) const {
    auto it = m_listeners.find(type);
    return it != m_listeners.end() && it->second != nullptr;
} 
