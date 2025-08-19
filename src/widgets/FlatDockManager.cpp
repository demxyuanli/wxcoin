#include "widgets/FlatDockManager.h"
#include <wx/settings.h>
#include <wx/dcbuffer.h>
#include <wx/splitter.h>
#include <wx/dcclient.h>
#include <wx/overlay.h>

FlatDockManager::FlatDockManager(wxWindow* parent)
	: wxPanel(parent, wxID_ANY)
	, m_mainHSplitter(nullptr)
	, m_leftVSplitter(nullptr)
	, m_leftTopPane(nullptr)
	, m_leftBottomPane(nullptr)
	, m_centerPane(nullptr)
    , m_bottomContainer(nullptr)
	, m_pendingLeftHeightPx(-1)
    , m_pendingBottomHeightPx(-1)
    , m_previewVisible(false)
    , m_previewTarget(nullptr)
    , m_previewRegion(DockRegion::Center)
    , m_previewInsertIndex(-1)
    , m_previewCaretX(-1)
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetDoubleBuffered(true);
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

    // Create root sizer first so EnsureSplitters can safely add bottom container
    wxBoxSizer* root = new wxBoxSizer(wxVERTICAL);
    SetSizer(root);

    EnsureSplitters();

    // Add the main splitter first (fills center), then bottom container to ensure it is at the bottom
    GetSizer()->Add(m_mainHSplitter, 1, wxEXPAND);
    if (m_bottomContainer) {
        GetSizer()->Add(m_bottomContainer, 0, wxEXPAND);
    }

    Bind(wxEVT_PAINT, &FlatDockManager::OnPaint, this);
    // Hide overlays during sash drag to avoid trails/flicker
    if (m_mainHSplitter) {
        m_mainHSplitter->Bind(wxEVT_SPLITTER_SASH_POS_CHANGING, [this](wxSplitterEvent& e){
            if (m_leftTopPane) m_leftTopPane->ShowOverlay(false);
            if (m_leftBottomPane) m_leftBottomPane->ShowOverlay(false);
            if (m_centerPane) m_centerPane->ShowOverlay(false);
            if (m_bottomContainer) m_bottomContainer->ShowOverlay(false);
            // update vertical rubber band rect and trigger minimal repaint
            wxPoint sashScr = m_mainHSplitter->ClientToScreen(wxPoint(e.GetSashPosition(), 0));
            int x = ScreenToClient(sashScr).x;
            int penW = std::max(1, FromDIP(4, this));
            int left = x - penW / 2;
            wxRect band(left, 0, penW, GetClientSize().y);
            if (band != m_sashBandRect) {
                int inflate = penW + 2;
                wxRect dirty = m_sashBandRect;
                dirty.Inflate(inflate, inflate);
                m_sashBandRect = band;
                wxRect nb = band;
                nb.Inflate(inflate, inflate);
                dirty.Union(nb);
                m_sashBandVisible = true;
                RefreshRect(dirty, false);
            }
            e.Skip();
        });
        m_mainHSplitter->Bind(wxEVT_SPLITTER_SASH_POS_CHANGED, [this](wxSplitterEvent& e){
            if (m_leftTopPane) m_leftTopPane->ShowOverlay(true);
            if (m_leftBottomPane) m_leftBottomPane->ShowOverlay(true);
            if (m_centerPane) m_centerPane->ShowOverlay(true);
            if (m_bottomContainer) m_bottomContainer->ShowOverlay(true);
            EraseSashRubberBand();
            e.Skip();
        });
    }
    if (m_leftVSplitter) {
        m_leftVSplitter->Bind(wxEVT_SPLITTER_SASH_POS_CHANGING, [this](wxSplitterEvent& e){
            if (m_leftTopPane) m_leftTopPane->ShowOverlay(false);
            if (m_leftBottomPane) m_leftBottomPane->ShowOverlay(false);
            // update horizontal rubber band rect and trigger minimal repaint
            wxPoint sashScr = m_leftVSplitter->ClientToScreen(wxPoint(0, e.GetSashPosition()));
            int y = ScreenToClient(sashScr).y;
            int penW = std::max(1, FromDIP(4, this));
            int top = y - penW / 2;
            wxRect band(0, top, GetClientSize().x, penW);
            if (band != m_sashBandRect) {
                int inflate = penW + 2;
                wxRect dirty = m_sashBandRect;
                dirty.Inflate(inflate, inflate);
                m_sashBandRect = band;
                wxRect nb = band;
                nb.Inflate(inflate, inflate);
                dirty.Union(nb);
                m_sashBandVisible = true;
                RefreshRect(dirty, false);
            }
            e.Skip();
        });
        m_leftVSplitter->Bind(wxEVT_SPLITTER_SASH_POS_CHANGED, [this](wxSplitterEvent& e){
            if (m_leftTopPane) m_leftTopPane->ShowOverlay(true);
            if (m_leftBottomPane) m_leftBottomPane->ShowOverlay(true);
            EraseSashRubberBand();
            e.Skip();
        });
    }
}

