#pragma once
#include "CommandListener.h"
#include "CommandType.h"
class OCCViewer;
class ShowEdgesListener : public CommandListener {
public:
    explicit ShowEdgesListener(OCCViewer* viewer);
    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "ShowEdgesListener"; }
private:
    OCCViewer* m_viewer;
}; 