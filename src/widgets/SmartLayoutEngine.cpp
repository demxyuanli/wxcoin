#include "widgets/SmartLayoutEngine.h"
#include "widgets/ModernDockManager.h"
#include "widgets/ModernDockPanel.h"
#include "widgets/LayoutEngine.h"
#include <wx/log.h>
#include <algorithm>
#include <numeric>
#include <cmath>

wxBEGIN_EVENT_TABLE(SmartLayoutEngine, wxEvtHandler)
wxEND_EVENT_TABLE()

// SmartLayoutEngine implementation
SmartLayoutEngine::SmartLayoutEngine(ModernDockManager* manager)
    : m_manager(manager),
      m_smartModeEnabled(true),
      m_lastQualityScore(0.0)
{
    // Initialize smart components
    m_dockOptimizer = std::make_unique<AutoDockOptimizer>(this);
    m_analyzer = std::make_unique<LayoutAnalyzer>(this);
    m_constraintSolver = std::make_unique<ConstraintLayoutSolver>(this);
    m_splitterOptimizer = std::make_unique<SplitterOptimizer>(this);
    
    // Set default constraints
    m_constraints = SmartLayoutConstraints();
    
    // Initialize with smart layout
    InitializeSmartLayout();
}

SmartLayoutEngine::~SmartLayoutEngine() = default;

void SmartLayoutEngine::InitializeSmartLayout()
{
    wxLogDebug("SmartLayoutEngine: Initializing smart layout system");
    
    // Load user preferences
    LoadLayoutPreferences();
    
    // Analyze current layout if exists
    if (auto* root = GetRootNode()) {
        m_lastMetrics = AnalyzeLayout();
        m_lastProblems = DetectProblems();
        m_lastQualityScore = GetLayoutQualityScore();
        
        wxLogDebug("SmartLayoutEngine: Initial quality score: %.2f", m_lastQualityScore);
        
        // Auto-fix critical problems
        if (m_lastQualityScore < 0.5) {
            AutoFixProblems();
        }
    }
}

void SmartLayoutEngine::EnableSmartMode(bool enable)
{
    m_smartModeEnabled = enable;
    wxLogDebug("SmartLayoutEngine: Smart mode %s", enable ? "enabled" : "disabled");
}

void SmartLayoutEngine::AutoCreateLayout(const std::vector<ModernDockPanel*>& panels)
{
    if (panels.empty()) return;
    
    wxLogDebug("SmartLayoutEngine: Auto-creating layout for %zu panels", panels.size());
    
    // Group panels by their preferred areas
    std::map<DockArea, std::vector<ModernDockPanel*>> areaGroups;
    
    for (auto* panel : panels) {
        DockArea preferredArea = panel->GetDockArea();
        
        // Use smart heuristics if no area is set
        if (preferredArea == DockArea::Center) {
            wxString title = panel->GetTitle().Lower();
            
            // Smart area assignment based on panel title/type
            if (title.Contains("tree") || title.Contains("explorer") || 
                title.Contains("properties") || title.Contains("inspector")) {
                preferredArea = DockArea::Left;
            } else if (title.Contains("output") || title.Contains("console") ||
                       title.Contains("message") || title.Contains("log")) {
                preferredArea = DockArea::Bottom;
            } else if (title.Contains("tool") || title.Contains("palette")) {
                preferredArea = DockArea::Right;
            } else if (title.Contains("toolbar") || title.Contains("menu")) {
                preferredArea = DockArea::Top;
            }
        }
        
        areaGroups[preferredArea].push_back(panel);
    }
    
    // Create optimal initial layout
    CreateOptimalInitialLayout(panels);
    
    // Apply constraints
    ApplyConstraints();
    
    // Optimize the layout
    AutoOptimizeLayout();
}

SmartDockingDecision SmartLayoutEngine::AutoSelectDockPosition(ModernDockPanel* panel)
{
    if (!panel || !m_smartModeEnabled) {
        return SmartDockingDecision();
    }
    
    wxLogDebug("SmartLayoutEngine: Auto-selecting dock position for %s", panel->GetTitle());
    
    return m_dockOptimizer->FindBestDockPosition(panel);
}

OptimizationResult SmartLayoutEngine::AutoOptimizeLayout()
{
    OptimizationResult result;
    
    if (!m_constraints.autoOptimize) {
        result.summary = "Auto-optimization is disabled";
        return result;
    }
    
    wxLogDebug("SmartLayoutEngine: Starting auto-optimization");
    
    // Analyze current state
    LayoutMetrics beforeMetrics = AnalyzeLayout();
    double beforeScore = GetLayoutQualityScore();
    
    // Fix detected problems
    AutoFixProblems();
    result.problemsFixed = static_cast<int>(m_lastProblems.size());
    
    // Optimize splitters
    m_splitterOptimizer->OptimizeSplitters(GetRootNode());
    m_splitterOptimizer->BalanceSplitters(GetRootNode());
    
    // Merge redundant splitters
    auto* root = GetRootNode();
    int splittersBefore = CountSplitters(root);
    m_splitterOptimizer->MergeRedundantSplitters(root);
    int splittersAfter = CountSplitters(root);
    result.splittersMerged = splittersBefore - splittersAfter;
    
    // Remove empty areas
    int emptyBefore = CountEmptyAreas(root);
    RemoveEmptyAreas();
    int emptyAfter = CountEmptyAreas(root);
    result.emptyAreasRemoved = emptyBefore - emptyAfter;
    
    // Balance the layout
    if (m_constraints.autoBalance) {
        SmartBalanceLayout();
    }
    
    // Calculate improvement
    LayoutMetrics afterMetrics = AnalyzeLayout();
    double afterScore = GetLayoutQualityScore();
    result.improvementScore = afterScore - beforeScore;
    
    result.success = result.improvementScore >= 0;
    result.summary = wxString::Format(
        "Optimization complete: Fixed %d problems, merged %d splitters, "
        "removed %d empty areas. Quality improved by %.1f%%",
        result.problemsFixed, result.splittersMerged, result.emptyAreasRemoved,
        result.improvementScore * 100
    );
    
    wxLogDebug("SmartLayoutEngine: %s", result.summary);
    
    return result;
}