void FlatDockManager::EnsureSplitters()
{
	if (!m_mainHSplitter) {
		m_mainHSplitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D);
		m_mainHSplitter->SetSashGravity(0.0);
		m_mainHSplitter->SetMinimumPaneSize(100);
	}
	if (!m_leftVSplitter) {
		m_leftVSplitter = new wxSplitterWindow(m_mainHSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D);
		m_leftVSplitter->SetSashGravity(0.0);
		m_leftVSplitter->SetMinimumPaneSize(100);
	}

	// temporary holders until panes are provided
    if (!m_leftTopPane) {
        m_leftTopPane = new FlatDockContainer(this, m_leftVSplitter);
    }
    if (!m_leftBottomPane) {
        m_leftBottomPane = new FlatDockContainer(this, m_leftVSplitter);
    }
    if (!m_centerPane) {
        m_centerPane = new FlatDockContainer(this, m_mainHSplitter);
        // Center: no title bar and no docking into center
        m_centerPane->SetSystemButtonsVisible(false);
        m_centerPane->SetDockingEnabled(false);
        m_centerPane->SetHideTabs(true);
    }
    if (!m_bottomContainer) {
        m_bottomContainer = new FlatDockContainer(this, this);
    }

	if (!m_leftVSplitter->IsSplit()) {
		m_leftVSplitter->SplitHorizontally(m_leftTopPane, m_leftBottomPane, 200);
	}
	if (!m_mainHSplitter->IsSplit()) {
		m_mainHSplitter->SplitVertically(m_leftVSplitter, m_centerPane, 220);
	}
}

void FlatDockManager::AddPane(wxWindow* pane, DockPos pos, int sizePx)
{
	EnsureSplitters();

	switch (pos) {
    case DockPos::LeftTop:
        pane->Reparent(m_leftTopPane->GetNotebook());
        m_leftTopPane->AddPage(pane, pane->GetName().empty() ? "Pane" : pane->GetName(), true);
        m_pendingLeftHeightPx = sizePx;
        break;
    case DockPos::LeftBottom:
        pane->Reparent(m_leftBottomPane->GetNotebook());
        m_leftBottomPane->AddPage(pane, pane->GetName().empty() ? "Pane" : pane->GetName(), true);
        break;
    case DockPos::Center:
        if (m_centerPane->IsHideTabs()) {
            if (!m_centerPane->AddContentToHost(pane)) {
                // Fallback
                pane->Reparent(m_centerPane);
                if (m_centerPane->GetSizer()) m_centerPane->GetSizer()->Add(pane, 1, wxEXPAND);
                m_centerPane->Layout();
            }
        } else {
            pane->Reparent(m_centerPane->GetNotebook());
            m_centerPane->AddPage(pane, pane->GetName().empty() ? "Pane" : pane->GetName(), true);
        }
        break;
    case DockPos::Bottom:
        pane->Reparent(m_bottomContainer->GetNotebook());
        m_bottomContainer->AddPage(pane, pane->GetName().empty() ? "Pane" : pane->GetName(), true);
        m_pendingBottomHeightPx = sizePx;
        break;
	}

	ApplyPendingSizes();
	Layout();
}

void FlatDockManager::ApplyPendingSizes()
{
	if (m_pendingLeftHeightPx > 0 && m_leftVSplitter) {
		m_leftVSplitter->SetSashPosition(m_pendingLeftHeightPx);
		m_pendingLeftHeightPx = -1;
	}
    if (m_pendingBottomHeightPx > 0 && m_bottomContainer) {
        m_bottomContainer->SetMinSize(wxSize(-1, m_pendingBottomHeightPx));
		m_pendingBottomHeightPx = -1;
	}
}

