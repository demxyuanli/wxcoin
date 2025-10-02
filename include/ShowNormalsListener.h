#pragma once

#include "CommandListener.h"
#include "CommandType.h"

class OCCViewer;

/**
 * @brief Command listener for toggling node normals display
 */
class ShowNormalsListener : public CommandListener {
public:
    explicit ShowNormalsListener(OCCViewer* viewer);
    
    CommandResult executeCommand(const std::string& commandType,
        const std::unordered_map<std::string, std::string>& parameters) override;
    
    bool canHandleCommand(const std::string& commandType) const override;
    
    std::string getListenerName() const override { return "ShowNormalsListener"; }

private:
    OCCViewer* m_viewer;
};