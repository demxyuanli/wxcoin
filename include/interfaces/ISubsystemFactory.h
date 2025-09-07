#pragma once

class Canvas;
class SceneManager;
class InputManager;
class NavigationCubeManager;
class RenderingEngine;
class EventCoordinator;
class ViewportManager;

class ISubsystemFactory {
public:
    virtual ~ISubsystemFactory() = default;

    virtual SceneManager* createSceneManager(Canvas* canvas) = 0;
    virtual InputManager* createInputManager(Canvas* canvas) = 0;
    virtual NavigationCubeManager* createNavigationCubeManager(Canvas* canvas, SceneManager* sceneManager) = 0;
    virtual RenderingEngine* createRenderingEngine(Canvas* canvas) = 0;
    virtual EventCoordinator* createEventCoordinator() = 0;
    virtual ViewportManager* createViewportManager(Canvas* canvas) = 0;
};


