#pragma once

#include "Canvas.h"
#include <memory>
#include <Inventor/nodes/SoCamera.h>

// Forward declarations
class SceneManager;
class RenderingEngine;
class OCCViewer;

class RenderPreviewCanvas : public Canvas
{
public:
    RenderPreviewCanvas(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);
    virtual ~RenderPreviewCanvas();

    void render(bool fastMode = false);
    void resetView();

private:
    void createDefaultPreviewScene();
    void setLightGreenBackground();
    void createCheckerboardPlane();
    void createBasicGeometryObjects();
    void setupDefaultCamera();

    // Event handlers
    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onEraseBackground(wxEraseEvent& event);

    DECLARE_EVENT_TABLE()
};