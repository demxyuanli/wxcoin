#include "widgets/layouts/FlexibleLayoutStrategy.h"
#include "widgets/LayoutEngine.h"
#include "widgets/ModernDockPanel.h"
#include <wx/window.h>
#include <algorithm>
#include <sstream>

FlexibleLayoutStrategy::FlexibleLayoutStrategy() 
    : m_hasErrors(false)
    , m_layoutCachingEnabled(true)
    , m_layoutUpdateMode(0)
    , m_autoOrganizeEnabled(true)
    , m_balanceStructureEnabled(true)
{
    InitializeDefaultParameters();
}

void FlexibleLayoutStrategy::CreateLayout(LayoutNode* root) {
    if (!root) return;
    
    m_hasErrors = false;
    
    // Create a completely dynamic layout structure
    CreateDynamicLayout(root);
}

void FlexibleLayoutStrategy::InitializeLayout(LayoutNode* root) {
    if (!root) return;
    
    m_hasErrors = false;
    
    // Reset area usage counts
    m_areaUsageCount.clear();
    m_optimizedNodes.clear();
    
    // Analyze existing layout structure
    AnalyzeLayoutStructure(root);
}

void FlexibleLayoutStrategy::DestroyLayout(LayoutNode* root) {
    if (!root) return;
    
    // Clear all state
    m_areaUsageCount.clear();
    m_optimizedNodes.clear();
    m_lastError.clear();
    m_hasErrors = false;
}

void FlexibleLayoutStrategy::CalculateLayout(LayoutNode* node, const wxRect& rect) {
    if (!node) return;
    
    CalculateNodeLayout(node, rect);
}

void FlexibleLayoutStrategy::CalculateNodeLayout(LayoutNode* node, const wxRect& rect) {
    if (!node) return;
    
    // Set the node's rectangle
    node->SetRect(rect);
    
    // If it's a panel, set the panel size
    if (node->GetType() == LayoutNodeType::Panel && node->GetPanel()) {
        node->GetPanel()->SetSize(rect);
    }
    
    // If it's a splitter, calculate splitter layout
    if (node->GetType() == LayoutNodeType::HorizontalSplitter || 
        node->GetType() == LayoutNodeType::VerticalSplitter) {
        CalculateSplitterLayout(node, rect);
    }
    
    // Recursively process children
    for (auto& child : node->GetChildren()) {
        if (child) {
            CalculateNodeLayout(child.get(), rect);
        }
    }
}

void FlexibleLayoutStrategy::CalculateSplitterLayout(LayoutNode* splitter, const wxRect& rect) {
    if (!splitter || splitter->GetChildren().size() != 2) return;
    
    auto& children = splitter->GetChildren();
    auto* firstChild = children[0].get();
    auto* secondChild = children[1].get();
    
    if (!firstChild || !secondChild) return;
    
    wxRect firstRect, secondRect;
    double ratio = splitter->GetSplitterRatio();
    
    if (splitter->GetType() == LayoutNodeType::HorizontalSplitter) {
        // Left-right split
        int splitPos = rect.x + static_cast<int>(rect.width * ratio);
        int minWidth = 100;
        
        firstRect = wxRect(rect.x, rect.y, splitPos - rect.x, rect.height);
        secondRect = wxRect(splitPos, rect.y, rect.width - (splitPos - rect.x), rect.height);
        
        // Ensure minimum sizes
        if (firstRect.width < minWidth) {
            firstRect.width = minWidth;
            secondRect.x = firstRect.x + firstRect.width;
            secondRect.width = rect.width - firstRect.width;
        }
    } else if (splitter->GetType() == LayoutNodeType::VerticalSplitter) {
        // Top-bottom split
        int splitPos = rect.y + static_cast<int>(rect.height * ratio);
        int minHeight = 100;
        
        firstRect = wxRect(rect.x, rect.y, rect.width, splitPos - rect.y);
        secondRect = wxRect(rect.x, splitPos, rect.width, rect.height - (splitPos - rect.y));
        
        // Ensure minimum sizes
        if (firstRect.height < minHeight) {
            firstRect.height = minHeight;
            secondRect.y = firstRect.y + firstRect.height;
            secondRect.height = rect.height - firstRect.height;
        }
    }
    
    // Apply calculated rectangles to children
    CalculateNodeLayout(firstChild, firstRect);
    CalculateNodeLayout(secondChild, secondRect);
}

