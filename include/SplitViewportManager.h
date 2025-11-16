#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <vector>
#include <memory>

class Canvas;
class SceneManager;

enum class SplitMode {
    SINGLE = 1,
    HORIZONTAL_2 = 2,
    VERTICAL_2 = 3,
    QUAD = 4,
    SIX = 6
};

struct SplitViewportInfo {
    int x, y, width, height;
    SoCamera* camera;
    SoSeparator* sceneRoot;
    bool isActive;
    int viewportIndex;
    
    SplitViewportInfo()
        : x(0), y(0), width(0), height(0)
        , camera(nullptr)
        , sceneRoot(nullptr)
        , isActive(false)
        , viewportIndex(0) {
    }
};

class SplitViewportManager {
public:
    SplitViewportManager(Canvas* canvas, SceneManager* sceneManager);
    ~SplitViewportManager();
    
    void setSplitMode(SplitMode mode);
    SplitMode getSplitMode() const { return m_currentMode; }
    
    void render();
    void handleSizeChange(const wxSize& canvasSize);
    bool handleMouseEvent(wxMouseEvent& event);
    
    void setActiveViewport(int index);
    int getActiveViewport() const { return m_activeViewportIndex; }
    
    void syncAllCamerasToMain();
    void setCameraSyncEnabled(bool enabled);
    bool isCameraSyncEnabled() const { return m_cameraSyncEnabled; }
    
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled);

private:
    void initializeViewports();
    void createViewportScenes();
    void updateViewportLayouts(const wxSize& canvasSize);
    
    void applySingleViewLayout(const wxSize& canvasSize);
    void applyHorizontal2Layout(const wxSize& canvasSize);
    void applyVertical2Layout(const wxSize& canvasSize);
    void applyQuadLayout(const wxSize& canvasSize);
    void applySixViewLayout(const wxSize& canvasSize);
    
    void renderViewport(const SplitViewportInfo& viewport);
    void setViewport(const SplitViewportInfo& viewport);
    void syncCameraToMain(SoCamera* targetCamera);
    void syncMainCameraToViewport(int viewportIndex);
    void drawViewportBackground(const SplitViewportInfo& viewport,
                                const double topColor[3],
                                const double bottomColor[3]);
    
    void drawViewportBorders();
    void drawBorder(int x, int y, int width, int height, bool isActive);
    
    int findViewportAtPosition(const wxPoint& pos);
    
    Canvas* m_canvas;
    SceneManager* m_sceneManager;
    
    SplitMode m_currentMode;
    std::vector<SplitViewportInfo> m_viewports;
    int m_activeViewportIndex;
    
    bool m_enabled;
    float m_dpiScale;
    int m_borderWidth;
    bool m_cameraSyncEnabled;
    
    wxSize m_lastCanvasSize;
};

