#ifndef FLAT_TREE_VIEW_H
#define FLAT_TREE_VIEW_H

#include <wx/wx.h>
#include <wx/control.h>
#include <wx/scrolwin.h>
#include <wx/scrolbar.h>
#include <wx/bitmap.h>
#include <wx/dc.h>
#include <vector>
#include <memory>
#include <functional>
#include <map>

// Forward declarations
class FlatTreeView;
class FlatTreeItem;
class FlatTreeColumn;

// Custom events
wxDECLARE_EVENT(wxEVT_FLAT_TREE_ITEM_SELECTED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_FLAT_TREE_ITEM_EXPANDED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_FLAT_TREE_ITEM_COLLAPSED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_FLAT_TREE_ITEM_CLICKED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_FLAT_TREE_COLUMN_CLICKED, wxCommandEvent);

// Tree item data structure
class FlatTreeItem
{
public:
	enum class ItemType {
		ROOT,
		FOLDER,
		FILE,
		SKETCH,
		BODY,
		PAD,
		ORIGIN,
		REFERENCE
	};

	FlatTreeItem(const wxString& text, ItemType type = ItemType::FOLDER);
	virtual ~FlatTreeItem();

	// Basic properties
	void SetText(const wxString& text);
	wxString GetText() const { return m_text; }

	void SetType(ItemType type);
	ItemType GetType() const { return m_type; }

	void SetIcon(const wxBitmap& icon);
	wxBitmap GetIcon() const { return m_icon; }

	void SetVisible(bool visible);
	bool IsVisible() const { return m_visible; }

	void SetSelected(bool selected);
	bool IsSelected() const { return m_selected; }

	void SetExpanded(bool expanded);
	bool IsExpanded() const { return m_expanded; }

	// Hierarchy management
	void AddChild(std::shared_ptr<FlatTreeItem> child);
	void RemoveChild(std::shared_ptr<FlatTreeItem> child);
	void ClearChildren();

	std::vector<std::shared_ptr<FlatTreeItem>>& GetChildren() { return m_children; }
	const std::vector<std::shared_ptr<FlatTreeItem>>& GetChildren() const { return m_children; }

	void SetParent(FlatTreeItem* parent);
	FlatTreeItem* GetParent() const { return m_parent; }

	// Column data
	void SetColumnData(int column, const wxString& data);
	wxString GetColumnData(int column) const;

	void SetColumnIcon(int column, const wxBitmap& icon);
	wxBitmap GetColumnIcon(int column) const;

	// Utility methods
	bool HasChildren() const { return !m_children.empty(); }
	int GetLevel() const;
	bool IsRoot() const { return m_parent == nullptr; }

private:
	wxString m_text;
	ItemType m_type;
	wxBitmap m_icon;
	bool m_visible;
	bool m_selected;
	bool m_expanded;

	FlatTreeItem* m_parent;
	std::vector<std::shared_ptr<FlatTreeItem>> m_children;

	// Column data storage
	std::map<int, wxString> m_columnData;
	std::map<int, wxBitmap> m_columnIcons;
};

// Column definition
class FlatTreeColumn
{
public:
	enum class ColumnType {
		TREE,           // Tree structure column
		TEXT,           // Text only
		ICON,           // Icon only
		BUTTON,         // Button
		CHECKBOX,       // Checkbox
		COMBOBOX        // Combobox
	};

	FlatTreeColumn(const wxString& title, ColumnType type = ColumnType::TEXT, int width = 100);

	void SetTitle(const wxString& title);
	wxString GetTitle() const { return m_title; }

	void SetType(ColumnType type);
	ColumnType GetType() const { return m_type; }

	void SetWidth(int width);
	int GetWidth() const { return m_width; }

	void SetVisible(bool visible);
	bool IsVisible() const { return m_visible; }

	void SetSortable(bool sortable);
	bool IsSortable() const { return m_sortable; }

private:
	wxString m_title;
	ColumnType m_type;
	int m_width;
	bool m_visible;
	bool m_sortable;
};

// Main tree view control
class FlatTreeView : public wxScrolledWindow
{
public:
	FlatTreeView(wxWindow* parent,
		wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0);

	virtual ~FlatTreeView();

	// Tree structure management
	void SetRoot(std::shared_ptr<FlatTreeItem> root);
	std::shared_ptr<FlatTreeItem> GetRoot() const { return m_root; }

