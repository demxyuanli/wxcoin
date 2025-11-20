#pragma once

#include <wx/event.h>
#include <wx/gdicmn.h>

enum class SplitMode;

class ISplitViewportManager {
public:
    virtual ~ISplitViewportManager() = default;
    
    virtual void setSplitMode(SplitMode mode) = 0;
    virtual SplitMode getSplitMode() const = 0;
    
    virtual void render() = 0;
    virtual void handleSizeChange(const wxSize& canvasSize) = 0;
    virtual bool handleMouseEvent(wxMouseEvent& event) = 0;
    
    virtual void setActiveViewport(int index) = 0;
    virtual int getActiveViewport() const = 0;
    
    virtual bool isEnabled() const = 0;
    virtual void setEnabled(bool enabled) = 0;
};





