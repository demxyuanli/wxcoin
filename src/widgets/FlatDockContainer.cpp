#include "widgets/FlatDockContainer.h"
#include "widgets/FlatDockManager.h"
#include "widgets/FlatDockCaptionBar.h"
#include "widgets/FlatDockFloatFrame.h"
#include <wx/frame.h>

FlatDockContainer::FlatDockContainer(FlatDockManager* manager, wxWindow* parent)
	: wxPanel(parent, wxID_ANY)
	, m_manager(manager)
	, m_notebook(new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP))
	, m_dragging(false)
	, m_dragStarted(false)
	, m_dragPageIndex(-1)
{
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    // Caption bar with float/close buttons similar to wxAUI
    FlatDockCaptionBar* caption = new FlatDockCaptionBar(this, this);
    caption->SetTitle("Performance");
    s->Add(caption, 0, wxEXPAND);
    s->Add(m_notebook, 1, wxEXPAND);
	SetSizer(s);

    m_notebook->Bind(wxEVT_LEFT_DOWN, &FlatDockContainer::OnLeftDown, this);
    m_notebook->Bind(wxEVT_MOTION, &FlatDockContainer::OnMouseMove, this);
    m_notebook->Bind(wxEVT_LEFT_UP, &FlatDockContainer::OnLeftUp, this);
}

void FlatDockContainer::AddPage(wxWindow* page, const wxString& label, bool select)
{
	// wxNotebook requires pages to have the notebook as direct parent
	page->Reparent(m_notebook);
	m_notebook->AddPage(page, label, select);
}

void FlatDockContainer::AcceptDraggedPage(wxWindow* page, const wxString& label, bool select)
{
	AddPage(page, label, select);
}

int FlatDockContainer::GetPageCount() const
{
	return static_cast<int>(m_notebook->GetPageCount());
}

void FlatDockContainer::OnLeftDown(wxMouseEvent& e)
{
	// Record tab index if clicked on a tab area
	if (m_notebook->GetPageCount() == 0) { e.Skip(); return; }
    m_dragPageIndex = m_notebook->GetSelection();
    // Use notebook as coordinate origin because events are fired on m_notebook
    m_dragStartScreen = m_notebook->ClientToScreen(e.GetPosition());
    m_dragging = true;
    m_dragStarted = false; // wait until movement exceeds threshold
	e.Skip();
}

void FlatDockContainer::OnMouseMove(wxMouseEvent& e)
{
	if (!m_dragging || m_dragPageIndex < 0) { e.Skip(); return; }
    if (!e.Dragging()) { e.Skip(); return; }
    // Only start a real drag when movement exceeds threshold to avoid click creating windows
    // Convert mouse position from notebook-client to screen
    wxPoint cur = m_notebook->ClientToScreen(e.GetPosition());
    if (!m_dragStarted) {
        if (std::abs(cur.x - m_dragStartScreen.x) < kDragThresholdPx && std::abs(cur.y - m_dragStartScreen.y) < kDragThresholdPx) {
            e.Skip();
            return;
        }
        m_dragStarted = true;
    }
    if (m_manager) {
        // Compute insertion index preview within this notebook
        wxRect nbRectScreen = wxRect(m_notebook->GetScreenPosition(), m_notebook->GetSize());
        // Translate to notebook client coordinates
        wxPoint nbClient = m_notebook->ScreenToClient(m_notebook->ClientToScreen(e.GetPosition()));
        int idx = computeInsertIndexFromLocalX(nbClient.x);
        // caret x at tab start
        int approxTabW = std::max(60, m_notebook->GetSize().x / std::max(1, (int)m_notebook->GetPageCount()));
        int caretScreenX = nbRectScreen.GetX() + 12 + idx * approxTabW;
        m_manager->SetInsertionPreview(this, idx, nbRectScreen, caretScreenX);
        m_manager->ShowDockPreview(cur);
    }
	e.Skip();
}

void FlatDockContainer::OnLeftUp(wxMouseEvent& e)
{
	if (!m_dragging || m_dragPageIndex < 0) { e.Skip(); return; }
    m_dragging = false;
    if (!m_dragStarted) { // treat as a simple click, do nothing special
        m_dragPageIndex = -1;
        if (m_manager) m_manager->HideDockPreview();
        e.Skip();
        return;
    }
	// Determine drop target by asking the manager
    // Convert from notebook-client to screen coordinates
    wxPoint dropScreen = m_notebook->ClientToScreen(e.GetPosition());
    wxRect nbRectScreen = wxRect(m_notebook->GetScreenPosition(), m_notebook->GetSize());
    if (nbRectScreen.Contains(dropScreen)) {
        wxPoint nbClient = m_notebook->ScreenToClient(dropScreen);
        int newIndex = computeInsertIndexFromLocalX(nbClient.x);
        if (newIndex != m_dragPageIndex) {
            wxWindow* page = m_notebook->GetPage(m_dragPageIndex);
            wxString label = m_notebook->GetPageText(m_dragPageIndex);
            m_notebook->RemovePage(m_dragPageIndex);
            if (newIndex > (int)m_notebook->GetPageCount()) newIndex = (int)m_notebook->GetPageCount();
            page->Reparent(m_notebook);
            m_notebook->InsertPage(newIndex, page, label, true);
        }
    } else {
        EndDrag(dropScreen);
    }
    if (m_manager) m_manager->HideDockPreview();
	e.Skip();
}

