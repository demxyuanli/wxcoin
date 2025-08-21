#pragma once

#include "ILayoutStrategy.h"
#include "LayoutEngine.h"
#include "ModernDockPanel.h"
#include <map>
#include <set>

// Flexible layout strategy - completely dynamic tree-based layout
// This strategy allows panels to be placed anywhere in the layout tree
// and automatically creates splitters and containers as needed
class FlexibleLayoutStrategy : public ILayoutStrategy {
public:
    FlexibleLayoutStrategy();
    virtual ~FlexibleLayoutStrategy() = default;

    // Layout creation and initialization
    void CreateLayout(LayoutNode* root) override;
    void InitializeLayout(LayoutNode* root) override;
    void DestroyLayout(LayoutNode* root) override;
    
    // Layout calculation and positioning
    void CalculateLayout(LayoutNode* node, const wxRect& rect) override;
    void CalculateNodeLayout(LayoutNode* node, const wxRect& rect) override;
    void CalculateSplitterLayout(LayoutNode* splitter, const wxRect& rect) override;
    void CalculateContainerLayout(LayoutNode* container, const wxRect& rect) override;
    
    // Panel management
    void AddPanel(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea area) override;
    void RemovePanel(LayoutNode* root, ModernDockPanel* panel) override;
    void MovePanel(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea newArea) override;
    void SwapPanels(LayoutNode* root, ModernDockPanel* panel1, ModernDockPanel* panel2) override;
    
    // Layout structure management
    LayoutNode* FindBestInsertionPoint(LayoutNode* root, UnifiedDockArea area) override;
    LayoutNode* CreateSplitterNode(SplitterOrientation orientation) override;
    LayoutNode* CreateContainerNode() override;
    void InsertNode(LayoutNode* parent, std::unique_ptr<LayoutNode> child) override;
    void RemoveNode(LayoutNode* parent, LayoutNode* child) override;
    
    // Layout constraints and validation
    bool ValidateLayout(LayoutNode* root) const override;
    bool CanAddPanel(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea area) const override;
    bool CanRemovePanel(LayoutNode* root, ModernDockPanel* panel) const override;
    bool CanMovePanel(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea newArea) const override;
    
    // Layout optimization
    void OptimizeLayout(LayoutNode* root) override;
    void CompactLayout(LayoutNode* root) override;
    void BalanceSplitters(LayoutNode* root) override;
    void MinimizeEmptySpace(LayoutNode* root) override;
    
    // Splitter management
    void SetSplitterRatio(LayoutNode* splitter, double ratio) override;
    double GetSplitterRatio(LayoutNode* splitter) const override;
    void SetSplitterPosition(LayoutNode* splitter, int position) override;
    int GetSplitterPosition(LayoutNode* splitter) const override;
    void SetSplitterSashSize(LayoutNode* splitter, int size) override;
    int GetSplitterSashSize(LayoutNode* splitter) const override;
    
    // Container management
    void SetContainerTabPosition(LayoutNode* container, int position) override;
    int GetContainerTabPosition(LayoutNode* container) const override;
    void SetContainerTabStyle(LayoutNode* container, int style) override;
    int GetContainerTabStyle(LayoutNode* container) const override;
    
    // Layout information and statistics
    void GetMinimumSize(LayoutNode* root, int& width, int& height) const override;
    void GetBestSize(LayoutNode* root, int& width, int& height) const override;
    void GetPanelBounds(LayoutNode* root, ModernDockPanel* panel, int& x, int& y, int& width, int& height) const override;
    UnifiedDockArea GetPanelArea(LayoutNode* root, ModernDockPanel* panel) const override;
    int GetPanelDepth(LayoutNode* root, ModernDockPanel* panel) const override;
    
