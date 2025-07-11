#ifndef FLATUIEVENTMANAGER_H
#define FLATUIEVENTMANAGER_H

#include <wx/wx.h>
#include <functional> 
#include <map>
#include <vector>

class FlatUIFrame;
class FlatUIBar;
class FlatUIPage;
class FlatUIPanel;
class FlatUIButtonBar;
class FlatUIHomeSpace;
class FlatUISystemButtons;
class FlatUIFunctionSpace;
class FlatUIProfileSpace;
class FlatUIGallery;

class FlatUIEventManager
{
public:
    static FlatUIEventManager& getInstance();
    
    void bindFrameEvents(FlatUIFrame* frame);
    void unbindFrameEvents(FlatUIFrame* frame);
    
    void bindBarEvents(FlatUIBar* bar);
    void unbindBarEvents(FlatUIBar* bar);
    
    void bindPageEvents(FlatUIPage* page);
    void unbindPageEvents(FlatUIPage* page);
    
    void bindPanelEvents(FlatUIPanel* panel);
    void unbindPanelEvents(FlatUIPanel* panel);
    
    void bindButtonBarEvents(FlatUIButtonBar* buttonBar);
    void unbindButtonBarEvents(FlatUIButtonBar* buttonBar);
    
    void bindHomeSpaceEvents(FlatUIHomeSpace* homeSpace);
    void unbindHomeSpaceEvents(FlatUIHomeSpace* homeSpace);
    
    void bindSystemButtonsEvents(FlatUISystemButtons* systemButtons);
    void unbindSystemButtonsEvents(FlatUISystemButtons* systemButtons);
    
    void bindFunctionSpaceEvents(FlatUIFunctionSpace* functionSpace);
    void unbindFunctionSpaceEvents(FlatUIFunctionSpace* functionSpace);
    
    void bindProfileSpaceEvents(FlatUIProfileSpace* profileSpace);
    void unbindProfileSpaceEvents(FlatUIProfileSpace* profileSpace);
    
    void bindGalleryEvents(FlatUIGallery* gallery);
    void unbindGalleryEvents(FlatUIGallery* gallery);
    
    void bindSizeEvents(wxWindow* control, std::function<void(wxSizeEvent&)> handler);
    void unbindSizeEvents(wxWindow* control);
    
    void bindButtonEvent(wxWindow* window, void (wxEvtHandler::*func)(wxCommandEvent&), wxWindowID id);
    void unbindButtonEvent(wxWindow* window, void (wxEvtHandler::*func)(wxCommandEvent&), wxWindowID id);
    
    void bindMenuEvent(wxWindow* window, void (wxEvtHandler::*func)(wxCommandEvent&), wxWindowID id);
    void unbindMenuEvent(wxWindow* window, void (wxEvtHandler::*func)(wxCommandEvent&), wxWindowID id);

    template<typename T>
    void bindCustomEvents(T* customControl, void (T::* paintHandler)(wxPaintEvent&),
        void (T::* mouseDownHandler)(wxMouseEvent&),
        void (T::* mouseMoveHandler)(wxMouseEvent&),
        void (T::* mouseLeaveHandler)(wxMouseEvent&))
    {
        if (!customControl) return;

        if (paintHandler) {
            customControl->Bind(wxEVT_PAINT, paintHandler, customControl);
        }

        if (mouseDownHandler) {
            customControl->Bind(wxEVT_LEFT_DOWN, mouseDownHandler, customControl);
        }

        if (mouseMoveHandler) {
            customControl->Bind(wxEVT_MOTION, mouseMoveHandler, customControl);
        }

        if (mouseLeaveHandler) {
            customControl->Bind(wxEVT_LEAVE_WINDOW, mouseLeaveHandler, customControl);
        }
    }

    template<typename T, typename EventType>
    void bindEvent(T* control, wxEventType eventType, void (T::* handler)(EventType&), int id)
    {
        if (!control || !handler) return;

        if (id == wxID_ANY) {
            control->Bind(eventType, handler, control);
        }
        else {
            control->Bind(eventType, handler, control, id);
        }
    }

    template<typename T>
    void bindButtonEvent(T* control, void (T::* handler)(wxCommandEvent&), int id)
    {
        if (!control || !handler) return;

        if (id == wxID_ANY) {
            control->Bind(wxEVT_BUTTON, handler, control);
        }
        else {
            control->Bind(wxEVT_BUTTON, handler, control, id);
        }
    }

    template<typename T>
    void bindMenuEvent(T* control, void (T::* handler)(wxCommandEvent&), int id)
    {
        if (!control || !handler) return;

        if (id == wxID_ANY) {
            control->Bind(wxEVT_MENU, handler, control);
        }
        else {
            control->Bind(wxEVT_MENU, handler, control, id);
        }
    }

private:
    FlatUIEventManager() {}
    FlatUIEventManager(const FlatUIEventManager&) = delete;
    FlatUIEventManager& operator=(const FlatUIEventManager&) = delete;

    std::map<wxWindow*, std::vector<wxEventType>> m_boundEvents;
};

#endif // FLATUIEVENTMANAGER_H 