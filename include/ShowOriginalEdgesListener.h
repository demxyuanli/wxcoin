#pragma once
#include "CommandListener.h"
#include "CommandType.h"
#include <memory>

class OCCViewer;
class AsyncIntersectionManager;
class FlatFrame;

class ShowOriginalEdgesListener : public CommandListener {
public:
	explicit ShowOriginalEdgesListener(OCCViewer* viewer, wxFrame* frame = nullptr);
	CommandResult executeCommand(const std::string& commandType,
		const std::unordered_map<std::string, std::string>& parameters) override;
	bool canHandleCommand(const std::string& commandType) const override;
	std::string getListenerName() const override { return "ShowOriginalEdgesListener"; }
	
	std::shared_ptr<AsyncIntersectionManager> getIntersectionManager() const { return m_intersectionManager; }
	
private:
	OCCViewer* m_viewer;
	wxFrame* m_frame;
	std::shared_ptr<AsyncIntersectionManager> m_intersectionManager;
};