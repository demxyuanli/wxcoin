#include "widgets/LayoutEngine.h"
#include "widgets/ModernDockManager.h"
#include "widgets/ModernDockPanel.h"
#include "DPIManager.h"
#include <wx/splitter.h>
#include <wx/sizer.h>
#include <wx/log.h>
#include <algorithm>

// LayoutNode implementation
LayoutNode::LayoutNode(LayoutNodeType type, LayoutNode* parent)
    : m_type(type),
      m_parent(parent),
      m_panel(nullptr),
      m_splitter(nullptr),
      m_splitterRatio(0.5),
      m_dockArea(DockArea::Center)
{
}

LayoutNode::~LayoutNode()
{
    // Children are automatically cleaned up by unique_ptr
}

void LayoutNode::AddChild(std::unique_ptr<LayoutNode> child)
{
    if (child) {
        child->m_parent = this;
        m_children.push_back(std::move(child));
    }
}

void LayoutNode::RemoveChild(LayoutNode* child)
{
    if (!child) return;
    
    auto it = std::find_if(m_children.begin(), m_children.end(),
        [child](const std::unique_ptr<LayoutNode>& node) {
            return node.get() == child;
        });
    
    if (it != m_children.end()) {
        m_children.erase(it);
    }
}

void LayoutNode::SetSashPosition(int position)
{
    if (m_splitter && m_splitter->IsSplit()) {
        m_splitter->SetSashPosition(position);
    }
}

int LayoutNode::GetSashPosition() const
{
    if (m_splitter && m_splitter->IsSplit()) {
        return m_splitter->GetSashPosition();
    }
    return -1;
}

LayoutNode* LayoutNode::FindPanel(ModernDockPanel* panel)
{
    if (m_type == LayoutNodeType::Panel && m_panel == panel) {
        return this;
    }
    
    for (auto& child : m_children) {
        LayoutNode* result = child->FindPanel(panel);
        if (result) return result;
    }
    
    return nullptr;
}

LayoutNode* LayoutNode::FindNodeAt(const wxPoint& pos)
{
    if (m_rect.Contains(pos)) {
        // Check children first (more specific)
        for (auto& child : m_children) {
            LayoutNode* result = child->FindNodeAt(pos);
            if (result) return result;
        }
        
        // Return this node if no child contains the point
        return this;
    }
    
    return nullptr;
}

void LayoutNode::GetAllPanels(std::vector<ModernDockPanel*>& panels)
{
    if (m_type == LayoutNodeType::Panel && m_panel) {
        panels.push_back(m_panel);
    }
    
    for (auto& child : m_children) {
        child->GetAllPanels(panels);
    }
}

// LayoutEngine implementation
wxBEGIN_EVENT_TABLE(LayoutEngine, wxEvtHandler)
    EVT_TIMER(wxID_ANY, LayoutEngine::OnAnimationTimer)
wxEND_EVENT_TABLE()

LayoutEngine::LayoutEngine(wxWindow* parent, IDockManager* manager)
    : wxEvtHandler(),
      m_parent(parent),
      m_manager(manager),
      m_animationTimer(this, wxID_ANY),
      m_animationEnabled(true),
      m_layoutDirty(false),
      m_delayedCleanupPending(false),
      m_isUpdatingLayout(false),
      m_debounceTimer(this, wxID_ANY + 1),
      m_pendingLayoutUpdate(false)
{
    // Initialize DPI-aware sizes
    double dpiScale = DPIManager::getInstance().getDPIScale();
    m_minPanelSize = wxSize(static_cast<int>(DEFAULT_MIN_PANEL_WIDTH * dpiScale),
                           static_cast<int>(DEFAULT_MIN_PANEL_HEIGHT * dpiScale));
    m_splitterSashSize = static_cast<int>(DEFAULT_SPLITTER_SASH_SIZE * dpiScale);
    m_defaultAnimationDuration = DEFAULT_ANIMATION_DURATION;
    
    // Bind debounce timer
    Bind(wxEVT_TIMER, &LayoutEngine::OnDebounceTimer, this, m_debounceTimer.GetId());
}

LayoutEngine::~LayoutEngine()
{
    if (m_animationTimer.IsRunning()) {
        m_animationTimer.Stop();
    }
    if (m_debounceTimer.IsRunning()) {
        m_debounceTimer.Stop();
    }
}

void LayoutEngine::InitializeLayout(wxWindow* rootWindow)
{
    if (!rootWindow) return;
    
    // Create root node
    m_rootNode = std::make_unique<LayoutNode>(LayoutNodeType::Root);
    m_rootNode->SetRect(rootWindow->GetClientRect());
    
    m_lastClientRect = rootWindow->GetClientRect();
}

void LayoutEngine::AddPanel(ModernDockPanel* panel, DockArea area, ModernDockPanel* relativeTo)
{
    wxUnusedVar(relativeTo);
    if (!panel || !m_rootNode) return;
    
    // Find insertion point
    LayoutNode* insertionPoint = FindBestInsertionPoint(area);
    if (!insertionPoint) {
        insertionPoint = m_rootNode.get();
    }
    
    // Create panel node
    auto panelNode = CreatePanelNode(panel);
    
    // Determine dock position based on area
    DockPosition position = DockPosition::Center;
    switch (area) {
        case DockArea::Left:   position = DockPosition::Left; break;
        case DockArea::Right:  position = DockPosition::Right; break;
        case DockArea::Top:    position = DockPosition::Top; break;
        case DockArea::Bottom: position = DockPosition::Bottom; break;
        case DockArea::Center: position = DockPosition::Center; break;
        default: break;
    }
    
    // Insert panel into tree
    InsertPanelIntoTree(panel, insertionPoint, position);
    
    m_layoutDirty = true;
}

void LayoutEngine::RemovePanel(ModernDockPanel* panel)
{
    if (!panel || !m_rootNode) return;
    
    // Find panel node
    LayoutNode* panelNode = m_rootNode->FindPanel(panel);
    if (!panelNode) return;
    
    // Remove from tree
    RemovePanelFromTree(panelNode);
    
    // Clean up empty nodes - but be careful not to remove nodes we might need
    CleanupEmptyNodes();
    
    m_layoutDirty = true;
}

void LayoutEngine::MovePanel(ModernDockPanel* panel, DockArea newArea, ModernDockPanel* relativeTo)
{
    if (!panel) return;
    
    // Remove panel from current location
    RemovePanel(panel);
    
    // Add to new location
    AddPanel(panel, newArea, relativeTo);
}

bool LayoutEngine::DockPanel(ModernDockPanel* panel, ModernDockPanel* target, DockPosition position)
{
    if (!panel || !target || !m_rootNode) return false;
    
    wxLogDebug("DockPanel: panel=%s, target=%s, position=%d", 
               panel->GetTitle(), target->GetTitle(), (int)position);
    
    // Find target node and save parent information before removing anything
    LayoutNode* targetNode = m_rootNode->FindPanel(target);
    if (!targetNode) {
        wxLogDebug("DockPanel: Target node not found!");
        return false;
    }
    
    // Save target parent info before any modifications
    LayoutNode* targetParent = targetNode->GetParent();
    if (!targetParent) {
        targetParent = m_rootNode.get();
    }
    
    // If panel and target are the same, do nothing
    if (panel == target) {
        wxLogDebug("DockPanel: Panel and target are the same!");
        return false;
    }
    
    // Find panel node if it's already in the tree
    LayoutNode* panelNode = m_rootNode->FindPanel(panel);
    
    // Remove panel from current location if it's already in the tree
    // Always use RemovePanelFromTree to avoid triggering aggressive cleanup
    wxLogDebug("DockPanel: Removing panel from current location");
    if (panelNode) {
        RemovePanelFromTree(panelNode);
        wxLogDebug("DockPanel: Panel removed from tree");
    }
    
    // Re-find target node in case the tree structure changed
    targetNode = m_rootNode->FindPanel(target);
    if (!targetNode) {
        // Target was removed during cleanup, use saved parent
        wxLogDebug("DockPanel: Target was removed during cleanup, using saved parent");
        InsertPanelIntoTree(panel, targetParent, position);
    } else {
        // Target still exists, use its current parent
        LayoutNode* currentParent = targetNode->GetParent();
        if (!currentParent) {
            currentParent = m_rootNode.get();
        }
        
        wxLogDebug("DockPanel: Inserting panel relative to target");
        // For center docking, use the target node itself
        if (position == DockPosition::Center) {
            InsertPanelIntoTree(panel, targetNode, position);
        } else {
            // For other positions, insert relative to target node
            InsertPanelIntoTree(panel, targetNode, position);
        }
    }
    
    m_layoutDirty = true;
    
    // Schedule a delayed cleanup instead of immediate cleanup
    // This prevents aggressive cleanup from interfering with the docking operation
    m_delayedCleanupPending = true;
    if (m_animationTimer.IsRunning()) {
        m_animationTimer.Stop();
    }
    m_animationTimer.StartOnce(100); // Cleanup after 100ms
    
    wxLogDebug("DockPanel: Complete");
    return true;
}

