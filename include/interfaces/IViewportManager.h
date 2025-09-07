#pragma once

class wxSize;
class wxGLCanvas;
class NavigationCubeManager;

class IViewportManager {
public:
    virtual ~IViewportManager() = default;
    virtual void handleSizeChange(const wxSize& size) = 0;
    virtual void updateDPISettings() = 0;
    virtual void applyDPIScalingToUI() = 0;
    virtual float getDPIScale() const = 0;
    virtual void setNavigationCubeManager(NavigationCubeManager* navCubeManager) = 0;
};