void FlexibleLayoutStrategy::CalculateContainerLayout(LayoutNode* container, const wxRect& rect) {
    if (!container) return;
    
    // For containers, distribute space among children
    auto& children = container->GetChildren();
    if (children.empty()) return;
    
    if (children.size() == 1) {
        // Single child gets all space
        CalculateNodeLayout(children[0].get(), rect);
    } else {
        // Multiple children - create a grid-like layout
        int cols = static_cast<int>(std::ceil(std::sqrt(children.size())));
        int rows = static_cast<int>(std::ceil(static_cast<double>(children.size()) / cols));
        
        int cellWidth = rect.width / cols;
        int cellHeight = rect.height / rows;
        
        for (size_t i = 0; i < children.size(); ++i) {
            if (children[i]) {
                int row = static_cast<int>(i) / cols;
                int col = static_cast<int>(i) % cols;
                
                wxRect childRect(
                    rect.x + col * cellWidth,
                    rect.y + row * cellHeight,
                    cellWidth,
                    cellHeight
                );
                
                CalculateNodeLayout(children[i].get(), childRect);
            }
        }
    }
}

void FlexibleLayoutStrategy::AddPanel(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea area) {
    if (!root || !panel) return;
    
    // Find or create the best insertion point
    LayoutNode* insertionPoint = FindOrCreateInsertionPoint(root, area);
    if (!insertionPoint) return;
    
    // Place the panel in the optimal location
    LayoutNode* panelNode = PlacePanelInArea(root, panel, area);
    if (panelNode) {
        // Update area usage count
        m_areaUsageCount[area]++;
        
        // Auto-organize if enabled
        if (m_autoOrganizeEnabled) {
            AutoOrganizeLayout(root);
        }
    }
}

LayoutNode* FlexibleLayoutStrategy::FindBestInsertionPoint(LayoutNode* root, UnifiedDockArea area) {
    if (!root) return nullptr;
    
    return FindOrCreateInsertionPoint(root, area);
}

LayoutNode* FlexibleLayoutStrategy::CreateSplitterNode(SplitterOrientation orientation) {
    LayoutNodeType type = (orientation == SplitterOrientation::Horizontal) ? 
                          LayoutNodeType::HorizontalSplitter : LayoutNodeType::VerticalSplitter;
    return new LayoutNode(type);
}

LayoutNode* FlexibleLayoutStrategy::CreateContainerNode() {
    return new LayoutNode(LayoutNodeType::Root);
}

void FlexibleLayoutStrategy::InsertNode(LayoutNode* parent, std::unique_ptr<LayoutNode> child) {
    if (parent && child) {
        parent->AddChild(std::move(child));
    }
}

void FlexibleLayoutStrategy::RemoveNode(LayoutNode* parent, LayoutNode* child) {
    if (parent && child) {
        parent->RemoveChild(child);
    }
}

void FlexibleLayoutStrategy::RemovePanel(LayoutNode* root, ModernDockPanel* panel) {
    if (!root || !panel) return;
    
    // Find the node containing this panel
    LayoutNode* panelNode = FindPanelNode(root, panel);
    if (!panelNode) return;
    
    LayoutNode* parent = panelNode->GetParent();
    if (!parent) return;
    
    // Remove the panel node from its parent
    parent->RemoveChild(panelNode);
    
    // Update area usage count
    UnifiedDockArea area = GetPanelArea(root, panel);
    if (m_areaUsageCount.find(area) != m_areaUsageCount.end()) {
        m_areaUsageCount[area] = std::max(0, m_areaUsageCount[area] - 1);
    }
    
    // Clean up empty containers or splitters with only one child
    CompactLayout(root);
}

void FlexibleLayoutStrategy::MovePanel(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea newArea) {
    if (!root || !panel) return;
    
    // Remove from current location
    RemovePanel(root, panel);
    
    // Add to new area
    AddPanel(root, panel, newArea);
}

void FlexibleLayoutStrategy::SwapPanels(LayoutNode* root, ModernDockPanel* panel1, ModernDockPanel* panel2) {
    if (!root || !panel1 || !panel2) return;
    
    // Find both panels
    LayoutNode* node1 = FindPanelNode(root, panel1);
    LayoutNode* node2 = FindPanelNode(root, panel2);
    
    if (!node1 || !node2) return;
    
    // Get their current areas
    UnifiedDockArea area1 = GetPanelArea(root, panel1);
    UnifiedDockArea area2 = GetPanelArea(root, panel2);
    
    // Remove both panels
    RemovePanel(root, panel1);
    RemovePanel(root, panel2);
    
    // Add them back in swapped positions
    AddPanel(root, panel1, area2);
    AddPanel(root, panel2, area1);
}

