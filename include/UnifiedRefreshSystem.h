#pragma once

#include "RefreshCommandListener.h"
#include "CommandDispatcher.h"
#include "ViewRefreshManager.h"
#include <memory>

class Canvas;
class OCCViewer;
class SceneManager;

/**
 * @brief Unified refresh system that integrates command-based refresh with existing systems
 */
class UnifiedRefreshSystem {
public:
    UnifiedRefreshSystem(Canvas* canvas, OCCViewer* occViewer, SceneManager* sceneManager);
    ~UnifiedRefreshSystem();

    // Initialize the system - registers listeners with command dispatcher
    void initialize(CommandDispatcher* commandDispatcher);

    // Shutdown the system - unregisters listeners
    void shutdown();

    // Update OCCViewer after it's been created
    void setOCCViewer(OCCViewer* occViewer);

    // Set Canvas, OCCViewer, and SceneManager after construction
    void setComponents(Canvas* canvas, OCCViewer* occViewer, SceneManager* sceneManager);
    void setCanvas(Canvas* canvas);
    void setSceneManager(SceneManager* sceneManager);

    // Convenience methods for triggering refreshes via commands
    void refreshView(const std::string& objectId = "", bool immediate = false);
    void refreshScene(const std::string& objectId = "", bool immediate = false);
    void refreshObject(const std::string& objectId = "", bool immediate = false);
    void refreshMaterial(const std::string& objectId = "", bool immediate = false);
    void refreshGeometry(const std::string& objectId = "", bool immediate = false);
    void refreshUI(const std::string& componentType = "", bool immediate = false);

    // Direct refresh methods (for backwards compatibility)
    void directRefreshView(ViewRefreshManager::RefreshReason reason = ViewRefreshManager::RefreshReason::MANUAL_REQUEST);
    void directRefreshAll();

    // Getters
    RefreshCommandListener* getRefreshListener() const { return m_refreshListener.get(); }
    bool isInitialized() const { return m_initialized; }

private:
    Canvas* m_canvas;
    OCCViewer* m_occViewer;
    SceneManager* m_sceneManager;
    CommandDispatcher* m_commandDispatcher;

    std::shared_ptr<RefreshCommandListener> m_refreshListener;
    bool m_initialized;

    // Helper method for creating command parameters
    std::unordered_map<std::string, std::string> createRefreshParams(
        const std::string& objectId = "",
        const std::string& componentType = "",
        bool immediate = false);
}; 