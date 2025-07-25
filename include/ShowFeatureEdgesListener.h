#pragma once
#include "Command.h"
#include "CommandListener.h"
#include "EdgeTypes.h"

class OCCViewer;

class ShowFeatureEdgesListener : public CommandListener {
public:
    explicit ShowFeatureEdgesListener(OCCViewer* viewer);
    
    CommandResult executeCommand(const std::string& commandType,
                                const std::unordered_map<std::string, std::string>& parameters) override;
    
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override;

private:
    OCCViewer* m_viewer;
}; 