bool FlexibleLayoutStrategy::ValidateLayout(LayoutNode* root) const {
    if (!root) return false;
    
    // Check if all panels are accessible
    bool hasPanels = false;
    TraversePanels(root, [&hasPanels](ModernDockPanel*) { hasPanels = true; });
    
    // Check if structure is intact
    return hasPanels && root->GetChildren().size() > 0;
}

bool FlexibleLayoutStrategy::CanAddPanel(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea area) const {
    if (!root || !panel) return false;
    
    // Check if area is available
    return IsAreaAvailable(root, area);
}

bool FlexibleLayoutStrategy::CanRemovePanel(LayoutNode* root, ModernDockPanel* panel) const {
    if (!root || !panel) return false;
    
    // Check if panel exists in the layout
    return FindPanelNode(root, panel) != nullptr;
}

bool FlexibleLayoutStrategy::CanMovePanel(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea newArea) const {
    if (!root || !panel) return false;
    
    // Check if panel exists and new area is available
    return FindPanelNode(root, panel) != nullptr && IsAreaAvailable(root, newArea);
}

void FlexibleLayoutStrategy::OptimizeLayout(LayoutNode* root) {
    if (!root) return;
    
    // Analyze current structure
    AnalyzeLayoutStructure(root);
    
    // Optimize splitter ratios
    OptimizeSplitterRatios(root);
    
    // Remove redundant nodes
    RemoveRedundantNodes(root);
    
    // Consolidate similar containers
    ConsolidateSimilarContainers(root);
    
    // Balance tree structure if enabled
    if (m_balanceStructureEnabled) {
        BalanceTreeStructure(root);
    }
}

void FlexibleLayoutStrategy::CompactLayout(LayoutNode* root) {
    if (!root) return;
    
    // Remove empty containers
    auto& children = root->GetChildren();
    children.erase(
        std::remove_if(children.begin(), children.end(),
            [](const std::unique_ptr<LayoutNode>& child) {
                return child && child->GetChildren().empty() && 
                       child->GetType() == LayoutNodeType::Root;
            }),
        children.end()
    );
    
    // If a splitter has only one child, promote the child
    if ((root->GetType() == LayoutNodeType::HorizontalSplitter || 
         root->GetType() == LayoutNodeType::VerticalSplitter) && 
        children.size() == 1) {
        
        LayoutNode* parent = root->GetParent();
        if (parent) {
            auto child = std::move(children[0]);
            parent->RemoveChild(root);
            parent->AddChild(std::move(child));
        }
    }
    
    // Recursively compact children
    for (const auto& child : children) {
        if (child) {
            CompactLayout(child.get());
        }
    }
}

void FlexibleLayoutStrategy::BalanceSplitters(LayoutNode* root) {
    if (!root) return;
    
    // Balance all splitters to equal ratios
    TraverseSplitters(root, [](LayoutNode* splitter) {
        splitter->SetSplitterRatio(0.5);
    });
}

void FlexibleLayoutStrategy::MinimizeEmptySpace(LayoutNode* root) {
    if (!root) return;
    
    // Remove empty containers
    CompactLayout(root);
    
    // Optimize splitter ratios to minimize empty areas
    OptimizeSplitterRatios(root);
}

void FlexibleLayoutStrategy::SetSplitterRatio(LayoutNode* splitter, double ratio) {
    if (splitter) {
        splitter->SetSplitterRatio(ratio);
    }
}

double FlexibleLayoutStrategy::GetSplitterRatio(LayoutNode* splitter) const {
    return splitter ? splitter->GetSplitterRatio() : 0.5;
}

void FlexibleLayoutStrategy::SetSplitterPosition(LayoutNode* splitter, int position) {
    if (splitter) {
        splitter->SetSashPosition(position);
    }
}

int FlexibleLayoutStrategy::GetSplitterPosition(LayoutNode* splitter) const {
    return splitter ? splitter->GetSashPosition() : 0;
}

void FlexibleLayoutStrategy::SetSplitterSashSize(LayoutNode* splitter, int size) {
    // Splitter sash size setting - not implemented in LayoutNode
    wxUnusedVar(splitter);
    wxUnusedVar(size);
}

int FlexibleLayoutStrategy::GetSplitterSashSize(LayoutNode* splitter) const {
    wxUnusedVar(splitter);
    return 4; // Default sash size
}

void FlexibleLayoutStrategy::SetContainerTabPosition(LayoutNode* container, int position) {
    // Container tab position setting - not implemented in LayoutNode
    wxUnusedVar(container);
    wxUnusedVar(position);
}

