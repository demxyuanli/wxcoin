#include "ChessboardGridToggleListener.h"
#include "SceneManager.h"
#include "CommandType.h"

CommandResult ChessboardGridToggleListener::executeCommand(const std::string& commandType,
                                                         const std::unordered_map<std::string, std::string>&) {
    if (!m_sceneManager) {
        return CommandResult(false, "SceneManager not available", commandType);
    }
    bool newState = !m_sceneManager->isCheckerboardVisible();
    m_sceneManager->setCheckerboardVisible(newState);
    return CommandResult(true, newState ? "Chessboard grid shown" : "Chessboard grid hidden", commandType);
}

bool ChessboardGridToggleListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ToggleChessboardGrid);
}

std::string ChessboardGridToggleListener::getListenerName() const {
    return "ChessboardGridToggleListener";
}
