#include "widgets/ModernDockManager.h"
#include "widgets/ModernDockPanel.h"
#include "widgets/DockGuides.h"
#include "widgets/GhostWindow.h"
#include "widgets/DragDropController.h"
#include "widgets/LayoutEngine.h"
#include "DPIManager.h"
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <algorithm>

wxBEGIN_EVENT_TABLE(ModernDockManager, wxPanel)
    EVT_PAINT(ModernDockManager::OnPaint)
    EVT_SIZE(ModernDockManager::OnSize)
    EVT_TIMER(wxID_ANY, ModernDockManager::OnTimer)
    EVT_MOTION(ModernDockManager::OnMouseMove)
    EVT_LEAVE_WINDOW(ModernDockManager::OnMouseLeave)
wxEND_EVENT_TABLE()

ModernDockManager::ModernDockManager(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE),
      m_dragState(DragState::None),
      m_draggedPanel(nullptr),
      m_hoveredPanel(nullptr),
      m_previewVisible(false),
      m_previewPosition(DockPosition::None),
      m_animationTimer(this),
      m_animationProgress(0.0),
      m_dpiScale(1.0)
{
    InitializeComponents();
    
    // Set background style for custom painting
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetDoubleBuffered(true);
    
    // Initialize DPI scale
    m_dpiScale = DPIManager::getInstance().getDPIScale();
    
    // Set minimum size
    SetMinSize(wxSize(400, 300));
}

ModernDockManager::~ModernDockManager()
{
    // Cleanup is handled by smart pointers
}

void ModernDockManager::InitializeComponents()
{
    // Initialize core components
    m_dockGuides = std::make_unique<DockGuides>(this);
    m_ghostWindow = std::make_unique<GhostWindow>();
    m_dragController = std::make_unique<DragDropController>(this);
    m_layoutEngine = std::make_unique<LayoutEngine>(this);
    
    // Initialize layout engine with this window as root
    m_layoutEngine->InitializeLayout(this);
    
    // Configure drag controller callbacks
    m_dragController->SetDragStartCallback([this](const DragSession& session) {
        m_dragState = DragState::Started;
        m_draggedPanel = session.sourcePanel;
        
        // Show ghost window
        if (session.draggedContent) {
            m_ghostWindow->ShowGhost(session.sourcePanel, session.startPos);
        }
        
        // Show dock guides at start of drag (they stay visible throughout drag)
        if (session.sourcePanel) {
            // Show central indicator, but disable Center/Top/Right responses
            if (m_dockGuides) {
                // Center responsiveness disabled, but central indicator stays visible
                m_dockGuides->SetEnabledDirections(false, true, false, false, true);
                m_dockGuides->SetCentralVisible(true);
            }
            ShowDockGuides(session.sourcePanel);
        }
    });
    
    m_dragController->SetDragUpdateCallback([this](const DragSession& session) {
        m_dragState = DragState::Active;
        
        // Update ghost window position
        m_ghostWindow->UpdatePosition(session.currentPos);
        
        // Validate drop and show preview
        auto validation = m_dragController->ValidateDrop(session.currentPos);
        if (validation.valid) {
            ShowPreviewRect(validation.previewRect, validation.position);
            m_hoveredPanel = validation.targetPanel;
        } else {
            HidePreviewRect();
            m_hoveredPanel = nullptr;
        }
        
        // Always update dock guides during drag (don't hide them)
        m_dockGuides->UpdateGuides(session.currentPos);
    });
    
    m_dragController->SetDragCompleteCallback([this](const DragSession& session, const DropValidation& validation) {
        m_dragState = DragState::Completing;
        
        // Hide visual feedback
        m_ghostWindow->HideGhost();
        HideDockGuides();
        HidePreviewRect();
        
        // Perform the actual docking operation
        if (validation.valid && validation.targetPanel) {
            bool success = m_layoutEngine->DockPanel(
                session.sourcePanel, 
                validation.targetPanel, 
                validation.position
            );
            
            if (success) {
                // Animate layout to new configuration
                m_layoutEngine->AnimateLayout();
            }
        }
        
        // Reset drag state
        m_dragState = DragState::None;
        m_draggedPanel = nullptr;
        m_hoveredPanel = nullptr;
    });
    
    m_dragController->SetDragCancelCallback([this](const DragSession& session) {
        wxUnusedVar(session);
        // Hide visual feedback
        m_ghostWindow->HideGhost();
        HideDockGuides();
        HidePreviewRect();
        
        // Reset drag state
        m_dragState = DragState::None;
        m_draggedPanel = nullptr;
        m_hoveredPanel = nullptr;
    });
}