	void AddItem(std::shared_ptr<FlatTreeItem> parent, std::shared_ptr<FlatTreeItem> child);
	void RemoveItem(std::shared_ptr<FlatTreeItem> item);
	void Clear();

	// Column management
	void AddColumn(const wxString& title, FlatTreeColumn::ColumnType type = FlatTreeColumn::ColumnType::TEXT, int width = 100);
	void RemoveColumn(int index);
	void ClearColumns();

	void SetColumnWidth(int column, int width);
	int GetColumnWidth(int column) const;

	// Selection and expansion
	void SelectItem(std::shared_ptr<FlatTreeItem> item, bool select = true);
	void ExpandItem(std::shared_ptr<FlatTreeItem> item, bool expand = true);
	void ExpandAll();
	void CollapseAll();

	// Item finding
	std::shared_ptr<FlatTreeItem> FindItem(const wxString& text, std::shared_ptr<FlatTreeItem> startFrom = nullptr);
	std::vector<std::shared_ptr<FlatTreeItem>> GetSelectedItems() const;

	// Visual customization
	void SetItemHeight(int height);
	int GetItemHeight() const { return m_itemHeight; }

	void SetIndentWidth(int width);
	int GetIndentWidth() const { return m_indentWidth; }

	void SetShowLines(bool show);
	bool IsShowingLines() const { return m_showLines; }

	void SetShowRootLines(bool show);
	bool IsShowingRootLines() const { return m_showRootLines; }

	// Colors and styling
	void SetBackgroundColor(const wxColour& color);
	wxColour GetBackgroundColor() const { return m_backgroundColor; }

	void SetTextColor(const wxColour& color);
	wxColour GetTextColor() const { return m_textColor; }

	void SetSelectionColor(const wxColour& color);
	wxColour GetSelectionColor() const { return m_selectionColor; }

	void SetLineColor(const wxColour& color);
	wxColour GetLineColor() const { return m_lineColor; }

	// Font control (consistent with other flat widgets)
	void SetCustomFont(const wxFont& font);
	wxFont GetCustomFont() const { return m_customFont; }
	void UseConfigFont(bool useConfig = true);
	bool IsUsingConfigFont() const { return m_useConfigFont; }
	void ReloadFontFromConfig();

	// SVG icon support
	void SetSvgIcon(const wxString& iconName, const wxSize& size = wxSize(16, 16));
	void SetColumnSvgIcon(int column, const wxString& iconName, const wxSize& size = wxSize(16, 16));
	void SetItemSvgIcon(std::shared_ptr<FlatTreeItem> item, const wxString& iconName, const wxSize& size = wxSize(16, 16));
	void SetItemColumnSvgIcon(std::shared_ptr<FlatTreeItem> item, int column, const wxString& iconName, const wxSize& size = wxSize(16, 16));
	wxString GetSvgIconName() const { return m_svgIconName; }
	wxString GetColumnSvgIconName(int column) const;
	wxString GetItemSvgIconName(std::shared_ptr<FlatTreeItem> item) const;
	wxString GetItemColumnSvgIconName(std::shared_ptr<FlatTreeItem> item, int column) const;

	// Header display control
	void SetShowHeaderText(bool show) { m_showHeaderText = show; Refresh(); }
	bool IsShowingHeaderText() const { return m_showHeaderText; }

	// Scrollbar control
	void SetAlwaysShowScrollbars(bool alwaysShow) { m_alwaysShowScrollbars = alwaysShow; Refresh(); }
	bool IsAlwaysShowingScrollbars() const { return m_alwaysShowScrollbars; }

