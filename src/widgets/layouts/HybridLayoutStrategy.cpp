#include "widgets/layouts/HybridLayoutStrategy.h"
#include "widgets/layouts/IDELayoutStrategy.h"
#include "widgets/layouts/FlexibleLayoutStrategy.h"
#include "widgets/LayoutEngine.h"
#include "widgets/ModernDockPanel.h"
#include <wx/window.h>
#include <algorithm>
#include <sstream>

HybridLayoutStrategy::HybridLayoutStrategy()
	: m_hasErrors(false)
	, m_layoutCachingEnabled(true)
	, m_layoutUpdateMode(0)
	, m_autoBalanceEnabled(true)
	, m_adaptiveStrategyEnabled(true)
{
	InitializeDefaultParameters();
	InitializeStrategies();
}

void HybridLayoutStrategy::CreateLayout(LayoutNode* root) {
	if (!root) return;

	m_hasErrors = false;

	// Create hybrid layout structure
	CreateHybridLayout(root);
}

void HybridLayoutStrategy::InitializeLayout(LayoutNode* root) {
	if (!root) return;

	m_hasErrors = false;

	// Reset area maps
	m_ideAreas.clear();
	m_flexibleAreas.clear();

	// Analyze existing hybrid structure
	AnalyzeHybridStructure(root);
}

void HybridLayoutStrategy::DestroyLayout(LayoutNode* root) {
	if (!root) return;

	// Clear all state
	m_ideAreas.clear();
	m_flexibleAreas.clear();
	m_areaStrategyMap.clear();
	m_lastError.clear();
	m_hasErrors = false;
}

void HybridLayoutStrategy::CalculateLayout(LayoutNode* node, const wxRect& rect) {
	if (!node) return;

	CalculateNodeLayout(node, rect);
}

