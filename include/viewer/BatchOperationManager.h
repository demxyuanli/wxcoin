#pragma once

#include <memory>

class SceneManager;
class ObjectTreeSync;
class ViewUpdateService;

class BatchOperationManager {
public:
    BatchOperationManager(SceneManager* sceneManager,
                          ObjectTreeSync* objectTree,
                          ViewUpdateService* viewUpdater);

    void begin();
    void end();
    bool isActive() const { return m_active; }
    void markNeedsViewRefresh() { m_needsViewRefresh = true; }

private:
    SceneManager* m_sceneManager;
    ObjectTreeSync* m_objectTree;
    ViewUpdateService* m_viewUpdater;

    bool m_active{false};
    bool m_needsViewRefresh{false};
};



