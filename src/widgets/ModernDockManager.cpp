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
      m_dpiScale(1.0),
      m_currentStrategy(LayoutStrategy::IDE),
      m_autoSaveLayout(true),
      m_layoutUpdateInterval(100),
      m_layoutCachingEnabled(true),
      m_layoutUpdateMode(LayoutUpdateMode::Immediate)
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
    m_layoutEngine = std::make_unique<LayoutEngine>(this, this);
    
    // Initialize layout engine with this window as root
    m_layoutEngine->InitializeLayout(this);
    
    // Set initial docking restrictions: disable Center and Right areas
    SetAreaDockingEnabled(DockArea::Center, false);
    SetAreaDockingEnabled(DockArea::Right, false);
    
    // Ensure center indicator is always visible, even when center docking is disabled
    if (m_dockGuides) {
        m_dockGuides->SetCentralVisible(true);
    }
    
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
            // Use current area restrictions instead of hardcoded values
            if (m_dockGuides) {
                bool centerEnabled = IsAreaDockingEnabled(DockArea::Center);
                bool leftEnabled = IsAreaDockingEnabled(DockArea::Left);
                bool rightEnabled = IsAreaDockingEnabled(DockArea::Right);
                bool topEnabled = IsAreaDockingEnabled(DockArea::Top);
                bool bottomEnabled = IsAreaDockingEnabled(DockArea::Bottom);
                
                m_dockGuides->SetEnabledDirections(centerEnabled, leftEnabled, rightEnabled, topEnabled, bottomEnabled);
                // Center indicator should always be visible, even when center docking is disabled
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
                // Force layout update immediately after docking
                m_layoutEngine->UpdateLayout();
                
                // Refresh the display to show changes
                Refresh();
                
                // Then animate layout to new configuration
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
    
    // Check if we need to create a tabbed panel for Message/Performance area
    if (area == DockArea::Bottom && (title == "Message" || title == "Performance")) {
        // Look for existing bottom panel to add as tab
        ModernDockPanel* existingBottomPanel = nullptr;
        for (auto* panel : m_panels[area]) {
            if (panel->GetTitle() == "Output" || panel->GetTitle() == "Message Output" || 
                panel->GetContentCount() > 0) {
                existingBottomPanel = panel;
                break;
            }
        }
        
        if (existingBottomPanel) {
            // Add as tab to existing panel
            existingBottomPanel->AddContent(content, title);
            return;
        } else {
            // Create new tabbed panel for Message/Performance
            auto* panel = new ModernDockPanel(this, this, "Output");
            panel->SetDockArea(area);
            
            // Apply docking restrictions based on area
            if (area == DockArea::Center || area == DockArea::Right) {
                panel->SetDockingEnabled(false);
                panel->SetSystemButtonsVisible(false);
            }
            
            panel->AddContent(content, title);
            
            // Add to panels collection
            m_panels[area].push_back(panel);
            
            // Add to layout engine
            m_layoutEngine->AddPanel(panel, area);
            
            // Update layout
            m_layoutEngine->UpdateLayout();
            return;
        }
    }
    
    // Create new modern dock panel for other cases
    auto* panel = new ModernDockPanel(this, this, title);
    
    // Set the dock area BEFORE adding content
    panel->SetDockArea(area);
    
    // Apply docking restrictions based on area
    if (area == DockArea::Center || area == DockArea::Right) {
        panel->SetDockingEnabled(false);
        panel->SetSystemButtonsVisible(false);
    }
    
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

wxWindow* ModernDockManager::HitTest(const wxPoint& screenPos) const
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
                    return panel->GetContent();
                }
            }
        }
    }
    
    return nullptr;
}

