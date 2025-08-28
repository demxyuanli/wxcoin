#pragma once
#include "CommandListener.h"
#include "CommandType.h"
class OCCViewer;
class FixNormalsListener : public CommandListener {
public:
	explicit FixNormalsListener(OCCViewer* viewer);
	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "FixNormalsListener"; }
private:
	OCCViewer* m_viewer;
};