    // Layout traversal and iteration
    void TraversePanels(LayoutNode* root, 
                        std::function<void(ModernDockPanel*)> visitor) const override;
    void TraverseSplitters(LayoutNode* root, 
                           std::function<void(LayoutNode*)> visitor) const override;
    void TraverseContainers(LayoutNode* root, 
                            std::function<void(LayoutNode*)> visitor) const override;
    void TraverseNodes(LayoutNode* root, 
                       std::function<void(LayoutNode*)> visitor) const override;
    
    // Layout serialization
    std::string SerializeLayout(LayoutNode* root) const override;
    bool DeserializeLayout(LayoutNode* root, const std::string& data) override;
    std::string ExportLayout(LayoutNode* root, const std::string& format) const override;
    bool ImportLayout(LayoutNode* root, const std::string& data, const std::string& format) override;
    
    // Layout comparison and merging
    bool IsLayoutEqual(LayoutNode* root1, LayoutNode* root2) const override;
    bool CanMergeLayouts(LayoutNode* root1, LayoutNode* root2) const override;
    void MergeLayouts(LayoutNode* target, LayoutNode* source) override;
    
    // Performance and caching
    void EnableLayoutCaching(bool enable) override;
    bool IsLayoutCachingEnabled() const override;
    void ClearLayoutCache() override;
    void SetLayoutUpdateMode(int mode) override;
    int GetLayoutUpdateMode() const override;
    
    // Error handling and debugging
    std::string GetLastError() const override;
    void ClearLastError() override;
    bool HasErrors() const override;
    void DumpLayoutDebugInfo(LayoutNode* root) const override;
    
    // Strategy-specific configuration
    void SetStrategyParameter(const std::string& name, const std::string& value) override;
    std::string GetStrategyParameter(const std::string& name) const override;
    std::vector<std::string> GetAvailableParameters() const override;
    void ResetToDefaultParameters() override;

private:
    // Flexible layout specific methods
    void CreateDynamicLayout(LayoutNode* root);
    LayoutNode* FindOrCreateInsertionPoint(LayoutNode* root, UnifiedDockArea area);
    LayoutNode* CreateOptimalSplitter(LayoutNode* parent, SplitterOrientation orientation);
    void AutoOrganizeLayout(LayoutNode* root);
    void BalanceTreeStructure(LayoutNode* root);
    
    // Panel placement strategies
    LayoutNode* PlacePanelInArea(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea area);
    LayoutNode* CreateTabbedContainer(LayoutNode* parent, ModernDockPanel* panel);
    LayoutNode* CreateSplitterForPanel(LayoutNode* parent, ModernDockPanel* panel, UnifiedDockArea area);
    
    // Layout analysis and optimization
    void AnalyzeLayoutStructure(LayoutNode* root);
    void OptimizeSplitterRatios(LayoutNode* root);
    void RemoveRedundantNodes(LayoutNode* root);
    void ConsolidateSimilarContainers(LayoutNode* root);
    
    // Helper methods
    LayoutNode* FindPanelNode(LayoutNode* root, ModernDockPanel* panel) const;
    LayoutNode* FindBestParentForArea(LayoutNode* root, UnifiedDockArea area);
    bool IsAreaAvailable(LayoutNode* root, UnifiedDockArea area) const;
    SplitterOrientation GetOptimalSplitterOrientation(UnifiedDockArea area);
    double CalculateOptimalRatio(LayoutNode* splitter, UnifiedDockArea area);
    
    // Serialization helpers
    void SerializeNode(LayoutNode* node, std::string& result, int indent) const;
    bool DeserializeNode(LayoutNode* parent, const std::string& data, size_t& pos);
    
    // Internal state
    std::string m_lastError;
    bool m_hasErrors;
    bool m_layoutCachingEnabled;
    int m_layoutUpdateMode;
    std::map<std::string, std::string> m_strategyParameters;
    
    // Flexible layout specific state
    std::map<UnifiedDockArea, int> m_areaUsageCount;
    std::set<LayoutNode*> m_optimizedNodes;
    bool m_autoOrganizeEnabled;
    bool m_balanceStructureEnabled;
    
    // Default parameters for flexible layout
    void InitializeDefaultParameters();
};


