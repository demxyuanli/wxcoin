#include "widgets/FlatTreeBuilder.h"
#include "widgets/FlatTreeView.h"
#include "logger/Logger.h"

// FlatTreeBuilder implementation
FlatTreeBuilder::FlatTreeBuilder(FlatTreeView* treeView)
	: m_treeView(treeView)
{
}

FlatTreeBuilder::~FlatTreeBuilder()
{
}

FlatTreeBuilder::TreeNode* FlatTreeBuilder::createRoot(const wxString& text, FlatTreeItem::ItemType type)
{
	m_root = std::make_unique<TreeNode>(text, type);
	return m_root.get();
}

FlatTreeBuilder::TreeNode* FlatTreeBuilder::addNode(TreeNode* parent, const wxString& text, FlatTreeItem::ItemType type)
{
	if (!parent) return nullptr;
	return parent->addChild(text, type);
}

FlatTreeBuilder::TreeNode* FlatTreeBuilder::addFolder(TreeNode* parent, const wxString& text, bool expanded)
{
	if (!parent) return nullptr;
	auto node = parent->addChild(text, FlatTreeItem::ItemType::FOLDER);
	node->expanded = expanded;
	return node;
}

FlatTreeBuilder::TreeNode* FlatTreeBuilder::addFile(TreeNode* parent, const wxString& text)
{
	if (!parent) return nullptr;
	return parent->addChild(text, FlatTreeItem::ItemType::FILE);
}

void FlatTreeBuilder::addNodes(TreeNode* parent, const std::vector<wxString>& texts, FlatTreeItem::ItemType type)
{
	if (!parent) return;
	for (const auto& text : texts) {
		parent->addChild(text, type);
	}
}

void FlatTreeBuilder::addFileGroup(TreeNode* parent, const wxString& folderName, const std::vector<wxString>& fileNames, bool expanded)
{
	if (!parent) return;
	auto folder = addFolder(parent, folderName, expanded);
	for (const auto& fileName : fileNames) {
		addFile(folder, fileName);
	}
}

FlatTreeBuilder::TreeNode* FlatTreeBuilder::setNodeIcon(TreeNode* node, const wxString& icon)
{
	if (node) {
		node->iconName = icon;
	}
	return node;
}

FlatTreeBuilder::TreeNode* FlatTreeBuilder::setNodeColumnData(TreeNode* node, int column, const wxString& data)
{
	if (node) {
		node->setColumnData(column, data);
	}
	return node;
}

FlatTreeBuilder::TreeNode* FlatTreeBuilder::setNodeColumnIcon(TreeNode* node, int column, const wxString& icon)
{
	if (node) {
		node->setColumnIcon(column, icon);
	}
	return node;
}

FlatTreeBuilder::TreeNode* FlatTreeBuilder::setNodeExpanded(TreeNode* node, bool expanded)
{
	if (node) {
		node->expanded = expanded;
	}
	return node;
}

FlatTreeBuilder::TreeNode* FlatTreeBuilder::setNodeVisible(TreeNode* node, bool visible)
{
	if (node) {
		node->visible = visible;
	}
	return node;
}

void FlatTreeBuilder::build()
{
	if (!m_treeView || !m_root) return;
	
	// Clear existing tree
	m_treeView->Clear();
	
	// Convert builder tree to FlatTreeView tree
	auto rootItem = convertToTreeItem(m_root.get());
	if (rootItem) {
		m_treeView->SetRoot(rootItem);
	}
}

void FlatTreeBuilder::clear()
{
	m_root.reset();
}

FlatTreeBuilder::TreeNode* FlatTreeBuilder::findNode(const wxString& text, TreeNode* startFrom)
{
	TreeNode* searchRoot = startFrom ? startFrom : m_root.get();
	if (!searchRoot) return nullptr;
	
	if (searchRoot->text == text) {
		return searchRoot;
	}
	
	for (auto& child : searchRoot->children) {
		TreeNode* found = findNode(text, child.get());
		if (found) return found;
	}
	
	return nullptr;
}

int FlatTreeBuilder::getNodeCount() const
{
	return getNodeCountRecursive(m_root.get());
}

int FlatTreeBuilder::getNodeCountRecursive(TreeNode* node) const
{
	if (!node) return 0;
	
	int count = 1; // Count this node
	for (const auto& child : node->children) {
		count += getNodeCountRecursive(child.get());
	}
	
	return count;
}

std::shared_ptr<FlatTreeItem> FlatTreeBuilder::convertToTreeItem(TreeNode* node)
{
	if (!node) return nullptr;
	
	auto item = std::make_shared<FlatTreeItem>(node->text, node->type);
	item->SetExpanded(node->expanded);
	item->SetVisible(node->visible);
	
	// Set icon if specified
	if (!node->iconName.IsEmpty()) {
		m_treeView->SetItemSvgIcon(item, node->iconName, wxSize(16, 16));
	}
	
	// Set column data
	for (const auto& colData : node->columnData) {
		item->SetColumnData(colData.first, colData.second);
	}
	
	// Set column icons
	for (const auto& colIcon : node->columnIcons) {
		m_treeView->SetItemColumnSvgIcon(item, colIcon.first, colIcon.second, wxSize(16, 16));
	}
	
	// Convert children
	for (const auto& child : node->children) {
		auto childItem = convertToTreeItem(child.get());
		if (childItem) {
			item->AddChild(childItem);
		}
	}
	
	return item;
}

void FlatTreeBuilder::convertRecursive(TreeNode* node, std::shared_ptr<FlatTreeItem> parentItem)
{
	if (!node || !parentItem) return;
	
	auto item = convertToTreeItem(node);
	if (item) {
		parentItem->AddChild(item);
	}
}