void SmartLayoutEngine::AutoCleanupLayout()
{
    if (!m_constraints.autoCleanup) return;
    
    wxLogDebug("SmartLayoutEngine: Auto-cleaning layout");
    
    // Remove empty areas
    RemoveEmptyAreas();
    
    // Merge redundant splitters
    m_splitterOptimizer->MergeRedundantSplitters(GetRootNode());
    
    // Compact deep nesting
    CompactDeepNesting();
    
    // Update layout
    if (m_manager) {
        m_manager->UpdateLayout();
    }
}

void SmartLayoutEngine::SmartDockPanel(ModernDockPanel* panel, ModernDockPanel* target)
{
    if (!panel) return;
    
    wxLogDebug("SmartLayoutEngine: Smart docking %s", panel->GetTitle());
    
    SmartDockingDecision decision;
    
    if (target) {
        // Use specified target but find best position
        auto* targetNode = FindPanelNode(target);
        if (targetNode) {
            decision.targetNode = targetNode;
            decision.position = CalculateOptimalDockPosition(panel, targetNode);
            decision.confidence = 0.9;
            decision.reasoning = "User-specified target with optimized position";
        }
    } else {
        // Fully automatic docking
        decision = AutoSelectDockPosition(panel);
    }
    
    if (decision.targetNode && decision.position != DockPosition::None) {
        // Perform the docking
        if (m_manager && m_manager->GetLayoutEngine()) {
            m_manager->GetLayoutEngine()->DockPanel(panel, 
                decision.targetNode->GetPanel(), decision.position);
        }
        
        // Learn from this action
        m_dockOptimizer->LearnFromDocking(panel, decision.targetNode, decision.position);
        
        // Auto-optimize after docking
        if (m_constraints.autoOptimize) {
            AutoOptimizeLayout();
        }
    }
}

void SmartLayoutEngine::SmartArrangePanels(const std::vector<ModernDockPanel*>& panels)
{
    if (panels.empty()) return;
    
    wxLogDebug("SmartLayoutEngine: Smart arranging %zu panels", panels.size());
    
    // Sort panels by priority and type
    std::vector<ModernDockPanel*> sortedPanels = panels;
    std::sort(sortedPanels.begin(), sortedPanels.end(), 
        [this](ModernDockPanel* a, ModernDockPanel* b) {
            // Priority based on usage frequency
            int usageA = m_panelUsageCount[a];
            int usageB = m_panelUsageCount[b];
            if (usageA != usageB) return usageA > usageB;
            
            // Then by area preference
            return static_cast<int>(a->GetDockArea()) < static_cast<int>(b->GetDockArea());
        });
    
    // Dock panels in optimal order
    for (auto* panel : sortedPanels) {
        SmartDockPanel(panel);
    }
    
    // Final optimization
    AutoOptimizeLayout();
}

void SmartLayoutEngine::SmartBalanceLayout()
{
    auto* root = GetRootNode();
    if (!root) return;
    
    wxLogDebug("SmartLayoutEngine: Smart balancing layout");
    
    // Balance all splitters
    m_splitterOptimizer->BalanceSplitters(root);
    
    // Ensure minimum sizes are respected
    ApplyConstraints();
    
    // Update display
    if (m_manager) {
        m_manager->UpdateLayout();
    }
}

void SmartLayoutEngine::SmartResizeLayout(const wxSize& newSize)
{
    if (!m_constraints.adaptiveLayout) return;
    
    wxLogDebug("SmartLayoutEngine: Smart resize to %dx%d", newSize.x, newSize.y);
    
    auto* root = GetRootNode();
    if (!root) return;
    
    // Adjust splitter ratios based on new size
    m_splitterOptimizer->AutoAdjustSplitterRatios(root, newSize);
    
    // Re-apply constraints for new size
    ApplyConstraints();
    
    // Update layout
    if (m_manager) {
        m_manager->UpdateLayout();
    }
}

void SmartLayoutEngine::SetConstraints(const SmartLayoutConstraints& constraints)
{
    m_constraints = constraints;
    m_constraintSolver->SetConstraints(constraints);
    
    // Apply new constraints immediately
    ApplyConstraints();
}

void SmartLayoutEngine::ApplyConstraints()
{
    auto* root = GetRootNode();
    if (!root) return;
    
    m_constraintSolver->SolveLayout(root);
    
    // Validate and fix any violations
    auto violations = m_constraintSolver->GetViolatedConstraints(root);
    if (!violations.empty()) {
        wxLogDebug("SmartLayoutEngine: Found %zu constraint violations", violations.size());
        
        // Try to fix violations
        for (const auto& violation : violations) {
            wxLogDebug("  - Violation: %s", violation);
        }
    }
}

LayoutMetrics SmartLayoutEngine::AnalyzeLayout()
{
    auto* root = GetRootNode();
    if (!root) return LayoutMetrics();
    
    return m_analyzer->AnalyzeLayout(root);
}

std::vector<LayoutProblem> SmartLayoutEngine::DetectProblems()
{
    auto* root = GetRootNode();
    if (!root) return std::vector<LayoutProblem>();
    
    return m_analyzer->DetectProblems(root);
}

double SmartLayoutEngine::GetLayoutQualityScore()
{
    m_lastMetrics = AnalyzeLayout();
    return m_analyzer->CalculateQualityScore(m_lastMetrics);
}

void SmartLayoutEngine::AutoFixProblems()
{
    m_lastProblems = DetectProblems();
    
    wxLogDebug("SmartLayoutEngine: Auto-fixing %zu problems", m_lastProblems.size());
    
    // Sort problems by severity
    std::sort(m_lastProblems.begin(), m_lastProblems.end(),
        [](const LayoutProblem& a, const LayoutProblem& b) {
            return a.severity > b.severity;
        });
    
    // Fix each problem
    for (const auto& problem : m_lastProblems) {
        FixSpecificProblem(problem);
    }
}

