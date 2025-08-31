#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCamera.h>

class wxGLCanvas;

// Abstract interface for outline rendering context
class IOutlineRenderer {
public:
    virtual ~IOutlineRenderer() = default;
    
    // Get the OpenGL canvas for rendering
    virtual wxGLCanvas* getGLCanvas() const = 0;
    
    // Get the camera node
    virtual SoCamera* getCamera() const = 0;
    
    // Get the scene root to attach overlay nodes
    virtual SoSeparator* getSceneRoot() const = 0;
    
    // Request a redraw
    virtual void requestRedraw() = 0;
};