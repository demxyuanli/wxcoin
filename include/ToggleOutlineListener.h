#pragma once

#include "CommandListener.h"

class OCCViewer;

class ToggleOutlineListener : public CommandListener {
public:
    explicit ToggleOutlineListener(OCCViewer* viewer) : m_viewer(viewer) {}
    std::string getListenerName() const override { return "ToggleOutlineListener"; }

    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;

private:
    OCCViewer* m_viewer;
};


