#pragma once

#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <memory>
#include <atomic>

namespace ads {

/**
 * Double-buffered layout system for smooth resize
 * 
 * This class maintains two layout states:
 * 1. Active layout - currently visible
 * 2. Background layout - being calculated
 * 
 * During resize, the background layout is updated while the active
 * layout continues to display, then they are swapped.
 */
class DoubleBufferedLayout : public wxPanel {
public:
    DoubleBufferedLayout(wxWindow* parent);
    virtual ~DoubleBufferedLayout();
    
    // Add child to the layout
    void addChild(wxWindow* child, int proportion = 1, int flag = wxEXPAND);
    
    // Begin/end resize transaction
    void beginResize();
    void endResize();
    
    // Force immediate layout update
    void forceLayoutUpdate();
    
protected:
    void onPaint(wxPaintEvent& event);
    void onSize(wxSizeEvent& event);
    void onEraseBackground(wxEraseEvent& event);
    
private:
    struct LayoutState {
        struct ChildInfo {
            wxWindow* window;
            wxRect rect;
            bool visible;
        };
        std::vector<ChildInfo> children;
        wxSize size;
    };
    
    // Double buffer state
    std::unique_ptr<LayoutState> m_activeLayout;
    std::unique_ptr<LayoutState> m_pendingLayout;
    
    // Resize state
    std::atomic<bool> m_isResizing{false};
    wxTimer* m_swapTimer;
    
    // Calculate layout in background
    void calculateLayout(LayoutState* state, const wxSize& size);
    void swapLayouts();
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace ads