int FlexibleLayoutStrategy::GetContainerTabPosition(LayoutNode* container) const {
    wxUnusedVar(container);
    return 0; // 0 = top, 1 = bottom, 2 = left, 3 = right
}

void FlexibleLayoutStrategy::SetContainerTabStyle(LayoutNode* container, int style) {
    // Container tab style setting - not implemented in LayoutNode
    wxUnusedVar(container);
    wxUnusedVar(style);
}

int FlexibleLayoutStrategy::GetContainerTabStyle(LayoutNode* container) const {
    wxUnusedVar(container);
    return 0;
}

void FlexibleLayoutStrategy::GetMinimumSize(LayoutNode* root, int& width, int& height) const {
    width = 200;
    height = 150;
    
    if (!root) return;
    
    // Calculate minimum size based on content
    int panelCount = 0;
    TraversePanels(root, [&panelCount](ModernDockPanel*) { panelCount++; });
    
    if (panelCount > 0) {
        width = std::max(width, panelCount * 150);
        height = std::max(height, panelCount * 100);
    }
}

void FlexibleLayoutStrategy::GetBestSize(LayoutNode* root, int& width, int& height) const {
    width = 1200;
    height = 800;
    
    if (!root) return;
    
    // Calculate best size based on content and structure
    int panelCount = 0;
    TraversePanels(root, [&panelCount](ModernDockPanel*) { panelCount++; });
    
    if (panelCount > 0) {
        width = std::max(width, panelCount * 200);
        height = std::max(height, panelCount * 150);
    }
}

void FlexibleLayoutStrategy::GetPanelBounds(LayoutNode* root, ModernDockPanel* panel, int& x, int& y, int& width, int& height) const {
    x = 0;
    y = 0;
    width = 0;
    height = 0;
    
    if (!root || !panel) return;
    
    // Find the panel node
    LayoutNode* panelNode = FindPanelNode(root, panel);
    if (!panelNode) return;
    
    // Get the panel's rectangle
    wxRect rect = panelNode->GetRect();
    x = rect.x;
    y = rect.y;
    width = rect.width;
    height = rect.height;
}

UnifiedDockArea FlexibleLayoutStrategy::GetPanelArea(LayoutNode* root, ModernDockPanel* panel) const {
    if (!root || !panel) return UnifiedDockArea::Center;
    
    // Find the panel node
    LayoutNode* panelNode = FindPanelNode(root, panel);
    if (!panelNode) return UnifiedDockArea::Center;
    
    // Try to determine area based on position in the tree
    // This is a simplified approach - in a real implementation,
    // you might want to analyze the actual screen coordinates
    LayoutNode* current = panelNode->GetParent();
    int depth = 0;
    
    while (current && current != root) {
        depth++;
        current = current->GetParent();
    }
    
    // Simple heuristic based on depth and position
    if (depth <= 2) return UnifiedDockArea::Center;
    if (depth <= 3) return UnifiedDockArea::Left;
    if (depth <= 4) return UnifiedDockArea::Right;
    if (depth <= 5) return UnifiedDockArea::Top;
    return UnifiedDockArea::Bottom;
}

int FlexibleLayoutStrategy::GetPanelDepth(LayoutNode* root, ModernDockPanel* panel) const {
    if (!root || !panel) return 0;
    
    // Find the panel node
    LayoutNode* panelNode = FindPanelNode(root, panel);
    if (!panelNode) return 0;
    
    // Count the depth from root to panel
    int depth = 0;
    LayoutNode* current = panelNode;
    while (current && current != root) {
        depth++;
        current = current->GetParent();
    }
    
    return depth;
}

void FlexibleLayoutStrategy::TraversePanels(LayoutNode* root, 
                                          std::function<void(ModernDockPanel*)> visitor) const {
    if (!root || !visitor) return;
    
    // If this is a panel node, visit it
    if (root->GetType() == LayoutNodeType::Panel && root->GetPanel()) {
        visitor(reinterpret_cast<ModernDockPanel*>(root->GetPanel()));
    }
    
    // Recursively traverse children
    for (const auto& child : root->GetChildren()) {
        if (child) {
            TraversePanels(child.get(), visitor);
        }
    }
}

void FlexibleLayoutStrategy::TraverseSplitters(LayoutNode* root, 
                                             std::function<void(LayoutNode*)> visitor) const {
    if (!root || !visitor) return;
    
    // If this is a splitter node, visit it
    if (root->GetType() == LayoutNodeType::HorizontalSplitter || 
        root->GetType() == LayoutNodeType::VerticalSplitter) {
        visitor(root);
    }
    
    // Recursively traverse children
    for (const auto& child : root->GetChildren()) {
        if (child) {
            TraverseSplitters(child.get(), visitor);
        }
    }
}

