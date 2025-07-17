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
    Canvas* getCanvas() const { return m_canvas; }
    bool initScene();
    void initializeScene();  // Add missing method declaration
    void cleanup();
    void resetView();
    void viewAll();
    void toggleCameraMode();
    void setView(const std::string& viewName);
    void render(const wxSize& size, bool fastMode);
    void updateAspectRatio(const wxSize& size);
    bool screenToWorld(const wxPoint& screenPos, SbVec3f& worldPos);

    SoSeparator* getObjectRoot() const { return m_objectRoot; }
    SoCamera* getCamera() const { return m_camera; }
    PickingAidManager* getPickingAidManager() const { return m_pickingAidManager.get(); }

    // Scene bounds and coordinate system management
    void updateSceneBounds();
    float getSceneBoundingBoxSize() const;
    void updateCoordinateSystemScale();
    void getSceneBoundingBoxMinMax(SbVec3f& min, SbVec3f& max) const;

private:
    Canvas* m_canvas;
    SoSeparator* m_sceneRoot;
    SoCamera* m_camera;
    SoDirectionalLight* m_light;
    SoSeparator* m_lightRoot;  // Add missing member variable
    SoSeparator* m_objectRoot;
    std::unique_ptr<CoordinateSystemRenderer> m_coordSystemRenderer;
    std::unique_ptr<PickingAidManager> m_pickingAidManager;
    bool m_isPerspectiveCamera;
    SbBox3f m_sceneBoundingBox;
};