void SmartLayoutEngine::FixSpecificProblem(const LayoutProblem& problem)
{
    wxLogDebug("SmartLayoutEngine: Fixing %s (severity: %.2f)", 
               problem.description, problem.severity);
    
    if (problem.autoFix) {
        problem.autoFix();
    } else {
        // Manual fixes based on problem type
        switch (problem.type) {
            case LayoutProblem::UnbalancedSplitter:
                if (problem.affectedNode) {
                    m_splitterOptimizer->BalanceSplitters(problem.affectedNode);
                }
                break;
                
            case LayoutProblem::EmptySpace:
                RemoveEmptyAreas();
                break;
                
            case LayoutProblem::DeepNesting:
                CompactDeepNesting();
                break;
                
            case LayoutProblem::RedundantSplitter:
                if (problem.affectedNode) {
                    m_splitterOptimizer->MergeRedundantSplitters(problem.affectedNode);
                }
                break;
                
            default:
                wxLogDebug("SmartLayoutEngine: No automatic fix for problem type %d", 
                          problem.type);
                break;
        }
    }
}

void SmartLayoutEngine::LearnFromUserAction(const wxString& action, const wxString& context)
{
    // Record action
    m_actionHistory[action]++;
    
    // Store context-specific preferences
    wxString key = action + ":" + context;
    RecordUserPreference(key, wxDateTime::Now().FormatISOCombined());
    
    wxLogDebug("SmartLayoutEngine: Learned action %s in context %s", action, context);
}

