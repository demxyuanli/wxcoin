#pragma once

#include "CommandListener.h"
#include "CommandType.h"

class OCCViewer;

class SliceToggleListener : public CommandListener {
public:
    explicit SliceToggleListener(OCCViewer* viewer);
    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "SliceToggleListener"; }
private:
    OCCViewer* m_viewer;
};


