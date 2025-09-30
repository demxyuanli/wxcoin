#ifndef FLAT_TREE_BUILDER_H
#define FLAT_TREE_BUILDER_H

#include <wx/wx.h>
#include <vector>
#include <memory>
#include <map>
#include "widgets/FlatTreeView.h"

// Forward declarations
class FlatTreeView;
class FlatTreeItem;

// Tree builder for constructing tree structures
class FlatTreeBuilder
{
public:
	// Node data structure for building
	struct TreeNode {
		wxString text;
		FlatTreeItem::ItemType type;
		wxString iconName;
		std::map<int, wxString> columnData;
		std::map<int, wxString> columnIcons;
		bool expanded;
		bool visible;
		std::vector<std::unique_ptr<TreeNode>> children;
		
		TreeNode(const wxString& t, FlatTreeItem::ItemType ty = FlatTreeItem::ItemType::FOLDER)
			: text(t), type(ty), expanded(false), visible(true) {}
		
		TreeNode* addChild(const wxString& childText, FlatTreeItem::ItemType childType = FlatTreeItem::ItemType::FILE) {
			auto child = std::make_unique<TreeNode>(childText, childType);
			TreeNode* ptr = child.get();
			children.push_back(std::move(child));
			return ptr;
		}
		
		TreeNode* addChild(std::unique_ptr<TreeNode> child) {
			TreeNode* ptr = child.get();
			children.push_back(std::move(child));
			return ptr;
		}
		
		void setColumnData(int column, const wxString& data) {
			columnData[column] = data;
		}
		
		void setColumnIcon(int column, const wxString& iconName) {
			columnIcons[column] = iconName;
		}
	};

	FlatTreeBuilder(FlatTreeView* treeView);
	~FlatTreeBuilder();

	// Building methods
	TreeNode* createRoot(const wxString& text, FlatTreeItem::ItemType type = FlatTreeItem::ItemType::ROOT);
	TreeNode* addNode(TreeNode* parent, const wxString& text, FlatTreeItem::ItemType type = FlatTreeItem::ItemType::FILE);
	TreeNode* addFolder(TreeNode* parent, const wxString& text, bool expanded = false);
	TreeNode* addFile(TreeNode* parent, const wxString& text);
	
	// Batch operations
	void addNodes(TreeNode* parent, const std::vector<wxString>& texts, FlatTreeItem::ItemType type = FlatTreeItem::ItemType::FILE);
	void addFileGroup(TreeNode* parent, const wxString& folderName, const std::vector<wxString>& fileNames, bool expanded = true);
	
	// Configuration methods
	TreeNode* setNodeIcon(TreeNode* node, const wxString& icon);
	TreeNode* setNodeColumnData(TreeNode* node, int column, const wxString& data);
	TreeNode* setNodeColumnIcon(TreeNode* node, int column, const wxString& icon);
	TreeNode* setNodeExpanded(TreeNode* node, bool expanded);
	TreeNode* setNodeVisible(TreeNode* node, bool visible);
	
	// Build and apply
	void build();
	void clear();
	
	// Utility methods
	TreeNode* findNode(const wxString& text, TreeNode* startFrom = nullptr);
	TreeNode* getRoot() const { return m_root.get(); }
	int getNodeCount() const;
	int getNodeCountRecursive(TreeNode* node) const;

private:
	FlatTreeView* m_treeView;
	std::unique_ptr<TreeNode> m_root;
	
	// Helper methods
	std::shared_ptr<FlatTreeItem> convertToTreeItem(TreeNode* node);
	void convertRecursive(TreeNode* node, std::shared_ptr<FlatTreeItem> parentItem);
};

#endif // FLAT_TREE_BUILDER_H
