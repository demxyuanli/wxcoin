#pragma once

#include <wx/glcanvas.h>
#include <wx/gdicmn.h>


#include <Inventor/nodes/SoSeparator.h>


#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoMaterial.h>




#include <Inventor/actions/SoRayPickAction.h>

#include <Inventor/nodes/SoSphere.h>


#include <Inventor/SbName.h>

// Forward declarations
class SoPickedPoint;
class SoPath;

class Canvas;
class SceneManager;
class NavigationCubeManager;

struct ViewportInfo {
    int x, y, width, height;
    bool enabled;
    
    ViewportInfo() : x(0), y(0), width(100), height(100), enabled(true) {}
    ViewportInfo(int _x, int _y, int _w, int _h, bool _enabled = true) 
        : x(_x), y(_y), width(_w), height(_h), enabled(_enabled) {}
};

class MultiViewportManager {
public:
    enum ViewportType {
        VIEWPORT_NAVIGATION_CUBE = 0,
        VIEWPORT_CUBE_OUTLINE = 1,
        VIEWPORT_COORDINATE_SYSTEM = 2,
        VIEWPORT_COUNT = 3
    };

public:
    MultiViewportManager(Canvas* canvas, SceneManager* sceneManager);
    ~MultiViewportManager();

    // Core functionality
    void render();
    void handleSizeChange(const wxSize& canvasSize);
    void handleDPIChange();
    bool handleMouseEvent(wxMouseEvent& event);

    // Viewport management
    void setViewportEnabled(ViewportType type, bool enabled);
    bool isViewportEnabled(ViewportType type) const;
    void setViewportRect(ViewportType type, int x, int y, int width, int height);
    ViewportInfo getViewportInfo(ViewportType type) const;

    // Navigation cube integration
    void setNavigationCubeManager(NavigationCubeManager* manager);

private:
    void syncCoordinateSystemCameraToMain();
    void initializeViewports();
    void createCubeOutlineScene();
    void createCoordinateSystemScene();
    void updateViewportLayouts(const wxSize& canvasSize);
    
    void renderNavigationCube();
    void renderCubeOutline();
    void renderCoordinateSystem();
    



    // Shape creation methods
    void createTopRightCircle(float scale);
    void createSmallCube(float scale);
    void createCurvedArrow(int dir, float scale);
    void createEquilateralTriangle(float x, float y, float angleRad);
    
    // Event handling methods
    std::string findShapeNameFromPath(SoPath* path);
    
    Canvas* m_canvas;
    SceneManager* m_sceneManager;
    NavigationCubeManager* m_navigationCubeManager;
    
    ViewportInfo m_viewports[VIEWPORT_COUNT];
    
    // Scene graphs for additional viewports
    SoSeparator* m_cubeOutlineRoot;
    SoSeparator* m_coordinateSystemRoot;
    
    // Cameras for additional viewports
    SoOrthographicCamera* m_cubeOutlineCamera;
    SoOrthographicCamera* m_coordinateSystemCamera;
    
    // Layout parameters
    int m_margin;
    float m_dpiScale;
    bool m_initialized;  // Add this flag
    
    // Composite shape management
    struct CompositeShape {
        SoSeparator* rootNode;
        std::string shapeName;
        
        CompositeShape(SoSeparator* root, const std::string& name) 
            : rootNode(root), shapeName(name) {}
    };
    
    std::vector<CompositeShape> m_compositeShapes;
};