#include "FileNewListener.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "Command.h"
#include "logger/Logger.h"

FileNewListener::FileNewListener(Canvas* canvas, CommandManager* cmdMgr)
    : m_canvas(canvas), m_cmdMgr(cmdMgr) {}

CommandResult FileNewListener::executeCommand(const std::string& commandType,
                                              const std::unordered_map<std::string, std::string>& /*parameters*/) {
    if (!m_canvas) {
        return CommandResult(false, "Canvas not available", commandType);
    }

    auto sceneMgr = m_canvas->getSceneManager();
    if (!sceneMgr) {
        return CommandResult(false, "Scene manager not available", commandType);
    }

    sceneMgr->cleanup();
    sceneMgr->initScene();

    if (m_cmdMgr) {
        m_cmdMgr->clearHistory();
    }

    LOG_INF_S("New project created");
    return CommandResult(true, "New project created", commandType);
}

bool FileNewListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::FileNew);
} 