void HybridLayoutStrategy::CalculateNodeLayout(LayoutNode* node, const wxRect& rect) {
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

void HybridLayoutStrategy::CalculateSplitterLayout(LayoutNode* splitter, const wxRect& rect) {
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
	}
	else if (splitter->GetType() == LayoutNodeType::VerticalSplitter) {
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

void HybridLayoutStrategy::CalculateContainerLayout(LayoutNode* container, const wxRect& rect) {
	if (!container) return;

	// For containers, distribute space among children
	auto& children = container->GetChildren();
	if (children.empty()) return;

	if (children.size() == 1) {
		// Single child gets all space
		CalculateNodeLayout(children[0].get(), rect);
	}
	else {
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

void HybridLayoutStrategy::AddPanel(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea area) {
	if (!root || !panel) return;

	// Find or create the best insertion point
	LayoutNode* insertionPoint = FindOrCreateHybridInsertionPoint(root, area);
	if (!insertionPoint) return;

	// Place the panel using hybrid strategy
	LayoutNode* panelNode = PlacePanelHybrid(root, panel, area);
	if (panelNode) {
		// Auto-balance if enabled
		if (m_autoBalanceEnabled) {
			BalanceIDEAndFlexibleAreas(root);
		}
	}
}

LayoutNode* HybridLayoutStrategy::FindBestInsertionPoint(LayoutNode* root, UnifiedDockArea area) {
	if (!root) return nullptr;

	return FindOrCreateHybridInsertionPoint(root, area);
}

LayoutNode* HybridLayoutStrategy::CreateSplitterNode(SplitterOrientation orientation) {
	LayoutNodeType type = (orientation == SplitterOrientation::Horizontal) ?
		LayoutNodeType::HorizontalSplitter : LayoutNodeType::VerticalSplitter;
	return new LayoutNode(type);
}

LayoutNode* HybridLayoutStrategy::CreateContainerNode() {
	return new LayoutNode(LayoutNodeType::Root);
}

void HybridLayoutStrategy::InsertNode(LayoutNode* parent, std::unique_ptr<LayoutNode> child) {
	if (parent && child) {
		parent->AddChild(std::move(child));
	}
}

void HybridLayoutStrategy::RemoveNode(LayoutNode* parent, LayoutNode* child) {
	if (parent && child) {
		parent->RemoveChild(child);
	}
}

void HybridLayoutStrategy::RemovePanel(LayoutNode* root, ModernDockPanel* panel) {
	if (!root || !panel) return;

	// Find the node containing this panel
	LayoutNode* panelNode = FindPanelNode(root, panel);
	if (!panelNode) return;

	LayoutNode* parent = panelNode->GetParent();
	if (!parent) return;

	// Remove the panel node from its parent
	parent->RemoveChild(panelNode);

	// Clean up empty containers or splitters with only one child
	CompactLayout(root);
}

void HybridLayoutStrategy::MovePanel(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea newArea) {
	if (!root || !panel) return;

	// Remove from current location
	RemovePanel(root, panel);

	// Add to new area
	AddPanel(root, panel, newArea);
}

void HybridLayoutStrategy::SwapPanels(LayoutNode* root, ModernDockPanel* panel1, ModernDockPanel* panel2) {
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

bool HybridLayoutStrategy::ValidateLayout(LayoutNode* root) const {
	if (!root) return false;

	// Check if all panels are accessible
	bool hasPanels = false;
	TraversePanels(root, [&hasPanels](ModernDockPanel*) { hasPanels = true; });

	// Check if structure is intact
	return hasPanels && root->GetChildren().size() > 0;
}

bool HybridLayoutStrategy::CanAddPanel(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea area) const {
	if (!root || !panel) return false;

	// Check if area is available based on strategy
	if (ShouldUseIDEStrategy(area)) {
		return m_ideStrategy->CanAddPanel(root, panel, area);
	}
	else {
		return m_flexibleStrategy->CanAddPanel(root, panel, area);
	}
}

bool HybridLayoutStrategy::CanRemovePanel(LayoutNode* root, ModernDockPanel* panel) const {
	if (!root || !panel) return false;

	// Both strategies should support panel removal
	return m_ideStrategy->CanRemovePanel(root, panel) ||
		m_flexibleStrategy->CanRemovePanel(root, panel);
}

bool HybridLayoutStrategy::CanMovePanel(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea newArea) const {
	if (!root || !panel) return false;

	// Check if panel exists and new area is available
	return CanRemovePanel(root, panel) && CanAddPanel(root, panel, newArea);
}

void HybridLayoutStrategy::OptimizeLayout(LayoutNode* root) {
	if (!root) return;

	// Analyze current hybrid structure
	AnalyzeHybridStructure(root);

	// Optimize IDE areas
	for (auto* ideArea : m_ideAreas) {
		m_ideStrategy->OptimizeLayout(ideArea);
	}

	// Optimize flexible areas
	for (auto* flexibleArea : m_flexibleAreas) {
		m_flexibleStrategy->OptimizeLayout(flexibleArea);
	}

	// Balance between IDE and flexible areas
	BalanceIDEAndFlexibleAreas(root);
}

void HybridLayoutStrategy::CompactLayout(LayoutNode* root) {
	if (!root) return;

	// Compact IDE areas
	for (auto* ideArea : m_ideAreas) {
		m_ideStrategy->CompactLayout(ideArea);
	}

	// Compact flexible areas
	for (auto* flexibleArea : m_flexibleAreas) {
		m_flexibleStrategy->CompactLayout(flexibleArea);
	}

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
}

void HybridLayoutStrategy::BalanceSplitters(LayoutNode* root) {
	if (!root) return;

	// Balance IDE area splitters
	for (auto* ideArea : m_ideAreas) {
		m_ideStrategy->BalanceSplitters(ideArea);
	}

	// Balance flexible area splitters
	for (auto* flexibleArea : m_flexibleAreas) {
		m_flexibleStrategy->BalanceSplitters(flexibleArea);
	}
}

void HybridLayoutStrategy::MinimizeEmptySpace(LayoutNode* root) {
	if (!root) return;

	// Minimize empty space in all areas
	CompactLayout(root);

	// Optimize each area type
	for (auto* ideArea : m_ideAreas) {
		m_ideStrategy->MinimizeEmptySpace(ideArea);
	}

	for (auto* flexibleArea : m_flexibleAreas) {
		m_flexibleStrategy->MinimizeEmptySpace(flexibleArea);
	}
}

void HybridLayoutStrategy::SetSplitterRatio(LayoutNode* splitter, double ratio) {
	if (splitter) {
		splitter->SetSplitterRatio(ratio);
	}
}

double HybridLayoutStrategy::GetSplitterRatio(LayoutNode* splitter) const {
	return splitter ? splitter->GetSplitterRatio() : 0.5;
}

void HybridLayoutStrategy::SetSplitterPosition(LayoutNode* splitter, int position) {
	if (splitter) {
		splitter->SetSashPosition(position);
	}
}

int HybridLayoutStrategy::GetSplitterPosition(LayoutNode* splitter) const {
	return splitter ? splitter->GetSashPosition() : 0;
}

void HybridLayoutStrategy::SetSplitterSashSize(LayoutNode* splitter, int size) {
	wxUnusedVar(splitter);
	wxUnusedVar(size);
}

int HybridLayoutStrategy::GetSplitterSashSize(LayoutNode* splitter) const {
	wxUnusedVar(splitter);
	return 4; // Default sash size
}

void HybridLayoutStrategy::SetContainerTabPosition(LayoutNode* container, int position) {
	wxUnusedVar(container);
	wxUnusedVar(position);
}

int HybridLayoutStrategy::GetContainerTabPosition(LayoutNode* container) const {
	wxUnusedVar(container);
	return 0; // 0 = top, 1 = bottom, 2 = left, 3 = right
}

void HybridLayoutStrategy::SetContainerTabStyle(LayoutNode* container, int style) {
	wxUnusedVar(container);
	wxUnusedVar(style);
}

int HybridLayoutStrategy::GetContainerTabStyle(LayoutNode* container) const {
	wxUnusedVar(container);
	return 0;
}

void HybridLayoutStrategy::GetMinimumSize(LayoutNode* root, int& width, int& height) const {
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

void HybridLayoutStrategy::GetBestSize(LayoutNode* root, int& width, int& height) const {
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

void HybridLayoutStrategy::GetPanelBounds(LayoutNode* root, ModernDockPanel* panel, int& x, int& y, int& width, int& height) const {
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

UnifiedDockArea HybridLayoutStrategy::GetPanelArea(LayoutNode* root, ModernDockPanel* panel) const {
	if (!root || !panel) return UnifiedDockArea::Center;

	// Find the panel node
	LayoutNode* panelNode = FindPanelNode(root, panel);
	if (!panelNode) return UnifiedDockArea::Center;

	// Try to determine area based on position in the tree
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

int HybridLayoutStrategy::GetPanelDepth(LayoutNode* root, ModernDockPanel* panel) const {
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

void HybridLayoutStrategy::TraversePanels(LayoutNode* root,
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

void HybridLayoutStrategy::TraverseSplitters(LayoutNode* root,
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

void HybridLayoutStrategy::TraverseContainers(LayoutNode* root,
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

void HybridLayoutStrategy::TraverseNodes(LayoutNode* root,
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

std::string HybridLayoutStrategy::SerializeLayout(LayoutNode* root) const {
	if (!root) return "";

	std::string result = "HybridLayout:{\n";
	SerializeNode(root, result, 0);
	result += "}";
	return result;
}

bool HybridLayoutStrategy::DeserializeLayout(LayoutNode* root, const std::string& data) {
	if (!root || data.empty()) return false;

	// Check if this is our format
	if (data.find("HybridLayout:{") != 0) return false;

	try {
		// Clear existing layout
		auto& children = root->GetChildren();
		children.clear();

		// Reset state
		m_ideAreas.clear();
		m_flexibleAreas.clear();

		// Parse the layout data
		size_t pos = data.find('{') + 1;
		if (!DeserializeNode(root, data, pos)) {
			m_lastError = "Failed to parse layout data";
			return false;
		}

		m_lastError.clear();
		return true;
	}
	catch (...) {
		m_lastError = "Failed to deserialize layout";
		return false;
	}
}

std::string HybridLayoutStrategy::ExportLayout(LayoutNode* root, const std::string& format) const {
	wxUnusedVar(root);
	wxUnusedVar(format);
	return "";
}

bool HybridLayoutStrategy::ImportLayout(LayoutNode* root, const std::string& data, const std::string& format) {
	wxUnusedVar(format);
	return DeserializeLayout(root, data);
}

bool HybridLayoutStrategy::IsLayoutEqual(LayoutNode* root1, LayoutNode* root2) const {
	if (!root1 || !root2) return false;

	// Simple comparison - check if both have the same number of panels
	int panels1 = 0, panels2 = 0;
	TraversePanels(root1, [&panels1](ModernDockPanel*) { panels1++; });
	TraversePanels(root2, [&panels2](ModernDockPanel*) { panels2++; });

	return panels1 == panels2;
}

bool HybridLayoutStrategy::CanMergeLayouts(LayoutNode* root1, LayoutNode* root2) const {
	if (!root1 || !root2) return false;

	// Check if layouts can be merged (not too many panels)
	int totalPanels = 0;
	TraversePanels(root1, [&totalPanels](ModernDockPanel*) { totalPanels++; });
	TraversePanels(root2, [&totalPanels](ModernDockPanel*) { totalPanels++; });

	return totalPanels <= 15; // Higher limit for hybrid strategy
}

void HybridLayoutStrategy::MergeLayouts(LayoutNode* target, LayoutNode* source) {
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

void HybridLayoutStrategy::EnableLayoutCaching(bool enable) {
	m_layoutCachingEnabled = enable;
}

bool HybridLayoutStrategy::IsLayoutCachingEnabled() const {
	return m_layoutCachingEnabled;
}

void HybridLayoutStrategy::ClearLayoutCache() {
	// Clear optimization cache
	m_ideAreas.clear();
	m_flexibleAreas.clear();
}

void HybridLayoutStrategy::SetLayoutUpdateMode(int mode) {
	m_layoutUpdateMode = mode;
}

int HybridLayoutStrategy::GetLayoutUpdateMode() const {
	return m_layoutUpdateMode;
}

std::string HybridLayoutStrategy::GetLastError() const {
	return m_lastError;
}

void HybridLayoutStrategy::ClearLastError() {
	m_lastError.clear();
	m_hasErrors = false;
}

bool HybridLayoutStrategy::HasErrors() const {
	return m_hasErrors;
}

void HybridLayoutStrategy::DumpLayoutDebugInfo(LayoutNode* root) const {
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

void HybridLayoutStrategy::SetStrategyParameter(const std::string& name, const std::string& value) {
	m_strategyParameters[name] = value;
}

std::string HybridLayoutStrategy::GetStrategyParameter(const std::string& name) const {
	auto it = m_strategyParameters.find(name);
	if (it != m_strategyParameters.end()) {
		return it->second;
	}
	return "";
}

std::vector<std::string> HybridLayoutStrategy::GetAvailableParameters() const {
	std::vector<std::string> params;
	for (const auto& param : m_strategyParameters) {
		params.push_back(param.first);
	}
	return params;
}

void HybridLayoutStrategy::ResetToDefaultParameters() {
	InitializeDefaultParameters();
}

// Private method implementations

void HybridLayoutStrategy::CreateHybridLayout(LayoutNode* root) {
	if (!root) return;

	// Create IDE structure first
	CreateIDEStructure(root);

	// Create flexible areas
	CreateFlexibleAreas(root);
}

void HybridLayoutStrategy::CreateIDEStructure(LayoutNode* root) {
	if (!root) return;

	// Use IDE strategy to create the main structure
	m_ideStrategy->CreateLayout(root);

	// Mark IDE areas
	AnalyzeHybridStructure(root);
}

void HybridLayoutStrategy::CreateFlexibleAreas(LayoutNode* root) {
	if (!root) return;

	// Create flexible areas for dynamic content
	// This will be populated as panels are added
}

LayoutNode* HybridLayoutStrategy::FindOrCreateHybridInsertionPoint(LayoutNode* root, UnifiedDockArea area) {
	if (!root) return nullptr;

	// Determine which strategy to use for this area
	if (ShouldUseIDEStrategy(area)) {
		return m_ideStrategy->FindBestInsertionPoint(root, area);
	}
	else {
		return m_flexibleStrategy->FindBestInsertionPoint(root, area);
	}
}

bool HybridLayoutStrategy::ShouldUseIDEStrategy(UnifiedDockArea area) const {
	// Use IDE strategy for common, stable areas
	switch (area) {
	case UnifiedDockArea::Left:      // Object tree, properties
	case UnifiedDockArea::Center:    // Main canvas
	case UnifiedDockArea::Bottom:    // Status bar, output
		return true;
	case UnifiedDockArea::Right:     // Dynamic tool panels
	case UnifiedDockArea::Top:       // Dynamic toolbars
	case UnifiedDockArea::Tab:       // Tabbed containers
	case UnifiedDockArea::Floating:  // Floating windows
		return false;
	default:
		return true;
	}
}

bool HybridLayoutStrategy::ShouldUseFlexibleStrategy(UnifiedDockArea area) const {
	return !ShouldUseIDEStrategy(area);
}

UnifiedDockArea HybridLayoutStrategy::GetStrategyForArea(UnifiedDockArea area) const {
	return ShouldUseIDEStrategy(area) ? UnifiedDockArea::Left : UnifiedDockArea::Right;
}

LayoutNode* HybridLayoutStrategy::PlacePanelHybrid(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea area) {
	if (!root || !panel) return nullptr;

	if (ShouldUseIDEStrategy(area)) {
		return PlacePanelInIDEArea(root, panel, area);
	}
	else {
		return PlacePanelInFlexibleArea(root, panel, area);
	}
}

LayoutNode* HybridLayoutStrategy::PlacePanelInIDEArea(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea area) {
	if (!root || !panel) return nullptr;

	// Use IDE strategy for placement
	m_ideStrategy->AddPanel(root, panel, area);

	// Find the placed panel node
	return FindPanelNode(root, panel);
}

LayoutNode* HybridLayoutStrategy::PlacePanelInFlexibleArea(LayoutNode* root, ModernDockPanel* panel, UnifiedDockArea area) {
	if (!root || !panel) return nullptr;

	// Use flexible strategy for placement
	m_flexibleStrategy->AddPanel(root, panel, area);

	// Find the placed panel node
	return FindPanelNode(root, panel);
}

void HybridLayoutStrategy::AnalyzeHybridStructure(LayoutNode* root) {
	if (!root) return;

	// Reset area maps
	m_ideAreas.clear();
	m_flexibleAreas.clear();

	// Analyze the tree structure to identify IDE vs flexible areas
	TraverseNodes(root, [this](LayoutNode* node) {
		if (node->GetType() == LayoutNodeType::Root) {
			// Determine if this is an IDE or flexible area based on content
			bool hasIDEStructure = false;
			bool hasFlexibleStructure = false;

			// Simple heuristic: check for common IDE patterns
			if (node->GetChildren().size() > 0) {
				auto& firstChild = node->GetChildren()[0];
				if (firstChild && firstChild->GetType() == LayoutNodeType::VerticalSplitter) {
					hasIDEStructure = true;
				}
			}

			if (hasIDEStructure) {
				m_ideAreas.insert(node);
			}
			else {
				m_flexibleAreas.insert(node);
			}
		}
		});
}

void HybridLayoutStrategy::OptimizeHybridLayout(LayoutNode* root) {
	if (!root) return;

	// Optimize each area type using its appropriate strategy
	for (auto* ideArea : m_ideAreas) {
		m_ideStrategy->OptimizeLayout(ideArea);
	}

	for (auto* flexibleArea : m_flexibleAreas) {
		m_flexibleStrategy->OptimizeLayout(flexibleArea);
	}
}

void HybridLayoutStrategy::BalanceIDEAndFlexibleAreas(LayoutNode* root) {
	if (!root || !m_autoBalanceEnabled) return;

	// Balance the distribution of space between IDE and flexible areas
	// This is a simplified implementation
	wxUnusedVar(root);
}

LayoutNode* HybridLayoutStrategy::FindPanelNode(LayoutNode* root, ModernDockPanel* panel) const {
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

LayoutNode* HybridLayoutStrategy::FindIDEArea(LayoutNode* root, UnifiedDockArea area) const {
	// Find IDE area for the specified dock area
	wxUnusedVar(root);
	wxUnusedVar(area);
	return nullptr;
}

LayoutNode* HybridLayoutStrategy::FindFlexibleArea(LayoutNode* root, UnifiedDockArea area) const {
	// Find flexible area for the specified dock area
	wxUnusedVar(root);
	wxUnusedVar(area);
	return nullptr;
}

bool HybridLayoutStrategy::IsIDEArea(LayoutNode* node) const {
	return m_ideAreas.find(node) != m_ideAreas.end();
}

bool HybridLayoutStrategy::IsFlexibleArea(LayoutNode* node) const {
	return m_flexibleAreas.find(node) != m_flexibleAreas.end();
}

void HybridLayoutStrategy::SerializeNode(LayoutNode* node, std::string& result, int indent) const {
	if (!node) return;

	std::string indentStr(indent * 2, ' ');

	// Add node type
	result += indentStr + "Node: ";
	switch (node->GetType()) {
	case LayoutNodeType::Root:
		result += "Root";
		if (IsIDEArea(node)) {
			result += " (IDE Area)";
		}
		else if (IsFlexibleArea(node)) {
			result += " (Flexible Area)";
		}
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

bool HybridLayoutStrategy::DeserializeNode(LayoutNode* parent, const std::string& data, size_t& pos) {
	if (!parent || pos >= data.length()) return false;

	// This is a simplified deserialization
	// In a real implementation, you would parse the actual data structure

	// Skip to next node
	size_t nextNode = data.find("Node:", pos);
	if (nextNode == std::string::npos) return true;

	pos = nextNode;
	return true;
}

void HybridLayoutStrategy::InitializeDefaultParameters() {
	m_strategyParameters["autoBalance"] = "true";
	m_strategyParameters["adaptiveStrategy"] = "true";
	m_strategyParameters["ideAreaRatio"] = "0.7";
	m_strategyParameters["flexibleAreaRatio"] = "0.3";
	m_strategyParameters["enableAnimations"] = "true";
	m_strategyParameters["animationDuration"] = "300";
	m_strategyParameters["strategyThreshold"] = "5";
}

void HybridLayoutStrategy::InitializeStrategies() {
	// Create strategy instances
	m_ideStrategy = std::make_unique<IDELayoutStrategy>();
	m_flexibleStrategy = std::make_unique<FlexibleLayoutStrategy>();
}