#include "ReferenceGridToggleListener.h"
#include "SceneManager.h"
#include "PickingAidManager.h"
#include "CommandType.h"

// Reference grid toggle (uses PickingAidManager)
CommandResult ReferenceGridToggleListener::executeCommand(const std::string& commandType,
                                                         const std::unordered_map<std::string, std::string>&) {
    if (!m_sceneManager) {
        return CommandResult(false, "SceneManager not available", commandType);
    }
    auto pam = m_sceneManager->getPickingAidManager();
    if (!pam) {
        return CommandResult(false, "PickingAidManager not available", commandType);
    }
    bool newState = !pam->isReferenceGridVisible();
    pam->showReferenceGrid(newState);
    return CommandResult(true, newState ? "Reference grid shown" : "Reference grid hidden", commandType);
}

bool ReferenceGridToggleListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::ToggleReferenceGrid);
}

std::string ReferenceGridToggleListener::getListenerName() const {
    return "ReferenceGridToggleListener";
}

// (chessboard handler moved to ChessboardGridToggleListener.cpp)