void FlexibleLayoutStrategy::TraverseContainers(LayoutNode* root, 
                                               std::function<void(LayoutNode*)> visitor) const {
    if (!root || !visitor) return;
    
    // If this is a container (Root) node, visit it
    if (root->GetType() == LayoutNodeType::Root) {
        visitor(root);
    }
    
    // Recursively traverse children
    for (const auto& child : root->GetChildren()) {
        if (child) {
            TraverseContainers(child.get(), visitor);
        }
    }
}

void FlexibleLayoutStrategy::TraverseNodes(LayoutNode* root, 
                                          std::function<void(LayoutNode*)> visitor) const {
    if (!root || !visitor) return;
    
    // Visit this node
    visitor(root);
    
    // Recursively traverse children
    for (const auto& child : root->GetChildren()) {
        if (child) {
            TraverseNodes(child.get(), visitor);
        }
    }
}

std::string FlexibleLayoutStrategy::SerializeLayout(LayoutNode* root) const {
    if (!root) return "";
    
    std::string result = "FlexibleLayout:{\n";
    SerializeNode(root, result, 0);
    result += "}";
    return result;
}

bool FlexibleLayoutStrategy::DeserializeLayout(LayoutNode* root, const std::string& data) {
    if (!root || data.empty()) return false;
    
    // Check if this is our format
    if (data.find("FlexibleLayout:{") != 0) return false;
    
    try {
        // Clear existing layout
        auto& children = root->GetChildren();
        children.clear();
        
        // Reset state
        m_areaUsageCount.clear();
        m_optimizedNodes.clear();
        
        // Parse the layout data
        size_t pos = data.find('{') + 1;
        if (!DeserializeNode(root, data, pos)) {
            m_lastError = "Failed to parse layout data";
            return false;
        }
        
        m_lastError.clear();
        return true;
    } catch (...) {
        m_lastError = "Failed to deserialize layout";
        return false;
    }
}

std::string FlexibleLayoutStrategy::ExportLayout(LayoutNode* root, const std::string& format) const {
    wxUnusedVar(root);
    wxUnusedVar(format);
    return "";
}

bool FlexibleLayoutStrategy::ImportLayout(LayoutNode* root, const std::string& data, const std::string& format) {
    wxUnusedVar(format);
    return DeserializeLayout(root, data);
}

bool FlexibleLayoutStrategy::IsLayoutEqual(LayoutNode* root1, LayoutNode* root2) const {
    if (!root1 || !root2) return false;
    
    // Simple comparison - check if both have the same number of panels
    int panels1 = 0, panels2 = 0;
    TraversePanels(root1, [&panels1](ModernDockPanel*) { panels1++; });
    TraversePanels(root2, [&panels2](ModernDockPanel*) { panels2++; });
    
    return panels1 == panels2;
}

bool FlexibleLayoutStrategy::CanMergeLayouts(LayoutNode* root1, LayoutNode* root2) const {
    if (!root1 || !root2) return false;
    
    // Check if layouts can be merged (not too many panels)
    int totalPanels = 0;
    TraversePanels(root1, [&totalPanels](ModernDockPanel*) { totalPanels++; });
    TraversePanels(root2, [&totalPanels](ModernDockPanel*) { totalPanels++; });
    
    return totalPanels <= 10; // Arbitrary limit
}

void FlexibleLayoutStrategy::MergeLayouts(LayoutNode* target, LayoutNode* source) {
    if (!target || !source) return;
    
    // Move all panels from source to target
    std::vector<ModernDockPanel*> panels;
    TraversePanels(source, [&panels](ModernDockPanel* panel) { panels.push_back(panel); });
    
    for (auto* panel : panels) {
        AddPanel(target, panel, UnifiedDockArea::Center);
    }
    
    // Clear source
    auto& sourceChildren = source->GetChildren();
    sourceChildren.clear();
}

void FlexibleLayoutStrategy::EnableLayoutCaching(bool enable) {
    m_layoutCachingEnabled = enable;
}

bool FlexibleLayoutStrategy::IsLayoutCachingEnabled() const {
    return m_layoutCachingEnabled;
}

void FlexibleLayoutStrategy::ClearLayoutCache() {
    // Clear optimization cache
    m_optimizedNodes.clear();
}

void FlexibleLayoutStrategy::SetLayoutUpdateMode(int mode) {
    m_layoutUpdateMode = mode;
}

