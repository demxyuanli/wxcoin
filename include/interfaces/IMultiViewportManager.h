#pragma once

class wxSize;
class wxMouseEvent;
class NavigationCubeManager;

class IMultiViewportManager {
public:
    virtual ~IMultiViewportManager() = default;
    virtual void render() = 0;
    virtual void handleSizeChange(const wxSize& canvasSize) = 0;
    virtual void handleDPIChange() = 0;
    virtual bool handleMouseEvent(wxMouseEvent& event) = 0;
    virtual void setNavigationCubeManager(NavigationCubeManager* manager) = 0;
};


