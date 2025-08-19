#include "widgets/ModernDockAdapter.h"
#include "widgets/DockTypes.h"
#include <wx/sizer.h>

ModernDockAdapter::ModernDockAdapter(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
    , m_modernManager(nullptr)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetDoubleBuffered(true);
    
    // Create the modern dock manager
    m_modernManager = new ModernDockManager(this);
    
    // Set up layout
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_modernManager, 1, wxEXPAND);
    SetSizer(sizer);
    
    CreateDefaultLayout();
}

void ModernDockAdapter::CreateDefaultLayout()
{
    // Create default dock areas that match the original layout
    // We'll create panels on-demand when AddPane is called
}

void ModernDockAdapter::AddPane(wxWindow* pane, DockPos pos, int sizePx)
{
    if (!pane || !m_modernManager) return;
    
    // Store pending size for later use
    if (sizePx > 0) {
        m_pendingSizes[pos] = sizePx;
    }
    
    // Convert dock position to modern dock area
    DockArea area = ConvertDockPos(pos);
    
    // Create a modern dock panel to host the legacy window
    wxString panelTitle = pane->GetName();
    if (panelTitle.IsEmpty()) {
        // Generate default titles based on position
        switch (pos) {
            case DockPos::LeftTop:
                panelTitle = "Object Tree";
                break;
            case DockPos::LeftBottom:
                panelTitle = "Properties";
                break;
            case DockPos::Center:
                panelTitle = "Viewport";
                break;
            case DockPos::Bottom:
                panelTitle = "Output";
                break;
        }
    }
    
    // Apply size hints to the original pane if provided
    if (sizePx > 0) {
        switch (pos) {
            case DockPos::LeftTop:
            case DockPos::LeftBottom:
                pane->SetMinSize(wxSize(sizePx, -1));
                break;
            case DockPos::Bottom:
                pane->SetMinSize(wxSize(-1, sizePx));
                break;
        }
    }
    
    // Add to modern dock manager (this will create a ModernDockPanel internally)
    m_modernManager->AddPanel(pane, panelTitle, area);
    
    // Find the created panel for our mapping
    ModernDockPanel* createdPanel = m_modernManager->FindPanel(panelTitle);
    if (createdPanel) {
        m_panelMap[pos] = createdPanel;
    }
    
    Layout();
}

DockArea ModernDockAdapter::ConvertDockPos(DockPos pos)
{
    switch (pos) {
        case DockPos::LeftTop:
        case DockPos::LeftBottom:
            return DockArea::Left;
        case DockPos::Center:
            return DockArea::Center;
        case DockPos::Bottom:
            return DockArea::Bottom;
        default:
            return DockArea::Center;
    }
}

void ModernDockAdapter::ShowDockPreview(const wxPoint& screenPt)
{
    // Delegate to modern dock manager
    if (m_modernManager) {
        // Convert screen point to client coordinates
        wxPoint clientPt = m_modernManager->ScreenToClient(screenPt);
        // Modern system handles preview automatically during drag operations
    }
}

void ModernDockAdapter::HideDockPreview()
{
    // Modern system handles this automatically
    if (m_modernManager) {
        m_modernManager->Refresh();
    }
}