void LayoutEngine::FloatPanel(ModernDockPanel* panel)
{
    if (!panel) return;
    
    // Remove from layout tree
    RemovePanel(panel);
    
    // Create floating window
    wxFrame* floatFrame = new wxFrame(m_parent, wxID_ANY, panel->GetTitle(),
                                     wxDefaultPosition, wxSize(400, 300),
                                     wxDEFAULT_FRAME_STYLE);
    
    // Reparent panel to floating window
    panel->Reparent(floatFrame);
    panel->SetFloating(true);
    
    // Set up floating window layout
    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(panel, 1, wxEXPAND);
    floatFrame->SetSizer(sizer);
    
    floatFrame->Show();
}

void LayoutEngine::TabifyPanel(ModernDockPanel* panel, ModernDockPanel* target)
{
    if (!panel || !target) return;
    
    // Remove panel from current location if it's already in the tree
    RemovePanel(panel);
    
    // Find target node in the layout tree
    LayoutNode* targetNode = m_rootNode->FindPanel(target);
    if (!targetNode) return;
    
    // Create a new panel node for the panel to be tabified
    auto panelNode = CreatePanelNode(panel);
    if (!panelNode) return;
    
    // Set the dock area to match the target
    panelNode->SetDockArea(targetNode->GetDockArea());
    
    // Add the panel as a child of the target node
    // This will create a tabbed interface
    targetNode->AddChild(std::move(panelNode));
    
    // Mark layout as dirty for recalculation
    m_layoutDirty = true;
}

void LayoutEngine::RestorePanel(ModernDockPanel* panel, DockArea area)
{
    if (!panel) return;
    
    // If panel is floating, restore to docked state
    if (panel->IsFloating()) {
        panel->SetFloating(false);
        
        // Reparent back to parent
        panel->Reparent(m_parent);
        
        // Add to specified area
        AddPanel(panel, area);
    }
}

void LayoutEngine::UpdateLayout()
{
    if (!m_manager || !m_rootNode) return;
    
    // Use debounced update to prevent excessive layout calculations
    RequestLayoutUpdate();
}

void LayoutEngine::UpdateLayout(const wxRect& clientRect)
{
    if (!m_rootNode || m_isUpdatingLayout) return;
    
    // Prevent recursive layout updates
    m_isUpdatingLayout = true;
    
    m_lastClientRect = clientRect;
    m_rootNode->SetRect(clientRect);
    
    // Only recalculate if layout is dirty or size changed significantly
    if (m_layoutDirty || ShouldRecalculateLayout(clientRect)) {
        // Calculate layout recursively
        CalculateNodeLayout(m_rootNode.get(), clientRect);
        
        // Apply layout to widgets
        ApplyLayoutToWidgets(m_rootNode.get());
        
        // Force refresh of the parent window to ensure all changes are visible
        // Only refresh if we actually made changes
        if (m_parent) {
            m_parent->Refresh();
            // Don't call Update() here as it can cause excessive redraws
            // Let the system handle the refresh naturally
        }
    }
    
    m_layoutDirty = false;
    m_isUpdatingLayout = false;
}

void LayoutEngine::RecalculateLayout()
{
    if (!m_manager) return;
    
    UpdateLayout(m_manager->GetClientRect());
}

void LayoutEngine::RequestLayoutUpdate()
{
    if (m_isUpdatingLayout) return;
    
    m_pendingLayoutUpdate = true;
    
    // Start debounce timer (50ms delay)
    if (m_debounceTimer.IsRunning()) {
        m_debounceTimer.Stop();
    }
    m_debounceTimer.StartOnce(50);
}

bool LayoutEngine::ShouldRecalculateLayout(const wxRect& newRect) const
{
    if (!m_lastClientRect.IsEmpty()) {
        // Only recalculate if size changed significantly (more than 5 pixels)
        int widthDiff = abs(newRect.width - m_lastClientRect.width);
        int heightDiff = abs(newRect.height - m_lastClientRect.height);
        
        return (widthDiff > 5 || heightDiff > 5);
    }
    
    return true; // First time calculation
}

void LayoutEngine::OnDebounceTimer(wxTimerEvent& event)
{
    wxUnusedVar(event);
    
    if (m_pendingLayoutUpdate && m_manager && m_rootNode) {
        m_pendingLayoutUpdate = false;
        UpdateLayout(m_manager->GetClientRect());
    }
}

void LayoutEngine::OptimizeLayout()
{
    // Implementation for layout optimization
    // This could include things like:
    // - Removing unnecessary splitters
    // - Balancing splitter ratios
    // - Consolidating empty areas
    
    CleanupEmptyNodes();
    RecalculateLayout();
}

void LayoutEngine::CreateSplitter(LayoutNode* parent, bool horizontal)
{
    if (!parent || !m_manager) return;
    
    // Create splitter window
    wxSplitterWindow* splitter = CreateSplitterWindow(m_parent, horizontal);
    
    // Create splitter node
    auto splitterNode = CreateSplitterNode(horizontal);
    splitterNode->SetSplitter(splitter);
    
    // Add to parent
    parent->AddChild(std::move(splitterNode));
}

void LayoutEngine::RemoveSplitter(LayoutNode* splitterNode)
{
    if (!splitterNode || splitterNode->GetType() == LayoutNodeType::Panel) return;
    
    // Unsplit the splitter window
    if (splitterNode->GetSplitter()) {
        splitterNode->GetSplitter()->Unsplit();
    }
    
    // Remove from parent
    if (splitterNode->GetParent()) {
        splitterNode->GetParent()->RemoveChild(splitterNode);
    }
}

void LayoutEngine::UpdateSplitterConstraints()
{
    if (!m_rootNode) return;
    
    UpdateConstraintsRecursive(m_rootNode.get());
}

void LayoutEngine::AnimateLayout(int durationMs)
{
    if (!m_animationEnabled) {
        RecalculateLayout();
        return;
    }
    
    // Start layout animation
    LayoutTransition transition;
    transition.type = LayoutTransition::Resize;
    transition.node = m_rootNode.get();
    transition.duration = durationMs;
    transition.active = true;
    
    StartTransition(transition);
}

void LayoutEngine::StartTransition(const LayoutTransition& transition)
{
    // Add to active transitions
    m_activeTransitions.push_back(transition);
    
    // Start animation timer if not running
    if (!m_animationTimer.IsRunning()) {
        m_animationTimer.Start(ANIMATION_TIMER_INTERVAL);
    }
}

void LayoutEngine::UpdateTransitions()
{
    if (m_activeTransitions.empty()) return;
    
    // Update all active transitions
    for (auto& transition : m_activeTransitions) {
        if (transition.active) {
            transition.progress += static_cast<double>(ANIMATION_TIMER_INTERVAL) / transition.duration;
            
            if (transition.progress >= 1.0) {
                transition.progress = 1.0;
                CompleteTransition(transition);
            } else {
                ApplyTransition(transition);
            }
        }
    }
    
    // Remove completed transitions
    m_activeTransitions.erase(
        std::remove_if(m_activeTransitions.begin(), m_activeTransitions.end(),
            [](const LayoutTransition& t) { return !t.active; }),
        m_activeTransitions.end()
    );
    
    // Stop timer if no active transitions
    if (m_activeTransitions.empty()) {
        m_animationTimer.Stop();
    }
}

LayoutNode* LayoutEngine::FindPanelNode(ModernDockPanel* panel) const
{
    if (!panel || !m_rootNode) return nullptr;
    
    return m_rootNode->FindPanel(panel);
}

std::vector<ModernDockPanel*> LayoutEngine::GetAllPanels() const
{
    std::vector<ModernDockPanel*> panels;
    
    if (m_rootNode) {
        m_rootNode->GetAllPanels(panels);
    }
    
    return panels;
}

wxRect LayoutEngine::GetPanelRect(ModernDockPanel* panel) const
{
    LayoutNode* node = FindPanelNode(panel);
    return node ? node->GetRect() : wxRect();
}

