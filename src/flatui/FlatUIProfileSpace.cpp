#include "flatui/FlatUIProfileSpace.h"
#include "config/ThemeManager.h"


FlatUIProfileSpace::FlatUIProfileSpace(wxWindow* parent, wxWindowID id)
    : wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE),
    m_childControl(nullptr),
    m_spaceWidth(CFG_INT("SpaceDefaulWidth"))
{

    Bind(wxEVT_SIZE, &FlatUIProfileSpace::OnSize, this);
    Bind(wxEVT_PAINT, &FlatUIProfileSpace::OnPaint, this);
    
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

FlatUIProfileSpace::~FlatUIProfileSpace() {}

void FlatUIProfileSpace::SetChildControl(wxWindow* child)
{
    if (m_childControl && m_childControl != child) {
        m_childControl->Hide();
    }
    m_childControl = child;
    if (m_childControl) {
        if (m_childControl->GetParent() != this) {
            m_childControl->Reparent(this);
        }
        wxSize panelSz = GetClientSize();
        int w = panelSz.GetWidth();
        int h = panelSz.GetHeight();
        if (w < 0) w = 0;
        if (h < 0) h = 0;
        m_childControl->SetSize(w, h);
        m_childControl->Show();
    }
    Layout();
    Refresh();
}

void FlatUIProfileSpace::SetSpaceWidth(int width)
{
    if (width > 0) {
        m_spaceWidth = width;
        SetMinSize(wxSize(m_spaceWidth, GetMinSize().y));
        if (GetParent()) {
            GetParent()->Layout();
        }
    }
}

int FlatUIProfileSpace::GetSpaceWidth() const
{
    return m_spaceWidth;
}

void FlatUIProfileSpace::OnSize(wxSizeEvent& evt)
{
    if (m_childControl) {
        wxSize sz = evt.GetSize();
        int w = sz.GetWidth();
        int h = sz.GetHeight();
        if (w < 0) w = 0;
        if (h < 0) h = 0;
        m_childControl->SetSize(w, h);
    }
    evt.Skip();
}

void FlatUIProfileSpace::OnPaint(wxPaintEvent& evt)
{
    wxPaintDC dc(this);
    
    // Fill background with theme color
    wxColour bgColor = CFG_COLOUR("BarBgColour");
    dc.SetBackground(wxBrush(bgColor));
    dc.Clear();
}