void ModernDockManager::AddPanel(wxWindow* content, const wxString& title, DockArea area)
{
    if (!content) return;
    
    // Create new modern dock panel
    auto* panel = new ModernDockPanel(this, this, title);
    
    // Set the dock area BEFORE adding content
    panel->SetDockArea(area);
    
    panel->AddContent(content, title);
    
    // Add to panels collection
    m_panels[area].push_back(panel);
    
    // Add to layout engine
    m_layoutEngine->AddPanel(panel, area);
    
    // Update layout
    m_layoutEngine->UpdateLayout();
}

void ModernDockManager::RemovePanel(ModernDockPanel* panel)
{
    if (!panel) return;
    
    // Find and remove from panels collection
    for (auto& [area, panels] : m_panels) {
        auto it = std::find(panels.begin(), panels.end(), panel);
        if (it != panels.end()) {
            panels.erase(it);
            break;
        }
    }
    
    // Remove from layout engine
    m_layoutEngine->RemovePanel(panel);
    
    // Update layout
    m_layoutEngine->UpdateLayout();
    
    // Destroy the panel
    panel->Destroy();
}

ModernDockPanel* ModernDockManager::FindPanel(const wxString& title) const
{
    for (const auto& [area, panels] : m_panels) {
        for (auto* panel : panels) {
            if (panel->GetTitle() == title) {
                return panel;
            }
        }
    }
    return nullptr;
}

void ModernDockManager::StartDrag(ModernDockPanel* panel, const wxPoint& startPos)
{
    if (!panel || m_dragState != DragState::None) return;
    
    // Delegate to drag controller
    int selectedIndex = panel->GetSelectedIndex();
    if (selectedIndex >= 0) {
        m_dragController->StartDrag(panel, selectedIndex, startPos);
    }
}

void ModernDockManager::UpdateDrag(const wxPoint& currentPos)
{
    if (m_dragState == DragState::None) return;
    
    m_dragController->UpdateDrag(currentPos);
}

void ModernDockManager::CompleteDrag(const wxPoint& endPos)
{
    wxUnusedVar(endPos);
    if (m_dragState == DragState::None) return;
    
    m_dragController->CompleteDrag();
}

void ModernDockManager::CancelDrag()
{
    if (m_dragState == DragState::None) return;
    
    m_dragController->CancelDrag();
}

ModernDockPanel* ModernDockManager::HitTest(const wxPoint& screenPos) const
{
    // Test all visible panels using screen coordinates to avoid layout triggers
    for (const auto& [area, panels] : m_panels) {
        for (auto* panel : panels) {
            if (panel->IsShown()) {
                // Use screen rect to avoid triggering layout recalculation
                wxRect screenRect;
                wxPoint panelScreenPos = panel->GetScreenPosition();
                wxSize panelSize = panel->GetSize();
                screenRect = wxRect(panelScreenPos, panelSize);
                
                if (screenRect.Contains(screenPos)) {
                    return panel;
                }
            }
        }
    }
    
    return nullptr;
}

DockPosition ModernDockManager::GetDockPosition(ModernDockPanel* target, const wxPoint& screenPos) const
{
    if (!target) return DockPosition::None;
    
    // Calculate dock position based on mouse position relative to target panel
    wxRect targetRect;
    wxPoint targetScreenPos = target->GetScreenPosition();
    wxSize targetSize = target->GetSize();
    targetRect = wxRect(targetScreenPos, targetSize);
    
    if (!targetRect.Contains(screenPos)) {
        return DockPosition::None;
    }
    
    // Calculate relative position within the target panel
    wxPoint relativePos = screenPos - targetScreenPos;
    int x = relativePos.x;
    int y = relativePos.y;
    int w = targetSize.x;
    int h = targetSize.y;
    
    // Define dock zones (30% margin on each edge, center is 40%)
    const double margin = 0.3;
    int leftBound = static_cast<int>(w * margin);
    int rightBound = static_cast<int>(w * (1.0 - margin));
    int topBound = static_cast<int>(h * margin);
    int bottomBound = static_cast<int>(h * (1.0 - margin));
    
    // Determine dock position based on mouse location
    if (x < leftBound) {
        return DockPosition::Left;
    } else if (x > rightBound) {
        return DockPosition::Right;
    } else if (y < topBound) {
        return DockPosition::Top;
    } else if (y > bottomBound) {
        return DockPosition::Bottom;
    } else {
        return DockPosition::Center;
    }
}

