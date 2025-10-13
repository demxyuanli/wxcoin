#pragma once

#include <memory>
#include <vector>
#include <OpenCASCADE/Quantity_Color.hxx>

class SceneManager;
class SelectionManager;
class ViewUpdateService;
class ObjectTreeSync;

/**
 * @brief Service for managing view-level operations and batch commands
 *
 * This service encapsulates all view-related operations including fitting,
 * selection management, visibility control, and batch operations.
 */
class ViewOperationsService {
public:
    ViewOperationsService();
    ~ViewOperationsService();

    // View fitting operations
    void fitAll(SceneManager* sceneManager, ViewUpdateService* viewUpdater);

    // Visibility operations
    void hideAll(SelectionManager* selectionManager);
    void showAll(SelectionManager* selectionManager);

    // Selection operations
    void selectAll(SelectionManager* selectionManager);
    void deselectAll(SelectionManager* selectionManager);

    // Color operations
    void setAllColor(const Quantity_Color& color, const std::vector<std::shared_ptr<class OCCGeometry>>& geometries);

    // View refresh operations
    void requestViewRefresh(SceneManager* sceneManager, ViewUpdateService* viewUpdater);
    void updateObjectTreeDeferred(ObjectTreeSync* objectTreeSync);

    // Batch mode support
    void setBatchMode(bool batchMode) { m_batchMode = batchMode; }
    bool isBatchMode() const { return m_batchMode; }

private:
    bool m_batchMode;

    // Helper methods
    void updateSceneBounds(SceneManager* sceneManager);
    void resetView(SceneManager* sceneManager, ViewUpdateService* viewUpdater);
    void refreshCanvas(SceneManager* sceneManager);
};
