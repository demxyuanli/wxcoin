#pragma once

// Mock wxWidgets for testing compilation
#include <string>
#include <vector>

typedef std::string wxString;
typedef int wxWindowID;
typedef int wxEventType;

#define wxID_ANY -1
#define wxDefaultPosition wxPoint()
#define wxDefaultSize wxSize()
#define wxEmptyString ""

// Mock classes
class wxObject {};
class wxEvtHandler : public wxObject {};

class wxPoint {
public:
    wxPoint(int x = 0, int y = 0) : x(x), y(y) {}
    int x, y;
};

class wxSize {
public:
    wxSize(int w = -1, int h = -1) : width(w), height(h) {}
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    void SetWidth(int w) { width = w; }
    void SetHeight(int h) { height = h; }
    int width, height;
};

class wxRect {
public:
    wxRect() {}
    wxRect(int x, int y, int w, int h) : x(x), y(y), width(w), height(h) {}
    int GetLeft() const { return x; }
    int GetTop() const { return y; }
    int GetRight() const { return x + width; }
    int GetBottom() const { return y + height; }
    wxPoint GetBottomLeft() const { return wxPoint(x, y + height); }
    bool Contains(const wxPoint& pt) const { return false; }
    bool IsEmpty() const { return width == 0 || height == 0; }
    void Deflate(int dx, int dy) {}
    int x, y, width, height;
};

class wxWindow : public wxEvtHandler {
public:
    wxWindow() {}
    wxWindow(wxWindow* parent, wxWindowID id = wxID_ANY, 
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = 0,
             const wxString& name = wxEmptyString) {}
    virtual ~wxWindow() {}
    
    void SetSize(const wxSize& size) {}
    void SetSize(const wxRect& rect) {}
    void SetPosition(const wxPoint& pt) {}
    wxSize GetSize() const { return wxSize(); }
    wxSize GetClientSize() const { return wxSize(); }
    wxRect GetClientRect() const { return wxRect(); }
    void Show(bool show = true) {}
    void Hide() {}
    void Refresh() {}
    void Layout() {}
    void SetBackgroundColour(const wxColour& colour) {}
    void SetBackgroundStyle(int style) {}
    void SetMinSize(const wxSize& size) {}
    void SetSizer(wxSizer* sizer) {}
    wxSizer* GetSizer() { return nullptr; }
    void SetToolTip(const wxString& tip) {}
    void SetParent(wxWindow* parent) {}
    void Destroy() {}
    bool HasCapture() const { return false; }
    void CaptureMouse() {}
    void ReleaseMouse() {}
    bool IsShownOnScreen() const { return false; }
    void SetFocus() {}
    void SetTransparent(wxByte alpha) {}
    void Bind(wxEventType eventType, std::function<void(wxEvent&)> handler, int id = wxID_ANY) {}
};

class wxPanel : public wxWindow {
public:
    wxPanel() {}
    wxPanel(wxWindow* parent, wxWindowID id = wxID_ANY) : wxWindow(parent, id) {}
};

class wxFrame : public wxWindow {
public:
    wxFrame() {}
    wxFrame(wxWindow* parent, wxWindowID id, const wxString& title) : wxWindow(parent, id) {}
};

class wxSizer {};
class wxBoxSizer : public wxSizer {
public:
    wxBoxSizer(int orient) {}
    void Add(wxWindow* window, int prop = 0, int flag = 0, int border = 0) {}
    void Add(wxSizer* sizer, int prop = 0, int flag = 0, int border = 0) {}
    void Detach(wxWindow* window) {}
};

// More mock classes as needed...
#define wxBEGIN_EVENT_TABLE(a,b)
#define wxEND_EVENT_TABLE()
#define wxDECLARE_EVENT_TABLE()
#define wxDEFINE_EVENT(a,b) wxEventType a = 0
#define wxDECLARE_EVENT(a,b)

// Event types
#define EVT_PAINT(fn)
#define EVT_LEFT_DOWN(fn)
#define EVT_LEFT_UP(fn)
#define EVT_MOTION(fn)
#define EVT_LEAVE_WINDOW(fn)
#define EVT_SIZE(fn)
#define EVT_TIMER(id, fn)
#define EVT_MOUSE_CAPTURE_LOST(fn)
#define EVT_KILL_FOCUS(fn)
#define EVT_CLOSE(fn)