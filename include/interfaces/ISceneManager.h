#pragma once

#include <wx/gdicmn.h> // wxSize
class SoSeparator;
class SoCamera;

class ISceneManager {
public:
    virtual ~ISceneManager() = default;

    virtual bool initScene() = 0;
    virtual void render(const wxSize& size, bool fastMode) = 0;
    virtual void resetView(bool animate = false) = 0;
    virtual void updateAspectRatio(const wxSize& size) = 0;

    virtual SoSeparator* getObjectRoot() const = 0;
    virtual SoCamera* getCamera() const = 0;
};


