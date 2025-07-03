#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCamera.h>
#include <wx/frame.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <memory>

class Canvas;
class CoordinateSystemRenderer;
class PickingAidManager;
class NavigationCube;

class SceneManager {
public:
    SceneManager(Canvas* canvas);
    ~SceneManager();

    bool initScene();
    void cleanup();
    void resetView();
    void toggleCameraMode();
    void setView(const std::string& viewName); // New method for view switching
    void render(const wxSize& size, bool fastMode);
    void updateAspectRatio(const wxSize& size);
    bool screenToWorld(const wxPoint& screenPos, SbVec3f& worldPos);

    SoSeparator* getObjectRoot() const { return m_objectRoot; }
    SoCamera* getCamera() const { return m_camera; }
    PickingAidManager* getPickingAidManager() const { return m_pickingAidManager.get(); }

private:
    Canvas* m_canvas;
    SoSeparator* m_sceneRoot;
    SoCamera* m_camera;
    SoDirectionalLight* m_light;
    SoSeparator* m_objectRoot;
    std::unique_ptr<CoordinateSystemRenderer> m_coordSystemRenderer;
    std::unique_ptr<PickingAidManager> m_pickingAidManager;
    bool m_isPerspectiveCamera;
};