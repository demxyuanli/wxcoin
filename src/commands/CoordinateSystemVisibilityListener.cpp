#include "CoordinateSystemVisibilityListener.h"
#include "SceneManager.h"
#include "Canvas.h"

CoordinateSystemVisibilityListener::CoordinateSystemVisibilityListener(wxFrame* frame, SceneManager* sceneManager)
    : m_frame(frame)
    , m_sceneManager(sceneManager)
{
}

CoordinateSystemVisibilityListener::~CoordinateSystemVisibilityListener()
{
}

CommandResult CoordinateSystemVisibilityListener::executeCommand(const std::string& commandType, 
                                                               const std::unordered_map<std::string, std::string>& parameters)
{
    if (!m_sceneManager) {
        return CommandResult(false, "SceneManager is null", commandType);
    }

    // Toggle coordinate system visibility
    bool currentVisibility = m_sceneManager->isCoordinateSystemVisible();
    bool newVisibility = !currentVisibility;
    
    m_sceneManager->setCoordinateSystemVisible(newVisibility);
    
    // Force immediate canvas refresh to ensure visual update
    if (Canvas* canvas = m_sceneManager->getCanvas()) {
        canvas->Refresh(true);
        canvas->Update();
    }
    
    return CommandResult(true, "Coordinate system visibility toggled successfully", commandType);
}

bool CoordinateSystemVisibilityListener::canHandleCommand(const std::string& commandType) const
{
    return commandType == cmd::to_string(cmd::CommandType::ToggleCoordinateSystem);
} 