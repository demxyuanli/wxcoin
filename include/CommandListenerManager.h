#pragma once
#include <memory>
#include <unordered_map>
#include "CommandType.h"
#include "CommandListener.h"
#include "CommandDispatcher.h" // for CommandResult

class CommandListenerManager {
public:
    void registerListener(cmd::CommandType type, std::shared_ptr<CommandListener> listener);
    CommandResult dispatch(cmd::CommandType type, const std::unordered_map<std::string, std::string>& params = {});
    bool hasListener(cmd::CommandType type) const;
private:
    std::unordered_map<cmd::CommandType, std::shared_ptr<CommandListener>> m_listeners;
}; 