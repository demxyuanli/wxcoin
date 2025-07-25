#pragma once

#include "CommandListener.h"

class OCCViewer;

class ShowSilhouetteEdgesListener : public CommandListener {
public:
    ShowSilhouetteEdgesListener(OCCViewer* viewer);
    
    CommandResult executeCommand(const std::string& commandType,
                                const std::unordered_map<std::string, std::string>& parameters) override;
    
    bool canHandleCommand(const std::string& commandType) const override;
    
    std::string getListenerName() const override { return "ShowSilhouetteEdgesListener"; }

private:
    OCCViewer* m_viewer;
}; 