#ifndef FLAT_DOCK_CONTAINER_H
#define FLAT_DOCK_CONTAINER_H

#include <wx/panel.h>
#include <wx/notebook.h>
#include <vector>

class FlatDockManager;
class FlatEnhancedButton;

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

    wxPanel* m_tabOverlay; // overlay panel inside notebook tabbar region
    FlatEnhancedButton* m_btnFloat;
    FlatEnhancedButton* m_btnMaximize;
    FlatEnhancedButton* m_btnMinimize;
    FlatEnhancedButton* m_btnClose;
    wxFrame* m_dragPreview;
    bool m_dockingEnabled;
    bool m_systemButtonsVisible;
    bool m_hideTabs;
    wxPanel* m_singleHost;

    enum : int {
        ID_CTX_CLOSE = wxID_HIGHEST + 2001,
        ID_CTX_MOVE_LEFT,
        ID_CTX_MOVE_CENTER,
        ID_CTX_MOVE_BOTTOM,
        ID_CTX_FLOAT
    };

    enum : int {
        ID_BTN_FLOAT = wxID_HIGHEST + 3001,
        ID_BTN_MAXIMIZE,
        ID_BTN_MINIMIZE,
        ID_BTN_CLOSE
    };

    void OnButtonFloat(wxCommandEvent&);
    void OnButtonMaximize(wxCommandEvent&);
    void OnButtonMinimize(wxCommandEvent&);
    void OnButtonClose(wxCommandEvent&);
    void OnNotebookPageChanged(wxNotebookEvent&);
    void OnButtonRestore(wxCommandEvent&);
    void LayoutTabOverlay();
    int  GetTabBarHeightApprox() const;

public:
    void SetDockingEnabled(bool enabled) { m_dockingEnabled = enabled; }
    bool IsDockingEnabled() const { return m_dockingEnabled; }
    void SetSystemButtonsVisible(bool visible) { m_systemButtonsVisible = visible; if (m_tabOverlay) m_tabOverlay->Show(visible); }
    void SetHideTabs(bool hide);
    bool IsHideTabs() const { return m_hideTabs; }

    // When tabs are hidden, host content directly inside the container
    bool AddContentToHost(wxWindow* page);

    // Control overlay visibility without changing persistent flag
    void ShowOverlay(bool show);

	void BeginDrag(int pageIndex, const wxPoint& screenPos);
	void EndDrag(const wxPoint& screenPos);
};

#endif // FLAT_DOCK_CONTAINER_H


