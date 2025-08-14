#ifndef FLAT_DOCK_FLOAT_FRAME_H
#define FLAT_DOCK_FLOAT_FRAME_H

#include <wx/frame.h>

class FlatDockContainer;

class FlatDockFloatFrame : public wxFrame {
public:
	FlatDockFloatFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
	void AttachContainer(FlatDockContainer* container);

protected:
	void OnLeftDown(wxMouseEvent&);
	void OnMouseMove(wxMouseEvent&);
	void OnLeftUp(wxMouseEvent&);

private:
	bool m_dragging = false;
	wxPoint m_dragStartScreen;
};

#endif // FLAT_DOCK_FLOAT_FRAME_H