wxString LayoutEngine::SaveLayout() const
{
    // Simple layout serialization
    // In a real implementation, this would serialize the entire tree structure
    wxString layout;
    
    // For now, just save basic information
    std::vector<ModernDockPanel*> panels = GetAllPanels();
    for (size_t i = 0; i < panels.size(); ++i) {
        if (i > 0) layout += ";";
        layout += panels[i]->GetTitle() + ":" + 
                 wxString::Format("%d", static_cast<int>(panels[i]->GetDockArea()));
    }
    
    return layout;
}

bool LayoutEngine::LoadLayout(const wxString& layoutData)
{
    if (layoutData.IsEmpty()) return false;
    
    // Simple layout deserialization
    // In a real implementation, this would rebuild the tree structure
    
    wxArrayString panels = wxSplit(layoutData, ';');
    for (const wxString& panelData : panels) {
        wxArrayString parts = wxSplit(panelData, ':');
        if (parts.size() >= 2) {
            wxString title = parts[0];
            long areaCode;
            if (parts[1].ToLong(&areaCode)) {
                DockArea area = static_cast<DockArea>(areaCode);
                
                // Find panel by title and move to area
                auto allPanels = GetAllPanels();
                for (ModernDockPanel* panel : allPanels) {
                    if (panel->GetTitle() == title) {
                        MovePanel(panel, area);
                        break;
                    }
                }
            }
        }
    }
    
    return true;
}

std::unique_ptr<LayoutNode> LayoutEngine::CreatePanelNode(ModernDockPanel* panel)
{
    if (!panel) return nullptr;
    
    auto node = std::make_unique<LayoutNode>(LayoutNodeType::Panel);
    node->SetPanel(panel);
    
    // Set panel constraints
    LayoutConstraints constraints;
    constraints.minWidth = m_minPanelSize.x;
    constraints.minHeight = m_minPanelSize.y;
    node->SetConstraints(constraints);
    
    return node;
}

std::unique_ptr<LayoutNode> LayoutEngine::CreateSplitterNode(bool horizontal)
{
    LayoutNodeType type = horizontal ? LayoutNodeType::HorizontalSplitter : LayoutNodeType::VerticalSplitter;
    auto node = std::make_unique<LayoutNode>(type);
    
    return node;
}

void LayoutEngine::InsertPanelIntoTree(ModernDockPanel* panel, LayoutNode* parent, DockPosition position)
{
    if (!panel || !parent) return;
    
    // Validate parent node is still valid
    if (parent != m_rootNode.get() && !IsNodeValid(parent)) {
        // Parent is invalid, use root node
        parent = m_rootNode.get();
    }
    
    auto panelNode = CreatePanelNode(panel);
    if (!panelNode) return;
    
    // Store dock area info in panel node for layout calculation
    DockArea area = panel->GetDockArea();
    panelNode->SetDockArea(area);
    
    // Handle different dock positions
    switch (position) {
        case DockPosition::Left:
        case DockPosition::Right:
        case DockPosition::Top:
        case DockPosition::Bottom:
            // Insert as sibling to target with splitter
            InsertPanelWithSplitter(std::move(panelNode), parent, position);
            break;
            
        case DockPosition::Center:
        default:
            // Use OrganizeByDockAreas for center docking or default behavior
    OrganizeByDockAreas(std::move(panelNode), parent);
            break;
    }
}

void LayoutEngine::OrganizeByDockAreas(std::unique_ptr<LayoutNode> panelNode, LayoutNode* parent)
{
    if (!panelNode || !parent) return;
    
    DockArea newArea = panelNode->GetDockArea();
    
    // If this is the first panel, create main layout structure
    if (parent->GetChildren().empty()) {
        CreateMainLayoutStructure(parent);
    }
    
    // Navigate to the correct container based on our new IDE layout structure
    // Structure: Root -> VerticalSplitter(TopWorkArea | BottomStatusBar)
    //           TopWorkArea -> HorizontalSplitter(LeftSidebar | CenterCanvas)
    //           BottomStatusBar -> (Message + Performance as tabs)
    
    wxLogDebug("OrganizeByDockAreas: Checking structure for area %d", (int)newArea);
    
    if (parent->GetChildren().empty()) {
        wxLogDebug("OrganizeByDockAreas: No children, creating main structure");
        CreateMainLayoutStructure(parent);
    }
    
    if (parent->GetChildren().empty() || 
        parent->GetChildren()[0]->GetType() != LayoutNodeType::VerticalSplitter) {
        wxLogDebug("OrganizeByDockAreas: Main structure not ready, adding to parent");
        parent->AddChild(std::move(panelNode));
        return;
    }
    
    auto& mainVSplitter = parent->GetChildren()[0];
    wxLogDebug("OrganizeByDockAreas: Main VSplitter has %d children", (int)mainVSplitter->GetChildren().size());
    
    if (mainVSplitter->GetChildren().size() < 2) {
        wxLogDebug("OrganizeByDockAreas: Main structure incomplete, adding to parent");
        parent->AddChild(std::move(panelNode));
        return;
    }
    
    auto topWorkArea = mainVSplitter->GetChildren()[0].get();
    auto bottomStatusBar = mainVSplitter->GetChildren()[1].get();
    
    wxLogDebug("OrganizeByDockAreas: TopWorkArea has %d children", (int)topWorkArea->GetChildren().size());
    
    if (topWorkArea->GetChildren().empty() || 
        topWorkArea->GetChildren()[0]->GetType() != LayoutNodeType::HorizontalSplitter) {
        wxLogDebug("OrganizeByDockAreas: Top work area structure not ready, adding to parent");
        parent->AddChild(std::move(panelNode));
        return;
    }
    
    auto topHSplitter = topWorkArea->GetChildren()[0].get();
    wxLogDebug("OrganizeByDockAreas: TopHSplitter has %d children", (int)topHSplitter->GetChildren().size());
    
    if (topHSplitter->GetChildren().size() < 2) {
        wxLogDebug("OrganizeByDockAreas: Top horizontal splitter not ready, adding to parent");
        parent->AddChild(std::move(panelNode));
        return;
    }
    
    auto leftSidebar = topHSplitter->GetChildren()[0].get();
    auto centerCanvas = topHSplitter->GetChildren()[1].get();
    
    wxLogDebug("OrganizeByDockAreas: Found containers - LeftSidebar: %d children, CenterCanvas: %d children", 
               (int)leftSidebar->GetChildren().size(), (int)centerCanvas->GetChildren().size());
    
    // Now add panel to appropriate container
    switch (newArea) {
        case DockArea::Left:
            // Left sidebar: should stack panels vertically (Object Tree above Properties)
            if (leftSidebar->GetChildren().empty()) {
                // First left panel
                leftSidebar->AddChild(std::move(panelNode));
            } else if (leftSidebar->GetChildren().size() == 1 && 
                       leftSidebar->GetChildren()[0]->GetType() == LayoutNodeType::Panel) {
                // Second left panel - create vertical splitter
                auto existingPanel = std::move(leftSidebar->GetChildren()[0]);
                leftSidebar->GetChildren().clear();
                
                auto vSplitter = CreateSplitterNode(false); // vertical splitter
                
                // Create actual wxSplitterWindow control
                wxSplitterWindow* splitterWidget = CreateSplitterWindow(m_parent, false);
                vSplitter->SetSplitter(splitterWidget);
                
                // Reparent panels to splitter before splitting
                if (existingPanel->GetPanel() && panelNode->GetPanel()) {
                    // First reparent both panels to the splitter
                    existingPanel->GetPanel()->Reparent(splitterWidget);
                    panelNode->GetPanel()->Reparent(splitterWidget);
                    
                    // Now split the splitter with the reparented panels
                    // For vertical splitter in left sidebar, we want top/bottom layout
                    // So use SplitHorizontally (Object Tree on top, Properties on bottom)
                    splitterWidget->SplitHorizontally(existingPanel->GetPanel(), panelNode->GetPanel());
                }
                
                vSplitter->AddChild(std::move(existingPanel));  // Object Tree (top)
                vSplitter->AddChild(std::move(panelNode));      // Properties (bottom)
                
                leftSidebar->AddChild(std::move(vSplitter));
            } else {
                // Multiple panels - add to existing structure
                leftSidebar->AddChild(std::move(panelNode));
            }
            break;
            
        case DockArea::Center:
            // Center canvas area: Canvas should be alone here
            centerCanvas->AddChild(std::move(panelNode));
            break;
            
        case DockArea::Bottom:
            // Bottom status bar area: Multiple panels should be in tab container
            // All bottom panels go to the same container for tabbed display
            bottomStatusBar->AddChild(std::move(panelNode));
            break;
            
        default:
            // Fallback for other areas
            parent->AddChild(std::move(panelNode));
            break;
    }
}

