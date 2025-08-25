#pragma once

#include <wx/wx.h>
#include <memory>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include <algorithm>
#include "DockTypes.h"

// Forward declarations
class LayoutNode;
class ModernDockPanel;
class ModernDockManager;
class AutoDockOptimizer;
class LayoutAnalyzer;
class ConstraintLayoutSolver;
class SplitterOptimizer;

// Layout intelligence metrics
struct LayoutMetrics {
    double spaceUtilization = 0.0;     // 0.0 to 1.0
    double balanceScore = 0.0;         // 0.0 to 1.0
    double accessibilityScore = 0.0;   // 0.0 to 1.0
    double complexityScore = 0.0;      // 0.0 to 1.0 (lower is better)
    int splitterCount = 0;
    int emptyAreaCount = 0;
    int deepNestingCount = 0;          // Nodes deeper than optimal
};

// Layout constraints
struct SmartLayoutConstraints {
    // Size constraints
    wxSize minPanelSize = wxSize(150, 100);
    wxSize preferredPanelSize = wxSize(300, 200);
    wxSize maxPanelSize = wxSize(-1, -1);  // -1 means no limit
    
    // Layout preferences
    double idealSplitterRatio = 0.5;
    double minSplitterRatio = 0.15;
    double maxSplitterRatio = 0.85;
    
    // Optimization parameters
    int maxNestingDepth = 5;
    double minSpaceUtilization = 0.7;
    double targetBalanceScore = 0.8;
    
    // Behavior flags
    bool autoOptimize = true;
    bool autoBalance = true;
    bool autoCleanup = true;
    bool smartDocking = true;
    bool adaptiveLayout = true;
};

// Layout problem descriptor
struct LayoutProblem {
    enum Type {
        UnbalancedSplitter,
        EmptySpace,
        DeepNesting,
        PoorSpaceUtilization,
        InconsistentSizing,
        RedundantSplitter,
        InaccessiblePanel
    };
    
    Type type;
    LayoutNode* affectedNode;
    wxString description;
    double severity;  // 0.0 to 1.0
    
    // Suggested fix
    std::function<void()> autoFix;
};

// Smart docking decision
struct SmartDockingDecision {
    LayoutNode* targetNode = nullptr;
    DockPosition position = DockPosition::None;
    double confidence = 0.0;  // 0.0 to 1.0
    wxString reasoning;
    
    // Alternative options
    std::vector<std::pair<LayoutNode*, DockPosition>> alternatives;
};

// Layout optimization result
struct OptimizationResult {
    bool success = false;
    int problemsFixed = 0;
    int splittersMerged = 0;
    int emptyAreasRemoved = 0;
    double improvementScore = 0.0;
    wxString summary;
};

// Smart Layout Engine - Main intelligent layout system
class SmartLayoutEngine : public wxEvtHandler {
public:
    SmartLayoutEngine(ModernDockManager* manager);
    virtual ~SmartLayoutEngine();
    
    // Core functionality
    void InitializeSmartLayout();
    void EnableSmartMode(bool enable = true);
    bool IsSmartModeEnabled() const { return m_smartModeEnabled; }
    
    // Automatic layout management
    void AutoCreateLayout(const std::vector<ModernDockPanel*>& panels);
    SmartDockingDecision AutoSelectDockPosition(ModernDockPanel* panel);
    OptimizationResult AutoOptimizeLayout();
    void AutoCleanupLayout();
    
    // Intelligent operations
    void SmartDockPanel(ModernDockPanel* panel, ModernDockPanel* target = nullptr);
    void SmartArrangePanels(const std::vector<ModernDockPanel*>& panels);
    void SmartBalanceLayout();
    void SmartResizeLayout(const wxSize& newSize);
    
    // Constraint management
    void SetConstraints(const SmartLayoutConstraints& constraints);
    SmartLayoutConstraints GetConstraints() const { return m_constraints; }
    void ApplyConstraints();
    
    // Analysis and metrics
    LayoutMetrics AnalyzeLayout();
    std::vector<LayoutProblem> DetectProblems();
    double GetLayoutQualityScore();
    
    // Problem fixing
    void AutoFixProblems();
    void FixSpecificProblem(const LayoutProblem& problem);
    
    // Learning and adaptation
    void LearnFromUserAction(const wxString& action, const wxString& context);
    void AdaptToUsagePattern();
    void SaveLayoutPreferences();
    void LoadLayoutPreferences();
    
    // Events
    void OnLayoutChanged();
    void OnPanelDocked(ModernDockPanel* panel, DockPosition position);
    void OnPanelUndocked(ModernDockPanel* panel);
    void OnWindowResized(const wxSize& newSize);
    
private:
    // Smart layout algorithms
    void CreateOptimalInitialLayout(const std::vector<ModernDockPanel*>& panels);
    LayoutNode* FindOptimalDockTarget(ModernDockPanel* panel);
    DockPosition CalculateOptimalDockPosition(ModernDockPanel* panel, LayoutNode* target);
    
    // Optimization algorithms
    void OptimizeSplitterRatios();
    void MergeRedundantSplitters();
    void RemoveEmptyAreas();
    void BalanceLayoutTree();
    void CompactDeepNesting();
    
    // Analysis helpers
    void AnalyzeNode(LayoutNode* node, LayoutMetrics& metrics, int depth = 0);
    void DetectNodeProblems(LayoutNode* node, std::vector<LayoutProblem>& problems, int depth = 0);
    double CalculateNodeBalance(LayoutNode* node);
    double CalculateSpaceUtilization(LayoutNode* node);
    
