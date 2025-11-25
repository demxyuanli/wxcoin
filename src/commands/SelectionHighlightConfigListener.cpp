#include "SelectionHighlightConfigListener.h"
#include "SelectionHighlightConfigDialog.h"
#include "CommandDispatcher.h"
#include "FlatFrame.h"
#include "Canvas.h"
#include "InputManager.h"
#include "BaseSelectionListener.h"
#include "logger/Logger.h"
#include <wx/wx.h>

SelectionHighlightConfigListener::SelectionHighlightConfigListener(FlatFrame* frame)
	: m_frame(frame)
{
}

SelectionHighlightConfigListener::~SelectionHighlightConfigListener()
{
}

CommandResult SelectionHighlightConfigListener::executeCommand(const std::string& commandType,
	const std::unordered_map<std::string, std::string>& parameters)
{
	if (commandType == "SELECTION_HIGHLIGHT_CONFIG")
	{
		// Get the parent window (main frame)
		wxWindow* parent = wxGetActiveWindow();
		if (!parent) {
			parent = wxTheApp->GetTopWindow();
		}
		
		// Create and show the selection highlight config dialog
		SelectionHighlightConfigDialog dialog(parent);
		if (dialog.ShowModal() == wxID_OK) {
			// Clear highlight cache in active selection listener if available
			if (m_frame) {
				Canvas* canvas = m_frame->GetCanvas();
				if (canvas) {
					InputManager* inputManager = canvas->getInputManager();
					if (inputManager && inputManager->isCustomInputStateActive()) {
						InputState* currentState = inputManager->getCurrentInputState();
						if (currentState) {
							BaseSelectionListener* selectionListener = dynamic_cast<BaseSelectionListener*>(currentState);
							if (selectionListener) {
								selectionListener->clearHighlightCache();
								LOG_INF("Cleared highlight cache after configuration save", "SelectionHighlightConfigListener");
							}
						}
					}
				}
			}
			return CommandResult(true, "Selection highlight configuration saved", "SELECTION_HIGHLIGHT_CONFIG");
		}
		
		return CommandResult(true, "Selection highlight configuration dialog closed", "SELECTION_HIGHLIGHT_CONFIG");
	}
	
	return CommandResult(false, "Unknown command type", "SELECTION_HIGHLIGHT_CONFIG");
}

CommandResult SelectionHighlightConfigListener::executeCommand(cmd::CommandType commandType,
	const std::unordered_map<std::string, std::string>& parameters)
{
	return executeCommand(cmd::to_string(commandType), parameters);
}

bool SelectionHighlightConfigListener::canHandleCommand(const std::string& commandType) const
{
	return commandType == "SELECTION_HIGHLIGHT_CONFIG";
}

std::string SelectionHighlightConfigListener::getListenerName() const
{
	return "SelectionHighlightConfigListener";
}