void LayoutEngine::CreateMainLayoutStructure(LayoutNode* parent)
{
    if (!parent) return;
    
    // Clear existing children
    parent->GetChildren().clear();
    
    // Create main VERTICAL splitter: Top Work Area | Bottom Status Bar
    auto mainVSplitter = CreateSplitterNode(false); // vertical split
    
    // Create actual vertical splitter control
    wxSplitterWindow* mainVSplitterWidget = CreateSplitterWindow(m_parent, false);
    mainVSplitter->SetSplitter(mainVSplitterWidget);
    
    // Create top work area container (will hold Left Sidebar + Center Canvas)
    auto topWorkArea = std::make_unique<LayoutNode>(LayoutNodeType::Root);
    
    // Create bottom status bar container (will hold Message + Performance)
    auto bottomStatusBar = std::make_unique<LayoutNode>(LayoutNodeType::Root);
    
    // Create horizontal splitter for top work area: Left Sidebar | Center Canvas
    auto topHSplitter = CreateSplitterNode(true); // horizontal split
    
    // Create actual horizontal splitter control
    wxSplitterWindow* topHSplitterWidget = CreateSplitterWindow(m_parent, true);
    topHSplitter->SetSplitter(topHSplitterWidget);
    
    // Create left sidebar container (will hold Object Tree + Properties)
    auto leftSidebar = std::make_unique<LayoutNode>(LayoutNodeType::Root);
    
    // Create center canvas container (will hold Canvas)
    auto centerCanvas = std::make_unique<LayoutNode>(LayoutNodeType::Root);
    
    // Build the structure step by step
    // 1. Assemble top horizontal splitter
    topHSplitter->AddChild(std::move(leftSidebar));
    topHSplitter->AddChild(std::move(centerCanvas));
    
    // 2. Put horizontal splitter into top work area
    topWorkArea->AddChild(std::move(topHSplitter));
    
    // 3. Assemble main vertical splitter
    mainVSplitter->AddChild(std::move(topWorkArea));
    mainVSplitter->AddChild(std::move(bottomStatusBar));
    
    // 4. Add main vertical splitter to parent
    parent->AddChild(std::move(mainVSplitter));
}

void LayoutEngine::RemovePanelFromTree(LayoutNode* panelNode)
{
    if (!panelNode || panelNode->GetType() != LayoutNodeType::Panel) return;
    
    LayoutNode* parent = panelNode->GetParent();
    if (!parent) return;
    
    // Remove from parent
    parent->RemoveChild(panelNode);
}

void LayoutEngine::CalculateNodeLayout(LayoutNode* node, const wxRect& rect)
{
    if (!node) return;
    
    node->SetRect(rect);
    
    if (node->GetType() == LayoutNodeType::Panel) {
        // Panel node - apply rect to panel widget
        if (node->GetPanel()) {
            node->GetPanel()->SetSize(rect);
        }
    } else if (node->GetType() == LayoutNodeType::HorizontalSplitter ||
               node->GetType() == LayoutNodeType::VerticalSplitter) {
        // Splitter node - calculate layout for children using splitter ratios
        CalculateSplitterLayout(node, rect);
    } else {
        // Root or container nodes - handle different cases
        auto& children = node->GetChildren();
        if (children.empty()) {
            return;
        }
        
        if (children.size() == 1) {
            // Single child - give it the full rect
            CalculateNodeLayout(children[0].get(), rect);
        } else {
            // Multiple children - check what type of container this is
            
            // Special handling for main root with horizontal splitter
            if (children.size() == 1 && children[0]->GetType() == LayoutNodeType::HorizontalSplitter) {
                // This is the root node with main horizontal splitter
                CalculateNodeLayout(children[0].get(), rect);
                return;
            }
            
            // For multiple panels in the same container, arrange them as tabs
            // All panels occupy the same space (tab container behavior)
            for (auto& child : children) {
                CalculateNodeLayout(child.get(), rect);
            }
        }
    }
}

void LayoutEngine::CalculateSplitterLayout(LayoutNode* splitterNode, const wxRect& rect)
{
    if (!splitterNode || splitterNode->GetChildren().size() != 2) return;
    
    bool isHorizontal = (splitterNode->GetType() == LayoutNodeType::HorizontalSplitter);
    double ratio = splitterNode->GetSplitterRatio();
    
    // Set appropriate default ratios for IDE-style layout
    if (ratio == 0.5) { // Default ratio, set IDE-appropriate values
        if (isHorizontal) {
            // Horizontal splitter: Top work area splitter (Left Sidebar | Center Canvas)
            // Left sidebar should be smaller, about 1/6 of width
            ratio = 0.15;
        } else {
            // Vertical splitter: could be Main splitter or Left sidebar internal splitter
            if (IsLeftSidebarSplitter(splitterNode)) {
                // Left sidebar internal: Object Tree (top) should be larger than Properties (bottom)
                ratio = 0.6; // Object Tree gets 60%, Properties gets 40%
            } else {
                // Main splitter: Top Work Area | Bottom Status Bar
                // Top work area should be much larger than bottom status bar
                ratio = 0.85; // Top work area gets 85%, Bottom status bar gets 15%
            }
        }
        splitterNode->SetSplitterRatio(ratio);
    }
    
    wxRect firstRect, secondRect;
    
    if (isHorizontal) {
        // Horizontal split (left/right)
        int splitPos = static_cast<int>(rect.width * ratio);
        splitPos = std::max(splitPos, m_minPanelSize.x);
        splitPos = std::min(splitPos, rect.width - m_minPanelSize.x);
        
        firstRect = wxRect(rect.x, rect.y, splitPos, rect.height);
        secondRect = wxRect(rect.x + splitPos, rect.y, rect.width - splitPos, rect.height);
    } else {
        // Vertical split (top/bottom)
        int splitPos = static_cast<int>(rect.height * ratio);
        splitPos = std::max(splitPos, m_minPanelSize.y);
        splitPos = std::min(splitPos, rect.height - m_minPanelSize.y);
        
        firstRect = wxRect(rect.x, rect.y, rect.width, splitPos);
        secondRect = wxRect(rect.x, rect.y + splitPos, rect.width, rect.height - splitPos);
    }
    
    // Apply to children
    CalculateNodeLayout(splitterNode->GetChildren()[0].get(), firstRect);
    CalculateNodeLayout(splitterNode->GetChildren()[1].get(), secondRect);
}

void LayoutEngine::ApplyLayoutToWidgets(LayoutNode* node)
{
    if (!node) return;
    
    // If this is a panel node, apply its layout
    if (node->GetType() == LayoutNodeType::Panel && node->GetPanel()) {
        wxRect rect = node->GetRect();
        node->GetPanel()->SetSize(rect);
        node->GetPanel()->Show();
        
        // Force refresh of the panel to ensure proper display
        node->GetPanel()->Refresh();
        node->GetPanel()->Update();
    }
    
    // If this is a splitter node, ensure splitter control is properly displayed
    if (node->GetType() == LayoutNodeType::HorizontalSplitter || 
        node->GetType() == LayoutNodeType::VerticalSplitter) {
        
        if (node->GetSplitter()) {
            wxSplitterWindow* splitter = node->GetSplitter();
            
            // Set splitter position and size
            wxRect rect = node->GetRect();
            splitter->SetSize(rect);
            
            // Setup splitter panels if needed
            SetupSplitterPanels(node);
            
            // Ensure splitter is visible
            splitter->Show();
            
            // Update splitter sash position
            UpdateSplitterSashPosition(node);
            
            // Force refresh of the splitter
            splitter->Refresh();
            splitter->Update();
        }
    }
    
    // Apply layout recursively
    for (auto& child : node->GetChildren()) {
        ApplyLayoutToWidgets(child.get());
    }
}

wxSplitterWindow* LayoutEngine::CreateSplitterWindow(wxWindow* parent, bool horizontal)
{
    if (!parent) return nullptr;
    
    // Remove wxSP_NOBORDER flag to ensure splitter is visible
    wxSplitterWindow* splitter = new wxSplitterWindow(parent, wxID_ANY,
        wxDefaultPosition, wxDefaultSize,
        wxSP_PERMIT_UNSPLIT | wxSP_NOBORDER);
    
    ConfigureSplitter(splitter, horizontal);
    
    return splitter;
}

