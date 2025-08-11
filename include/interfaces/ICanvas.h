#pragma once

class IRenderingEngine;
class ISceneManager;

class ICanvas {
public:
    virtual ~ICanvas() = default;
    virtual void render(bool fastMode = false) = 0;
    virtual IRenderingEngine* getRenderingEngineInterface() const = 0;
    virtual ISceneManager* getSceneManagerInterface() const = 0;
};


