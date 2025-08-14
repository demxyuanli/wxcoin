#ifndef FLAT_DOCK_CONTAINER_H
#define FLAT_DOCK_CONTAINER_H

#include <wx/panel.h>
#include <wx/notebook.h>

// Forward declaration to avoid cyclic include
class FlatDockManager;
#include <vector>

class FlatDockManager;

// Tabbed container supporting simple drag-out/drag-in docking between containers.
class FlatDockContainer : public wxPanel {
public:
	FlatDockContainer(FlatDockManager* manager, wxWindow* parent);

	// Add a new pane as a tab
	void AddPage(wxWindow* page, const wxString& label, bool select = true);

	// Accept a page dragged from other container
	void AcceptDraggedPage(wxWindow* page, const wxString& label, bool select = true);

	// Helper to query the tab count
	int GetPageCount() const;

	// Expose notebook for manager if needed
	wxNotebook* GetNotebook() const { return m_notebook; }

    // Float selected tab
    void FloatSelectedTab();

    // Move selected tab to a manager region
    void MoveSelectedToManagerRegion(int pos);

protected:
	void OnLeftDown(wxMouseEvent& e);
	void OnMouseMove(wxMouseEvent& e);
	void OnLeftUp(wxMouseEvent& e);
    void OnLeftDClick(wxMouseEvent& e);
    void OnContextMenu(wxContextMenuEvent& e);
    void OnMenu(wxCommandEvent& e);

private:
	FlatDockManager* m_manager;
	wxNotebook* m_notebook;
	bool m_dragging;
    bool m_dragStarted;
	int m_dragPageIndex;
	wxPoint m_dragStartScreen;
	wxString m_dragLabel;
    static constexpr int kDragThresholdPx = 6;
    
    int computeInsertIndexFromLocalX(int localX) const;

    enum : int {
        ID_CTX_CLOSE = wxID_HIGHEST + 2001,
        ID_CTX_MOVE_LEFT,
        ID_CTX_MOVE_CENTER,
        ID_CTX_MOVE_BOTTOM,
        ID_CTX_FLOAT
    };

	void BeginDrag(int pageIndex, const wxPoint& screenPos);
	void EndDrag(const wxPoint& screenPos);
};

#endif // FLAT_DOCK_CONTAINER_H


