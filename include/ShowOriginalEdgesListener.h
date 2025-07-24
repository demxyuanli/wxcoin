#pragma once
#include "CommandListener.h"
#include "CommandType.h"
class OCCViewer;
class ShowOriginalEdgesListener : public CommandListener {
public:
    explicit ShowOriginalEdgesListener(OCCViewer* viewer);
    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "ShowOriginalEdgesListener"; }
private:
    OCCViewer* m_viewer;
}; 