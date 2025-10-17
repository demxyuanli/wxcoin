#include "NavigationAnimationListener.h"
#include "CameraAnimation.h"
#include "ZoomController.h"
#include <wx/msgdlg.h>
#include <wx/choice.h>

//==============================================================================
// NavigationAnimationListener Implementation
//==============================================================================

NavigationAnimationListener::NavigationAnimationListener()
    : m_camera(nullptr)
{
}

CommandResult NavigationAnimationListener::executeCommand(const std::string& commandType,
    const std::unordered_map<std::string, std::string>& parameters) {

    cmd::CommandType cmdType = cmd::from_string(commandType);

    if (!m_camera) {
        return CommandResult(false, "No camera available for animation operations");
    }

    switch (cmdType) {
        case cmd::CommandType::AnimationTypeLinear:
            setAnimationType(CameraAnimation::AnimationType::LINEAR);
            return CommandResult(true, "Animation type set to Linear");

        case cmd::CommandType::AnimationTypeSmooth:
            setAnimationType(CameraAnimation::AnimationType::SMOOTH);
            return CommandResult(true, "Animation type set to Smooth");

        case cmd::CommandType::AnimationTypeEaseIn:
            setAnimationType(CameraAnimation::AnimationType::EASE_IN);
            return CommandResult(true, "Animation type set to Ease-In");

        case cmd::CommandType::AnimationTypeEaseOut:
            setAnimationType(CameraAnimation::AnimationType::EASE_OUT);
            return CommandResult(true, "Animation type set to Ease-Out");

        case cmd::CommandType::AnimationTypeBounce:
            setAnimationType(CameraAnimation::AnimationType::BOUNCE);
            return CommandResult(true, "Animation type set to Bounce");

        default:
            return CommandResult(false, "Unknown animation command: " + commandType);
    }
}

bool NavigationAnimationListener::canHandleCommand(const std::string& commandType) const {
    cmd::CommandType cmdType = cmd::from_string(commandType);
    return cmdType == cmd::CommandType::AnimationTypeLinear ||
           cmdType == cmd::CommandType::AnimationTypeSmooth ||
           cmdType == cmd::CommandType::AnimationTypeEaseIn ||
           cmdType == cmd::CommandType::AnimationTypeEaseOut ||
           cmdType == cmd::CommandType::AnimationTypeBounce;
}

void NavigationAnimationListener::setAnimationType(CameraAnimation::AnimationType type) {
    NavigationAnimator::getInstance().setAnimationType(type);

    wxString typeName;
    switch (type) {
        case CameraAnimation::LINEAR: typeName = "Linear"; break;
        case CameraAnimation::SMOOTH: typeName = "Smooth"; break;
        case CameraAnimation::EASE_IN: typeName = "Ease In"; break;
        case CameraAnimation::EASE_OUT: typeName = "Ease Out"; break;
        case CameraAnimation::BOUNCE: typeName = "Bounce"; break;
        default: typeName = "Unknown"; break;
    }

    wxMessageBox("Animation type set to: " + typeName, "Animation Settings", wxOK | wxICON_INFORMATION);
}

void NavigationAnimationListener::animateToPosition(const SbVec3f& position, const SbRotation& rotation) {
    NavigationAnimator::getInstance().animateToPosition(position, rotation, 1.5f);
}

//==============================================================================
// ZoomControllerListener Implementation
//==============================================================================

ZoomControllerListener::ZoomControllerListener()
    : m_camera(nullptr)
{
}

CommandResult ZoomControllerListener::executeCommand(const std::string& commandType,
    const std::unordered_map<std::string, std::string>& parameters) {

    cmd::CommandType cmdType = cmd::from_string(commandType);

    if (!m_camera) {
        return CommandResult(false, "No camera available for zoom operations");
    }

    switch (cmdType) {
        case cmd::CommandType::ZoomIn:
            zoomIn();
            // Trigger view refresh
            triggerViewRefresh();
            return CommandResult(true, "Zoom in executed");

        case cmd::CommandType::ZoomOut:
            zoomOut();
            // Trigger view refresh
            triggerViewRefresh();
            return CommandResult(true, "Zoom out executed");

        case cmd::CommandType::ZoomReset:
            zoomReset();
            // Trigger view refresh
            triggerViewRefresh();
            return CommandResult(true, "Zoom reset to 100%");

        case cmd::CommandType::ZoomToFit:
            // This would typically be handled by existing ViewAll command
            return CommandResult(false, "Zoom to fit not implemented");

        case cmd::CommandType::ZoomSettings:
            showZoomSettings();
            return CommandResult(true, "Zoom settings dialog opened");

        case cmd::CommandType::ZoomLevel25:
            zoomToLevel(1); // Usually 25%
            return CommandResult(true, "Zoom set to 25%");

        case cmd::CommandType::ZoomLevel50:
            zoomToLevel(2); // Usually 50%
            return CommandResult(true, "Zoom set to 50%");

        case cmd::CommandType::ZoomLevel100:
            zoomToLevel(4); // Usually 100%
            return CommandResult(true, "Zoom set to 100%");

        case cmd::CommandType::ZoomLevel200:
            zoomToLevel(5); // Usually 200%
            return CommandResult(true, "Zoom set to 200%");

        case cmd::CommandType::ZoomLevel400:
            zoomToLevel(6); // Usually 400%
            return CommandResult(true, "Zoom set to 400%");

        default:
            return CommandResult(false, "Unknown zoom command: " + commandType);
    }
}

bool ZoomControllerListener::canHandleCommand(const std::string& commandType) const {
    cmd::CommandType cmdType = cmd::from_string(commandType);
    return cmdType == cmd::CommandType::ZoomIn ||
           cmdType == cmd::CommandType::ZoomOut ||
           cmdType == cmd::CommandType::ZoomReset ||
           cmdType == cmd::CommandType::ZoomToFit ||
           cmdType == cmd::CommandType::ZoomSettings ||
           cmdType == cmd::CommandType::ZoomLevel25 ||
           cmdType == cmd::CommandType::ZoomLevel50 ||
           cmdType == cmd::CommandType::ZoomLevel100 ||
           cmdType == cmd::CommandType::ZoomLevel200 ||
           cmdType == cmd::CommandType::ZoomLevel400;
}

void ZoomControllerListener::zoomIn() {
    ZoomManager::getInstance().zoomIn();
}

void ZoomControllerListener::zoomOut() {
    ZoomManager::getInstance().zoomOut();
}

void ZoomControllerListener::zoomReset() {
    ZoomManager::getInstance().zoomReset();
}

void ZoomControllerListener::zoomToLevel(size_t level) {
    ZoomManager::getInstance().zoomToLevel(level);
}

void ZoomControllerListener::showZoomSettings() {
    wxArrayString choices;
    choices.Add("Continuous - Smooth zoom with mouse");
    choices.Add("Discrete - Snap to predefined levels");
    choices.Add("Hybrid - Levels with continuous hints");

    wxString selection = wxGetSingleChoice("Select zoom mode:",
                                          "Zoom Settings", choices);

    auto controller = ZoomManager::getInstance().getController();
    if (controller) {
        if (selection.Contains("Continuous")) {
            controller->setZoomMode(ZoomController::CONTINUOUS);
        } else if (selection.Contains("Discrete")) {
            controller->setZoomMode(ZoomController::DISCRETE);
        } else if (selection.Contains("Hybrid")) {
            controller->setZoomMode(ZoomController::HYBRID);
        }
    }
}

void ZoomControllerListener::triggerViewRefresh() {
    if (m_viewRefreshCallback) {
        m_viewRefreshCallback();
    }
}
