#pragma once

#include <memory>
#include <wx/gdicmn.h>

class Canvas;
class SceneManager;
class wxMouseEvent;
class CuteNavCube;
class wxColour;

class NavigationCubeManager {
public:
    NavigationCubeManager(Canvas* canvas, SceneManager* sceneManager);
    ~NavigationCubeManager();

    void render();
    bool handleMouseEvent(wxMouseEvent& event);
    void handleSizeChange();
    void handleDPIChange();

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void showConfigDialog();
    
    void setRect(int x, int y, int size);
    void setColor(const wxColour& color);
    void setViewportSize(int size);

    void syncMainCameraToCube();
    void syncCubeCameraToMain();

private:
    void initCube();
    
    struct Layout {
        int x{ 20 }, y{ 20 }, size{ 200 };
        void update(int newX_logical, int newY_logical, int newSize_logical,
            const wxSize& windowSize_logical, float dpiScale);
    } m_cubeLayout;

    Canvas* m_canvas;
    SceneManager* m_sceneManager;

    std::unique_ptr<CuteNavCube> m_navCube;
    bool m_isEnabled;
    float m_marginx = 40.0f;
    float m_marginy = 40.0f;
}; 