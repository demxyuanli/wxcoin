#pragma once

#include <memory>
#include <unordered_map>
#include <wx/gdicmn.h>

class OCCGeometry;
class SceneManager;
class SoSeparator;

// Service that performs screen-space picking and resolves to top-level geometries
class PickingService {
public:
    PickingService(SceneManager* sceneManager,
                   SoSeparator* occRoot,
                   const std::unordered_map<SoSeparator*, std::shared_ptr<OCCGeometry>>* nodeToGeom);

    void setRoot(SoSeparator* occRoot);

    std::shared_ptr<OCCGeometry> pickGeometryAtScreen(const wxPoint& screenPos) const;

private:
    static SoSeparator* findTopLevelSeparatorInPath(class SoPath* path, SoSeparator* occRoot);

private:
    SceneManager* m_sceneManager{nullptr};
    SoSeparator* m_occRoot{nullptr};
    const std::unordered_map<SoSeparator*, std::shared_ptr<OCCGeometry>>* m_nodeToGeom{nullptr};
};