	// Event handlers
	void OnItemClicked(std::function<void(std::shared_ptr<FlatTreeItem>, int)> callback);
	void OnItemExpanded(std::function<void(std::shared_ptr<FlatTreeItem>)> callback);
	void OnItemCollapsed(std::function<void(std::shared_ptr<FlatTreeItem>)> callback);

public:
	// Event handling
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnMouse(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnScroll(wxScrollWinEvent& event);
	void OnEraseBackground(wxEraseEvent& event);

	// Drawing methods
	void DrawBackground(wxDC& dc);
	void DrawItems(wxDC& dc);
	void DrawItem(wxDC& dc, std::shared_ptr<FlatTreeItem> item, int y, int level);
	void DrawTreeColumnContent(wxDC& dc, std::shared_ptr<FlatTreeItem> item, int x, int y, int level);
	void DrawColumnContent(wxDC& dc, std::shared_ptr<FlatTreeItem> item, int columnIndex, int x, int y);
	void DrawTreeLines(wxDC& dc, std::shared_ptr<FlatTreeItem> item, int y, int level);
	void DrawColumnHeaders(wxDC& dc);

	// Hit testing
	std::shared_ptr<FlatTreeItem> HitTest(const wxPoint& point);
	int HitTestColumn(const wxPoint& point);
	int HitTestHeaderSeparator(const wxPoint& point) const;

	// Layout calculations
	void CalculateLayout();
	int CalculateItemY(std::shared_ptr<FlatTreeItem> item);
	void UpdateScrollbars();
	void UpdateTreeHScrollBar();
	void RepositionTreeHScrollBar();
	int CalculateTreeContentWidth();
	int CalculateItemHeightRecursive(std::shared_ptr<FlatTreeItem> item);
	void DrawItemRecursive(wxDC& dc, std::shared_ptr<FlatTreeItem> item, int& y, int level);
	int CalculateItemYRecursive(std::shared_ptr<FlatTreeItem> current, std::shared_ptr<FlatTreeItem> target, int& y);
	std::shared_ptr<FlatTreeItem> GetItemByIndex(std::shared_ptr<FlatTreeItem> item, int& index);
	void ExpandAllRecursive(std::shared_ptr<FlatTreeItem> item);
	void CollapseAllRecursive(std::shared_ptr<FlatTreeItem> item);
	void CollectSelectedItems(std::shared_ptr<FlatTreeItem> item, std::vector<std::shared_ptr<FlatTreeItem>>& selected) const;

	// Utility methods
	void RefreshItem(std::shared_ptr<FlatTreeItem> item);
	void EnsureVisible(std::shared_ptr<FlatTreeItem> item);
	void InvalidateItem(std::shared_ptr<FlatTreeItem> item);
	void RefreshContentArea();

private:
	// Data members
	std::shared_ptr<FlatTreeItem> m_root;
	std::vector<std::unique_ptr<FlatTreeColumn>> m_columns;

	// Visual properties
	int m_itemHeight;
	int m_indentWidth;
	bool m_showLines;
	bool m_showRootLines;

	// Colors
	wxColour m_backgroundColor;
	wxColour m_textColor;
	wxColour m_selectionColor;
	wxColour m_lineColor;

	// Layout and scrolling
	int m_scrollY;
	int m_scrollX;
	int m_totalHeight;
	bool m_needsLayout;

	// Event callbacks
	std::function<void(std::shared_ptr<FlatTreeItem>, int)> m_itemClickedCallback;
	std::function<void(std::shared_ptr<FlatTreeItem>)> m_itemExpandedCallback;
	std::function<void(std::shared_ptr<FlatTreeItem>)> m_itemCollapsedCallback;

	// Mouse state
	std::shared_ptr<FlatTreeItem> m_hoveredItem;
	std::shared_ptr<FlatTreeItem> m_lastClickedItem;

	// First column horizontal scrolling
	wxScrollBar* m_treeHScrollBar;
	int m_treeHScrollPos;
	int m_treeContentWidth;

	// Font config
	bool m_useConfigFont;
	wxFont m_customFont;

	// SVG icon support
	wxString m_svgIconName;
	wxSize m_svgIconSize;
	std::map<int, wxString> m_columnSvgIconNames;
	std::map<int, wxSize> m_columnSvgIconSizes;
	std::map<std::shared_ptr<FlatTreeItem>, wxString> m_itemSvgIconNames;
	std::map<std::shared_ptr<FlatTreeItem>, wxSize> m_itemSvgIconSizes;
	std::map<std::pair<std::shared_ptr<FlatTreeItem>, int>, wxString> m_itemColumnSvgIconNames;
	std::map<std::pair<std::shared_ptr<FlatTreeItem>, int>, wxSize> m_itemColumnSvgIconSizes;

	// Column resizing state
	bool m_isResizingColumn;
	int m_resizingColumnIndex; // index of the column being resized (left of separator)
	int m_resizeStartX;
	int m_initialColumnWidth;
	int m_headerResizeMargin; // px threshold near separator

	// Header display control
	bool m_showHeaderText;

	// Scrollbar control
	bool m_alwaysShowScrollbars;

	DECLARE_EVENT_TABLE()
};

#endif // FLAT_TREE_VIEW_H
