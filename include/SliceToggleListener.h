#pragma once

#include "CommandListener.h"
#include "CommandType.h"

class OCCViewer;
class SliceParamDialog;

class SliceToggleListener : public CommandListener {
public:
	explicit SliceToggleListener(OCCViewer* viewer);
	~SliceToggleListener();
	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "SliceToggleListener"; }
	
	// Check if mouse is in drag slice mode
	bool isInDragMode() const;
	
private:
	OCCViewer* m_viewer;
	SliceParamDialog* m_paramDialog{ nullptr };
};
