#pragma once

#include "CommandListener.h"

class Canvas;

class SplitViewToggleListener : public CommandListener {
public:
    explicit SplitViewToggleListener(Canvas* canvas);
    
    CommandResult executeCommand(const std::string& commandType,
                                 const std::unordered_map<std::string, std::string>& parameters) override;
    
    bool canHandleCommand(const std::string& commandType) const override;
    
    std::string getListenerName() const override;

private:
    Canvas* m_canvas;
};

