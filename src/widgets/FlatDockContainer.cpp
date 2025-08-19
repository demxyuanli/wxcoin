#include "widgets/FlatDockContainer.h"
#include "widgets/FlatDockManager.h"
#include <wx/frame.h>
#include <wx/menu.h>
#include <wx/statbmp.h>
#include <wx/dcmemory.h>
#include <wx/dcclient.h>
#include <wx/dcbuffer.h>
#include "widgets/FlatEnhancedButton.h"
#include "config/SvgIconManager.h"

FlatDockContainer::FlatDockContainer(FlatDockManager* manager, wxWindow* parent)
    : wxPanel(parent, wxID_ANY),
      m_manager(manager),
      m_notebook(new wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNB_TOP)),
      m_dragging(false),
      m_dragStarted(false),
      m_dragPageIndex(-1),
      m_tabOverlay(nullptr),
      m_btnFloat(nullptr),
      m_btnMaximize(nullptr),
      m_btnMinimize(nullptr),
      m_btnClose(nullptr),
      m_dragPreview(nullptr),
      m_dockingEnabled(true),
      m_systemButtonsVisible(true),
      m_hideTabs(false),
      m_singleHost(nullptr)
{
    // Create overlay inside notebook (sits on top of tabbar area)
    m_tabOverlay = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTRANSPARENT_WINDOW | wxBORDER_NONE);
    m_tabOverlay->Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent& ev){ ev.Skip(false); });
    m_tabOverlay->Raise();

    // System buttons hosted by overlay
    m_btnFloat = new FlatEnhancedButton(m_tabOverlay, ID_BTN_FLOAT, "", wxDefaultPosition, wxSize(24, 24));
    m_btnFloat->SetBitmap(SVG_ICON("float", wxSize(16, 16)));
    m_btnMaximize = new FlatEnhancedButton(m_tabOverlay, ID_BTN_MAXIMIZE, "", wxDefaultPosition, wxSize(24, 24));
    m_btnMaximize->SetBitmap(SVG_ICON("maximize", wxSize(16, 16)));
    m_btnMinimize = new FlatEnhancedButton(m_tabOverlay, ID_BTN_MINIMIZE, "", wxDefaultPosition, wxSize(24, 24));
    m_btnMinimize->SetBitmap(SVG_ICON("minimize", wxSize(16, 16)));
    m_btnClose = new FlatEnhancedButton(m_tabOverlay, ID_BTN_CLOSE, "", wxDefaultPosition, wxSize(24, 24));
    m_btnClose->SetBitmap(SVG_ICON("close", wxSize(16, 16)));
    // Layout: start with notebook; singleHost can replace it when hideTabs is true
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    s->Add(m_notebook, 1, wxEXPAND);
    SetSizer(s);

    // Bind button events
    Bind(wxEVT_BUTTON, &FlatDockContainer::OnButtonFloat, this, ID_BTN_FLOAT);
    Bind(wxEVT_BUTTON, &FlatDockContainer::OnButtonMaximize, this, ID_BTN_MAXIMIZE);
    Bind(wxEVT_BUTTON, &FlatDockContainer::OnButtonMinimize, this, ID_BTN_MINIMIZE);
    Bind(wxEVT_BUTTON, &FlatDockContainer::OnButtonClose, this, ID_BTN_CLOSE);

    // Existing bindings...
    m_notebook->Bind(wxEVT_LEFT_DOWN, &FlatDockContainer::OnLeftDown, this);
    m_notebook->Bind(wxEVT_MOTION, &FlatDockContainer::OnMouseMove, this);
    m_notebook->Bind(wxEVT_LEFT_UP, &FlatDockContainer::OnLeftUp, this);
    m_notebook->Bind(wxEVT_LEFT_DCLICK, &FlatDockContainer::OnLeftDClick, this);
    m_notebook->Bind(wxEVT_CONTEXT_MENU, &FlatDockContainer::OnContextMenu, this);
    m_notebook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &FlatDockContainer::OnNotebookPageChanged, this);
    Bind(wxEVT_SIZE, [this](wxSizeEvent& ev){ LayoutTabOverlay(); ev.Skip(); });
    m_notebook->Bind(wxEVT_SIZE, [this](wxSizeEvent& ev){ LayoutTabOverlay(); ev.Skip(); });
    LayoutTabOverlay();
}

