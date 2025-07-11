#pragma once

#include "CommandListener.h"
#include "CommandType.h"
#include <memory>
#include <wx/frame.h>

class Canvas;

class FileOpenListener : public CommandListener {
public:
    FileOpenListener(wxFrame* frame);
    ~FileOpenListener() override = default;

    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "FileOpenListener"; }

private:
    wxFrame* m_frame;
}; 