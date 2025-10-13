#pragma once

#include <memory>
#include <vector>
#include <string>
#include <OpenCASCADE/Quantity_Color.hxx>
#include "OCCGeometry.h"

class SceneManager;
class GeometryRepository;
class SceneAttachmentService;
class ObjectTreeSync;
class SelectionManager;
class ViewUpdateService;

/**
 * @brief Service for managing geometry objects lifecycle and properties
 *
 * This service encapsulates all geometry management operations including
 * adding, removing, finding geometries and controlling their visual properties.
 */
class GeometryManagementService {
public:
    GeometryManagementService(
        SceneManager* sceneManager,
        std::vector<std::shared_ptr<OCCGeometry>>* geometries,
        std::vector<std::shared_ptr<OCCGeometry>>* selectedGeometries
    );
    ~GeometryManagementService();

    // Geometry lifecycle management
    bool addGeometry(std::shared_ptr<OCCGeometry> geometry, bool batchMode = false);
    bool removeGeometry(std::shared_ptr<OCCGeometry> geometry);
    bool removeGeometry(const std::string& name);
    void clearAll();

    // Geometry lookup
    std::shared_ptr<OCCGeometry> findGeometry(const std::string& name) const;
    const std::vector<std::shared_ptr<OCCGeometry>>& getAllGeometries() const;
    const std::vector<std::shared_ptr<OCCGeometry>>& getSelectedGeometries() const;

    // Geometry property control
    void setGeometryVisible(const std::string& name, bool visible);
    void setGeometrySelected(const std::string& name, bool selected);
    void setGeometryColor(const std::string& name, const Quantity_Color& color);
    void setGeometryTransparency(const std::string& name, double transparency);

    // Batch operations
    void addGeometriesBatch(const std::vector<std::shared_ptr<OCCGeometry>>& geometries);

    // Service dependencies (injected)
    void setServices(
        GeometryRepository* repo,
        SceneAttachmentService* attach,
        ObjectTreeSync* treeSync,
        SelectionManager* selectionMgr,
        ViewUpdateService* viewUpdater
    );

    void setBatchMode(bool batchMode) { m_batchMode = batchMode; }

private:
    SceneManager* m_sceneManager;
    std::vector<std::shared_ptr<OCCGeometry>>* m_geometries;
    std::vector<std::shared_ptr<OCCGeometry>>* m_selectedGeometries;

    // Service dependencies
    GeometryRepository* m_geometryRepo;
    SceneAttachmentService* m_sceneAttach;
    ObjectTreeSync* m_objectTreeSync;
    SelectionManager* m_selectionManager;
    ViewUpdateService* m_viewUpdater;

    bool m_batchMode;

    // Helper methods
    void attachGeometryToScene(std::shared_ptr<OCCGeometry> geometry);
    void detachGeometryFromScene(std::shared_ptr<OCCGeometry> geometry);
    void updateGeometryInTree(std::shared_ptr<OCCGeometry> geometry);
    void rebuildSelectionAccelerator();
};
