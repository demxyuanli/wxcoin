#pragma once

#include <memory>
#include <wx/event.h>

class Canvas;
class SceneManager;
class NavigationController;
class InventorNavigationController;

enum class NavigationStyle {
    GESTURE = 0,
    INVENTOR = 1
};

class NavigationModeManager {
public:
    NavigationModeManager(Canvas* canvas, SceneManager* sceneManager);
    ~NavigationModeManager();

    // Navigation mode control
    void setNavigationStyle(NavigationStyle style);
    NavigationStyle getNavigationStyle() const;
    
    // Event handling delegation
    void handleMouseButton(wxMouseEvent& event);
    void handleMouseMotion(wxMouseEvent& event);
    void handleMouseWheel(wxMouseEvent& event);

    // View operations
    void viewAll();
    void viewTop();
    void viewFront();
    void viewRight();
    void viewIsometric();

    // Zoom speed control
    void setZoomSpeedFactor(float factor);
    float getZoomSpeedFactor() const;
    
    // Get current controller
    NavigationController* getCurrentController();
    InventorNavigationController* getInventorController();

private:
    Canvas* m_canvas;
    SceneManager* m_sceneManager;
    NavigationStyle m_currentStyle;
    
    std::unique_ptr<NavigationController> m_gestureController;
    std::unique_ptr<InventorNavigationController> m_inventorController;
    
    void initializeControllers();
};
