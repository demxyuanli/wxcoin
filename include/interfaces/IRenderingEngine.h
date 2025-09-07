#pragma once

#include <wx/gdicmn.h>

class ISceneManager;

class IRenderingEngine {
public:
    virtual ~IRenderingEngine() = default;

    virtual bool initialize() = 0;
    virtual void render(bool fastMode = false) = 0;
    virtual void renderWithoutSwap(bool fastMode = false) = 0;
    virtual void swapBuffers() = 0;
    virtual void handleResize(const wxSize& size) = 0;
    virtual void updateViewport(const wxSize& size, float dpiScale) = 0;

    virtual void setSceneManager(ISceneManager* sceneManager) = 0;
};


