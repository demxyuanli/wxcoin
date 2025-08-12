#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/SbVec3f.h>

class Canvas;
class SceneManager;
class InputManager;

class PickingAidManager {
public:
    PickingAidManager(SceneManager* sceneManager, Canvas* canvas, InputManager* inputManager);
    ~PickingAidManager();

    void update();
    void show();
    void hide();
    void clear();

    void startPicking();
    void stopPicking();
    bool isPicking() const;

    void createPickingAidLines();
    void showPickingAidLines(const SbVec3f& position);
    void hidePickingAidLines();
    
    // Enhanced picking methods
    void setReferenceZ(float z) { m_referenceZ = z; }
    float getReferenceZ() const { return m_referenceZ; }
    void showReferenceGrid(bool show);
    void updatePickingAidColor(const SbVec3f& color);
    void updateReferenceGrid();
    bool isReferenceGridVisible() const { return m_referenceGridVisible; }
    void setReferenceGridDynamicScaling(bool enable) { m_dynamicGridScaling = enable; }
    bool isReferenceGridDynamicScaling() const { return m_dynamicGridScaling; }
    void setReferenceGridScale(float s) { m_referenceGridScale = s; }
    float getReferenceGridScale() const { return m_referenceGridScale; }

private:
    void createPickingAids();
    void updatePickingAids();

    SceneManager* m_sceneManager;
    Canvas* m_canvas;
    InputManager* m_inputManager;
    SoSeparator* m_aidsRoot;
    bool m_isPickingPosition;
    SoSeparator* m_pickingAidSeparator;
    SoTransform* m_pickingAidTransform;
    bool m_pickingAidVisible;
    
    // Enhanced picking support
    float m_referenceZ;
    float m_referenceGridScale{1.0f};
    bool m_dynamicGridScaling{false};
    SoSeparator* m_referenceGridSeparator;
    bool m_referenceGridVisible;
    SoTransform* m_referenceGridTransform{nullptr};
    
    void createReferenceGrid();
};