#pragma once
#include "CommandListener.h"
class SceneManager;

class ReferenceGridToggleListener : public CommandListener {
public:
    explicit ReferenceGridToggleListener(SceneManager* sceneManager) : m_sceneManager(sceneManager) {}
    ~ReferenceGridToggleListener() override = default;

    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;

    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override;

private:
    SceneManager* m_sceneManager;
};
