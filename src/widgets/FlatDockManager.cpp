#include "widgets/FlatDockManager.h"
#include <wx/settings.h>
#include <wx/dcbuffer.h>

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
}

void FlatDockManager::EnsureSplitters()
{
	if (!m_mainHSplitter) {
		m_mainHSplitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D);
		m_mainHSplitter->SetSashGravity(0.0);
		m_mainHSplitter->SetMinimumPaneSize(100);
	}
	if (!m_leftVSplitter) {
		m_leftVSplitter = new wxSplitterWindow(m_mainHSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE | wxSP_3D);
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
        pane->Reparent(m_centerPane->GetNotebook());
        m_centerPane->AddPage(pane, pane->GetName().empty() ? "Pane" : pane->GetName(), true);
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
    wxPoint client = ScreenToClient(screenPt);
    wxRect area = GetClientRect();
    int w = area.GetWidth();
    int h = area.GetHeight();
    int edge = std::max(24, std::min(w, h) / 10);

    m_previewInsertIndex = -1;
    m_previewCaretX = -1;
    m_previewTarget = nullptr;

    if (client.x <= area.GetX() + edge) {
        m_previewTarget = m_leftTopPane;
        m_previewRegion = DockRegion::Left;
    } else if (client.x >= area.GetRight() - edge) {
        m_previewTarget = m_centerPane; // right maps to center in current layout
        m_previewRegion = DockRegion::Right;
    } else if (client.y <= area.GetY() + edge) {
        m_previewTarget = m_centerPane; // top maps to center until top container exists
        m_previewRegion = DockRegion::Top;
    } else if (client.y >= area.GetBottom() - edge) {
        m_previewTarget = m_bottomContainer;
        m_previewRegion = DockRegion::Bottom;
    } else {
        m_previewTarget = m_centerPane;
        m_previewRegion = DockRegion::Center;
    }

    if (m_previewTarget) {
        wxRect r = m_previewTarget->GetScreenRect();
        m_previewRect = wxRect(ScreenToClient(r.GetTopLeft()), r.GetSize());
        m_previewVisible = true;
        Refresh();
    }
}

void FlatDockManager::HideDockPreview()
{
    if (m_previewVisible) {
        m_previewVisible = false;
        Refresh();
    }
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

bool FlatDockManager::EvaluateDropTarget(const wxPoint& screenPt, FlatDockContainer*& outTarget, DockRegion& outRegion) const
{
    outTarget = nullptr;
    if (!IsShownOnScreen()) return false;
    wxPoint client = ScreenToClient(screenPt);
    wxRect area = GetClientRect();
    int w = area.GetWidth();
    int h = area.GetHeight();
    int edge = std::max(24, std::min(w, h) / 10);
    if (client.x <= area.GetX() + edge) { outTarget = m_leftTopPane; outRegion = DockRegion::Left; return outTarget != nullptr; }
    if (client.x >= area.GetRight() - edge) { outTarget = m_centerPane; outRegion = DockRegion::Right; return outTarget != nullptr; }
    if (client.y <= area.GetY() + edge) { outTarget = m_centerPane; outRegion = DockRegion::Top; return outTarget != nullptr; }
    if (client.y >= area.GetBottom() - edge) { outTarget = m_bottomContainer; outRegion = DockRegion::Bottom; return outTarget != nullptr; }
    outTarget = m_centerPane; outRegion = DockRegion::Center; return outTarget != nullptr;
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