void FlatDockContainer::SetHideTabs(bool hide)
{
    if (m_hideTabs == hide) { return; }
    m_hideTabs = hide;
    if (hide) {
        // Replace notebook with a simple host panel holding only the selected page
        if (!m_singleHost) m_singleHost = new wxPanel(this);
        if (m_notebook->GetPageCount() > 0) {
            int sel = m_notebook->GetSelection();
            if (sel < 0) sel = 0;
            wxWindow* page = m_notebook->GetPage(sel);
            m_notebook->RemovePage(sel);
            page->Reparent(m_singleHost);
            wxBoxSizer* hs = new wxBoxSizer(wxVERTICAL);
            hs->Add(page, 1, wxEXPAND);
            m_singleHost->SetSizer(hs);
        }
        GetSizer()->Detach(m_notebook);
        m_notebook->Hide();
        GetSizer()->Add(m_singleHost, 1, wxEXPAND);
        if (m_tabOverlay) m_tabOverlay->Hide();
    } else {
        // Restore notebook if available
        if (m_singleHost && m_singleHost->GetSizer() && m_singleHost->GetSizer()->GetItemCount() > 0) {
            wxWindow* page = m_singleHost->GetSizer()->GetItem((size_t)0)->GetWindow();
            if (page) {
                page->Reparent(m_notebook);
                wxString label = page->GetName().empty() ? "Page" : page->GetName();
                m_notebook->AddPage(page, label, true);
            }
        }
        if (m_singleHost) { GetSizer()->Detach(m_singleHost); m_singleHost->Destroy(); m_singleHost = nullptr; }
        GetSizer()->Add(m_notebook, 1, wxEXPAND);
        m_notebook->Show();
        if (m_tabOverlay) m_tabOverlay->Show(m_systemButtonsVisible);
    }
    Layout();
}

bool FlatDockContainer::AddContentToHost(wxWindow* page)
{
    if (!m_hideTabs) return false;
    if (!page) return false;
    if (!m_singleHost) m_singleHost = new wxPanel(this);
    // Ensure singleHost is in sizer
    if (!m_singleHost->GetContainingSizer()) {
        if (GetSizer()) {
            GetSizer()->Add(m_singleHost, 1, wxEXPAND);
        } else {
            wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
            s->Add(m_singleHost, 1, wxEXPAND);
            SetSizer(s);
        }
    }
    // Reparent and attach page
    page->Reparent(m_singleHost);
    wxBoxSizer* hs = dynamic_cast<wxBoxSizer*>(m_singleHost->GetSizer());
    if (!hs) { hs = new wxBoxSizer(wxVERTICAL); m_singleHost->SetSizer(hs); }
    hs->Add(page, 1, wxEXPAND);
    m_notebook->Hide();
    if (m_tabOverlay) m_tabOverlay->Hide();
    Layout();
    return true;
}

int FlatDockContainer::GetTabBarHeightApprox() const
{
    // Approximate tab height using font metrics + padding
    int h = FromDIP(28);
    return h;
}

void FlatDockContainer::LayoutTabOverlay()
{
    if (!m_tabOverlay) return;
    wxRect nb = m_notebook->GetClientRect();
    // Determine overlay width based on buttons
    int margin = FromDIP(4);
    wxSize bs = m_btnClose->GetSize();
    int buttonCount = 4;
    int overlayW = buttonCount * bs.x + (buttonCount + 1) * margin;
    int tabH = GetTabBarHeightApprox();

    // Position overlay at right side of tab area (inside parent's client coords)
    wxPoint nbTopLeft = m_notebook->GetPosition();
    int overlayX = nbTopLeft.x + nb.GetWidth() - overlayW;
    if (overlayX < nbTopLeft.x) overlayX = nbTopLeft.x; // guard
    m_tabOverlay->SetSize(wxSize(overlayW, tabH));
    m_tabOverlay->Move(wxPoint(overlayX, nbTopLeft.y));

    // Layout buttons inside overlay from right to left
    int x = overlayW - margin - bs.x;
    m_btnClose->SetPosition(wxPoint(x, (tabH - bs.y) / 2));
    x -= (bs.x + margin);
    m_btnMinimize->SetPosition(wxPoint(x, (tabH - bs.y) / 2));
    x -= (bs.x + margin);
    m_btnMaximize->SetPosition(wxPoint(x, (tabH - bs.y) / 2));
    x -= (bs.x + margin);
    m_btnFloat->SetPosition(wxPoint(x, (tabH - bs.y) / 2));
    m_tabOverlay->Raise();
    m_tabOverlay->Show(m_systemButtonsVisible);
}

