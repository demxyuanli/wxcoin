#pragma once

#include "CommandListener.h"
#include "CommandType.h"
#include "CameraAnimation.h"
#include "ZoomController.h"
#include <wx/string.h>

class NavigationAnimationListener : public CommandListener {
public:
    NavigationAnimationListener();
    ~NavigationAnimationListener() override = default;

    void setCamera(SoCamera* camera) {
        m_camera = camera;
        // Set camera to NavigationAnimator for view animations
        if (m_camera) {
            NavigationAnimator::getInstance().setCamera(m_camera);
        }
    }

    CommandResult executeCommand(const std::string& commandType,
        const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "NavigationAnimationListener"; }

private:
    SoCamera* m_camera;

    void setAnimationType(CameraAnimation::AnimationType type);
    void animateToPosition(const SbVec3f& position, const SbRotation& rotation);
};

class ZoomControllerListener : public CommandListener {
public:
    ZoomControllerListener();
    ~ZoomControllerListener() override = default;

    void setCamera(SoCamera* camera, std::function<void()> viewRefreshCallback = nullptr) {
        m_camera = camera;
        m_viewRefreshCallback = viewRefreshCallback;
        // Set camera to ZoomManager's controller
        if (m_camera) {
            ZoomManager::getInstance().getController()->setCamera(m_camera);
            // Set view refresh callback if provided
            if (viewRefreshCallback) {
                ZoomManager::getInstance().setViewRefreshCallback(viewRefreshCallback);
            }
        }
    }

    CommandResult executeCommand(const std::string& commandType,
        const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "ZoomControllerListener"; }

private:
    SoCamera* m_camera;
    std::function<void()> m_viewRefreshCallback;

    void zoomIn();
    void zoomOut();
    void zoomReset();
    void zoomToLevel(size_t level);
    void showZoomSettings();
    void triggerViewRefresh();
};