int FlexibleLayoutStrategy::GetLayoutUpdateMode() const {
    return m_layoutUpdateMode;
}

std::string FlexibleLayoutStrategy::GetLastError() const {
    return m_lastError;
}

void FlexibleLayoutStrategy::ClearLastError() {
    m_lastError.clear();
    m_hasErrors = false;
}

bool FlexibleLayoutStrategy::HasErrors() const {
    return m_hasErrors;
}

void FlexibleLayoutStrategy::DumpLayoutDebugInfo(LayoutNode* root) const {
    if (!root) return;
    
    // Dump layout structure information
    int panelCount = 0;
    int splitterCount = 0;
    int containerCount = 0;
    
    TraversePanels(root, [&panelCount](ModernDockPanel*) { panelCount++; });
    TraverseSplitters(root, [&splitterCount](LayoutNode*) { splitterCount++; });
    TraverseContainers(root, [&containerCount](LayoutNode*) { containerCount++; });
    
    // This would typically write to a log file or debug output
    wxUnusedVar(panelCount);
    wxUnusedVar(splitterCount);
    wxUnusedVar(containerCount);
}

void FlexibleLayoutStrategy::SetStrategyParameter(const std::string& name, const std::string& value) {
    m_strategyParameters[name] = value;
}

std::string FlexibleLayoutStrategy::GetStrategyParameter(const std::string& name) const {
    auto it = m_strategyParameters.find(name);
    if (it != m_strategyParameters.end()) {
        return it->second;
    }
    return "";
}

std::vector<std::string> FlexibleLayoutStrategy::GetAvailableParameters() const {
    std::vector<std::string> params;
    for (const auto& param : m_strategyParameters) {
        params.push_back(param.first);
    }
    return params;
}

void FlexibleLayoutStrategy::ResetToDefaultParameters() {
    InitializeDefaultParameters();
}

// Private method implementations

void FlexibleLayoutStrategy::CreateDynamicLayout(LayoutNode* root) {
    if (!root) return;
    
    // Create a simple initial structure - just a root container
    // The actual structure will be built dynamically as panels are added
    auto& children = root->GetChildren();
    children.clear();
    
    // Create a default center container
    auto centerContainer = std::make_unique<LayoutNode>(LayoutNodeType::Root);
    root->AddChild(std::move(centerContainer));
}

LayoutNode* FlexibleLayoutStrategy::FindOrCreateInsertionPoint(LayoutNode* root, UnifiedDockArea area) {
    if (!root) return nullptr;
    
    // First try to find an existing suitable container
    LayoutNode* existing = FindBestParentForArea(root, area);
    if (existing) return existing;
    
    // If none exists, create a new insertion point
    return CreateSplitterForPanel(root, nullptr, area);
}

LayoutNode* FlexibleLayoutStrategy::CreateOptimalSplitter(LayoutNode* parent, SplitterOrientation orientation) {
    if (!parent) return nullptr;
    
    auto splitter = std::make_unique<LayoutNode>(
        orientation == SplitterOrientation::Horizontal ? 
        LayoutNodeType::HorizontalSplitter : LayoutNodeType::VerticalSplitter
    );
    
    // Set optimal ratio based on orientation
    double ratio = (orientation == SplitterOrientation::Horizontal) ? 0.3 : 0.7;
    splitter->SetSplitterRatio(ratio);
    
    LayoutNode* splitterPtr = splitter.get();
    parent->AddChild(std::move(splitter));
    
    return splitterPtr;
}

void FlexibleLayoutStrategy::AutoOrganizeLayout(LayoutNode* root) {
    if (!root) return;
    
    // Analyze current structure
    AnalyzeLayoutStructure(root);
    
    // Optimize based on analysis
    OptimizeLayout(root);
}

void FlexibleLayoutStrategy::BalanceTreeStructure(LayoutNode* root) {
    if (!root) return;
    
    // Balance the tree by ensuring no branch is too deep
    // This is a simplified implementation
    TraverseNodes(root, [this](LayoutNode* node) {
        if (node->GetChildren().size() > 4) {
            // Too many children, consider reorganizing
            // This would involve creating intermediate containers
        }
    });
}

LayoutNode* FlexibleLayoutStrategy::PlacePanelInArea(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea area) {
    if (!root || !panel) return nullptr;
    
    // Find the best container for this area
    LayoutNode* container = FindBestParentForArea(root, area);
    if (!container) return nullptr;
    
    // Check if we should create a tabbed container
    if (container->GetChildren().size() > 0 && 
        container->GetChildren()[0]->GetType() == LayoutNodeType::Panel) {
        // Create tabbed container
        return CreateTabbedContainer(container, panel);
    } else {
        // Create a new panel node
        auto panelNode = std::make_unique<LayoutNode>(LayoutNodeType::Panel);
        panelNode->SetPanel(reinterpret_cast<::ModernDockPanel*>(panel));
        
        LayoutNode* panelPtr = panelNode.get();
        container->AddChild(std::move(panelNode));
        
        return panelPtr;
    }
}