void FlatDockContainer::ShowOverlay(bool show)
{
    if (!m_tabOverlay) return;
    m_tabOverlay->Show(show && m_systemButtonsVisible);
}

// Event handlers
void FlatDockContainer::OnButtonFloat(wxCommandEvent&)
{
    // Implement FloatSelectedTab inline here
    if (m_notebook->GetPageCount() <= 0) return;
    int sel = m_notebook->GetSelection();
    if (sel < 0) sel = 0;
    wxWindow* page = m_notebook->GetPage(sel);
    wxString label = m_notebook->GetPageText(sel);
    m_notebook->RemovePage(sel);
    wxPoint screenPos = wxGetMousePosition();
    wxFrame* floatFrame = new wxFrame(nullptr, wxID_ANY, label, screenPos, wxSize(480, 320));
    FlatDockContainer* floatContainer = new FlatDockContainer(m_manager, floatFrame);
    page->Reparent(floatContainer->GetNotebook());
    floatContainer->AddPage(page, label, true);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    s->Add(floatContainer, 1, wxEXPAND);
    floatFrame->SetSizer(s);
    FlatDockManager* mgr = m_manager;
    floatFrame->Bind(wxEVT_CLOSE_WINDOW, [mgr, floatContainer](wxCloseEvent& ev){
        if (mgr && !mgr->IsBeingDeleted() && floatContainer->GetPageCount() > 0) {
            wxWindow* page0 = floatContainer->GetNotebook()->GetPage(0);
            floatContainer->GetNotebook()->RemovePage(0);
            mgr->AddPane(page0, FlatDockManager::DockPos::Center);
        }
        ev.Skip();
    });
    floatFrame->Show();
}

void FlatDockContainer::OnButtonMaximize(wxCommandEvent&)
{
    if (!m_manager) return;
    // Simple maximize: hide other panes, expand this container to full
    // TODO: Save state for restore
    m_manager->GetMainSplitter()->Unsplit();
    m_manager->GetLeftSplitter()->Unsplit();
    // Assuming this is in left/center/bottom, reparent to manager with full sizer
    // For simplicity, set as only child
    wxSizer* root = m_manager->GetSizer();
    root->Clear(true);
    root->Add(this, 1, wxEXPAND);
    Layout();
}

void FlatDockContainer::OnButtonMinimize(wxCommandEvent&)
{
    // Collapse notebook, show only caption
    GetSizer()->Show(m_notebook, false);
    SetMinSize(wxSize(-1, GetTabBarHeightApprox()));
    GetParent()->Layout();
}

void FlatDockContainer::OnButtonRestore(wxCommandEvent&) // Call this from maximize/minimize toggle
{
    // TODO: Restore previous layout
    GetSizer()->Show(m_notebook, true);
    SetMinSize(wxDefaultSize);
    if (m_manager) m_manager->Layout();
}

