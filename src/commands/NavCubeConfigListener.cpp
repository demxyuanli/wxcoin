#include "NavCubeConfigListener.h"
#include "Canvas.h"
#include "Logger.h"

NavCubeConfigListener::NavCubeConfigListener(Canvas* canvas) : m_canvas(canvas) {}

CommandResult NavCubeConfigListener::executeCommand(const std::string& commandType,
                                                    const std::unordered_map<std::string, std::string>&) {
    if (!m_canvas) return CommandResult(false, "Canvas not available", commandType);
    m_canvas->ShowNavigationCubeConfigDialog();
    return CommandResult(true, "Navigation cube configuration opened", commandType);
}

bool NavCubeConfigListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::NavCubeConfig);
} 