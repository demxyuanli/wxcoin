#include "flatui/FlatUIFunctionSpace.h"
#include "config/ThemeManager.h"


FlatUIFunctionSpace::FlatUIFunctionSpace(wxWindow* parent, wxWindowID id)
    : wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL | wxBORDER_NONE | wxFULL_REPAINT_ON_RESIZE),
      m_childControl(nullptr),
      m_spaceWidth(CFG_INT("SpaceDefaulWidth"))
{
    Bind(wxEVT_SIZE, &FlatUIFunctionSpace::OnSize, this);
}

FlatUIFunctionSpace::~FlatUIFunctionSpace()
{
    // m_childControl is typically managed by its parent (this panel) if it was new'd with this as parent,
    // or by whoever created it if it was just passed to SetChildControl.
    // If FlatUIFunctionSpace explicitly takes ownership, then it should delete m_childControl here.
    // For now, assuming standard wxWidgets parent/child ownership or external management.
}

void FlatUIFunctionSpace::SetChildControl(wxWindow* child)
{
    if (m_childControl && m_childControl != child) {
        // If replacing an existing child that this panel owns, destroy it first.
        // For now, we assume simple replacement or that ownership is handled elsewhere.
        m_childControl->Hide(); 
    }

    m_childControl = child;

    if (m_childControl) {
        if (m_childControl->GetParent() != this) {
            m_childControl->Reparent(this);
        }
        // Apply margin inside this panel
        wxSize panelSz = GetClientSize();
        int w = panelSz.GetWidth();
        int h = panelSz.GetHeight();
        if (w < 0) w = 0;
        if (h < 0) h = 0;
        m_childControl->SetSize(w, h);
        m_childControl->Show();
    }
    Layout(); // Trigger layout for the panel itself if it had its own sizer (not in this simple case)
    Refresh(); // Refresh this panel
}

void FlatUIFunctionSpace::SetSpaceWidth(int width)
{
    if (width > 0) {
        m_spaceWidth = width;
        SetMinSize(wxSize(m_spaceWidth, GetMinSize().y));
        // FlatUIBar will call SetSize on this control later, this just updates the hint.
        // If this control is already created and sized, its parent (FlatUIBar) should re-layout.
        if (GetParent()) {
            GetParent()->Layout(); 
        }
    }
}

int FlatUIFunctionSpace::GetSpaceWidth() const
{
    return m_spaceWidth;
}

void FlatUIFunctionSpace::OnSize(wxSizeEvent& evt)
{
    if (m_childControl) {
        // Resize child control with margin
        wxSize sz = evt.GetSize();
        int w = sz.GetWidth();
        int h = sz.GetHeight();
        if (w < 0) w = 0;
        if (h < 0) h = 0;
        m_childControl->SetSize(w, h);
    }
    evt.Skip(); // Allow other handlers for this event if any
}