void FlatDockContainer::OnButtonClose(wxCommandEvent&)
{
    // Delegate to manager to close container properly
    if (m_manager) {
        m_manager->CloseContainer(this);
    } else {
        Hide();
        GetParent()->Layout();
    }
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
        // insertion preview within hovered container
        wxRect nbRectScreen = wxRect(m_notebook->GetScreenPosition(), m_notebook->GetSize());
        wxPoint nbClient = m_notebook->ScreenToClient(m_notebook->ClientToScreen(e.GetPosition()));
        int idx = computeInsertIndexFromLocalX(nbClient.x);
        int approxTabW = std::max(60, m_notebook->GetSize().x / std::max(1, (int)m_notebook->GetPageCount()));
        int caretScreenX = nbRectScreen.GetX() + 12 + idx * approxTabW;
        m_manager->SetInsertionPreview(this, idx, nbRectScreen, caretScreenX);

        // five-way preview around hovered container (wxAUI-like)
        FlatDockContainer* hovered = this;
        auto region = m_manager->ComputeRegionForPoint(hovered, cur);
        // build region-shaped preview rect
        wxRect hoveredScreen = hovered->GetScreenRect();
        wxRect regionRect = hoveredScreen;
        switch (region) {
            case FlatDockManager::DockRegion::Left:   regionRect.SetWidth(regionRect.GetWidth() / 2); break;
            case FlatDockManager::DockRegion::Right:  regionRect = wxRect(regionRect.GetX() + regionRect.GetWidth() / 2, regionRect.GetY(), regionRect.GetWidth() / 2, regionRect.GetHeight()); break;
            case FlatDockManager::DockRegion::Top:    regionRect.SetHeight(regionRect.GetHeight() / 2); break;
            case FlatDockManager::DockRegion::Bottom: regionRect = wxRect(regionRect.GetX(), regionRect.GetY() + regionRect.GetHeight() / 2, regionRect.GetWidth(), regionRect.GetHeight() / 2); break;
            default: break;
        }
        m_manager->ShowDockPreview(cur);
    }
    if (m_dragStarted && m_dragPreview) {
        wxPoint curScr = m_notebook->ClientToScreen(e.GetPosition());
        m_dragPreview->Move(curScr - wxPoint(10, 10)); // Offset for cursor
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
        // AUI-like: decide region around hovered container and dock accordingly
        wxWindow* hoveredW = wxFindWindowAtPoint(dropScreen);
        FlatDockContainer* hovered = nullptr;
        for (wxWindow* w = hoveredW; w; w = w->GetParent()) {
            hovered = dynamic_cast<FlatDockContainer*>(w);
            if (hovered) break;
        }
        wxWindow* page = m_notebook->GetPage(m_dragPageIndex);
        wxString label = m_notebook->GetPageText(m_dragPageIndex);
        m_notebook->RemovePage(m_dragPageIndex);
        if (m_manager && hovered && hovered->IsDockingEnabled()) {
            auto region = m_manager->ComputeRegionForPoint(hovered, dropScreen);
            if (!m_manager->PerformDock(page, label, hovered, region)) {
                // fallback to center
                m_manager->PerformDock(page, label, nullptr, FlatDockManager::DockRegion::Center);
            }
        } else {
            EndDrag(dropScreen);
        }
    }
    if (m_manager) m_manager->HideDockPreview();
    if (m_dragPreview) {
        m_dragPreview->Destroy();
        m_dragPreview = nullptr;
    }
	e.Skip();
}

// For drag effect: create a semi-transparent preview window
wxFrame* m_dragPreview = nullptr;