LayoutNode* FlexibleLayoutStrategy::CreateTabbedContainer(LayoutNode* parent, ModernDockPanel* panel) {
    if (!parent || !panel) return nullptr;
    
    // Create a new panel node
    auto panelNode = std::make_unique<LayoutNode>(LayoutNodeType::Panel);
    panelNode->SetPanel(reinterpret_cast<::ModernDockPanel*>(panel));
    
    LayoutNode* panelPtr = panelNode.get();
    parent->AddChild(std::move(panelNode));
    
    return panelPtr;
}

LayoutNode* FlexibleLayoutStrategy::CreateSplitterForPanel(LayoutNode* parent, ModernDockPanel* panel, UnifiedDockArea area) {
    if (!parent) return nullptr;
    
    // Determine optimal splitter orientation
    SplitterOrientation orientation = GetOptimalSplitterOrientation(area);
    
    // Create the splitter
    auto splitter = std::make_unique<LayoutNode>(
        orientation == SplitterOrientation::Horizontal ? 
        LayoutNodeType::HorizontalSplitter : LayoutNodeType::VerticalSplitter
    );
    
    // Set optimal ratio
    double ratio = CalculateOptimalRatio(splitter.get(), area);
    splitter->SetSplitterRatio(ratio);
    
    // Create containers for the split areas
    auto container1 = std::make_unique<LayoutNode>(LayoutNodeType::Root);
    auto container2 = std::make_unique<LayoutNode>(LayoutNodeType::Root);
    
    splitter->AddChild(std::move(container1));
    splitter->AddChild(std::move(container2));
    
    LayoutNode* splitterPtr = splitter.get();
    parent->AddChild(std::move(splitter));
    
    return splitterPtr;
}

void FlexibleLayoutStrategy::AnalyzeLayoutStructure(LayoutNode* root) {
    if (!root) return;
    
    // Reset analysis data
    m_areaUsageCount.clear();
    m_optimizedNodes.clear();
    
    // Count panels in each area
    TraversePanels(root, [this](ModernDockPanel* panel) {
        // This is a simplified analysis - in a real implementation,
        // you would analyze the actual screen coordinates and tree structure
        UnifiedDockArea area = UnifiedDockArea::Center; // Default
        m_areaUsageCount[area]++;
    });
}

void FlexibleLayoutStrategy::OptimizeSplitterRatios(LayoutNode* root) {
    if (!root) return;
    
    TraverseSplitters(root, [this](LayoutNode* splitter) {
        if (m_optimizedNodes.find(splitter) != m_optimizedNodes.end()) {
            return; // Already optimized
        }
        
        // Set optimal ratio based on content
        double ratio = 0.5; // Default
        
        if (splitter->GetChildren().size() == 2) {
            // Analyze children to determine optimal ratio
            auto& children = splitter->GetChildren();
            int panels1 = 0, panels2 = 0;
            
            // Count panels in each child
            TraversePanels(children[0].get(), [&panels1](ModernDockPanel*) { panels1++; });
            TraversePanels(children[1].get(), [&panels2](ModernDockPanel*) { panels2++; });
            
            if (panels1 + panels2 > 0) {
                ratio = static_cast<double>(panels1) / (panels1 + panels2);
            }
        }
        
        splitter->SetSplitterRatio(ratio);
        m_optimizedNodes.insert(splitter);
    });
}

void FlexibleLayoutStrategy::RemoveRedundantNodes(LayoutNode* root) {
    if (!root) return;
    
    // Remove empty containers
    CompactLayout(root);
    
    // Remove single-child splitters
    TraverseSplitters(root, [](LayoutNode* splitter) {
        if (splitter->GetChildren().size() == 1) {
            LayoutNode* parent = splitter->GetParent();
            if (parent) {
                auto child = std::move(splitter->GetChildren()[0]);
                parent->RemoveChild(splitter);
                parent->AddChild(std::move(child));
            }
        }
    });
}

void FlexibleLayoutStrategy::ConsolidateSimilarContainers(LayoutNode* root) {
    if (!root) return;
    
    // This is a placeholder for container consolidation logic
    // In a real implementation, you would merge containers that have similar purposes
    // or are in the same area
    wxUnusedVar(root);
}

