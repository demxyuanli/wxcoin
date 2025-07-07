#pragma once

#include "CommandListener.h"
#include "CommandType.h"
#include <memory>

class MainFrame;
class Canvas;
class OCCViewer;

class ImportStepListener : public CommandListener {
public:
    ImportStepListener(MainFrame* mainFrame, Canvas* canvas, OCCViewer* viewer);
    ~ImportStepListener() override = default;

    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    bool canHandleCommand(const std::string& commandType) const override;
    std::string getListenerName() const override { return "ImportStepListener"; }

private:
    MainFrame* m_mainFrame;
    Canvas* m_canvas;
    OCCViewer* m_viewer;
}; 