void FlatDockContainer::BeginDrag(int pageIndex, const wxPoint& screenPos)
{
    // Create preview: capture bitmap of current page
    if (pageIndex < 0 || pageIndex >= (int)m_notebook->GetPageCount()) return;
    wxWindow* page = m_notebook->GetPage(pageIndex);
    wxSize sz = page->GetClientSize();
    if (sz.x <= 0 || sz.y <= 0) return;
    wxBitmap bmp(sz.x, sz.y);
    wxMemoryDC memdc;
    memdc.SelectObject(bmp);
    wxClientDC clientdc(page);
    memdc.Blit(0, 0, sz.x, sz.y, &clientdc, 0, 0);
    memdc.SelectObject(wxNullBitmap);
    m_dragPreview = new wxFrame(nullptr, wxID_ANY, "", screenPos, sz, wxFRAME_NO_TASKBAR | wxFRAME_FLOAT_ON_PARENT | wxSTAY_ON_TOP);
    wxStaticBitmap* img = new wxStaticBitmap(m_dragPreview, wxID_ANY, bmp);
    wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
    s->Add(img, 1, wxEXPAND);
    m_dragPreview->SetSizer(s);
    m_dragPreview->SetTransparent(180);
    m_dragPreview->Show();
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
    if (m_dragPreview) {
        m_dragPreview->Destroy();
        m_dragPreview = nullptr;
    }
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

void FlatDockContainer::OnLeftDClick(wxMouseEvent& e)
{
    wxUnusedVar(e);
    // Float current tab into a new frame
    if (m_notebook->GetPageCount() <= 0) return;
    int sel = m_notebook->GetSelection();
    if (sel < 0) sel = 0;
    wxWindow* page = m_notebook->GetPage(sel);
    wxString label = m_notebook->GetPageText(sel);
    m_notebook->RemovePage(sel);
    wxPoint screenPos = m_notebook->ClientToScreen(wxPoint(50, 50));
    // Reuse EndDrag float path
    if (m_manager) {
        // Attach to a new frame
        wxFrame* floatFrame = new wxFrame(nullptr, wxID_ANY, label, screenPos, wxSize(480, 320));
        FlatDockContainer* floatContainer = new FlatDockContainer(m_manager, floatFrame);
        page->Reparent(floatContainer->GetNotebook());
        floatContainer->AddPage(page, label, true);
        wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
        s->Add(floatContainer, 1, wxEXPAND);
        floatFrame->SetSizer(s);
        FlatDockManager* mgr = m_manager;
        floatFrame->Bind(wxEVT_CLOSE_WINDOW, [mgr, floatContainer](wxCloseEvent& ev){
            if (mgr && !mgr->IsBeingDeleted() && floatContainer->GetPageCount() > 0) {
                wxWindow* page0 = floatContainer->GetNotebook()->GetPage(0);
                floatContainer->GetNotebook()->RemovePage(0);
                mgr->AddPane(page0, FlatDockManager::DockPos::Center);
            }
            ev.Skip();
        });
        floatFrame->Show();
    }
}

void FlatDockContainer::OnContextMenu(wxContextMenuEvent& e)
{
    wxPoint pt = e.GetPosition();
    if (pt == wxDefaultPosition) pt = wxGetMousePosition();
    wxMenu menu;
    menu.Append(ID_CTX_CLOSE, "Close");
    menu.AppendSeparator();
    menu.Append(ID_CTX_MOVE_LEFT, "Move to Left");
    menu.Append(ID_CTX_MOVE_CENTER, "Move to Center");
    menu.Append(ID_CTX_MOVE_BOTTOM, "Move to Bottom");
    menu.AppendSeparator();
    menu.Append(ID_CTX_FLOAT, "Float");
    Bind(wxEVT_MENU, &FlatDockContainer::OnMenu, this);
    PopupMenu(&menu, ScreenToClient(pt));
}

void FlatDockContainer::OnMenu(wxCommandEvent& e)
{
    int sel = m_notebook->GetSelection();
    if (sel < 0 || sel >= (int)m_notebook->GetPageCount()) return;
    wxWindow* page = m_notebook->GetPage(sel);
    wxString label = m_notebook->GetPageText(sel);
    switch (e.GetId()) {
        case ID_CTX_CLOSE:
            m_notebook->RemovePage(sel);
            page->Hide();
            return;
        case ID_CTX_MOVE_LEFT:
            m_notebook->RemovePage(sel);
            if (m_manager) {
                page->Reparent(m_manager->GetLeftSplitter()); // temp parent; will be reparented below
                m_manager->AddPane(page, FlatDockManager::DockPos::LeftTop);
            }
            return;
        case ID_CTX_MOVE_CENTER:
            m_notebook->RemovePage(sel);
            if (m_manager) {
                m_manager->AddPane(page, FlatDockManager::DockPos::Center);
            }
            return;
        case ID_CTX_MOVE_BOTTOM:
            m_notebook->RemovePage(sel);
            if (m_manager) {
                m_manager->AddPane(page, FlatDockManager::DockPos::Bottom);
            }
            return;
        case ID_CTX_FLOAT:
            m_notebook->RemovePage(sel);
            {
                wxPoint screenPos = wxGetMousePosition();
                wxFrame* floatFrame = new wxFrame(nullptr, wxID_ANY, label, screenPos, wxSize(480, 320));
                FlatDockContainer* floatContainer = new FlatDockContainer(m_manager, floatFrame);
                page->Reparent(floatContainer->GetNotebook());
                floatContainer->AddPage(page, label, true);
                wxBoxSizer* s = new wxBoxSizer(wxVERTICAL);
                s->Add(floatContainer, 1, wxEXPAND);
                floatFrame->SetSizer(s);
                FlatDockManager* mgr = m_manager;
                floatFrame->Bind(wxEVT_CLOSE_WINDOW, [mgr, floatContainer](wxCloseEvent& ev){
                    if (mgr && !mgr->IsBeingDeleted() && floatContainer->GetPageCount() > 0) {
                        wxWindow* page0 = floatContainer->GetNotebook()->GetPage(0);
                        floatContainer->GetNotebook()->RemovePage(0);
                        mgr->AddPane(page0, FlatDockManager::DockPos::Center);
                    }
                    ev.Skip();
                });
                floatFrame->Show();
            }
            return;
        default:
            return;
    }
}

void FlatDockContainer::OnNotebookPageChanged(wxNotebookEvent& e)
{
    LayoutTabOverlay();
    e.Skip();
}