void ModernDockManager::ShowDockGuides(ModernDockPanel* target)
{
    if (!target || !m_dockGuides) return;
    
    m_dockGuides->ShowGuides(target, wxGetMousePosition());
}

void ModernDockManager::HideDockGuides()
{
    if (m_dockGuides) {
        m_dockGuides->HideGuides();
    }
}

void ModernDockManager::ShowPreviewRect(const wxRect& rect, DockPosition position)
{
    wxRect clientRect = wxRect(ScreenToClient(rect.GetTopLeft()), rect.GetSize());
    
    if (m_previewRect != clientRect || m_previewPosition != position) {
        m_targetPreviewRect = clientRect;
        m_previewPosition = position;
        
        if (!m_previewVisible) {
            m_previewRect = clientRect;
            m_previewVisible = true;
            m_animationProgress = 1.0;
        } else {
            // Start animation to new position
            AnimatePreviewRect();
        }
        
        Refresh();
    }
}

void ModernDockManager::HidePreviewRect()
{
    if (m_previewVisible) {
        m_previewVisible = false;
        m_animationTimer.Stop();
        Refresh();
    }
}

void ModernDockManager::AnimatePreviewRect()
{
    if (m_animationTimer.IsRunning()) {
        m_animationTimer.Stop();
    }
    
    m_animationProgress = 0.0;
    m_animationTimer.Start(1000 / 60); // 60 FPS
}

void ModernDockManager::OnDPIChanged(double newScale)
{
    m_dpiScale = newScale;
    
    // Update all components for new DPI
    if (m_dockGuides) {
        // DockGuides will handle DPI internally
    }
    
    if (m_ghostWindow) {
        // GhostWindow will handle DPI internally
    }
    
    // Update layout engine
    if (m_layoutEngine) {
        m_layoutEngine->RecalculateLayout();
    }
    
    // Refresh display
    Refresh();
}

void ModernDockManager::OnPaint(wxPaintEvent& event)
{
    wxAutoBufferedPaintDC dc(this);
    
    // Create graphics context for smooth rendering
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (!gc) {
        event.Skip();
        return;
    }
    
    // Clear background
    gc->SetBrush(wxBrush(GetBackgroundColour()));
    gc->DrawRectangle(0, 0, GetSize().x, GetSize().y);
    
    // Render preview overlay if visible
    if (m_previewVisible) {
        RenderPreviewOverlay(gc);
    }
    
    delete gc;
}

void ModernDockManager::RenderPreviewOverlay(wxGraphicsContext* gc)
{
    if (!gc || !m_previewVisible) return;
    
    // Calculate current preview rect based on animation progress
    wxRect currentRect = m_previewRect;
    if (m_animationProgress < 1.0) {
        // Interpolate between current and target rect
        double t = m_animationProgress;
        currentRect.x = m_previewRect.x + (m_targetPreviewRect.x - m_previewRect.x) * t;
        currentRect.y = m_previewRect.y + (m_targetPreviewRect.y - m_previewRect.y) * t;
        currentRect.width = m_previewRect.width + (m_targetPreviewRect.width - m_previewRect.width) * t;
        currentRect.height = m_previewRect.height + (m_targetPreviewRect.height - m_previewRect.height) * t;
    } else {
        currentRect = m_targetPreviewRect;
    }
    
    // Set preview colors with transparency
    wxColour fillColor(30, 144, 255, static_cast<unsigned char>(PREVIEW_ALPHA * 255));
    wxColour borderColor(30, 144, 255, 200);
    
    // Draw preview rectangle
    gc->SetBrush(wxBrush(fillColor));
    gc->SetPen(wxPen(borderColor, static_cast<double>(2 * m_dpiScale)));
    gc->DrawRectangle(currentRect.x, currentRect.y, currentRect.width, currentRect.height);
    
    // Draw position indicator based on dock position
    DrawPositionIndicator(gc, currentRect, m_previewPosition);
}

void ModernDockManager::DrawPositionIndicator(wxGraphicsContext* gc, const wxRect& rect, DockPosition position)
{
    if (!gc) return;
    
    wxColour indicatorColor(255, 255, 255, 180);
    gc->SetBrush(wxBrush(indicatorColor));
    gc->SetPen(wxPen(indicatorColor, 1));
    
    // Draw directional arrow or indicator based on position
    wxPoint center = rect.GetTopLeft() + wxPoint(rect.width / 2, rect.height / 2);
    int arrowSize = static_cast<int>(16 * m_dpiScale);
    
    switch (position) {
        case DockPosition::Left:
            DrawArrow(gc, center, arrowSize, 180); // Left arrow
            break;
        case DockPosition::Right:
            DrawArrow(gc, center, arrowSize, 0);   // Right arrow
            break;
        case DockPosition::Top:
            DrawArrow(gc, center, arrowSize, 270); // Up arrow
            break;
        case DockPosition::Bottom:
            DrawArrow(gc, center, arrowSize, 90);  // Down arrow
            break;
        case DockPosition::Center:
            DrawTabIcon(gc, center, arrowSize);    // Tab icon
            break;
        default:
            break;
    }
}

