#pragma once

#include "CommandListener.h"
#include <wx/frame.h>

class HelpAboutListener : public CommandListener {
public:
    HelpAboutListener(wxFrame* frame);
    ~HelpAboutListener() override = default;

    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "HelpAboutListener"; }

private:
    wxFrame* m_frame;
}; 