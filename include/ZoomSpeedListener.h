#pragma once
#include "CommandListener.h"
#include "CommandType.h"
class Canvas;
class MainFrame;
class ZoomSpeedListener : public CommandListener {
public:
    ZoomSpeedListener(MainFrame* mainFrame, Canvas* canvas);
    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "ZoomSpeedListener"; }
private:
    MainFrame* m_mainFrame;
    Canvas* m_canvas;
}; 