void ModernDockManager::DrawArrow(wxGraphicsContext* gc, const wxPoint& center, int size, double angle)
{
    if (!gc) return;
    
    // Create arrow path
    wxGraphicsPath path = gc->CreatePath();
    
    // Arrow points (pointing right by default)
    wxPoint2DDouble points[] = {
        wxPoint2DDouble(-size/2, -size/4),
        wxPoint2DDouble(size/2, 0),
        wxPoint2DDouble(-size/2, size/4)
    };
    
    // Apply rotation
    double rad = wxDegToRad(angle);
    double cosA = cos(rad);
    double sinA = sin(rad);
    
    path.MoveToPoint(
        center.x + points[0].m_x * cosA - points[0].m_y * sinA,
        center.y + points[0].m_x * sinA + points[0].m_y * cosA
    );
    
    for (int i = 1; i < 3; ++i) {
        path.AddLineToPoint(
            center.x + points[i].m_x * cosA - points[i].m_y * sinA,
            center.y + points[i].m_x * sinA + points[i].m_y * cosA
        );
    }
    
    path.CloseSubpath();
    gc->FillPath(path);
}

void ModernDockManager::DrawTabIcon(wxGraphicsContext* gc, const wxPoint& center, int size)
{
    if (!gc) return;
    
    // Draw simple tab representation
    wxRect tabRect(center.x - size/2, center.y - size/4, size, size/2);
    gc->DrawRectangle(tabRect.x, tabRect.y, tabRect.width, tabRect.height);
    
    // Draw tab indicator
    wxRect indicatorRect(center.x - size/4, center.y - size/8, size/2, 2);
    gc->DrawRectangle(indicatorRect.x, indicatorRect.y, indicatorRect.width, indicatorRect.height);
}

void ModernDockManager::OnSize(wxSizeEvent& event)
{
    if (m_layoutEngine) {
        m_layoutEngine->UpdateLayout(GetClientRect());
    }
    
    event.Skip();
}

void ModernDockManager::OnTimer(wxTimerEvent& event)
{
    if (event.GetTimer().GetId() == m_animationTimer.GetId()) {
        // Update animation progress
        m_animationProgress += 1.0 / ANIMATION_STEPS;
        
        if (m_animationProgress >= 1.0) {
            m_animationProgress = 1.0;
            m_animationTimer.Stop();
            m_previewRect = m_targetPreviewRect;
        }
        
        Refresh();
    }
    
    // Update layout engine animations
    if (m_layoutEngine) {
        m_layoutEngine->UpdateTransitions();
    }
}

void ModernDockManager::OnMouseMove(wxMouseEvent& event)
{
    static bool inMouseMove = false;
    if (inMouseMove) {
        event.Skip();
        return;
    }
    
    inMouseMove = true;
    
    if (m_dragState != DragState::None) {
        UpdateDrag(ClientToScreen(event.GetPosition()));
    }
    
    inMouseMove = false;
    event.Skip();
}

void ModernDockManager::OnMouseLeave(wxMouseEvent& event)
{
    // Hide dock guides when mouse leaves manager area (but not during drag)
    if (m_dragState == DragState::None) {
        HideDockGuides();
    }
    // During drag, keep guides visible even when mouse leaves manager area
    
    event.Skip();
}

void ModernDockManager::SaveLayout()
{
    if (m_layoutEngine) {
        wxString layoutData = m_layoutEngine->SaveLayout();
        // Store layout data (implementation depends on requirements)
        // Could save to registry, config file, etc.
    }
}

void ModernDockManager::RestoreLayout()
{
    if (m_layoutEngine) {
        // Load layout data from storage
        wxString layoutData; // Load from storage
        if (!layoutData.IsEmpty()) {
            m_layoutEngine->LoadLayout(layoutData);
        }
    }
}

void ModernDockManager::OptimizeLayout()
{
    if (m_layoutEngine) {
        m_layoutEngine->OptimizeLayout();
    }
}