void FlatDockManager::ShowDockPreview(const wxPoint& screenPt)
{
    if (!IsShownOnScreen()) return;
    // Prefer hovered container under cursor
    FlatDockContainer* hovered = nullptr;
    HitTestContainer(screenPt, 24, hovered);
    if (!hovered) hovered = m_centerPane;
    m_previewTarget = hovered;
    m_previewRegion = ComputeRegionForPoint(hovered, screenPt);
    wxRect r = hovered->GetScreenRect();
    m_previewRect = wxRect(ScreenToClient(r.GetTopLeft()), r.GetSize());
    m_previewVisible = true;
    Refresh();
}

void FlatDockManager::HideDockPreview()
{
    if (m_previewVisible) {
        m_previewVisible = false;
        Refresh();
    }
}

void FlatDockManager::DrawSashRubberBand(const wxRect& band)
{
    // no-op: drawing moved to OnPaint to avoid artifacts
    m_sashBandRect = band;
    m_sashBandVisible = true;
}

void FlatDockManager::EraseSashRubberBand()
{
    if (!m_sashBandVisible) return;
    wxRect dirty = m_sashBandRect;
    m_sashBandVisible = false;
    m_sashBandRect = wxRect();
    RefreshRect(dirty, false);
}

bool FlatDockManager::HitTestContainer(const wxPoint& screenPt, int marginPx, FlatDockContainer*& out) const
{
    wxUnusedVar(marginPx);
    out = nullptr;
    wxWindow* target = wxFindWindowAtPoint(screenPt);
    for (wxWindow* w = target; w; w = w->GetParent()) {
        if (auto* c = dynamic_cast<FlatDockContainer*>(w)) { out = c; return true; }
    }
    return false;
}

FlatDockManager::DockRegion FlatDockManager::ComputeRegionForPoint(FlatDockContainer* hovered, const wxPoint& screenPt) const
{
    if (!hovered) return DockRegion::Center;
    wxRect r = hovered->GetScreenRect();
    if (!r.Contains(screenPt)) return DockRegion::Center;
    int w = r.GetWidth();
    int h = r.GetHeight();
    int margin = std::max(24, std::min(w, h) / 6);
    wxRect left(r.GetX(), r.GetY(), margin, h);
    wxRect right(r.GetRight() - margin + 1, r.GetY(), margin, h);
    wxRect top(r.GetX(), r.GetY(), w, margin);
    wxRect bottom(r.GetX(), r.GetBottom() - margin + 1, w, margin);
    if (left.Contains(screenPt)) return DockRegion::Left;
    if (right.Contains(screenPt)) return DockRegion::Right;
    if (top.Contains(screenPt)) return DockRegion::Top;
    if (bottom.Contains(screenPt)) return DockRegion::Bottom;
    return DockRegion::Center;
}

bool FlatDockManager::PerformDock(wxWindow* page, const wxString& label, FlatDockContainer* hovered, DockRegion region)
{
    if (!page) return false;
    EnsureSplitters();
    // Center: insert into hovered container
    if (!hovered) hovered = m_centerPane;

    if (region == DockRegion::Center) {
        page->Reparent(hovered->GetNotebook());
        hovered->AcceptDraggedPage(page, label, true);
        return true;
    }

    // Map side regions to known containers. For simplicity:
    // Left => leftTop by default, Top => leftTop, Bottom => bottom container, Right => center
    if (region == DockRegion::Left || region == DockRegion::Top) {
        page->Reparent(m_leftTopPane->GetNotebook());
        m_leftTopPane->AcceptDraggedPage(page, label, true);
        return true;
    }
    if (region == DockRegion::Bottom) {
        page->Reparent(m_bottomContainer->GetNotebook());
        m_bottomContainer->AcceptDraggedPage(page, label, true);
        return true;
    }
    if (region == DockRegion::Right) {
        page->Reparent(m_centerPane->GetNotebook());
        m_centerPane->AcceptDraggedPage(page, label, true);
        return true;
    }
    return false;
}

