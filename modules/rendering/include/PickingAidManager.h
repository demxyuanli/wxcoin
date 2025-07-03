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

private:
    SceneManager* m_sceneManager;
    Canvas* m_canvas;
    SoSeparator* m_pickingAidSeparator;
    SoTransform* m_pickingAidTransform;
    bool m_pickingAidVisible;
};