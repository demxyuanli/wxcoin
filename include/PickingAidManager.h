#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/SbVec3f.h>

class Canvas;
class SceneManager;

class PickingAidManager {
public:
    PickingAidManager(SceneManager* sceneManager, Canvas* canvas);
    ~PickingAidManager();

    void createPickingAidLines();
    void showPickingAidLines(const SbVec3f& position);
    void hidePickingAidLines();
    
    // Enhanced picking methods
    void setReferenceZ(float z) { m_referenceZ = z; }
    float getReferenceZ() const { return m_referenceZ; }
    void showReferenceGrid(bool show);
    void updatePickingAidColor(const SbVec3f& color);

private:
    SceneManager* m_sceneManager;
    Canvas* m_canvas;
    SoSeparator* m_pickingAidSeparator;
    SoTransform* m_pickingAidTransform;
    bool m_pickingAidVisible;
    
    // Enhanced picking support
    float m_referenceZ;
    SoSeparator* m_referenceGridSeparator;
    bool m_referenceGridVisible;
    
    void createReferenceGrid();
};