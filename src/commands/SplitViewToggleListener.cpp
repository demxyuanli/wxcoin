#include "SplitViewToggleListener.h"
#include "CommandType.h"
#include "logger/Logger.h"
#include "Canvas.h"
#include "SplitViewportManager.h"

SplitViewToggleListener::SplitViewToggleListener(Canvas* canvas)
    : m_canvas(canvas) {
}

CommandResult SplitViewToggleListener::executeCommand(const std::string& commandType,
    const std::unordered_map<std::string, std::string>& parameters) {
    
    if (!m_canvas) {
        return CommandResult(false, "Canvas not available", commandType);
    }
    
    SplitViewportManager* splitManager = m_canvas->getSplitViewportManager();
    
    if (commandType == cmd::to_string(cmd::CommandType::SplitViewSingle)) {
        // Single view mode means disabling split viewport entirely and returning to normal view
        m_canvas->setSplitViewportEnabled(false);
        LOG_INF_S("Split view disabled: returned to single view mode");
        return CommandResult(true, "Returned to single view mode", commandType);
    }
    
    if (commandType == cmd::to_string(cmd::CommandType::SplitViewHorizontal2)) {
        // Ensure split viewport is enabled
        if (!m_canvas->isSplitViewportEnabled()) {
            m_canvas->setSplitViewportEnabled(true);
        }
        splitManager = m_canvas->getSplitViewportManager();
        if (splitManager) {
            splitManager->setSplitMode(SplitMode::HORIZONTAL_2);
            LOG_INF_S("Split view mode: Horizontal 2");
            return CommandResult(true, "Horizontal split (2 views) enabled", commandType);
        } else {
            LOG_ERR_S("Failed to get split viewport manager after enabling split viewport");
            return CommandResult(false, "Failed to initialize split viewport manager", commandType);
        }
    }
    
    if (commandType == cmd::to_string(cmd::CommandType::SplitViewVertical2)) {
        // Ensure split viewport is enabled
        if (!m_canvas->isSplitViewportEnabled()) {
            m_canvas->setSplitViewportEnabled(true);
        }
        splitManager = m_canvas->getSplitViewportManager();
        if (splitManager) {
            splitManager->setSplitMode(SplitMode::VERTICAL_2);
            LOG_INF_S("Split view mode: Vertical 2");
            return CommandResult(true, "Vertical split (2 views) enabled", commandType);
        } else {
            LOG_ERR_S("Failed to get split viewport manager after enabling split viewport");
            return CommandResult(false, "Failed to initialize split viewport manager", commandType);
        }
    }
    
    if (commandType == cmd::to_string(cmd::CommandType::SplitViewQuad)) {
        // Ensure split viewport is enabled
        if (!m_canvas->isSplitViewportEnabled()) {
            m_canvas->setSplitViewportEnabled(true);
        }
        splitManager = m_canvas->getSplitViewportManager();
        if (splitManager) {
            splitManager->setSplitMode(SplitMode::QUAD);
            LOG_INF_S("Split view mode: Quad");
            return CommandResult(true, "Quad view (4 views) enabled", commandType);
        } else {
            LOG_ERR_S("Failed to get split viewport manager after enabling split viewport");
            return CommandResult(false, "Failed to initialize split viewport manager", commandType);
        }
    }
    
    if (commandType == cmd::to_string(cmd::CommandType::SplitViewSix)) {
        // Ensure split viewport is enabled
        if (!m_canvas->isSplitViewportEnabled()) {
            m_canvas->setSplitViewportEnabled(true);
        }
        splitManager = m_canvas->getSplitViewportManager();
        if (splitManager) {
            splitManager->setSplitMode(SplitMode::SIX);
            LOG_INF_S("Split view mode: Six");
            return CommandResult(true, "Six view mode enabled", commandType);
        } else {
            LOG_ERR_S("Failed to get split viewport manager after enabling split viewport");
            return CommandResult(false, "Failed to initialize split viewport manager", commandType);
        }
    }

	if (commandType == cmd::to_string(cmd::CommandType::SplitViewToggleSync)) {
		bool newState = !m_canvas->isSplitViewportCameraSyncEnabled();
		m_canvas->setSplitViewportCameraSyncEnabled(newState);
		LOG_INF_S(std::string("Split view camera sync ") + (newState ? "enabled" : "disabled"));
		return CommandResult(true, newState ? "Split view camera sync enabled" : "Split view camera sync disabled", commandType);
	}
    
    return CommandResult(false, "Unknown split view command", commandType);
}

bool SplitViewToggleListener::canHandleCommand(const std::string& commandType) const {
    return commandType == cmd::to_string(cmd::CommandType::SplitViewSingle) ||
           commandType == cmd::to_string(cmd::CommandType::SplitViewHorizontal2) ||
           commandType == cmd::to_string(cmd::CommandType::SplitViewVertical2) ||
	       commandType == cmd::to_string(cmd::CommandType::SplitViewQuad) ||
	       commandType == cmd::to_string(cmd::CommandType::SplitViewSix) ||
	       commandType == cmd::to_string(cmd::CommandType::SplitViewToggleSync);
}

std::string SplitViewToggleListener::getListenerName() const {
    return "SplitViewToggleListener";
}