void FlatDockContainer::BeginDrag(int pageIndex, const wxPoint& screenPos)
{
	wxUnusedVar(pageIndex);
	wxUnusedVar(screenPos);
}

void FlatDockContainer::EndDrag(const wxPoint& screenPos)
{
	if (m_dragPageIndex < 0 || m_dragPageIndex >= (int)m_notebook->GetPageCount()) return;
	wxWindow* page = m_notebook->GetPage(m_dragPageIndex);
	wxString label = m_notebook->GetPageText(m_dragPageIndex);

    // Ensure the page is detached from any previous sizer before re-adding elsewhere
    if (auto* oldSizer = page->GetContainingSizer()) {
        oldSizer->Detach(page);
        page->SetContainingSizer(nullptr);
    }

	// Ask manager to find target container under mouse, or create floating frame
	wxWindow* target = wxFindWindowAtPoint(screenPos);
	FlatDockContainer* targetContainer = nullptr;
	for (wxWindow* w = target; w; w = w->GetParent()) {
		targetContainer = dynamic_cast<FlatDockContainer*>(w);
		if (targetContainer) break;
	}

    if (targetContainer && targetContainer != this) {
		m_notebook->RemovePage(m_dragPageIndex);
		targetContainer->AcceptDraggedPage(page, label, true);
		return;
	}

	// Otherwise, float in a new frame
	m_notebook->RemovePage(m_dragPageIndex);
    if (auto* oldSizer2 = page->GetContainingSizer()) {
        oldSizer2->Detach(page);
        page->SetContainingSizer(nullptr);
    }
	wxFrame* floatFrame = new wxFrame(nullptr, wxID_ANY, label, screenPos, wxSize(400, 300));
	FlatDockContainer* floatContainer = new FlatDockContainer(m_manager, floatFrame);
    floatContainer->AddPage(page, label, true);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
	s->Add(floatContainer, 1, wxEXPAND);
    floatFrame->SetSizer(s);
    // Close policy: return first page back to center container of the original manager
    FlatDockManager* mgr = m_manager;
    floatFrame->Bind(wxEVT_CLOSE_WINDOW, [mgr, floatContainer](wxCloseEvent& ev){
        if (mgr && !mgr->IsBeingDeleted() && floatContainer->GetPageCount() > 0) {
            wxWindow* page0 = floatContainer->GetNotebook()->GetPage(0);
            floatContainer->GetNotebook()->RemovePage(0);
            // Safe default: dock back to center
            mgr->AddPane(page0, FlatDockManager::DockPos::Center);
        }
        ev.Skip();
    });
	floatFrame->Show();
}

int FlatDockContainer::computeInsertIndexFromLocalX(int localX) const
{
    // If wxNotebook supports tab geometry query in your wx version, use it here.
    // Fallback: distribute width evenly across tabs to estimate.
    int pageCount = (int)m_notebook->GetPageCount();
    if (pageCount <= 0) return 0;
    int w = m_notebook->GetClientSize().x;
    int tabW = std::max(60, w / pageCount);
    int idx = std::clamp(localX / tabW, 0, pageCount);
    return idx;
}

void FlatDockContainer::FloatSelectedTab()
{
    int sel = m_notebook->GetSelection();
    if (sel < 0) return;
    wxWindow* page = m_notebook->GetPage(sel);
    wxString label = m_notebook->GetPageText(sel);
    m_notebook->RemovePage(sel);
    FlatDockFloatFrame* floatFrame = new FlatDockFloatFrame(label, wxGetMousePosition(), wxSize(400,300));
    FlatDockContainer* cont = new FlatDockContainer(m_manager, floatFrame);
    cont->AddPage(page, label, true);
    floatFrame->AttachContainer(cont);
    floatFrame->Show();
}

void FlatDockContainer::MoveSelectedToManagerRegion(int pos)
{
    if (!m_manager) return;
    int sel = m_notebook->GetSelection();
    if (sel < 0) return;
    wxWindow* page = m_notebook->GetPage(sel);
    wxString label = m_notebook->GetPageText(sel);
    m_notebook->RemovePage(sel);
    // Reparent will be handled by AddPane
    m_manager->AddPane(page, static_cast<FlatDockManager::DockPos>(pos));
}


