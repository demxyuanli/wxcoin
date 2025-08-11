#pragma once
#include "CommandListener.h"
class SceneManager;

class ChessboardGridToggleListener : public CommandListener {
public:
    explicit ChessboardGridToggleListener(SceneManager* sceneManager) : m_sceneManager(sceneManager) {}
    ~ChessboardGridToggleListener() override = default;

    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;

    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override;

private:
    SceneManager* m_sceneManager;
};