void SmartLayoutEngine::AdaptToUsagePattern()
{
    if (!m_constraints.adaptiveLayout) return;
    
    wxLogDebug("SmartLayoutEngine: Adapting to usage patterns");
    
    // Analyze panel usage
    std::vector<std::pair<ModernDockPanel*, int>> usageList;
    for (const auto& [panel, count] : m_panelUsageCount) {
        usageList.push_back({panel, count});
    }
    
    // Sort by usage frequency
    std::sort(usageList.begin(), usageList.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Reorganize high-usage panels to more accessible positions
    for (const auto& [panel, count] : usageList) {
        if (count > 10) {  // High usage threshold
            // Consider moving to more prominent position
            auto decision = AutoSelectDockPosition(panel);
            if (decision.confidence > 0.8) {
                wxLogDebug("SmartLayoutEngine: Moving high-usage panel %s", 
                          panel->GetTitle());
                SmartDockPanel(panel);
            }
        }
    }
}

void SmartLayoutEngine::SaveLayoutPreferences()
{
    // Save to config or file
    wxLogDebug("SmartLayoutEngine: Saving layout preferences");
    
    // This would typically save to wxConfig or a preferences file
    // For now, just log the action
}

void SmartLayoutEngine::LoadLayoutPreferences()
{
    // Load from config or file
    wxLogDebug("SmartLayoutEngine: Loading layout preferences");
    
    // This would typically load from wxConfig or a preferences file
    // For now, just initialize with defaults
}

void SmartLayoutEngine::OnLayoutChanged()
{
    // Update metrics
    m_lastMetrics = AnalyzeLayout();
    m_lastQualityScore = GetLayoutQualityScore();
    
    // Auto-optimize if quality drops
    if (m_constraints.autoOptimize && m_lastQualityScore < 0.6) {
        wxLogDebug("SmartLayoutEngine: Quality dropped to %.2f, auto-optimizing", 
                   m_lastQualityScore);
        AutoOptimizeLayout();
    }
}

void SmartLayoutEngine::OnPanelDocked(ModernDockPanel* panel, DockPosition position)
{
    if (!panel) return;
    
    // Update usage statistics
    UpdateUsageStatistics(panel);
    
    // Learn from this docking action
    wxString context = wxString::Format("dock_%d", static_cast<int>(position));
    LearnFromUserAction("dock_panel", context);
    
    // Trigger layout change handler
    OnLayoutChanged();
}

void SmartLayoutEngine::OnPanelUndocked(ModernDockPanel* panel)
{
    if (!panel) return;
    
    // Update statistics
    LearnFromUserAction("undock_panel", panel->GetTitle());
    
    // Auto-cleanup if enabled
    if (m_constraints.autoCleanup) {
        AutoCleanupLayout();
    }
}

void SmartLayoutEngine::OnWindowResized(const wxSize& newSize)
{
    SmartResizeLayout(newSize);
}

// Private implementation methods

void SmartLayoutEngine::CreateOptimalInitialLayout(const std::vector<ModernDockPanel*>& panels)
{
    // This creates an optimal initial layout based on panel types and constraints
    // Implementation would create appropriate splitters and arrange panels
    
    wxLogDebug("SmartLayoutEngine: Creating optimal initial layout for %zu panels", 
               panels.size());
    
    // Group panels by area
    std::map<DockArea, std::vector<ModernDockPanel*>> areaGroups;
    for (auto* panel : panels) {
        areaGroups[panel->GetDockArea()].push_back(panel);
    }
    
    // Create layout structure
    // This is a simplified implementation - real one would be more sophisticated
    for (const auto& [area, areaPanels] : areaGroups) {
        for (auto* panel : areaPanels) {
            if (m_manager) {
                m_manager->AddPanel(panel->GetContent(), panel->GetTitle(), area);
            }
        }
    }
}

LayoutNode* SmartLayoutEngine::FindOptimalDockTarget(ModernDockPanel* panel)
{
    auto* root = GetRootNode();
    if (!root) return nullptr;
    
    LayoutNode* bestTarget = nullptr;
    double bestScore = 0.0;
    
    // Evaluate all possible targets
    std::function<void(LayoutNode*)> evaluateNode = [&](LayoutNode* node) {
        if (!node) return;
        
        // Score this node as a potential target
        double score = 0.0;
        
        // Prefer nodes with similar panels
        if (node->GetType() == LayoutNodeType::Panel && node->GetPanel()) {
            auto* targetPanel = node->GetPanel();
            
            // Similar titles get higher score
            if (panel->GetTitle().Lower().Find(targetPanel->GetTitle().Lower()) != wxNOT_FOUND ||
                targetPanel->GetTitle().Lower().Find(panel->GetTitle().Lower()) != wxNOT_FOUND) {
                score += 0.3;
            }
            
            // Same area preference
            if (panel->GetDockArea() == targetPanel->GetDockArea()) {
                score += 0.2;
            }
        }
        
        // Prefer less deeply nested nodes
        int depth = GetNodeDepth(node);
        score += (1.0 - (depth / 10.0)) * 0.2;
        
        // Prefer balanced areas
        score += CalculateNodeBalance(node) * 0.3;
        
        if (score > bestScore) {
            bestScore = score;
            bestTarget = node;
        }
        
        // Recurse to children
        for (const auto& child : node->GetChildren()) {
            evaluateNode(child.get());
        }
    };
    
    evaluateNode(root);
    
    return bestTarget;
}

DockPosition SmartLayoutEngine::CalculateOptimalDockPosition(ModernDockPanel* panel, 
                                                            LayoutNode* target)
{
    if (!panel || !target) return DockPosition::None;
    
    // Evaluate each position
    struct PositionScore {
        DockPosition position;
        double score;
    };
    
    std::vector<PositionScore> scores = {
        {DockPosition::Left, 0.0},
        {DockPosition::Right, 0.0},
        {DockPosition::Top, 0.0},
        {DockPosition::Bottom, 0.0},
        {DockPosition::Center, 0.0}
    };
    
    // Score each position
    for (auto& ps : scores) {
        ps.score = m_dockOptimizer->ScoreDockingOption(panel, target, ps.position);
    }
    
    // Find best position
    auto best = std::max_element(scores.begin(), scores.end(),
        [](const PositionScore& a, const PositionScore& b) {
            return a.score < b.score;
        });
    
    return best->position;
}

void SmartLayoutEngine::OptimizeSplitterRatios()
{
    auto* root = GetRootNode();
    if (!root) return;
    
    m_splitterOptimizer->OptimizeSplitters(root);
}

void SmartLayoutEngine::MergeRedundantSplitters()
{
    auto* root = GetRootNode();
    if (!root) return;
    
    m_splitterOptimizer->MergeRedundantSplitters(root);
}

void SmartLayoutEngine::RemoveEmptyAreas()
{
    auto* root = GetRootNode();
    if (!root || !m_manager || !m_manager->GetLayoutEngine()) return;
    
    // Find and remove empty nodes
    std::vector<LayoutNode*> emptyNodes;
    
    std::function<void(LayoutNode*)> findEmpty = [&](LayoutNode* node) {
        if (!node) return;
        
        if (IsNodeEmpty(node) && node != root) {
            emptyNodes.push_back(node);
        }
        
        for (const auto& child : node->GetChildren()) {
            findEmpty(child.get());
        }
    };
    
    findEmpty(root);
    
    // Remove empty nodes
    for (auto* node : emptyNodes) {
        if (node->GetParent()) {
            node->GetParent()->RemoveChild(node);
        }
    }
}

void SmartLayoutEngine::BalanceLayoutTree()
{
    auto* root = GetRootNode();
    if (!root) return;
    
    m_splitterOptimizer->BalanceSplitters(root);
}

void SmartLayoutEngine::CompactDeepNesting()
{
    auto* root = GetRootNode();
    if (!root) return;
    
    // Find deeply nested nodes
    std::vector<LayoutNode*> deepNodes;
    
    std::function<void(LayoutNode*, int)> findDeep = [&](LayoutNode* node, int depth) {
        if (!node) return;
        
        if (depth > m_constraints.maxNestingDepth) {
            deepNodes.push_back(node);
        }
        
        for (const auto& child : node->GetChildren()) {
            findDeep(child.get(), depth + 1);
        }
    };
    
    findDeep(root, 0);
    
    // Flatten deep nesting
    // This is a complex operation that would need careful implementation
    // to preserve the layout structure while reducing depth
}

double SmartLayoutEngine::CalculateNodeBalance(LayoutNode* node)
{
    if (!node) return 0.0;
    
    if (node->GetType() == LayoutNodeType::HorizontalSplitter ||
        node->GetType() == LayoutNodeType::VerticalSplitter) {
        double ratio = node->GetSplitterRatio();
        // Perfect balance is 0.5, score decreases as we move away
        return 1.0 - std::abs(ratio - 0.5) * 2.0;
    }
    
    // For non-splitter nodes, calculate based on children
    double totalBalance = 0.0;
    int count = 0;
    
    for (const auto& child : node->GetChildren()) {
        totalBalance += CalculateNodeBalance(child.get());
        count++;
    }
    
    return count > 0 ? totalBalance / count : 1.0;
}

double SmartLayoutEngine::CalculateSpaceUtilization(LayoutNode* node)
{
    if (!node) return 0.0;
    
    // Calculate how well the space is utilized
    wxRect rect = node->GetRect();
    if (rect.IsEmpty()) return 0.0;
    
    double totalArea = rect.width * rect.height;
    double usedArea = 0.0;
    
    if (node->GetType() == LayoutNodeType::Panel && node->GetPanel()) {
        // Panel uses all its space
        usedArea = totalArea;
    } else {
        // Container - sum up children's utilization
        for (const auto& child : node->GetChildren()) {
            wxRect childRect = child->GetRect();
            usedArea += childRect.width * childRect.height;
        }
    }
    
    return totalArea > 0 ? usedArea / totalArea : 0.0;
}

void SmartLayoutEngine::RecordUserPreference(const wxString& key, const wxString& value)
{
    m_userPreferences[key] = value;
}

wxString SmartLayoutEngine::GetUserPreference(const wxString& key) const
{
    auto it = m_userPreferences.find(key);
    return it != m_userPreferences.end() ? it->second : wxString();
}

void SmartLayoutEngine::UpdateUsageStatistics(ModernDockPanel* panel)
{
    if (panel) {
        m_panelUsageCount[panel]++;
    }
}

LayoutNode* SmartLayoutEngine::GetRootNode()
{
    if (m_manager && m_manager->GetLayoutEngine()) {
        return m_manager->GetLayoutEngine()->GetRootNode();
    }
    return nullptr;
}

std::vector<LayoutNode*> SmartLayoutEngine::GetAllNodes()
{
    std::vector<LayoutNode*> nodes;
    auto* root = GetRootNode();
    if (!root) return nodes;
    
    std::function<void(LayoutNode*)> collect = [&](LayoutNode* node) {
        if (!node) return;
        nodes.push_back(node);
        for (const auto& child : node->GetChildren()) {
            collect(child.get());
        }
    };
    
    collect(root);
    return nodes;
}

std::vector<ModernDockPanel*> SmartLayoutEngine::GetAllPanels()
{
    std::vector<ModernDockPanel*> panels;
    auto* root = GetRootNode();
    if (!root) return panels;
    
    std::function<void(LayoutNode*)> collect = [&](LayoutNode* node) {
        if (!node) return;
        if (node->GetType() == LayoutNodeType::Panel && node->GetPanel()) {
            panels.push_back(node->GetPanel());
        }
        for (const auto& child : node->GetChildren()) {
            collect(child.get());
        }
    };
    
    collect(root);
    return panels;
}

bool SmartLayoutEngine::IsNodeEmpty(LayoutNode* node)
{
    if (!node) return true;
    
    if (node->GetType() == LayoutNodeType::Panel) {
        return node->GetPanel() == nullptr;
    }
    
    // Container node - check if it has any non-empty children
    for (const auto& child : node->GetChildren()) {
        if (!IsNodeEmpty(child.get())) {
            return false;
        }
    }
    
    return true;
}

bool SmartLayoutEngine::IsRedundantSplitter(LayoutNode* node)
{
    if (!node) return false;
    
    if (node->GetType() != LayoutNodeType::HorizontalSplitter &&
        node->GetType() != LayoutNodeType::VerticalSplitter) {
        return false;
    }
    
    // Splitter is redundant if it has less than 2 children
    return node->GetChildren().size() < 2;
}

int SmartLayoutEngine::GetNodeDepth(LayoutNode* node)
{
    if (!node) return 0;
    
    int depth = 0;
    LayoutNode* current = node->GetParent();
    while (current) {
        depth++;
        current = current->GetParent();
    }
    
    return depth;
}

LayoutNode* SmartLayoutEngine::FindPanelNode(ModernDockPanel* panel)
{
    auto nodes = GetAllNodes();
    for (auto* node : nodes) {
        if (node->GetType() == LayoutNodeType::Panel && 
            node->GetPanel() == panel) {
            return node;
        }
    }
    return nullptr;
}

int SmartLayoutEngine::CountSplitters(LayoutNode* node)
{
    if (!node) return 0;
    
    int count = 0;
    if (node->GetType() == LayoutNodeType::HorizontalSplitter ||
        node->GetType() == LayoutNodeType::VerticalSplitter) {
        count = 1;
    }
    
    for (const auto& child : node->GetChildren()) {
        count += CountSplitters(child.get());
    }
    
    return count;
}

int SmartLayoutEngine::CountEmptyAreas(LayoutNode* node)
{
    if (!node) return 0;
    
    int count = IsNodeEmpty(node) ? 1 : 0;
    
    for (const auto& child : node->GetChildren()) {
        count += CountEmptyAreas(child.get());
    }
    
    return count;
}

// AutoDockOptimizer implementation

AutoDockOptimizer::AutoDockOptimizer(SmartLayoutEngine* engine)
    : m_engine(engine)
{
}

SmartDockingDecision AutoDockOptimizer::FindBestDockPosition(ModernDockPanel* panel)
{
    SmartDockingDecision decision;
    
    if (!panel || !m_engine) return decision;
    
    // Get all possible docking options
    auto options = GetAllDockingOptions(panel);
    
    if (options.empty()) {
        decision.reasoning = "No valid docking positions found";
        return decision;
    }
    
    // Score each option
    double bestScore = 0.0;
    for (const auto& [node, position] : options) {
        double score = ScoreDockingOption(panel, node, position);
        
        if (score > bestScore) {
            bestScore = score;
            decision.targetNode = node;
            decision.position = position;
            decision.confidence = score;
        }
        
        // Keep as alternative if score is good enough
        if (score > 0.6) {
            decision.alternatives.push_back({node, position});
        }
    }
    
    // Generate reasoning
    if (decision.targetNode) {
        decision.reasoning = wxString::Format(
            "Selected position with %.0f%% confidence based on panel type, "
            "usage patterns, and layout balance",
            decision.confidence * 100
        );
    }
    
    return decision;
}

std::vector<std::pair<LayoutNode*, DockPosition>> AutoDockOptimizer::GetAllDockingOptions(
    ModernDockPanel* panel)
{
    std::vector<std::pair<LayoutNode*, DockPosition>> options;
    
    auto nodes = m_engine->GetAllNodes();
    
    for (auto* node : nodes) {
        // Can dock to panels and some containers
        if (node->GetType() == LayoutNodeType::Panel ||
            (node->GetType() == LayoutNodeType::Root && node->GetChildren().empty())) {
            
            // Add all possible positions
            options.push_back({node, DockPosition::Left});
            options.push_back({node, DockPosition::Right});
            options.push_back({node, DockPosition::Top});
            options.push_back({node, DockPosition::Bottom});
            options.push_back({node, DockPosition::Center});
        }
    }
    
    return options;
}

double AutoDockOptimizer::ScoreDockingOption(ModernDockPanel* panel, 
                                            LayoutNode* target, 
                                            DockPosition position)
{
    if (!panel || !target) return 0.0;
    
    double score = 0.5;  // Base score
    
    // Score based on panel type and position
    wxString panelTitle = panel->GetTitle().Lower();
    
    // Position preferences based on panel type
    if (panelTitle.Contains("tree") || panelTitle.Contains("explorer")) {
        if (position == DockPosition::Left) score += 0.3;
        else if (position == DockPosition::Right) score += 0.1;
    } else if (panelTitle.Contains("output") || panelTitle.Contains("console")) {
        if (position == DockPosition::Bottom) score += 0.3;
    } else if (panelTitle.Contains("properties") || panelTitle.Contains("inspector")) {
        if (position == DockPosition::Right) score += 0.2;
        else if (position == DockPosition::Left) score += 0.1;
    }
    
    // Prefer docking to similar panels
    if (target->GetType() == LayoutNodeType::Panel && target->GetPanel()) {
        wxString targetTitle = target->GetPanel()->GetTitle().Lower();
        if (panelTitle.Find(targetTitle) != wxNOT_FOUND ||
            targetTitle.Find(panelTitle) != wxNOT_FOUND) {
            score += 0.2;
        }
    }
    
    // Consider depth - prefer less deeply nested positions
    int depth = m_engine->GetNodeDepth(target);
    score -= depth * 0.05;
    
    // Check history
    auto historyKey = panel->GetTitle();
    auto it = m_dockingHistory.find(historyKey);
    if (it != m_dockingHistory.end()) {
        // Boost score if this position was used before
        if (std::find(it->second.begin(), it->second.end(), position) != it->second.end()) {
            score += 0.15;
        }
    }
    
    // Clamp score
    return std::max(0.0, std::min(1.0, score));
}

void AutoDockOptimizer::SetPreferredAreas(const std::map<wxString, DockArea>& preferences)
{
    m_preferredAreas = preferences;
}

void AutoDockOptimizer::LearnFromDocking(ModernDockPanel* panel, 
                                        LayoutNode* target, 
                                        DockPosition position)
{
    if (!panel) return;
    
    // Record this docking action
    wxString key = panel->GetTitle();
    m_dockingHistory[key].push_back(position);
    
    // Keep history size reasonable
    if (m_dockingHistory[key].size() > 10) {
        m_dockingHistory[key].erase(m_dockingHistory[key].begin());
    }
}

// LayoutAnalyzer implementation

LayoutAnalyzer::LayoutAnalyzer(SmartLayoutEngine* engine)
    : m_engine(engine)
{
}

LayoutMetrics LayoutAnalyzer::AnalyzeLayout(LayoutNode* root)
{
    LayoutMetrics metrics;
    
    if (!root) return metrics;
    
    AnalyzeNodeRecursive(root, metrics, 0);
    
    // Calculate derived metrics
    int totalNodes = metrics.splitterCount + metrics.emptyAreaCount + 1;
    metrics.complexityScore = static_cast<double>(metrics.splitterCount) / 
                             std::max(1, totalNodes);
    
    return metrics;
}

std::vector<LayoutProblem> LayoutAnalyzer::DetectProblems(LayoutNode* root)
{
    std::vector<LayoutProblem> problems;
    
    if (!root) return problems;
    
    DetectNodeProblemsRecursive(root, problems, 0);
    
    return problems;
}

double LayoutAnalyzer::CalculateQualityScore(const LayoutMetrics& metrics)
{
    // Weighted quality score calculation
    double score = 0.0;
    
    // Space utilization (30%)
    score += metrics.spaceUtilization * 0.3;
    
    // Balance (25%)
    score += metrics.balanceScore * 0.25;
    
    // Accessibility (20%)
    score += metrics.accessibilityScore * 0.2;
    
    // Complexity penalty (25%)
    score += (1.0 - metrics.complexityScore) * 0.25;
    
    // Additional penalties
    score -= metrics.emptyAreaCount * 0.05;
    score -= metrics.deepNestingCount * 0.03;
    
    return std::max(0.0, std::min(1.0, score));
}

bool LayoutAnalyzer::IsLayoutBalanced(LayoutNode* root)
{
    if (!root) return false;
    
    double balance = m_engine->CalculateNodeBalance(root);
    return balance > 0.7;
}

bool LayoutAnalyzer::HasRedundantSplitters(LayoutNode* root)
{
    auto problematicNodes = FindProblematicNodes(root);
    
    for (auto* node : problematicNodes) {
        if (m_engine->IsRedundantSplitter(node)) {
            return true;
        }
    }
    
    return false;
}

bool LayoutAnalyzer::HasEmptyAreas(LayoutNode* root)
{
    return m_engine->CountEmptyAreas(root) > 0;
}

bool LayoutAnalyzer::HasDeepNesting(LayoutNode* root, int maxDepth)
{
    std::function<bool(LayoutNode*, int)> checkDepth = [&](LayoutNode* node, int depth) {
        if (!node) return false;
        
        if (depth > maxDepth) return true;
        
        for (const auto& child : node->GetChildren()) {
            if (checkDepth(child.get(), depth + 1)) {
                return true;
            }
        }
        
        return false;
    };
    
    return checkDepth(root, 0);
}

std::vector<LayoutNode*> LayoutAnalyzer::FindProblematicNodes(LayoutNode* root)
{
    std::vector<LayoutNode*> problematic;
    
    if (!root) return problematic;
    
    std::function<void(LayoutNode*)> findProblems = [&](LayoutNode* node) {
        if (!node) return;
        
        // Check for various issues
        if (m_engine->IsRedundantSplitter(node) ||
            m_engine->IsNodeEmpty(node) ||
            (node->GetType() == LayoutNodeType::HorizontalSplitter && 
             std::abs(node->GetSplitterRatio() - 0.5) > 0.35)) {
            problematic.push_back(node);
        }
        
        for (const auto& child : node->GetChildren()) {
            findProblems(child.get());
        }
    };
    
    findProblems(root);
    
    return problematic;
}

void LayoutAnalyzer::AnalyzeNodeRecursive(LayoutNode* node, LayoutMetrics& metrics, int depth)
{
    if (!node) return;
    
    // Count node types
    if (node->GetType() == LayoutNodeType::HorizontalSplitter ||
        node->GetType() == LayoutNodeType::VerticalSplitter) {
        metrics.splitterCount++;
        
        // Calculate balance
        metrics.balanceScore += m_engine->CalculateNodeBalance(node);
    }
    
    // Check for empty areas
    if (m_engine->IsNodeEmpty(node)) {
        metrics.emptyAreaCount++;
    }
    
    // Check nesting depth
    if (depth > 5) {
        metrics.deepNestingCount++;
    }
    
    // Calculate space utilization
    metrics.spaceUtilization += m_engine->CalculateSpaceUtilization(node);
    
    // Recurse to children
    for (const auto& child : node->GetChildren()) {
        AnalyzeNodeRecursive(child.get(), metrics, depth + 1);
    }
    
    // Normalize scores at root level
    if (depth == 0 && metrics.splitterCount > 0) {
        metrics.balanceScore /= metrics.splitterCount;
        
        int nodeCount = m_engine->GetAllNodes().size();
        if (nodeCount > 0) {
            metrics.spaceUtilization /= nodeCount;
        }
        
        // Simple accessibility score based on depth
        metrics.accessibilityScore = 1.0 - (metrics.deepNestingCount * 0.1);
        metrics.accessibilityScore = std::max(0.0, metrics.accessibilityScore);
    }
}

void LayoutAnalyzer::DetectNodeProblemsRecursive(LayoutNode* node, 
                                                std::vector<LayoutProblem>& problems, 
                                                int depth)
{
    if (!node) return;
    
    // Check for unbalanced splitters
    if (node->GetType() == LayoutNodeType::HorizontalSplitter ||
        node->GetType() == LayoutNodeType::VerticalSplitter) {
        double ratio = node->GetSplitterRatio();
        if (ratio < 0.2 || ratio > 0.8) {
            LayoutProblem problem;
            problem.type = LayoutProblem::UnbalancedSplitter;
            problem.affectedNode = node;
            problem.description = wxString::Format("Unbalanced splitter (ratio: %.2f)", ratio);
            problem.severity = std::abs(ratio - 0.5) * 2.0;
            problems.push_back(problem);
        }
        
        // Check for redundant splitters
        if (node->GetChildren().size() < 2) {
            LayoutProblem problem;
            problem.type = LayoutProblem::RedundantSplitter;
            problem.affectedNode = node;
            problem.description = "Redundant splitter with less than 2 children";
            problem.severity = 0.7;
            problems.push_back(problem);
        }
    }
    
    // Check for empty areas
    if (m_engine->IsNodeEmpty(node)) {
        LayoutProblem problem;
        problem.type = LayoutProblem::EmptySpace;
        problem.affectedNode = node;
        problem.description = "Empty area in layout";
        problem.severity = 0.5;
        problems.push_back(problem);
    }
    
    // Check for deep nesting
    if (depth > 5) {
        LayoutProblem problem;
        problem.type = LayoutProblem::DeepNesting;
        problem.affectedNode = node;
        problem.description = wxString::Format("Deeply nested node (depth: %d)", depth);
        problem.severity = 0.3 + (depth - 5) * 0.1;
        problems.push_back(problem);
    }
    
    // Recurse to children
    for (const auto& child : node->GetChildren()) {
        DetectNodeProblemsRecursive(child.get(), problems, depth + 1);
    }
}

// ConstraintLayoutSolver implementation

ConstraintLayoutSolver::ConstraintLayoutSolver(SmartLayoutEngine* engine)
    : m_engine(engine)
{
}

void ConstraintLayoutSolver::SetConstraints(const SmartLayoutConstraints& constraints)
{
    m_constraints = constraints;
}

void ConstraintLayoutSolver::SolveLayout(LayoutNode* root)
{
    if (!root) return;
    
    ApplyConstraintsRecursive(root);
}

bool ConstraintLayoutSolver::ValidateLayout(LayoutNode* root)
{
    if (!root) return false;
    
    return CheckConstraints(root);
}

void ConstraintLayoutSolver::AddCustomConstraint(const wxString& name, 
                                                std::function<bool(LayoutNode*)> constraint)
{
    m_customConstraints[name] = constraint;
}

void ConstraintLayoutSolver::RemoveCustomConstraint(const wxString& name)
{
    m_customConstraints.erase(name);
}

std::vector<wxString> ConstraintLayoutSolver::GetViolatedConstraints(LayoutNode* root)
{
    std::vector<wxString> violations;
    
    if (!root) return violations;
    
    // Check built-in constraints
    if (!CheckConstraints(root)) {
        violations.push_back("Built-in constraints violated");
    }
    
    // Check custom constraints
    for (const auto& [name, constraint] : m_customConstraints) {
        if (!constraint(root)) {
            violations.push_back(name);
        }
    }
    
    return violations;
}

void ConstraintLayoutSolver::ApplyConstraintsRecursive(LayoutNode* node)
{
    if (!node) return;
    
    // Apply size constraints
    wxRect rect = node->GetRect();
    bool modified = false;
    
    if (rect.width < m_constraints.minPanelSize.x) {
        rect.width = m_constraints.minPanelSize.x;
        modified = true;
    }
    
    if (rect.height < m_constraints.minPanelSize.y) {
        rect.height = m_constraints.minPanelSize.y;
        modified = true;
    }
    
    if (m_constraints.maxPanelSize.x > 0 && rect.width > m_constraints.maxPanelSize.x) {
        rect.width = m_constraints.maxPanelSize.x;
        modified = true;
    }
    
    if (m_constraints.maxPanelSize.y > 0 && rect.height > m_constraints.maxPanelSize.y) {
        rect.height = m_constraints.maxPanelSize.y;
        modified = true;
    }
    
    if (modified) {
        node->SetRect(rect);
    }
    
    // Apply splitter constraints
    if (node->GetType() == LayoutNodeType::HorizontalSplitter ||
        node->GetType() == LayoutNodeType::VerticalSplitter) {
        double ratio = node->GetSplitterRatio();
        
        if (ratio < m_constraints.minSplitterRatio) {
            node->SetSplitterRatio(m_constraints.minSplitterRatio);
        } else if (ratio > m_constraints.maxSplitterRatio) {
            node->SetSplitterRatio(m_constraints.maxSplitterRatio);
        }
    }
    
    // Recurse to children
    for (const auto& child : node->GetChildren()) {
        ApplyConstraintsRecursive(child.get());
    }
}

bool ConstraintLayoutSolver::CheckConstraints(LayoutNode* node)
{
    if (!node) return false;
    
    // Check size constraints
    wxRect rect = node->GetRect();
    
    if (rect.width < m_constraints.minPanelSize.x ||
        rect.height < m_constraints.minPanelSize.y) {
        return false;
    }
    
    if (m_constraints.maxPanelSize.x > 0 && rect.width > m_constraints.maxPanelSize.x) {
        return false;
    }
    
    if (m_constraints.maxPanelSize.y > 0 && rect.height > m_constraints.maxPanelSize.y) {
        return false;
    }
    
    // Check splitter constraints
    if (node->GetType() == LayoutNodeType::HorizontalSplitter ||
        node->GetType() == LayoutNodeType::VerticalSplitter) {
        double ratio = node->GetSplitterRatio();
        
        if (ratio < m_constraints.minSplitterRatio ||
            ratio > m_constraints.maxSplitterRatio) {
            return false;
        }
    }
    
    // Check nesting depth
    int depth = m_engine->GetNodeDepth(node);
    if (depth > m_constraints.maxNestingDepth) {
        return false;
    }
    
    // Recurse to children
    for (const auto& child : node->GetChildren()) {
        if (!CheckConstraints(child.get())) {
            return false;
        }
    }
    
    return true;
}

// SplitterOptimizer implementation

SplitterOptimizer::SplitterOptimizer(SmartLayoutEngine* engine)
    : m_engine(engine)
{
}

void SplitterOptimizer::OptimizeSplitters(LayoutNode* root)
{
    if (!root) return;
    
    OptimizeSplitterRecursive(root);
}

void SplitterOptimizer::BalanceSplitters(LayoutNode* root)
{
    if (!root) return;
    
    std::function<void(LayoutNode*)> balance = [&](LayoutNode* node) {
        if (!node) return;
        
        if (node->GetType() == LayoutNodeType::HorizontalSplitter ||
            node->GetType() == LayoutNodeType::VerticalSplitter) {
            double optimalRatio = CalculateOptimalRatio(node);
            node->SetSplitterRatio(optimalRatio);
        }
        
        for (const auto& child : node->GetChildren()) {
            balance(child.get());
        }
    };
    
    balance(root);
}

void SplitterOptimizer::MergeRedundantSplitters(LayoutNode* root)
{
    if (!root) return;
    
    auto mergeableSplitters = FindMergeableSplitters(root);
    
    for (auto* splitter : mergeableSplitters) {
        if (splitter->GetParent()) {
            // Merge logic would go here
            // This is complex and needs careful implementation
            // to preserve the layout structure
        }
    }
}

double SplitterOptimizer::CalculateOptimalRatio(LayoutNode* splitter)
{
    if (!splitter || splitter->GetChildren().size() != 2) {
        return 0.5;
    }
    
    // Calculate based on content
    auto& children = splitter->GetChildren();
    
    // Count panels in each side
    int leftPanels = 0, rightPanels = 0;
    
    std::function<int(LayoutNode*)> countPanels = [&](LayoutNode* node) -> int {
        if (!node) return 0;
        int count = 0;
        if (node->GetType() == LayoutNodeType::Panel) count = 1;
        for (const auto& child : node->GetChildren()) {
            count += countPanels(child.get());
        }
        return count;
    };
    
    leftPanels = countPanels(children[0].get());
    rightPanels = countPanels(children[1].get());
    
    if (leftPanels + rightPanels == 0) return 0.5;
    
    // Calculate ratio based on panel count
    double ratio = static_cast<double>(leftPanels) / (leftPanels + rightPanels);
    
    // Apply constraints
    auto constraints = m_engine->GetConstraints();
    ratio = std::max(constraints.minSplitterRatio, 
                     std::min(constraints.maxSplitterRatio, ratio));
    
    return ratio;
}

bool SplitterOptimizer::ShouldMergeSplitter(LayoutNode* splitter)
{
    if (!splitter) return false;
    
    // Check if splitter is redundant
    if (splitter->GetChildren().size() < 2) {
        return true;
    }
    
    // Check if both children are empty
    bool allEmpty = true;
    for (const auto& child : splitter->GetChildren()) {
        if (!m_engine->IsNodeEmpty(child.get())) {
            allEmpty = false;
            break;
        }
    }
    
    return allEmpty;
}

void SplitterOptimizer::AutoAdjustSplitterRatios(LayoutNode* root, const wxSize& availableSize)
{
    if (!root) return;
    
    // Adjust ratios based on available size
    std::function<void(LayoutNode*)> adjust = [&](LayoutNode* node) {
        if (!node) return;
        
        if (node->GetType() == LayoutNodeType::HorizontalSplitter) {
            // For horizontal splitters, consider width
            if (availableSize.x < 800) {
                // Small width - give more space to primary content
                node->SetSplitterRatio(0.3);
            } else if (availableSize.x > 1600) {
                // Large width - balance more evenly
                node->SetSplitterRatio(0.4);
            }
        } else if (node->GetType() == LayoutNodeType::VerticalSplitter) {
            // For vertical splitters, consider height
            if (availableSize.y < 600) {
                // Small height - give more space to main content
                node->SetSplitterRatio(0.7);
            }
        }
        
        for (const auto& child : node->GetChildren()) {
            adjust(child.get());
        }
    };
    
    adjust(root);
}

void SplitterOptimizer::OptimizeSplitterRecursive(LayoutNode* node)
{
    if (!node) return;
    
    if (node->GetType() == LayoutNodeType::HorizontalSplitter ||
        node->GetType() == LayoutNodeType::VerticalSplitter) {
        
        // Check if this splitter should be optimized
        double currentRatio = node->GetSplitterRatio();
        double optimalRatio = CalculateOptimalRatio(node);
        
        // Only adjust if significantly different
        if (std::abs(currentRatio - optimalRatio) > 0.1) {
            node->SetSplitterRatio(optimalRatio);
        }
    }
    
    for (const auto& child : node->GetChildren()) {
        OptimizeSplitterRecursive(child.get());
    }
}

double SplitterOptimizer::ScoreSplitterBalance(LayoutNode* splitter)
{
    if (!splitter) return 0.0;
    
    double ratio = splitter->GetSplitterRatio();
    
    // Perfect balance at 0.5, score decreases as we move away
    return 1.0 - std::abs(ratio - 0.5) * 2.0;
}

std::vector<LayoutNode*> SplitterOptimizer::FindMergeableSplitters(LayoutNode* root)
{
    std::vector<LayoutNode*> mergeable;
    
    if (!root) return mergeable;
    
    std::function<void(LayoutNode*)> find = [&](LayoutNode* node) {
        if (!node) return;
        
        if ((node->GetType() == LayoutNodeType::HorizontalSplitter ||
             node->GetType() == LayoutNodeType::VerticalSplitter) &&
            ShouldMergeSplitter(node)) {
            mergeable.push_back(node);
        }
        
        for (const auto& child : node->GetChildren()) {
            find(child.get());
        }
    };
    
    find(root);
    
    return mergeable;
}