void FlatDockManager::CloseContainer(FlatDockContainer* container)
{
    if (!container) return;
    EnsureSplitters();
    if (container == m_bottomContainer) {
        // Hide bottom container area
        m_bottomContainer->Hide();
        Layout();
        return;
    }
    if (container == m_leftTopPane || container == m_leftBottomPane) {
        // If both left panes exist, try to unsplit and leave the other
        if (m_leftVSplitter && m_leftVSplitter->IsSplit()) {
            wxWindow* other = (container == m_leftTopPane) ? static_cast<wxWindow*>(m_leftBottomPane)
                                                           : static_cast<wxWindow*>(m_leftTopPane);
            m_leftVSplitter->Unsplit(container);
            if (other) other->Show();
            Layout();
            return;
        }
    }
    if (container == m_centerPane) {
        // Center cannot be removed; hide all pages instead
        container->Hide();
        Layout();
        return;
    }
}

FlatDockContainer* FlatDockManager::FindSnapTarget(const wxPoint& screenPt, int marginPx) const
{
    if (!this || IsBeingDeleted()) return nullptr;
    FlatDockContainer* c = nullptr;
    if (HitTestContainer(screenPt, marginPx, c)) return c;

    // Fallback: map to nearest logical region
    wxPoint client = ScreenToClient(screenPt);
    wxRect area = GetClientRect();
    if (!area.Contains(client)) return nullptr;
    int w = area.GetWidth();
    int h = area.GetHeight();
    int third = w / 3;
    if (client.x < third) {
        return m_leftTopPane ? m_leftTopPane : nullptr;
    }
    if (client.x > 2 * third) {
        return m_centerPane ? m_centerPane : nullptr;
    }
    if (client.y > (3 * h) / 4) {
        // bottom is not a tab container; snap to center by default
        return m_centerPane ? m_centerPane : nullptr;
    }
    return m_centerPane ? m_centerPane : nullptr;
}

void FlatDockManager::SetInsertionPreview(FlatDockContainer* target, int insertIndex, const wxRect& notebookScreenRect, int caretScreenX)
{
    if (!target) {
        // clear
        const_cast<FlatDockManager*>(this)->m_previewInsertIndex = -1;
        const_cast<FlatDockManager*>(this)->m_previewCaretX = -1;
        const_cast<FlatDockManager*>(this)->Refresh();
        return;
    }
    m_previewTarget = target;
    m_previewInsertIndex = insertIndex;
    m_previewNotebookRect = wxRect(ScreenToClient(notebookScreenRect.GetTopLeft()), notebookScreenRect.GetSize());
    m_previewCaretX = ScreenToClient(wxPoint(caretScreenX, 0)).x;
    m_previewVisible = true;
    Refresh();
}

void FlatDockManager::OnPaint(wxPaintEvent&)
{
    wxAutoBufferedPaintDC dc(this);
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();
    // draw sash rubber band in paint to avoid XOR/overlay artifacts
    if (m_sashBandVisible && !m_sashBandRect.IsEmpty()) {
        int penW = std::max(1, FromDIP(4, this));
        dc.SetPen(wxPen(wxColour(0, 120, 215), penW));
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(m_sashBandRect);
    }
    if (m_previewVisible) {
        wxColour c(30, 144, 255, 80); // dodger blue with alpha
        dc.SetBrush(wxBrush(c));
        dc.SetPen(wxPen(wxColour(30, 144, 255), 2));
        wxRect r = m_previewRect;
        // Optional region shaping (left/right/top/bottom/center)
        if (m_previewRegion == DockRegion::Left) {
            r.SetWidth(r.GetWidth() / 2);
        } else if (m_previewRegion == DockRegion::Right) {
            r = wxRect(r.GetX() + r.GetWidth() / 2, r.GetY(), r.GetWidth() / 2, r.GetHeight());
        } else if (m_previewRegion == DockRegion::Top) {
            r.SetHeight(r.GetHeight() / 2);
        } else if (m_previewRegion == DockRegion::Bottom) {
            r = wxRect(r.GetX(), r.GetY() + r.GetHeight() / 2, r.GetWidth(), r.GetHeight() / 2);
        }
        dc.DrawRectangle(r);

        if (m_previewInsertIndex >= 0 && m_previewCaretX >= 0) {
            wxRect nbr = m_previewNotebookRect;
            int x = m_previewCaretX;
            x = std::max(nbr.GetX() + 10, std::min(nbr.GetRight() - 10, x));
            dc.SetPen(wxPen(wxColour(255, 80, 80), 2));
            dc.DrawLine(x, nbr.GetY() + 4, x, nbr.GetY() + 24);
        }
    }
}


