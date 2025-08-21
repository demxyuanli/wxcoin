#pragma once

#include "UnifiedDockTypes.h"
#include "LayoutEngine.h"  // Include the full definition for LayoutNode
#include <functional>
#include <string>
#include <vector>

// Abstract layout strategy interface
class ILayoutStrategy {
public:
    virtual ~ILayoutStrategy() = default;

    // Layout creation and initialization
    virtual void CreateLayout(LayoutNode* root) = 0;
    virtual void InitializeLayout(LayoutNode* root) = 0;
    virtual void DestroyLayout(LayoutNode* root) = 0;
    
    // Layout calculation and positioning
    virtual void CalculateLayout(LayoutNode* node, const wxRect& rect) = 0;
    virtual void CalculateNodeLayout(LayoutNode* node, const wxRect& rect) = 0;
    virtual void CalculateSplitterLayout(LayoutNode* splitter, const wxRect& rect) = 0;
    virtual void CalculateContainerLayout(LayoutNode* container, const wxRect& rect) = 0;
    
    // Panel management
    virtual void AddPanel(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea area) = 0;
    virtual void RemovePanel(LayoutNode* root, ModernDockPanel* panel) = 0;
    virtual void MovePanel(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea newArea) = 0;
    virtual void SwapPanels(LayoutNode* root, ModernDockPanel* panel1, ModernDockPanel* panel2) = 0;
    
    // Layout structure management
    virtual LayoutNode* FindBestInsertionPoint(LayoutNode* root, UnifiedDockArea area) = 0;
    virtual LayoutNode* CreateSplitterNode(SplitterOrientation orientation) = 0;
    virtual LayoutNode* CreateContainerNode() = 0;
    virtual void InsertNode(LayoutNode* parent, std::unique_ptr<LayoutNode> child) = 0;
    virtual void RemoveNode(LayoutNode* parent, LayoutNode* child) = 0;
    
    // Layout constraints and validation
    virtual bool ValidateLayout(LayoutNode* root) const = 0;
    virtual bool CanAddPanel(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea area) const = 0;
    virtual bool CanRemovePanel(LayoutNode* root, ModernDockPanel* panel) const = 0;
    virtual bool CanMovePanel(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea newArea) const = 0;
    
    // Layout optimization
    virtual void OptimizeLayout(LayoutNode* root) = 0;
    virtual void CompactLayout(LayoutNode* root) = 0;
    virtual void BalanceSplitters(LayoutNode* root) = 0;
    virtual void MinimizeEmptySpace(LayoutNode* root) = 0;
    
    // Splitter management
    virtual void SetSplitterRatio(LayoutNode* splitter, double ratio) = 0;
    virtual double GetSplitterRatio(LayoutNode* splitter) const = 0;
    virtual void SetSplitterPosition(LayoutNode* splitter, int position) = 0;
    virtual int GetSplitterPosition(LayoutNode* splitter) const = 0;
    virtual void SetSplitterSashSize(LayoutNode* splitter, int size) = 0;
    virtual int GetSplitterSashSize(LayoutNode* splitter) const = 0;
    
    // Container management
    virtual void SetContainerTabPosition(LayoutNode* container, int position) = 0;
    virtual int GetContainerTabPosition(LayoutNode* container) const = 0;
    virtual void SetContainerTabStyle(LayoutNode* container, int style) = 0;
    virtual int GetContainerTabStyle(LayoutNode* container) const = 0;
    
    // Layout information and statistics
    virtual void GetMinimumSize(LayoutNode* root, int& width, int& height) const = 0;
    virtual void GetBestSize(LayoutNode* root, int& width, int& height) const = 0;
    virtual void GetPanelBounds(LayoutNode* root, ModernDockPanel* panel, int& x, int& y, int& width, int& height) const = 0;
    virtual UnifiedDockArea GetPanelArea(LayoutNode* root, ModernDockPanel* panel) const = 0;
    virtual int GetPanelDepth(LayoutNode* root, ModernDockPanel* panel) const = 0;
    
    // Layout traversal and iteration
    virtual void TraversePanels(LayoutNode* root, 
                               std::function<void(ModernDockPanel*)> visitor) const = 0;
    virtual void TraverseSplitters(LayoutNode* root, 
                                  std::function<void(LayoutNode*)> visitor) const = 0;
    virtual void TraverseContainers(LayoutNode* root, 
                                   std::function<void(LayoutNode*)> visitor) const = 0;
    virtual void TraverseNodes(LayoutNode* root, 
                              std::function<void(LayoutNode*)> visitor) const = 0;
    
    // Layout serialization
    virtual std::string SerializeLayout(LayoutNode* root) const = 0;
    virtual bool DeserializeLayout(LayoutNode* root, const std::string& data) = 0;
    virtual std::string ExportLayout(LayoutNode* root, const std::string& format) const = 0;
    virtual bool ImportLayout(LayoutNode* root, const std::string& data, const std::string& format) = 0;
    
    // Layout comparison and merging
    virtual bool IsLayoutEqual(LayoutNode* root1, LayoutNode* root2) const = 0;
    virtual bool CanMergeLayouts(LayoutNode* root1, LayoutNode* root2) const = 0;
    virtual void MergeLayouts(LayoutNode* target, LayoutNode* source) = 0;
    
    // Performance and caching
    virtual void EnableLayoutCaching(bool enable) = 0;
    virtual bool IsLayoutCachingEnabled() const = 0;
    virtual void ClearLayoutCache() = 0;
    virtual void SetLayoutUpdateMode(int mode) = 0;
    virtual int GetLayoutUpdateMode() const = 0;
    
    // Error handling and debugging
    virtual std::string GetLastError() const = 0;
    virtual void ClearLastError() = 0;
    virtual bool HasErrors() const = 0;
    virtual void DumpLayoutDebugInfo(LayoutNode* root) const = 0;
    
    // Strategy-specific configuration
    virtual void SetStrategyParameter(const std::string& name, const std::string& value) = 0;
    virtual std::string GetStrategyParameter(const std::string& name) const = 0;
    virtual std::vector<std::string> GetAvailableParameters() const = 0;
    virtual void ResetToDefaultParameters() = 0;
};