LayoutNode* FlexibleLayoutStrategy::FindPanelNode(LayoutNode* root, ModernDockPanel* panel) const {
    if (!root || !panel) return nullptr;
    
    // Check if this node contains the panel
    if (root->GetType() == LayoutNodeType::Panel && 
        root->GetPanel() == reinterpret_cast<::ModernDockPanel*>(panel)) {
        return root;
    }
    
    // Recursively search children
    for (const auto& child : root->GetChildren()) {
        if (child) {
            LayoutNode* found = FindPanelNode(child.get(), panel);
            if (found) return found;
        }
    }
    
    return nullptr;
}

LayoutNode* FlexibleLayoutStrategy::FindBestParentForArea(LayoutNode* root, UnifiedDockArea area) {
    if (!root) return nullptr;
    
    // Simple heuristic: find the first available container
    // In a real implementation, you would analyze the tree structure
    // and find the most appropriate container based on area and content
    
    if (root->GetType() == LayoutNodeType::Root) {
        return root;
    }
    
    for (const auto& child : root->GetChildren()) {
        if (child && child->GetType() == LayoutNodeType::Root) {
            return child.get();
        }
    }
    
    return root;
}

bool FlexibleLayoutStrategy::IsAreaAvailable(LayoutNode* root, UnifiedDockArea area) const {
    if (!root) return false;
    
    // Check if the area is not too crowded
    auto it = m_areaUsageCount.find(area);
    if (it != m_areaUsageCount.end()) {
        return it->second < 5; // Arbitrary limit
    }
    
    return true;
}

SplitterOrientation FlexibleLayoutStrategy::GetOptimalSplitterOrientation(UnifiedDockArea area) {
    switch (area) {
        case UnifiedDockArea::Left:
        case UnifiedDockArea::Right:
            return SplitterOrientation::Horizontal;
        case UnifiedDockArea::Top:
        case UnifiedDockArea::Bottom:
            return SplitterOrientation::Vertical;
        default:
            return SplitterOrientation::Horizontal;
    }
}

double FlexibleLayoutStrategy::CalculateOptimalRatio(LayoutNode* splitter, UnifiedDockArea area) {
    wxUnusedVar(splitter);
    
    switch (area) {
        case UnifiedDockArea::Left:
            return 0.25;
        case UnifiedDockArea::Right:
            return 0.75;
        case UnifiedDockArea::Top:
            return 0.3;
        case UnifiedDockArea::Bottom:
            return 0.7;
        default:
            return 0.5;
    }
}

void FlexibleLayoutStrategy::SerializeNode(LayoutNode* node, std::string& result, int indent) const {
    if (!node) return;
    
    std::string indentStr(indent * 2, ' ');
    
    // Add node type
    result += indentStr + "Node: ";
    switch (node->GetType()) {
        case LayoutNodeType::Root:
            result += "Root";
            break;
        case LayoutNodeType::Panel:
            result += "Panel";
            if (node->GetPanel()) {
                result += " (HasPanel)";
            }
            break;
        case LayoutNodeType::HorizontalSplitter:
            result += "HorizontalSplitter";
            result += " (Ratio: " + std::to_string(node->GetSplitterRatio()) + ")";
            break;
        case LayoutNodeType::VerticalSplitter:
            result += "VerticalSplitter";
            result += " (Ratio: " + std::to_string(node->GetSplitterRatio()) + ")";
            break;
    }
    result += "\n";
    
    // Serialize children
    for (const auto& child : node->GetChildren()) {
        if (child) {
            SerializeNode(child.get(), result, indent + 1);
        }
    }
}

bool FlexibleLayoutStrategy::DeserializeNode(LayoutNode* parent, const std::string& data, size_t& pos) {
    if (!parent || pos >= data.length()) return false;
    
    // This is a simplified deserialization
    // In a real implementation, you would parse the actual data structure
    
    // Skip to next node
    size_t nextNode = data.find("Node:", pos);
    if (nextNode == std::string::npos) return true;
    
    pos = nextNode;
    return true;
}

void FlexibleLayoutStrategy::InitializeDefaultParameters() {
    m_strategyParameters["autoOrganize"] = "true";
    m_strategyParameters["balanceStructure"] = "true";
    m_strategyParameters["maxPanelsPerArea"] = "5";
    m_strategyParameters["minSplitterRatio"] = "0.1";
    m_strategyParameters["maxSplitterRatio"] = "0.9";
    m_strategyParameters["enableAnimations"] = "true";
    m_strategyParameters["animationDuration"] = "300";
}


