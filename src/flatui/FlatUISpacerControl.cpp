#include "flatui/FlatUISpacerControl.h"
#include <wx/dcbuffer.h>
#include "logger/Logger.h"
#include "config/ThemeManager.h"


FlatUISpacerControl::FlatUISpacerControl(wxWindow* parent, int width, wxWindowID id)
    : wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE),
    m_width(width),
    m_drawSeparator(false),
    m_autoExpand(false),
    m_canDragWindow(false), m_dragging(false)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    
    Bind(wxEVT_PAINT, &FlatUISpacerControl::OnPaint, this);
    Bind(wxEVT_LEFT_DOWN, &FlatUISpacerControl::OnLeftDown, this);
    Bind(wxEVT_LEFT_UP, &FlatUISpacerControl::OnLeftUp, this);
    Bind(wxEVT_MOTION, &FlatUISpacerControl::OnMotion, this);
    
    Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {
        if (m_canDragWindow) {
            SetCursor(wxCursor(wxCURSOR_ARROW));
        }
        event.Skip();
    });
    
    Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {
        SetCursor(wxCursor(wxCURSOR_ARROW));
        event.Skip();
    });
}

FlatUISpacerControl::~FlatUISpacerControl()
{
    Unbind(wxEVT_PAINT, &FlatUISpacerControl::OnPaint, this);
    Unbind(wxEVT_LEFT_DOWN, &FlatUISpacerControl::OnLeftDown, this);
    Unbind(wxEVT_LEFT_UP, &FlatUISpacerControl::OnLeftUp, this);
    Unbind(wxEVT_MOTION, &FlatUISpacerControl::OnMotion, this);
    Unbind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent& event) {});
    Unbind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent& event) {});
}

void FlatUISpacerControl::SetSpacerWidth(int width)
{
    if (width >= 0 && width != m_width)
    {
        m_width = width;
        if (!m_autoExpand)
        {
            SetMinSize(wxSize(m_width, -1));
            SetSize(wxSize(m_width, -1));
        }
        
        wxWindow* parent = GetParent();
        if (parent)
        {
            parent->Layout();
        }
        Refresh();
    }
}

int FlatUISpacerControl::CalculateAutoWidth(int availableWidth) const
{
    if (!m_autoExpand)
    {
        return m_width;
    }
    return wxMax(m_width, availableWidth);
}

void FlatUISpacerControl::OnPaint(wxPaintEvent& evt)
{
    wxAutoBufferedPaintDC dc(this);
    wxSize size = GetSize();
    wxColour bgColor = CFG_COLOUR("BarBgColour");
    dc.SetBackground(bgColor);
    dc.Clear();
    
    int contentX = 0;
    int contentY = 0;
    int contentW = size.GetWidth();
    int contentH = size.GetHeight();
    if (contentW < 0) contentW = 0;
    if (contentH < 0) contentH = 0;
    
    if (m_drawSeparator)
    {
        dc.SetPen(wxPen(wxColour(CFG_COLOUR("PanelSepatatorColor")), CFG_INT("PanelSepatatorWidth")));
        int x = contentX + contentW/2;
        dc.DrawLine(x, contentY + 2, x, contentY + contentH - 2);
    }
}

void FlatUISpacerControl::OnLeftDown(wxMouseEvent& evt)
{
    if (m_canDragWindow)
    {
        m_dragging = true;
        m_dragStartPos = evt.GetPosition();
        
        wxWindow* topWin = wxGetTopLevelParent(this);
        if (topWin)
        {
            wxPoint screenPos = ClientToScreen(m_dragStartPos);
            m_dragStartPos = topWin->ScreenToClient(screenPos);
            
            wxMouseEvent downEvt(wxEVT_LEFT_DOWN);
            downEvt.SetPosition(m_dragStartPos);
            downEvt.SetEventObject(topWin);
            topWin->ProcessWindowEvent(downEvt);
        }
    }
    evt.Skip(false); 
}

void FlatUISpacerControl::OnLeftUp(wxMouseEvent& evt)
{
    if (m_dragging)
    {
        m_dragging = false;
        
        wxWindow* topWin = wxGetTopLevelParent(this);
        if (topWin)
        {
            wxPoint screenPos = ClientToScreen(evt.GetPosition());
            wxPoint clientPos = topWin->ScreenToClient(screenPos);
            
            wxMouseEvent releaseEvt(wxEVT_LEFT_UP);
            releaseEvt.SetPosition(clientPos);
            releaseEvt.SetEventObject(topWin);
            topWin->ProcessWindowEvent(releaseEvt);
        }
    }
    evt.Skip(false); 
}

void FlatUISpacerControl::OnMotion(wxMouseEvent& evt)
{
    if (m_dragging && evt.Dragging() && evt.LeftIsDown())
    {
        wxWindow* topWin = wxGetTopLevelParent(this);
        if (topWin)
        {
            wxPoint screenPos = ClientToScreen(evt.GetPosition());
            wxPoint clientPos = topWin->ScreenToClient(screenPos);
            
            wxMouseEvent motionEvt(wxEVT_MOTION);
            motionEvt.SetPosition(clientPos);
            motionEvt.SetEventObject(topWin);
            motionEvt.SetLeftDown(true);
            motionEvt.SetEventType(wxEVT_MOTION);
            topWin->ProcessWindowEvent(motionEvt);
        }
    }
    evt.Skip();
}

void FlatUISpacerControl::SetCanDragWindow(bool canDrag) 
{ 
    m_canDragWindow = canDrag; 
    Refresh(); 
}
