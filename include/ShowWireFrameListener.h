#pragma once
#include "CommandListener.h"
#include "CommandType.h"
class OCCViewer;
class ShowWireFrameListener : public CommandListener {
public:
	explicit ShowWireFrameListener(OCCViewer* viewer);
	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "ShowWireFrameListener"; }
private:
	OCCViewer* m_viewer;
};