    // Layout scoring
    double ScoreDockingOption(ModernDockPanel* panel, LayoutNode* target, DockPosition position);
    double ScoreLayoutQuality(const LayoutMetrics& metrics);
    double ScoreAccessibility(LayoutNode* node);
    
    // Constraint solving
    void SolveLayoutConstraints();
    bool ValidateConstraints(LayoutNode* node);
    void EnforceConstraints(LayoutNode* node);
    
    // Adaptive learning
    void RecordUserPreference(const wxString& key, const wxString& value);
    wxString GetUserPreference(const wxString& key) const;
    void UpdateUsageStatistics(ModernDockPanel* panel);
    
    // Utility functions
    LayoutNode* GetRootNode();
    std::vector<LayoutNode*> GetAllNodes();
    std::vector<ModernDockPanel*> GetAllPanels();
    bool IsNodeEmpty(LayoutNode* node);
    bool IsRedundantSplitter(LayoutNode* node);
    int GetNodeDepth(LayoutNode* node);
    
private:
    ModernDockManager* m_manager;
    
    // Smart components
    std::unique_ptr<AutoDockOptimizer> m_dockOptimizer;
    std::unique_ptr<LayoutAnalyzer> m_analyzer;
    std::unique_ptr<ConstraintLayoutSolver> m_constraintSolver;
    std::unique_ptr<SplitterOptimizer> m_splitterOptimizer;
    
    // Configuration
    SmartLayoutConstraints m_constraints;
    bool m_smartModeEnabled;
    
    // Learning data
    std::map<wxString, wxString> m_userPreferences;
    std::map<ModernDockPanel*, int> m_panelUsageCount;
    std::map<wxString, int> m_actionHistory;
    
    // Cache
    LayoutMetrics m_lastMetrics;
    std::vector<LayoutProblem> m_lastProblems;
    double m_lastQualityScore;
    
    wxDECLARE_EVENT_TABLE();
};

// Auto Dock Optimizer - Automatically selects best docking positions
class AutoDockOptimizer {
public:
    AutoDockOptimizer(SmartLayoutEngine* engine);
    
    SmartDockingDecision FindBestDockPosition(ModernDockPanel* panel);
    std::vector<std::pair<LayoutNode*, DockPosition>> GetAllDockingOptions(ModernDockPanel* panel);
    double ScoreDockingOption(ModernDockPanel* panel, LayoutNode* target, DockPosition position);
    
    void SetPreferredAreas(const std::map<wxString, DockArea>& preferences);
    void LearnFromDocking(ModernDockPanel* panel, LayoutNode* target, DockPosition position);
    
private:
    SmartLayoutEngine* m_engine;
    std::map<wxString, DockArea> m_preferredAreas;
    std::map<wxString, std::vector<DockPosition>> m_dockingHistory;
};

// Layout Analyzer - Detects and diagnoses layout problems
class LayoutAnalyzer {
public:
    LayoutAnalyzer(SmartLayoutEngine* engine);
    
    LayoutMetrics AnalyzeLayout(LayoutNode* root);
    std::vector<LayoutProblem> DetectProblems(LayoutNode* root);
    double CalculateQualityScore(const LayoutMetrics& metrics);
    
    bool IsLayoutBalanced(LayoutNode* root);
    bool HasRedundantSplitters(LayoutNode* root);
    bool HasEmptyAreas(LayoutNode* root);
    bool HasDeepNesting(LayoutNode* root, int maxDepth);
    
    std::vector<LayoutNode*> FindProblematicNodes(LayoutNode* root);
    
private:
    SmartLayoutEngine* m_engine;
    
    void AnalyzeNodeRecursive(LayoutNode* node, LayoutMetrics& metrics, int depth);
    void DetectNodeProblemsRecursive(LayoutNode* node, std::vector<LayoutProblem>& problems, int depth);
};

// Constraint Layout Solver - Applies intelligent constraints to layout
class ConstraintLayoutSolver {
public:
    ConstraintLayoutSolver(SmartLayoutEngine* engine);
    
    void SetConstraints(const SmartLayoutConstraints& constraints);
    void SolveLayout(LayoutNode* root);
    bool ValidateLayout(LayoutNode* root);
    
    void AddCustomConstraint(const wxString& name, std::function<bool(LayoutNode*)> constraint);
    void RemoveCustomConstraint(const wxString& name);
    
    std::vector<wxString> GetViolatedConstraints(LayoutNode* root);
    
private:
    SmartLayoutEngine* m_engine;
    SmartLayoutConstraints m_constraints;
    std::map<wxString, std::function<bool(LayoutNode*)>> m_customConstraints;
    
    void ApplyConstraintsRecursive(LayoutNode* node);
    bool CheckConstraints(LayoutNode* node);
};

// Splitter Optimizer - Intelligently manages splitters
class SplitterOptimizer {
public:
    SplitterOptimizer(SmartLayoutEngine* engine);
    
    void OptimizeSplitters(LayoutNode* root);
    void BalanceSplitters(LayoutNode* root);
    void MergeRedundantSplitters(LayoutNode* root);
    
    double CalculateOptimalRatio(LayoutNode* splitter);
    bool ShouldMergeSplitter(LayoutNode* splitter);
    void AutoAdjustSplitterRatios(LayoutNode* root, const wxSize& availableSize);
    
private:
    SmartLayoutEngine* m_engine;
    
    void OptimizeSplitterRecursive(LayoutNode* node);
    double ScoreSplitterBalance(LayoutNode* splitter);
    std::vector<LayoutNode*> FindMergeableSplitters(LayoutNode* root);
};

#endif // SMARTLAYOUTENGINE_H