DockPosition ModernDockManager::GetDockPosition(wxWindow* target, const wxPoint& screenPos) const
{
    if (!target) return DockPosition::None;
    
    // First check if mouse is over a dock guide - this takes priority
    if (m_dockGuides && m_dockGuides->IsVisible()) {
        DockPosition guidePosition = m_dockGuides->GetActivePosition();
        if (guidePosition != DockPosition::None) {
            return guidePosition;
        }
    }
    
    // If not over a guide, calculate dock position based on mouse position relative to target panel
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

void ModernDockManager::ShowPreviewRect(const wxRect& rect, DockPosition position)
{
    wxRect clientRect = wxRect(wxPanel::ScreenToClient(rect.GetTopLeft()), rect.GetSize());
    
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
        m_layoutEngine->UpdateLayout(wxPanel::GetClientRect());
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
        UpdateDrag(wxPanel::ClientToScreen(event.GetPosition()));
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



void ModernDockManager::OptimizeLayout()
{
    if (m_layoutEngine) {
        m_layoutEngine->OptimizeLayout();
    }
}

// Layout strategy management
void ModernDockManager::SetLayoutStrategy(LayoutStrategy strategy)
{
    // Implementation depends on requirements
    // For now, just store the strategy
    m_currentStrategy = strategy;
}

LayoutStrategy ModernDockManager::GetLayoutStrategy() const
{
    return m_currentStrategy;
}

void ModernDockManager::SetLayoutConstraints(const LayoutConstraints& constraints)
{
    wxUnusedVar(constraints);
    if (m_layoutEngine) {
        // Pass constraints to layout engine
        // Implementation depends on layout engine interface
    }
}

LayoutConstraints ModernDockManager::GetLayoutConstraints() const
{
    // Return current constraints
    // Implementation depends on requirements
    return LayoutConstraints();
}

// Configuration
void ModernDockManager::SetAutoSaveLayout(bool autoSave)
{
    m_autoSaveLayout = autoSave;
}

bool ModernDockManager::GetAutoSaveLayout() const
{
    return m_autoSaveLayout;
}

void ModernDockManager::SetLayoutUpdateInterval(int milliseconds)
{
    m_layoutUpdateInterval = milliseconds;
}

int ModernDockManager::GetLayoutUpdateInterval() const
{
    return m_layoutUpdateInterval;
}

void ModernDockManager::EnableLayoutCaching(bool enabled)
{
    m_layoutCachingEnabled = enabled;
}

void ModernDockManager::SetLayoutUpdateMode(LayoutUpdateMode mode)
{
    m_layoutUpdateMode = mode;
}

// IDockManager interface implementation
void ModernDockManager::AddPanel(wxWindow* content, const wxString& title, UnifiedDockArea area)
{
    // Basic implementation - delegate to existing AddPanel method
    DockArea dockArea = DockArea::Center; // Default
    switch (area) {
        case UnifiedDockArea::Left: dockArea = DockArea::Left; break;
        case UnifiedDockArea::Right: dockArea = DockArea::Right; break;
        case UnifiedDockArea::Top: dockArea = DockArea::Top; break;
        case UnifiedDockArea::Bottom: dockArea = DockArea::Bottom; break;
        case UnifiedDockArea::Center: dockArea = DockArea::Center; break;
        case UnifiedDockArea::Tab: dockArea = DockArea::Center; break;
    }
    
    // Create panel and add to appropriate area
    auto panel = std::make_unique<ModernDockPanel>(this, content, title);
    m_panels[dockArea].push_back(panel.get());
    panel.release(); // Transfer ownership to m_panels
}

void ModernDockManager::RemovePanel(wxWindow* content)
{
    // Find and remove panel by content
    for (auto& [area, panels] : m_panels) {
        auto it = std::find_if(panels.begin(), panels.end(),
            [content](ModernDockPanel* panel) {
                return panel->GetContent() == content;
            });
        if (it != panels.end()) {
            panels.erase(it);
            break;
        }
    }
}

void ModernDockManager::ShowPanel(wxWindow* content)
{
    // Find and show panel
    for (auto& [area, panels] : m_panels) {
        for (auto* panel : panels) {
            if (panel->GetContent() == content) {
                panel->Show();
                break;
            }
        }
    }
}

void ModernDockManager::HidePanel(wxWindow* content)
{
    // Find and hide panel
    for (auto& [area, panels] : m_panels) {
        for (auto* panel : panels) {
            if (panel->GetContent() == content) {
                panel->Hide();
                break;
            }
        }
    }
}

bool ModernDockManager::HasPanel(wxWindow* content) const
{
    for (const auto& [area, panels] : m_panels) {
        for (auto* panel : panels) {
            if (panel->GetContent() == content) {
                return true;
            }
        }
    }
    return false;
}



// Layout persistence
void ModernDockManager::ResetToDefaultLayout()
{
    // Reset to default layout
    if (m_layoutEngine) {
        m_layoutEngine->UpdateLayout();
    }
}

bool ModernDockManager::LoadLayoutFromFile(const wxString& filename)
{
    wxUnusedVar(filename);
    // Load layout from file
    return true; // Placeholder
}

bool ModernDockManager::SaveLayoutToFile(const wxString& filename)
{
    wxUnusedVar(filename);
    // Save layout to file
    return true; // Placeholder
}

void ModernDockManager::SaveLayout()
{
    // Save current layout
    if (m_layoutEngine) {
        // Serialize layout data
        // This is a placeholder implementation
    }
}

void ModernDockManager::RestoreLayout()
{
    // Restore saved layout
    if (m_layoutEngine) {
        // Deserialize layout data
        // This is a placeholder implementation
    }
}

// Panel positioning and docking
void ModernDockManager::DockPanel(wxWindow* panel, wxWindow* target, DockPosition position)
{
    wxUnusedVar(panel);
    wxUnusedVar(target);
    wxUnusedVar(position);
    // Dock panel to target at position
    // Implementation depends on requirements
}

void ModernDockManager::UndockPanel(wxWindow* panel)
{
    wxUnusedVar(panel);
    // Undock panel
    // Implementation depends on requirements
}

void ModernDockManager::FloatPanel(wxWindow* panel)
{
    wxUnusedVar(panel);
    // Make panel floating
    // Implementation depends on requirements
}

void ModernDockManager::TabifyPanel(wxWindow* panel, wxWindow* target)
{
    wxUnusedVar(panel);
    wxUnusedVar(target);
    // Add panel as tab to target
    // Implementation depends on requirements
}

// Layout information
wxRect ModernDockManager::GetPanelRect(wxWindow* panel) const
{
    // Get panel rectangle
    for (const auto& [area, panels] : m_panels) {
        for (auto* dockPanel : panels) {
            if (dockPanel->GetContent() == panel) {
                return dockPanel->GetRect();
            }
        }
    }
    return wxRect();
}

UnifiedDockArea ModernDockManager::GetPanelArea(wxWindow* panel) const
{
    // Get panel area
    for (const auto& [area, panels] : m_panels) {
        for (auto* dockPanel : panels) {
            if (dockPanel->GetContent() == panel) {
                switch (area) {
                    case DockArea::Left: return UnifiedDockArea::Left;
                    case DockArea::Right: return UnifiedDockArea::Right;
                    case DockArea::Top: return UnifiedDockArea::Top;
                    case DockArea::Bottom: return UnifiedDockArea::Bottom;
                    case DockArea::Center: return UnifiedDockArea::Center;
                    default: return UnifiedDockArea::Center;
                }
            }
        }
    }
    return UnifiedDockArea::Center;
}

bool ModernDockManager::IsPanelFloating(wxWindow* panel) const
{
    wxUnusedVar(panel);
    // Check if panel is floating
    return false; // Placeholder
}

bool ModernDockManager::IsPanelDocked(wxWindow* panel) const
{
    // Check if panel is docked
    return HasPanel(panel);
}

// Visual feedback control
void ModernDockManager::ShowDockGuides()
{
    if (m_dockGuides) {
        m_dockGuides->ShowGuides(nullptr, wxGetMousePosition());
    }
}

void ModernDockManager::HideDockGuides()
{
    if (m_dockGuides) {
        m_dockGuides->HideGuides();
    }
}

void ModernDockManager::SetDockGuideConfig(const DockGuideConfig& config)
{
    wxUnusedVar(config);
    // Set dock guide configuration
    // Implementation depends on requirements
}

DockGuideConfig ModernDockManager::GetDockGuideConfig() const
{
    // Get dock guide configuration
    return DockGuideConfig(); // Placeholder
}

ModernDockPanel* ModernDockManager::GetDockGuideTarget() const
{
    if (m_dockGuides) {
        return m_dockGuides->GetCurrentTarget();
    }
    return nullptr;
}

// Event handling
void ModernDockManager::BindDockEvent(wxEventType eventType, 
                                      std::function<void(const DockEventData&)> handler)
{
    wxUnusedVar(eventType);
    wxUnusedVar(handler);
    // Bind dock event
    // Implementation depends on requirements
}

void ModernDockManager::UnbindDockEvent(wxEventType eventType)
{
    wxUnusedVar(eventType);
    // Unbind dock event
    // Implementation depends on requirements
}

// Drag and drop
void ModernDockManager::StartDrag(wxWindow* panel, const wxPoint& startPos)
{
    wxUnusedVar(panel);
    wxUnusedVar(startPos);
    // Start drag operation
    // Implementation depends on requirements
}



void ModernDockManager::EndDrag(const wxPoint& endPos)
{
    wxUnusedVar(endPos);
    // End drag operation
    // Implementation depends on requirements
}

bool ModernDockManager::IsDragging() const
{
    return m_dragState != DragState::None;
}

// Layout tree access
LayoutNode* ModernDockManager::GetRootNode() const
{
    if (m_layoutEngine) {
        return m_layoutEngine->GetRootNode();
    }
    return nullptr;
}

LayoutNode* ModernDockManager::FindNode(wxWindow* panel) const
{
    if (m_layoutEngine) {
        return m_layoutEngine->FindPanelNode(reinterpret_cast<ModernDockPanel*>(panel));
    }
    return nullptr;
}

void ModernDockManager::TraverseNodes(std::function<void(LayoutNode*)> visitor) const
{
    if (m_layoutEngine) {
        auto* root = m_layoutEngine->GetRootNode();
        if (root) {
            visitor(root);
        }
    }
}

// Layout update methods
void ModernDockManager::UpdateLayout()
{
    if (m_layoutEngine) {
        m_layoutEngine->UpdateLayout(wxPanel::GetClientRect());
    }
}

// Rectangle and coordinate conversion methods
wxRect ModernDockManager::GetScreenRect() const
{
    return wxPanel::GetScreenRect();
}

wxRect ModernDockManager::GetClientRect() const
{
    return wxPanel::GetClientRect();
}

wxPoint ModernDockManager::ClientToScreen(const wxPoint& point) const
{
    return wxPanel::ClientToScreen(point);
}

wxPoint ModernDockManager::ScreenToClient(const wxPoint& point) const
{
    return wxPanel::ScreenToClient(point);
}

// Utility functions
void ModernDockManager::RefreshLayout()
{
    if (m_layoutEngine) {
        m_layoutEngine->UpdateLayout();
    }
}



void ModernDockManager::FitLayout()
{
    if (m_layoutEngine) {
        m_layoutEngine->UpdateLayout();
    }
}

wxSize ModernDockManager::GetMinimumSize() const
{
    return wxSize(400, 300);
}

wxSize ModernDockManager::GetBestSize() const
{
    return GetSize();
}

// Statistics and debugging
int ModernDockManager::GetPanelCount() const
{
    int count = 0;
    for (const auto& [area, panels] : m_panels) {
        count += panels.size();
    }
    return count;
}

std::vector<ModernDockPanel*> ModernDockManager::GetAllPanels() const
{
    std::vector<ModernDockPanel*> allPanels;
    for (const auto& [area, panels] : m_panels) {
        allPanels.insert(allPanels.end(), panels.begin(), panels.end());
    }
    return allPanels;
}

int ModernDockManager::GetContainerCount() const
{
    return 0; // Placeholder
}

int ModernDockManager::GetSplitterCount() const
{
    return 0; // Placeholder
}

wxString ModernDockManager::GetLayoutStatistics() const
{
    return wxString::Format("Panels: %d, Containers: %d, Splitters: %d",
                           GetPanelCount(), GetContainerCount(), GetSplitterCount());
}

void ModernDockManager::DumpLayoutTree() const
{
    // Dump layout tree for debugging
    // Implementation depends on requirements
}

// Additional IDockManager interface methods
void ModernDockManager::ShowDockGuides(wxWindow* target)
{
    if (m_dockGuides) {
        m_dockGuides->ShowGuides(reinterpret_cast<ModernDockPanel*>(target), wxGetMousePosition());
    }
}

// Docking control methods
void ModernDockManager::SetPanelDockingEnabled(ModernDockPanel* panel, bool enabled)
{
    if (panel) {
        panel->SetDockingEnabled(enabled);
    }
}

void ModernDockManager::SetPanelSystemButtonsVisible(ModernDockPanel* panel, bool visible)
{
    if (panel) {
        panel->SetSystemButtonsVisible(visible);
    }
}

void ModernDockManager::SetAreaDockingEnabled(DockArea area, bool enabled)
{
    // Apply docking control to all panels in the specified area
    auto it = m_panels.find(area);
    if (it != m_panels.end()) {
        for (auto* panel : it->second) {
            if (panel) {
                panel->SetDockingEnabled(enabled);
                // Also control system buttons visibility based on docking state
                panel->SetSystemButtonsVisible(enabled);
            }
        }
    }
    
    // Also update dock guides to reflect the area restrictions
    if (m_dockGuides) {
        bool centerEnabled = true, leftEnabled = true, rightEnabled = true, topEnabled = true, bottomEnabled = true;
        
        // Disable specific areas based on current restrictions
        if (!enabled) {
            switch (area) {
                case DockArea::Center:
                    centerEnabled = false;
                    break;
                case DockArea::Left:
                    leftEnabled = false;
                    break;
                case DockArea::Right:
                    rightEnabled = false;
                    break;
                case DockArea::Top:
                    topEnabled = false;
                    break;
                case DockArea::Bottom:
                    bottomEnabled = false;
                    break;
                default:
                    break;
            }
        }
        
        // Apply the restrictions to dock guides
        m_dockGuides->SetEnabledDirections(centerEnabled, leftEnabled, rightEnabled, topEnabled, bottomEnabled);
    }
}

bool ModernDockManager::IsAreaDockingEnabled(DockArea area) const
{
    // Check if the area has any panels and if they are enabled
    auto it = m_panels.find(area);
    if (it != m_panels.end() && !it->second.empty()) {
        // Check if the first panel in the area is enabled
        return it->second.front()->IsDockingEnabled();
    }
    
    // Default to enabled if no panels in the area
    return true;
}









