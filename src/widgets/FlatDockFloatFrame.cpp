#include "widgets/FlatDockFloatFrame.h"
#include "widgets/FlatDockContainer.h"
#include <wx/sizer.h>

FlatDockFloatFrame::FlatDockFloatFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
	: wxFrame(nullptr, wxID_ANY, title, pos, size, wxFRAME_NO_TASKBAR | wxFRAME_SHAPED | wxBORDER_NONE)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetDoubleBuffered(true);
	Bind(wxEVT_LEFT_DOWN, &FlatDockFloatFrame::OnLeftDown, this);
	Bind(wxEVT_MOTION, &FlatDockFloatFrame::OnMouseMove, this);
	Bind(wxEVT_LEFT_UP, &FlatDockFloatFrame::OnLeftUp, this);
}

void FlatDockFloatFrame::AttachContainer(FlatDockContainer* container)
{
	wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
	s->Add(container, 1, wxEXPAND);
	SetSizer(s);
}

void FlatDockFloatFrame::OnLeftDown(wxMouseEvent& e)
{
	m_dragging = true;
	m_dragStartScreen = ClientToScreen(e.GetPosition());
	CaptureMouse();
}

void FlatDockFloatFrame::OnMouseMove(wxMouseEvent& e)
{
	if (!m_dragging || !e.Dragging()) { e.Skip(); return; }
	wxPoint cur = ClientToScreen(e.GetPosition());
	wxPoint delta = cur - m_dragStartScreen;
	Move(GetPosition() + delta);
	m_dragStartScreen = cur;
}

void FlatDockFloatFrame::OnLeftUp(wxMouseEvent&)
{
	if (m_dragging) { m_dragging = false; if (HasCapture()) ReleaseMouse(); }
}


