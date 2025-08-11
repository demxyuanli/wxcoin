#pragma once

#include "interfaces/ISubsystemFactory.h"

class DefaultSubsystemFactory final : public ISubsystemFactory {
public:
    DefaultSubsystemFactory() = default;
    ~DefaultSubsystemFactory() override = default;

    SceneManager* createSceneManager(Canvas* canvas) override;
    InputManager* createInputManager(Canvas* canvas) override;
    NavigationCubeManager* createNavigationCubeManager(Canvas* canvas, SceneManager* sceneManager) override;
    RenderingEngine* createRenderingEngine(Canvas* canvas) override;
    EventCoordinator* createEventCoordinator() override;
    ViewportManager* createViewportManager(Canvas* canvas) override;
};


