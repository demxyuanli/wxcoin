#pragma once

#include "CommandListener.h"
#include "OCCViewer.h"
#include <memory>
#include <unordered_map>
#include <string>

class ViewModeListener : public CommandListener
{
public:
	ViewModeListener(OCCViewer* viewer);
	virtual ~ViewModeListener() = default;

	CommandResult executeCommand(const std::string& commandType, const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override;

private:
	OCCViewer* m_viewer;
};