void LayoutEngine::ConfigureSplitter(wxSplitterWindow* splitter, bool horizontal)
{
    if (!splitter) return;
    
    splitter->SetSashGravity(0.5);
    splitter->SetMinimumPaneSize(m_minPanelSize.x);
    
    // Set splitter visual style - use modern methods
    // splitter->SetSashSize(m_splitterSashSize); // Deprecated, removed
    
    // Bind events
    splitter->Bind(wxEVT_SPLITTER_SASH_POS_CHANGED, &LayoutEngine::OnSplitterMoved, this);
    splitter->Bind(wxEVT_SPLITTER_DOUBLECLICKED, &LayoutEngine::OnSplitterDoubleClick, this);
    
    // Set splitter color to ensure visibility
    splitter->SetBackgroundColour(wxColour(200, 200, 200));
    
    // Set splitter mode based on horizontal parameter
    if (horizontal) {
        splitter->SetSplitMode(wxSPLIT_VERTICAL); // Left/Right split
    } else {
        splitter->SetSplitMode(wxSPLIT_HORIZONTAL); // Top/Bottom split
    }
}

void LayoutEngine::SetupSplitterPanels(LayoutNode* splitterNode)
{
    if (!splitterNode || !splitterNode->GetSplitter()) return;
    if (splitterNode->GetChildren().size() != 2) return;
    
    wxSplitterWindow* splitter = splitterNode->GetSplitter();
    
    // Don't re-split if already split
    if (splitter->IsSplit()) return;
    
    // Get child nodes
    LayoutNode* firstChild = splitterNode->GetChildren()[0].get();
    LayoutNode* secondChild = splitterNode->GetChildren()[1].get();
    
    wxWindow* firstWindow = GetChildWindow(firstChild);
    wxWindow* secondWindow = GetChildWindow(secondChild);
    
    if (firstWindow && secondWindow) {
        // Reparent windows to splitter
        firstWindow->Reparent(splitter);
        secondWindow->Reparent(splitter);
        
        // Split based on splitter type
        // Note: wxWidgets naming convention:
        // - HorizontalSplitter splits vertically (left/right)
        // - VerticalSplitter splits horizontally (top/bottom)
        bool isHorizontal = (splitterNode->GetType() == LayoutNodeType::HorizontalSplitter);
        if (isHorizontal) {
            splitter->SplitVertically(firstWindow, secondWindow); // Left/Right split
        } else {
            splitter->SplitHorizontally(firstWindow, secondWindow); // Top/Bottom split
        }
        
        // Force refresh of all components after reparenting
        firstWindow->Refresh();
        secondWindow->Refresh();
        splitter->Refresh();
        
        // Ensure proper layout update
        splitter->Layout();
        firstWindow->Layout();
        secondWindow->Layout();
    }
}

wxWindow* LayoutEngine::GetChildWindow(LayoutNode* node)
{
    if (!node) return nullptr;
    
    // If this is a panel node, return the panel
    if (node->GetType() == LayoutNodeType::Panel && node->GetPanel()) {
        return node->GetPanel();
    }
    
    // If this is a splitter node, return the splitter
    if ((node->GetType() == LayoutNodeType::HorizontalSplitter || 
         node->GetType() == LayoutNodeType::VerticalSplitter) && 
        node->GetSplitter()) {
        return node->GetSplitter();
    }
    
    // For container nodes, create a temporary panel if it has children
    if (!node->GetChildren().empty()) {
        // For now, just return the first child's window
        // In a more sophisticated implementation, you might create a container widget
        return GetChildWindow(node->GetChildren()[0].get());
    }
    
    return nullptr;
}

void LayoutEngine::UpdateSplitterChildrenLayout(LayoutNode* splitterNode)
{
    if (!splitterNode || !splitterNode->GetSplitter()) return;
    if (splitterNode->GetChildren().size() != 2) return;
    
    wxSplitterWindow* splitter = splitterNode->GetSplitter();
    if (!splitter->IsSplit()) return;
    
    // Get current splitter rect
    wxRect splitterRect = splitterNode->GetRect();
    
    // Calculate new layout for children based on current sash position
    bool isHorizontal = (splitterNode->GetType() == LayoutNodeType::HorizontalSplitter);
    int sashPos = splitter->GetSashPosition();
    
    wxRect firstRect, secondRect;
    
    if (isHorizontal) {
        // Horizontal splitter: left/right layout
        int splitPos = std::max(sashPos, m_minPanelSize.x);
        splitPos = std::min(splitPos, splitterRect.width - m_minPanelSize.x);
        
        firstRect = wxRect(splitterRect.x, splitterRect.y, splitPos, splitterRect.height);
        secondRect = wxRect(splitterRect.x + splitPos, splitterRect.y, 
                           splitterRect.width - splitPos, splitterRect.height);
    } else {
        // Vertical splitter: top/bottom layout
        int splitPos = std::max(sashPos, m_minPanelSize.y);
        splitPos = std::min(splitPos, splitterRect.height - m_minPanelSize.y);
        
        firstRect = wxRect(splitterRect.x, splitterRect.y, splitterRect.width, splitPos);
        secondRect = wxRect(splitterRect.x, splitterRect.y + splitPos, 
                           splitterRect.width, splitterRect.height - splitPos);
    }
    
    // Apply new layout to children
    CalculateNodeLayout(splitterNode->GetChildren()[0].get(), firstRect);
    CalculateNodeLayout(splitterNode->GetChildren()[1].get(), secondRect);
    
    // Apply layout to widgets
    ApplyLayoutToWidgets(splitterNode->GetChildren()[0].get());
    ApplyLayoutToWidgets(splitterNode->GetChildren()[1].get());
}

void LayoutEngine::UpdateSplitterSashPosition(LayoutNode* splitterNode)
{
    if (!splitterNode || !splitterNode->GetSplitter()) return;
    
    wxSplitterWindow* splitter = splitterNode->GetSplitter();
    if (!splitter->IsSplit()) return;
    
    bool isHorizontal = (splitterNode->GetType() == LayoutNodeType::HorizontalSplitter);
    wxRect rect = splitterNode->GetRect();
    double ratio = splitterNode->GetSplitterRatio();
    
    int sashPos;
    if (isHorizontal) {
        sashPos = static_cast<int>(rect.width * ratio);
    } else {
        sashPos = static_cast<int>(rect.height * ratio);
    }
    
    splitter->SetSashPosition(sashPos);
}

bool LayoutEngine::ValidateConstraints(LayoutNode* node, const wxRect& rect) const
{
    if (!node) return false;
    
    const LayoutConstraints& constraints = node->GetConstraints();
    
    return (rect.width >= constraints.minWidth && 
            rect.height >= constraints.minHeight &&
            (constraints.maxWidth < 0 || rect.width <= constraints.maxWidth) &&
            (constraints.maxHeight < 0 || rect.height <= constraints.maxHeight));
}

wxRect LayoutEngine::EnforceConstraints(LayoutNode* node, const wxRect& rect) const
{
    if (!node) return rect;
    
    const LayoutConstraints& constraints = node->GetConstraints();
    wxRect constrainedRect = rect;
    
    constrainedRect.width = std::max(constraints.minWidth, constrainedRect.width);
    constrainedRect.height = std::max(constraints.minHeight, constrainedRect.height);
    
    if (constraints.maxWidth > 0) {
        constrainedRect.width = std::min(constraints.maxWidth, constrainedRect.width);
    }
    if (constraints.maxHeight > 0) {
        constrainedRect.height = std::min(constraints.maxHeight, constrainedRect.height);
    }
    
    return constrainedRect;
}

void LayoutEngine::UpdateConstraintsRecursive(LayoutNode* node)
{
    if (!node) return;
    
    // Update constraints for this node
    // Implementation depends on specific requirements
    
    // Update children
    for (auto& child : node->GetChildren()) {
        UpdateConstraintsRecursive(child.get());
    }
}

DockArea LayoutEngine::PositionToDockArea(DockPosition position) const
{
    switch (position) {
        case DockPosition::Left:   return DockArea::Left;
        case DockPosition::Right:  return DockArea::Right;
        case DockPosition::Top:    return DockArea::Top;
        case DockPosition::Bottom: return DockArea::Bottom;
        case DockPosition::Center: return DockArea::Center;
        default: return DockArea::Center;
    }
}

bool LayoutEngine::CanDockAtPosition(LayoutNode* target, DockPosition position) const
{
    // Implementation for validating dock operations
    return target != nullptr && position != DockPosition::None;
}

