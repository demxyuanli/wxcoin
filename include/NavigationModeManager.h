#pragma once

#include <memory>
#include <wx/event.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <Inventor/SbLinear.h>

class Canvas;
class SceneManager;
class NavigationController;
class InventorNavigationController;

enum class NavigationStyle {
    GESTURE = 0,
    INVENTOR = 1,
    CAD = 2,
    TOUCHPAD = 3,
    MAYA_GESTURE = 4,
    BLENDER = 5,
    REVIT = 6,
    TINKERCAD = 7
};

class INavigationStyle {
public:
    virtual ~INavigationStyle() = default;

    virtual void handleMouseButton(wxMouseEvent& event) = 0;
    virtual void handleMouseMotion(wxMouseEvent& event) = 0;
    virtual void handleMouseWheel(wxMouseEvent& event) = 0;

    virtual void viewAll() = 0;
    virtual void viewTop() = 0;
    virtual void viewFront() = 0;
    virtual void viewRight() = 0;
    virtual void viewIsometric() = 0;

    virtual void setZoomSpeedFactor(float factor) = 0;
    virtual float getZoomSpeedFactor() const = 0;

    virtual void setRotationCenter(const SbVec3f& center) {}
    virtual void clearRotationCenter() {}
    virtual bool hasRotationCenter() const { return false; }
    virtual const SbVec3f& getRotationCenter() const {
        static SbVec3f empty(0, 0, 0);
        return empty;
    }

    virtual std::string getStyleName() const = 0;
    virtual std::string getStyleDescription() const = 0;
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
    
    // Get current controller (backward compatibility)
    NavigationController* getCurrentController();
    InventorNavigationController* getInventorController();

    // New unified interface
    INavigationStyle* getCurrentNavigationStyle();
    std::string getCurrentStyleName() const;
    std::string getCurrentStyleDescription() const;
    std::vector<std::pair<NavigationStyle, std::string>> getAvailableStyles() const;

    // Configuration support
    void loadNavigationStyleFromConfig();
    void saveNavigationStyleToConfig() const;

private:
    Canvas* m_canvas;
    SceneManager* m_sceneManager;
    NavigationStyle m_currentStyle;
    
    std::unique_ptr<NavigationController> m_gestureController;
    std::unique_ptr<InventorNavigationController> m_inventorController;
    
    std::unordered_map<NavigationStyle, std::unique_ptr<INavigationStyle>> m_navigationStyles;
    
    void initializeControllers();
    void initializeNavigationStyles();
    INavigationStyle* getNavigationStyleFor(NavigationStyle style);
    const INavigationStyle* getNavigationStyleFor(NavigationStyle style) const;
};
