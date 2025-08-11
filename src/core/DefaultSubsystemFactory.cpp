#include "interfaces/DefaultSubsystemFactory.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "InputManager.h"
#include "NavigationCubeManager.h"
#include "RenderingEngine.h"
#include "EventCoordinator.h"
#include "ViewportManager.h"

SceneManager* DefaultSubsystemFactory::createSceneManager(Canvas* canvas) { return new SceneManager(canvas); }
InputManager* DefaultSubsystemFactory::createInputManager(Canvas* canvas) { return new InputManager(canvas); }
NavigationCubeManager* DefaultSubsystemFactory::createNavigationCubeManager(Canvas* canvas, SceneManager* sceneManager) { return new NavigationCubeManager(canvas, sceneManager); }
RenderingEngine* DefaultSubsystemFactory::createRenderingEngine(Canvas* canvas) { return new RenderingEngine(canvas); }
EventCoordinator* DefaultSubsystemFactory::createEventCoordinator() { return new EventCoordinator(); }
ViewportManager* DefaultSubsystemFactory::createViewportManager(Canvas* canvas) { return new ViewportManager(canvas); }