bool LayoutEngine::IsLeftSidebarSplitter(LayoutNode* splitterNode) const
{
    if (!splitterNode || !m_rootNode) return false;
    
    // Navigate the expected structure to check if this splitter is in the left sidebar
    // Structure: Root -> VerticalSplitter(TopWorkArea | BottomStatusBar)
    //           TopWorkArea -> HorizontalSplitter(LeftSidebar | CenterCanvas)
    
    // Check if we can find the main vertical splitter
    if (m_rootNode->GetChildren().empty() || 
        m_rootNode->GetChildren()[0]->GetType() != LayoutNodeType::VerticalSplitter) {
        return false;
    }
    
    auto& mainVSplitter = m_rootNode->GetChildren()[0];
    if (mainVSplitter->GetChildren().size() < 2) {
        return false;
    }
    
    auto topWorkArea = mainVSplitter->GetChildren()[0].get();
    if (topWorkArea->GetChildren().empty() || 
        topWorkArea->GetChildren()[0]->GetType() != LayoutNodeType::HorizontalSplitter) {
        return false;
    }
    
    auto topHSplitter = topWorkArea->GetChildren()[0].get();
    if (topHSplitter->GetChildren().size() < 2) {
        return false;
    }
    
    auto leftSidebar = topHSplitter->GetChildren()[0].get();
    
    // Check if the splitter is within the left sidebar hierarchy
    return IsNodeInHierarchy(leftSidebar, splitterNode);
}

bool LayoutEngine::IsNodeInHierarchy(LayoutNode* ancestor, LayoutNode* target) const
{
    if (!ancestor || !target) return false;
    if (ancestor == target) return true;
    
    // Recursively check children
    for (auto& child : ancestor->GetChildren()) {
        if (IsNodeInHierarchy(child.get(), target)) {
            return true;
        }
    }
    
    return false;
}

bool LayoutEngine::IsMainStructureNode(LayoutNode* node) const
{
    if (!node || !m_rootNode) return false;
    
    // Check if this node is part of the main IDE layout structure
    // Main structure: Root -> VerticalSplitter(TopWorkArea | BottomStatusBar)
    //                TopWorkArea -> HorizontalSplitter(LeftSidebar | CenterCanvas)
    
    // Navigate from root to find if this node is in the main structure
    if (m_rootNode->GetChildren().empty()) {
        return false;
    }
    
    auto& mainVSplitter = m_rootNode->GetChildren()[0];
    if (mainVSplitter->GetType() != LayoutNodeType::VerticalSplitter) {
        return false;
    }
    
    // Check if node is the main vertical splitter
    if (node == mainVSplitter.get()) {
        return true;
    }
    
    // Check if node is in top work area
    if (mainVSplitter->GetChildren().size() > 0) {
        auto& topWorkArea = mainVSplitter->GetChildren()[0];
        if (node == topWorkArea.get()) {
            return true;
        }
        
        // Check if node is the top horizontal splitter
        if (topWorkArea->GetChildren().size() > 0) {
            auto& topHSplitter = topWorkArea->GetChildren()[0];
            if (node == topHSplitter.get()) {
                return true;
            }
            
            // Check if node is in left sidebar or center canvas
            if (topHSplitter->GetChildren().size() >= 2) {
                auto& leftSidebar = topHSplitter->GetChildren()[0];
                auto& centerCanvas = topHSplitter->GetChildren()[1];
                
                if (node == leftSidebar.get() || node == centerCanvas.get()) {
                    return true;
                }
            }
        }
    }
    
    // Check if node is in bottom status bar
    if (mainVSplitter->GetChildren().size() > 1) {
        auto& bottomStatusBar = mainVSplitter->GetChildren()[1];
        if (node == bottomStatusBar.get()) {
            return true;
        }
    }
    
    return false;
}

void LayoutEngine::SetPanelDockAreaByPosition(LayoutNode* panelNode, DockPosition position)
{
    if (!panelNode || !panelNode->GetPanel()) return;
    
    // Set the dock area based on the docking position
    // This ensures the panel goes to the correct container in OrganizeByDockAreas
    DockArea newArea;
    
    switch (position) {
        case DockPosition::Left:
            newArea = DockArea::Left;
            break;
        case DockPosition::Right:
            newArea = DockArea::Right;
            break;
        case DockPosition::Top:
            newArea = DockArea::Top;
            break;
        case DockPosition::Bottom:
            newArea = DockArea::Bottom;
            break;
        case DockPosition::Center:
        default:
            newArea = DockArea::Center;
            break;
    }
    
    // Set the dock area on both the layout node and the panel
    panelNode->SetDockArea(newArea);
    panelNode->GetPanel()->SetDockArea(newArea);
    
    wxLogDebug("SetPanelDockAreaByPosition: Set panel %s to area %d based on position %d", 
               panelNode->GetPanel()->GetTitle(), (int)newArea, (int)position);
}

