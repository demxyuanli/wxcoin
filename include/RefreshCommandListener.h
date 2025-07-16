#pragma once

#include "CommandListener.h"
#include "RefreshCommand.h"
#include "CommandType.h"
#include <memory>

class Canvas;
class OCCViewer;
class SceneManager;

/**
 * @brief Listener for handling refresh commands through the command dispatcher
 */
class RefreshCommandListener : public CommandListener {
public:
    RefreshCommandListener(Canvas* canvas = nullptr, 
                          OCCViewer* occViewer = nullptr, 
                          SceneManager* sceneManager = nullptr);
    virtual ~RefreshCommandListener() = default;

    // CommandListener interface
    CommandResult executeCommand(const std::string& commandType, 
                                const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override;

    // Setters for dependency injection
    void setCanvas(Canvas* canvas) { m_canvas = canvas; }
    void setOCCViewer(OCCViewer* occViewer) { m_occViewer = occViewer; }
    void setSceneManager(SceneManager* sceneManager) { m_sceneManager = sceneManager; }

private:
    Canvas* m_canvas;
    OCCViewer* m_occViewer;
    SceneManager* m_sceneManager;

    void executeRefreshCommand(std::shared_ptr<RefreshCommand> command);
}; 