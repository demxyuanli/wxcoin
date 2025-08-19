#ifndef FLAT_DOCK_MANAGER_H
#define FLAT_DOCK_MANAGER_H

#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/sizer.h>
#include "widgets/FlatDockContainer.h"
#include <map>

// A lightweight custom docking manager inspired by wxAUI, tailored to current layout needs.
// Supports LeftTop, LeftBottom, Center and Bottom panes using internal splitters.
class FlatDockManager : public wxPanel {
public:
	enum class DockPos {
		LeftTop,
		LeftBottom,
		Center,
		Bottom
	};

    enum class DockRegion {
        Center,
        Left,
        Right,
        Top,
        Bottom
    };

	explicit FlatDockManager(wxWindow* parent);
	~FlatDockManager() override = default;

	// Adds a pane to the specified dock position. Optionally provide a target size in pixels.
	// For Bottom, sizePx sets the minimum height. For Left splitter, sizes are applied after both panes exist.
	void AddPane(wxWindow* pane, DockPos pos, int sizePx = -1);

	// Drag preview and snap target
	void ShowDockPreview(const wxPoint& screenPt);
	void HideDockPreview();
	class FlatDockContainer* FindSnapTarget(const wxPoint& screenPt, int marginPx = 24) const;
    // Called by containers during drag to show precise insertion caret
    void SetInsertionPreview(FlatDockContainer* target, int insertIndex, const wxRect& notebookScreenRect, int caretScreenX);

    // Compute five-way drop region around hovered container (wxAUI-like)
    DockRegion ComputeRegionForPoint(FlatDockContainer* hovered, const wxPoint& screenPt) const;

    // Execute docking of a page into hovered container at region. Returns true on success
    bool PerformDock(wxWindow* page, const wxString& label, FlatDockContainer* hovered, DockRegion region);

    // Close/hide a container appropriately depending on region (used by caption close button)
    void CloseContainer(FlatDockContainer* container);

    // Layout persistence (minimal JSON-like string)
    wxString SaveLayout() const;
    bool LoadLayout(const wxString& layout);

	// Accessors for fine tuning sash positions from outside.
	wxSplitterWindow* GetMainSplitter() const { return m_mainHSplitter; }
	wxSplitterWindow* GetLeftSplitter() const { return m_leftVSplitter; }

private:
	void EnsureSplitters();
	void ApplyPendingSizes();
	void OnPaint(wxPaintEvent&);
	bool HitTestContainer(const wxPoint& screenPt, int marginPx, class FlatDockContainer*& out) const;
    void DrawSashRubberBand(const wxRect& band);
    void EraseSashRubberBand();

private:
    // Root layout: vertical sizer with [topArea(mainHSplitter)] and [bottomArea]
	wxSplitterWindow* m_mainHSplitter; // splits left stack and center
	wxSplitterWindow* m_leftVSplitter; // splits left top/bottom
    FlatDockContainer* m_leftTopPane;
    FlatDockContainer* m_leftBottomPane;
    FlatDockContainer* m_centerPane;
    FlatDockContainer* m_bottomContainer;

	int m_pendingLeftHeightPx;   // desired height of left top pane when both sides ready
    int m_pendingBottomHeightPx; // desired height of bottom area

	bool m_previewVisible;
	wxRect m_previewRect; // in manager client coordinates
    FlatDockContainer* m_previewTarget; // current target container under cursor
    DockRegion m_previewRegion;
    int m_previewInsertIndex; // for tab insertion preview
    int m_previewCaretX; // client x for insertion caret within notebook rect
    wxRect m_previewNotebookRect; // client rect of target notebook

    // Track last owner of a page for snap-back / recycle
    std::map<wxWindow*, FlatDockContainer*> m_lastOwner;
    bool m_sashBandVisible{false};
    wxRect m_sashBandRect;
};

#endif // FLAT_DOCK_MANAGER_H