LayoutNode* LayoutEngine::FindBestInsertionPoint(DockArea area) const
{
    if (!m_rootNode) return nullptr;
    
    // If root has no children, return root
    if (m_rootNode->GetChildren().empty()) {
        return m_rootNode.get();
    }
    
    // Look for existing structure based on area
    switch (area) {
        case DockArea::Left: {
            // Look for left sidebar in main horizontal splitter
            if (m_rootNode->GetChildren().size() > 0 && 
                m_rootNode->GetChildren()[0]->GetType() == LayoutNodeType::HorizontalSplitter) {
                auto& mainSplitter = m_rootNode->GetChildren()[0];
                if (mainSplitter->GetChildren().size() > 0) {
                    return mainSplitter->GetChildren()[0].get(); // Left sidebar
                }
            }
            break;
        }
        case DockArea::Center: {
            // Look for center area in center+bottom vertical splitter
            if (m_rootNode->GetChildren().size() > 0 && 
                m_rootNode->GetChildren()[0]->GetType() == LayoutNodeType::HorizontalSplitter) {
                auto& mainSplitter = m_rootNode->GetChildren()[0];
                if (mainSplitter->GetChildren().size() > 1) {
                    auto& centerBottom = mainSplitter->GetChildren()[1];
                    if (centerBottom->GetType() == LayoutNodeType::VerticalSplitter &&
                        centerBottom->GetChildren().size() > 0) {
                        return centerBottom->GetChildren()[0].get(); // Center area
                    }
                }
            }
            break;
        }
        case DockArea::Bottom: {
            // Look for bottom area in center+bottom vertical splitter
            if (m_rootNode->GetChildren().size() > 0 && 
                m_rootNode->GetChildren()[0]->GetType() == LayoutNodeType::HorizontalSplitter) {
                auto& mainSplitter = m_rootNode->GetChildren()[0];
                if (mainSplitter->GetChildren().size() > 1) {
                    auto& centerBottom = mainSplitter->GetChildren()[1];
                    if (centerBottom->GetType() == LayoutNodeType::VerticalSplitter &&
                        centerBottom->GetChildren().size() > 1) {
                        return centerBottom->GetChildren()[1].get(); // Bottom area
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    
    // If no specific area found, return root
    return m_rootNode.get();
}

void LayoutEngine::CleanupEmptyNodes()
{
    if (!m_rootNode) return;
    
    wxLogDebug("CleanupEmptyNodes: Starting cleanup");
    
    // More conservative cleanup - only remove truly empty nodes
    // and preserve the main layout structure
    
    std::function<bool(LayoutNode*)> cleanup = [&](LayoutNode* node) -> bool {
        if (!node) return false;
        
        // Clean children first
        auto& children = node->GetChildren();
        for (auto it = children.begin(); it != children.end();) {
            bool childEmpty = cleanup(it->get());
            
            // VERY CONSERVATIVE: Only remove nodes that are truly empty
            // and don't have any meaningful content
            bool shouldRemove = false;
            
            if (childEmpty) {
                // Only remove if child is completely empty
                shouldRemove = true;
            } else if ((*it)->GetType() == LayoutNodeType::HorizontalSplitter ||
                       (*it)->GetType() == LayoutNodeType::VerticalSplitter) {
                // For splitters, be EXTREMELY conservative
                // Only remove if they have 0 children AND are not part of main structure
                if ((*it)->GetChildren().size() == 0) {
                    // Additional check: don't remove if this might be part of main structure
                    bool isMainStructure = IsMainStructureNode(it->get());
                    shouldRemove = !isMainStructure;
                }
                // Never remove splitters with 1 or more children
            } else if ((*it)->GetType() == LayoutNodeType::Panel) {
                // Never remove panel nodes
                shouldRemove = false;
            }
            
            if (shouldRemove) {
                wxLogDebug("CleanupEmptyNodes: Removing node type=%d with %d children", 
                          (int)(*it)->GetType(), (int)(*it)->GetChildren().size());
                
                // If this is a splitter, unsplit it first
                if ((*it)->GetSplitter()) {
                    (*it)->GetSplitter()->Unsplit();
                }
                it = children.erase(it);
            } else {
                wxLogDebug("CleanupEmptyNodes: Keeping node type=%d with %d children (main structure: %s)", 
                          (int)(*it)->GetType(), (int)(*it)->GetChildren().size(),
                          IsMainStructureNode(it->get()) ? "yes" : "no");
                ++it;
            }
        }
        
        // Return true if this node is now empty (but not if it's the root)
        return node != m_rootNode.get() && children.empty() && 
               node->GetType() != LayoutNodeType::Panel;
    };
    
    cleanup(m_rootNode.get());
    wxLogDebug("CleanupEmptyNodes: Cleanup complete");
}

bool LayoutEngine::IsNodeValid(LayoutNode* node) const
{
    if (!node || !m_rootNode) return false;
    
    // Check if node exists in the tree by traversing from root
    std::function<bool(LayoutNode*)> findNode = [&](LayoutNode* current) -> bool {
        if (current == node) return true;
        
        for (auto& child : current->GetChildren()) {
            if (findNode(child.get())) return true;
        }
        
        return false;
    };
    
    return findNode(m_rootNode.get());
}

void LayoutEngine::InitializeTransition(LayoutTransition& transition, LayoutNode* node)
{
    transition.node = node;
    transition.progress = 0.0;
    transition.active = true;
    
    if (node) {
        transition.startRect = node->GetRect();
        transition.targetRect = node->GetRect(); // Will be updated by caller
    }
}

void LayoutEngine::ApplyTransition(const LayoutTransition& transition)
{
    if (!transition.node || !transition.active) return;
    
    // Interpolate between start and target rect
    wxRect currentRect;
    currentRect.x = transition.startRect.x + 
        static_cast<int>((transition.targetRect.x - transition.startRect.x) * transition.progress);
    currentRect.y = transition.startRect.y + 
        static_cast<int>((transition.targetRect.y - transition.startRect.y) * transition.progress);
    currentRect.width = transition.startRect.width + 
        static_cast<int>((transition.targetRect.width - transition.startRect.width) * transition.progress);
    currentRect.height = transition.startRect.height + 
        static_cast<int>((transition.targetRect.height - transition.startRect.height) * transition.progress);
    
    // Apply interpolated rect
    CalculateNodeLayout(transition.node, currentRect);
    ApplyLayoutToWidgets(transition.node);
}

void LayoutEngine::CompleteTransition(LayoutTransition& transition)
{
    if (!transition.node) return;
    
    // Apply final layout
    CalculateNodeLayout(transition.node, transition.targetRect);
    ApplyLayoutToWidgets(transition.node);
    
    transition.active = false;
}

void LayoutEngine::OnAnimationTimer(wxTimerEvent& event)
{
    wxUnusedVar(event);
    
    // Check if this is a delayed cleanup timer
    if (m_delayedCleanupPending) {
        // This is a delayed cleanup timer - perform conservative cleanup
        wxLogDebug("LayoutEngine: Performing delayed cleanup");
        m_delayedCleanupPending = false;
        CleanupEmptyNodes();
        
        // Update layout after cleanup
        if (m_manager) {
            UpdateLayout();
        }
    } else if (m_layoutDirty && !m_activeTransitions.empty()) {
        // This is a regular animation timer
        UpdateTransitions();
    }
}

void LayoutEngine::OnSplitterMoved(wxSplitterEvent& event)
{
    // Update splitter ratio when user moves sash
    wxSplitterWindow* splitter = dynamic_cast<wxSplitterWindow*>(event.GetEventObject());
    if (!splitter || m_isUpdatingLayout) {
        event.Skip();
        return;
    }
    
    // Find corresponding layout node
    LayoutNode* splitterNode = FindSplitterNode(splitter);
    if (splitterNode) {
        // Calculate new ratio
        bool isHorizontal = (splitterNode->GetType() == LayoutNodeType::HorizontalSplitter);
        int sashPos = splitter->GetSashPosition();
        wxSize splitterSize = splitter->GetSize();
        
        double newRatio;
        if (isHorizontal) {
            newRatio = static_cast<double>(sashPos) / splitterSize.GetWidth();
        } else {
            newRatio = static_cast<double>(sashPos) / splitterSize.GetHeight();
        }
        
        // Limit ratio range
        newRatio = std::max(MIN_SPLITTER_RATIO, std::min(MAX_SPLITTER_RATIO, newRatio));
        
        // Update node ratio
        splitterNode->SetSplitterRatio(newRatio);
        
        // Don't call UpdateLayout() here as it will override the user's manual adjustment
        // Just mark layout as dirty for later update if needed
        m_layoutDirty = true;
        
        // Use debounced update to prevent excessive calls during drag
        RequestLayoutUpdate();
    }
    
    event.Skip();
}

// Helper method to find splitter node
LayoutNode* LayoutEngine::FindSplitterNode(wxSplitterWindow* splitter) const
{
    if (!splitter || !m_rootNode) return nullptr;
    
    std::function<LayoutNode*(LayoutNode*)> findNode = [&](LayoutNode* node) -> LayoutNode* {
        if (node->GetSplitter() == splitter) {
            return node;
        }
        
        for (auto& child : node->GetChildren()) {
            LayoutNode* result = findNode(child.get());
            if (result) return result;
        }
        
        return nullptr;
    };
    
    return findNode(m_rootNode.get());
}

void LayoutEngine::OnSplitterDoubleClick(wxSplitterEvent& event)
{
    // Reset splitter to center position
    wxSplitterWindow* splitter = dynamic_cast<wxSplitterWindow*>(event.GetEventObject());
    if (splitter && splitter->IsSplit()) {
        bool isHorizontal = (splitter->GetSplitMode() == wxSPLIT_HORIZONTAL);
        wxSize size = splitter->GetSize();
        int centerPos = isHorizontal ? size.y / 2 : size.x / 2;
        splitter->SetSashPosition(centerPos);
    }
    
    event.Skip();
}

void LayoutEngine::InsertPanelWithSplitter(std::unique_ptr<LayoutNode> panelNode, LayoutNode* parent, DockPosition position)
{
    if (!panelNode || !parent) return;
    
    wxLogDebug("InsertPanelWithSplitter: panel=%s, parent type=%d, position=%d", 
               panelNode->GetPanel() ? panelNode->GetPanel()->GetTitle() : "null", 
               (int)parent->GetType(), (int)position);
    
    // Determine the actual parent to insert into and the target panel
    LayoutNode* insertParent = parent;
    ModernDockPanel* targetPanel = nullptr;
    
    if (parent->GetType() == LayoutNodeType::Panel) {
        // Parent is a panel node - we need to insert at its parent level
        targetPanel = parent->GetPanel();
        insertParent = parent->GetParent();
        if (!insertParent) {
            insertParent = m_rootNode.get();
        }
        
        // CRITICAL: Check if we're about to destroy a main structure splitter
        // If the parent of this panel is a main structure splitter, we need to be more careful
        if (insertParent && IsMainStructureNode(insertParent)) {
            wxLogDebug("InsertPanelWithSplitter: insertParent is main structure, being careful");
            // Don't use root - preserve the main structure
            // We'll insert the new splitter as a sibling to the target panel
        } else if (insertParent != m_rootNode.get() && insertParent->GetChildren().size() <= 1) {
            wxLogDebug("InsertPanelWithSplitter: insertParent has only 1 child, using root instead");
            insertParent = m_rootNode.get();
        }
        
        wxLogDebug("InsertPanelWithSplitter: Parent is panel, targetPanel=%s, insertParent=%s", 
                   targetPanel ? targetPanel->GetTitle() : "null",
                   insertParent == m_rootNode.get() ? "root" : "other");
    } else {
        // Parent is a container - find the first panel child
        for (auto& child : parent->GetChildren()) {
            if (child->GetType() == LayoutNodeType::Panel) {
                targetPanel = child->GetPanel();
                break;
            }
        }
        wxLogDebug("InsertPanelWithSplitter: Parent is container, targetPanel=%s", 
                   targetPanel ? targetPanel->GetTitle() : "null");
    }
    
    if (!targetPanel) {
        // Fallback to OrganizeByDockAreas if no target panel found
        wxLogDebug("InsertPanelWithSplitter: No target panel found, using OrganizeByDockAreas");
        OrganizeByDockAreas(std::move(panelNode), insertParent);
        return;
    }
    
    // Create appropriate splitter based on position
    // Note: wxWidgets naming convention:
    // - Left/Right positions need vertical split (splitter line goes up/down)
    // - Top/Bottom positions need horizontal split (splitter line goes left/right)
    bool isVerticalSplit = (position == DockPosition::Left || position == DockPosition::Right);
    auto splitterNode = CreateSplitterNode(isVerticalSplit);
    
    // Create actual splitter window
    wxSplitterWindow* splitterWidget = CreateSplitterWindow(m_parent, isVerticalSplit);
    
    wxLogDebug("InsertPanelWithSplitter: Position=%d, isVerticalSplit=%s", 
               (int)position, isVerticalSplit ? "true" : "false");
    
    // IMPORTANT: Add splitter to parent window's layout
    // This ensures the splitter is actually visible in the UI
    if (m_parent) {
        wxLogDebug("InsertPanelWithSplitter: Adding splitter to parent window");
        // Note: We don't call m_parent->AddChild() here because that's for wxWidgets layout
        // The splitter is already created with m_parent as its parent window
        // We just need to ensure it's properly configured and visible
    }
    
    splitterNode->SetSplitter(splitterWidget);
    
    // Determine which panel goes first based on position
    std::unique_ptr<LayoutNode> firstPanel, secondPanel;
    
    if (position == DockPosition::Left || position == DockPosition::Top) {
        // New panel goes first (left/top)
        firstPanel = std::move(panelNode);
        secondPanel = CreatePanelNode(targetPanel);
        
        // Remove target panel from its current location
        wxLogDebug("InsertPanelWithSplitter: Removing target panel %s from insertParent (Left/Top case)", 
                   targetPanel ? targetPanel->GetTitle() : "null");
        
        // Check if we're dealing with a main structure node
        bool isMainStructure = insertParent && IsMainStructureNode(insertParent);
        
        if (parent->GetType() == LayoutNodeType::Panel) {
            if (isMainStructure) {
                wxLogDebug("InsertPanelWithSplitter: insertParent is main structure, using OrganizeByDockAreas instead");
                // Don't destroy main structure - use OrganizeByDockAreas instead
                // But first, set the correct dock area based on position
                SetPanelDockAreaByPosition(panelNode.get(), position);
                OrganizeByDockAreas(std::move(panelNode), insertParent);
                return;
            } else {
                // Parent is the target panel node itself
                wxLogDebug("InsertPanelWithSplitter: Removing parent panel node");
                insertParent->RemoveChild(parent);
            }
        } else {
            // Remove target panel from its parent container
            wxLogDebug("InsertPanelWithSplitter: Removing target panel from container");
            for (auto& child : insertParent->GetChildren()) {
                if (child->GetType() == LayoutNodeType::Panel && child->GetPanel() == targetPanel) {
                    insertParent->RemoveChild(child.get());
                    break;
                }
            }
        }
        wxLogDebug("InsertPanelWithSplitter: After removal (Left/Top case), insertParent has %d children", 
                   (int)insertParent->GetChildren().size());
    } else {
        // New panel goes second (right/bottom)
        firstPanel = CreatePanelNode(targetPanel);
        secondPanel = std::move(panelNode);
        
        // Remove target panel from its current location
        wxLogDebug("InsertPanelWithSplitter: Removing target panel %s from insertParent (Right/Bottom case)", 
                   targetPanel ? targetPanel->GetTitle() : "null");
        
        // Check if we're dealing with a main structure node
        bool isMainStructure = insertParent && IsMainStructureNode(insertParent);
        
        if (parent->GetType() == LayoutNodeType::Panel) {
            if (isMainStructure) {
                wxLogDebug("InsertPanelWithSplitter: insertParent is main structure, using OrganizeByDockAreas instead");
                // Don't destroy main structure - use OrganizeByDockAreas instead
                // But first, set the correct dock area based on position
                SetPanelDockAreaByPosition(panelNode.get(), position);
                OrganizeByDockAreas(std::move(panelNode), insertParent);
                return;
            } else {
                // Parent is the target panel node itself
                wxLogDebug("InsertPanelWithSplitter: Removing parent panel node");
                insertParent->RemoveChild(parent);
            }
        } else {
            // Remove target panel from its parent container
            wxLogDebug("InsertPanelWithSplitter: Removing target panel from container");
            for (auto& child : insertParent->GetChildren()) {
                if (child->GetType() == LayoutNodeType::Panel && child->GetPanel() == targetPanel) {
                    insertParent->RemoveChild(child.get());
                    break;
                }
            }
        }
        wxLogDebug("InsertPanelWithSplitter: After removal (Right/Bottom case), insertParent has %d children", 
                   (int)insertParent->GetChildren().size());
    }
    
    // Add panels to splitter
    wxLogDebug("InsertPanelWithSplitter: Adding panels to splitter");
    splitterNode->AddChild(std::move(firstPanel));
    splitterNode->AddChild(std::move(secondPanel));
    
    wxLogDebug("InsertPanelWithSplitter: Splitter now has %d children", 
               (int)splitterNode->GetChildren().size());
    
    // Store children references before moving splitterNode
    auto& firstChild = splitterNode->GetChildren()[0];
    auto& secondChild = splitterNode->GetChildren()[1];
    
    wxLogDebug("InsertPanelWithSplitter: About to add splitter to insertParent (currently has %d children)", 
               (int)insertParent->GetChildren().size());
    
    // Add splitter to parent
    insertParent->AddChild(std::move(splitterNode));
    
    wxLogDebug("InsertPanelWithSplitter: After adding splitter, insertParent has %d children", 
               (int)insertParent->GetChildren().size());
    
    // Reparent panels to splitter and split
    if (splitterWidget && firstChild && secondChild) {
        if (firstChild->GetPanel() && secondChild->GetPanel()) {
            wxLogDebug("InsertPanelWithSplitter: Reparenting panels to splitter");
            
            // Ensure panels are reparented to splitter
            firstChild->GetPanel()->Reparent(splitterWidget);
            secondChild->GetPanel()->Reparent(splitterWidget);
            
            // Split the splitter
            if (isVerticalSplit) {
                wxLogDebug("InsertPanelWithSplitter: Splitting vertically (left/right)");
                splitterWidget->SplitVertically(firstChild->GetPanel(), secondChild->GetPanel());
            } else {
                wxLogDebug("InsertPanelWithSplitter: Splitting horizontally (top/bottom)");
                splitterWidget->SplitHorizontally(firstChild->GetPanel(), secondChild->GetPanel());
            }
            
            // Set initial sash position to middle
            splitterWidget->SetSashPosition(splitterWidget->GetSize().GetWidth() / 2);
            
            // Ensure splitter is visible and properly sized
            wxLogDebug("InsertPanelWithSplitter: Setting splitter size and showing it");
            
            // Get parent size and set splitter size
            wxSize parentSize = m_parent->GetClientSize();
            wxLogDebug("InsertPanelWithSplitter: Parent size: %dx%d", parentSize.GetWidth(), parentSize.GetHeight());
            
            // Set splitter size to parent size
            splitterWidget->SetSize(parentSize);
            splitterWidget->Show();
            
            // Ensure panels are also visible
            firstChild->GetPanel()->Show();
            secondChild->GetPanel()->Show();
            
            wxLogDebug("InsertPanelWithSplitter: Splitter size set to: %dx%d", 
                      splitterWidget->GetSize().GetWidth(), splitterWidget->GetSize().GetHeight());
            
            // Force layout update
            splitterWidget->Layout();
            m_parent->Layout();
            
            // Additional refresh to ensure visibility
            splitterWidget->Refresh();
            m_parent->Refresh();
            
            // CRITICAL: Ensure the splitter is properly integrated into the parent's layout
            // This is the key fix for making panels visible
            wxLogDebug("InsertPanelWithSplitter: Final integration check");
            
            // Force the parent to recognize the new splitter
            if (m_parent->GetSizer()) {
                wxLogDebug("InsertPanelWithSplitter: Parent has sizer, updating layout");
                m_parent->GetSizer()->Layout();
            }
            
            // Ensure the splitter is the top-most child (if needed)
            splitterWidget->Raise();
            
            // Final layout and refresh
            m_parent->Layout();
            m_parent->Refresh();
        } else {
            wxLogDebug("InsertPanelWithSplitter: Cannot split - missing panel references");
        }
    } else {
        wxLogDebug("InsertPanelWithSplitter: Cannot split - missing splitter or child references");
    }
}

