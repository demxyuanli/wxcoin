#include "ObjectTreePanel.h"
#include "widgets/FlatTreeBuilder.h"
#include "GeometryObject.h"
#include "OCCGeometry.h"
#include "OCCViewer.h"
#include "widgets/FlatTreeView.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "ViewRefreshManager.h"
#include "logger/Logger.h"
#include "PropertyPanel.h"
#include "ui/FlatBarNotebook.h"
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include <wx/colordlg.h>
#include <wx/dataview.h>
#include <wx/sizer.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/renderer.h>
#include <vector>
#include <algorithm>

// Custom data view button renderer for action columns
class ButtonRenderer : public wxDataViewCustomRenderer {
public:
	ButtonRenderer(const wxString& label, const wxBitmapBundle& icon, const wxBitmapBundle& altIcon = wxBitmapBundle())
		: wxDataViewCustomRenderer("long", wxDATAVIEW_CELL_ACTIVATABLE, wxALIGN_CENTER)
		, m_label(label)
		, m_icon(icon)
		, m_altIcon(altIcon) {
	}

	bool Render(wxRect rect, wxDC* dc, int state) override {
		// Draw a push button background
		int flags = 0;
		if (state & wxDATAVIEW_CELL_SELECTED) {
			flags |= wxCONTROL_PRESSED;
		}
		wxRendererNative::Get().DrawPushButton(nullptr, *dc, rect, flags);

		// Draw icon centered (fallback to label text if no icon)
		wxBitmap bmp;
		long v = 1;
		if (m_value.IsType("long")) {
			v = m_value.GetLong();
		}
		if (m_altIcon.IsOk() && v == 0) {
			bmp = m_altIcon.GetBitmap(wxSize(12, 12));
		}
		else if (m_icon.IsOk()) {
			bmp = m_icon.GetBitmap(wxSize(12, 12));
		}
		if (bmp.IsOk()) {
			int x = rect.x + (rect.width - bmp.GetWidth()) / 2;
			int y = rect.y + (rect.height - bmp.GetHeight()) / 2;
			dc->DrawBitmap(bmp, x, y, true);
		}
		else {
			// Draw label text centered
			wxSize ts = dc->GetTextExtent(m_label);
			int x = rect.x + (rect.width - ts.x) / 2;
			int y = rect.y + (rect.height - ts.y) / 2;
			dc->DrawText(m_label, x, y);
		}
		return true;
	}

	bool ActivateCell(const wxRect& WXUNUSED(cell), wxDataViewModel* WXUNUSED(model), const wxDataViewItem& WXUNUSED(item), unsigned int WXUNUSED(col), const wxMouseEvent* WXUNUSED(mouseEvent)) override {
		// Tell control to emit activation event
		return true;
	}

	bool SetValue(const wxVariant& value) override {
		m_value = value;
		return true;
	}

	bool GetValue(wxVariant& value) const override {
		value = m_value;
		return true;
	}

	wxSize GetSize() const override {
		return wxSize(28, 20);
	}

private:
	wxString m_label;
	wxBitmapBundle m_icon;
	wxBitmapBundle m_altIcon;
	wxVariant m_value;
};

ObjectTreePanel::ObjectTreePanel(wxWindow* parent)
	: FlatUITitledPanel(parent, "CAD Object Tree")
	, m_propertyPanel(nullptr)
	, m_occViewer(nullptr)
	, m_isUpdatingSelection(false)
	, m_contextMenu(nullptr)
{
	LOG_INF_S("ObjectTreePanel initializing");
	// Tabs - using FlatBar-style notebook
	m_notebook = new FlatBarNotebook(this, wxID_ANY);
	m_tabPanel = new wxPanel(m_notebook);
	m_tabHistory = new wxPanel(m_notebook);
	m_tabVersion = new wxPanel(m_notebook);

	m_notebook->AddPage(m_tabPanel, "Panel", true);
	m_notebook->AddPage(m_tabHistory, "History");
	m_notebook->AddPage(m_tabVersion, "Version");

	m_mainSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 2);

	// Tab 1: Object tree
	{
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		m_tabPanel->SetSizer(sizer);
		m_treeView = new FlatTreeView(m_tabPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize);
		m_treeView->SetItemHeight(18);
		m_treeView->SetIndentWidth(14);
		m_treeView->SetDoubleBuffered(true);
		m_treeView->SetAlwaysShowScrollbars(true); // Enable scrollbars
		sizer->Add(m_treeView, 1, wxEXPAND | wxALL, 0);
	}

	// Columns: Tree | Vis | Color | Edit | Del
	// Reorganized for better UX: visibility -> color -> edit -> delete (increasing destructive impact)
	m_treeView->AddColumn("Vis", FlatTreeColumn::ColumnType::ICON, 28);
	m_treeView->AddColumn("Color", FlatTreeColumn::ColumnType::ICON, 28);
	m_treeView->AddColumn("Edit", FlatTreeColumn::ColumnType::ICON, 28);
	m_treeView->AddColumn("Del", FlatTreeColumn::ColumnType::ICON, 28);

	// Set SVG icons for columns (12x12 size, no text)
	m_treeView->SetColumnSvgIcon(1, "eyeopen", wxSize(12, 12));      // Visibility column
	m_treeView->SetColumnSvgIcon(2, "palette", wxSize(12, 12));      // Color column
	m_treeView->SetColumnSvgIcon(3, "edit", wxSize(12, 12));         // Edit/Notes column
	m_treeView->SetColumnSvgIcon(4, "delete", wxSize(12, 12));       // Delete column

	// Icons (fallback to bitmap if SVG not available)
	m_bmpEyeOpen = wxArtProvider::GetBitmap(wxART_TICK_MARK, wxART_BUTTON, wxSize(12, 12));
	m_bmpEyeClosed = wxArtProvider::GetBitmap(wxART_CROSS_MARK, wxART_BUTTON, wxSize(12, 12));
	m_bmpDelete = wxArtProvider::GetBitmap(wxART_DELETE, wxART_BUTTON, wxSize(12, 12));
	m_bmpColor = wxArtProvider::GetBitmap(wxART_TIP, wxART_BUTTON, wxSize(12, 12));
	m_bmpEdit = wxArtProvider::GetBitmap(wxART_EDIT, wxART_BUTTON, wxSize(12, 12));

	// Tree starts empty - no fixed root node
	m_rootItem.reset();
	m_partRootItem.reset();

	// Click handling
	m_treeView->OnItemClicked([this](std::shared_ptr<FlatTreeItem> item, int col) { onTreeItemClicked(item, col); });

	// Keyboard shortcuts from panel
	Bind(wxEVT_KEY_DOWN, &ObjectTreePanel::onKeyDown, this);

	// Context menu
	createContextMenu();

	// Initialize tree data structure
	initializeTreeDataStructure();

	// Tab 2: History (Undo/Redo trees)
	{
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		m_tabHistory->SetSizer(sizer);
		m_historyView = new FlatTreeView(m_tabHistory, wxID_ANY, wxDefaultPosition, wxDefaultSize);
		m_historyView->SetItemHeight(18);
		m_historyView->SetIndentWidth(14);
		m_historyView->SetDoubleBuffered(true);
		m_historyView->SetAlwaysShowScrollbars(true); // Enable scrollbars
		// Two columns: action and info
		m_historyView->AddColumn("Action", FlatTreeColumn::ColumnType::TEXT, 120);
		m_historyView->AddColumn("Info", FlatTreeColumn::ColumnType::TEXT, 180);
		sizer->Add(m_historyView, 1, wxEXPAND | wxALL, 0);

		// Build Undo/Redo roots
		m_historyRoot = std::make_shared<FlatTreeItem>("Edit History", FlatTreeItem::ItemType::ROOT);
		m_historyRoot->SetExpanded(true);
		m_undoRoot = std::make_shared<FlatTreeItem>("Undo", FlatTreeItem::ItemType::FOLDER);
		m_redoRoot = std::make_shared<FlatTreeItem>("Redo", FlatTreeItem::ItemType::FOLDER);
		m_undoRoot->SetExpanded(true);
		m_redoRoot->SetExpanded(true);
		m_historyRoot->AddChild(m_undoRoot);
		m_historyRoot->AddChild(m_redoRoot);
		m_historyView->SetRoot(m_historyRoot);
	}

	// Tab 3: Version (placeholder)
	{
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		m_tabVersion->SetSizer(sizer);
		// Placeholder content can be added later
	}
}

ObjectTreePanel::~ObjectTreePanel()
{
	LOG_INF_S("ObjectTreePanel destroying");
	if (m_contextMenu) {
		delete m_contextMenu;
	}
}

void ObjectTreePanel::addObject(GeometryObject* object)
{
	if (!object) {
		LOG_ERR_S("Attempted to add null object to tree");
		return;
	}
	if (m_objectMap.find(object) != m_objectMap.end()) {
		LOG_WRN_S("Object already exists in tree: " + object->getName());
		return;
	}

	LOG_INF_S("Adding object to tree: " + object->getName());
	auto item = std::make_shared<FlatTreeItem>(object->getName(), FlatTreeItem::ItemType::FILE);
	m_rootItem->AddChild(item);
	m_objectMap[object] = item;
	m_treeView->Refresh();
}

void ObjectTreePanel::removeObject(GeometryObject* object)
{
	if (!object) {
		LOG_ERR_S("Attempted to remove null object from tree");
		return;
	}

	auto it = m_objectMap.find(object);
	if (it == m_objectMap.end()) {
		LOG_WRN_S("Object not found in tree: " + object->getName());
		return;
	}

	LOG_INF_S("Removing object from tree: " + object->getName());
	auto item = it->second;
	if (item && item->GetParent()) {
		item->GetParent()->RemoveChild(item);
	}
	m_objectMap.erase(it);
	m_treeView->Refresh();
}

void ObjectTreePanel::updateObjectName(GeometryObject* object)
{
	if (!object) {
		LOG_ERR_S("Attempted to update name of null object");
		return;
	}

	auto it = m_objectMap.find(object);
	if (it == m_objectMap.end()) {
		LOG_WRN_S("Object not found in tree for name update: " + object->getName());
		return;
	}

	LOG_INF_S("Updating object name in tree: " + object->getName());
	if (it->second) it->second->SetText(object->getName());
	m_treeView->Refresh();
}

void ObjectTreePanel::addOCCGeometry(std::shared_ptr<OCCGeometry> geometry, std::shared_ptr<OCCGeometry> parentGeometry)
{
	if (!geometry) {
		LOG_ERR_S("Attempted to add null OCCGeometry to tree");
		return;
	}
	if (m_occGeometryMap.find(geometry) != m_occGeometryMap.end()) {
		return;
	}

	m_treeView->Freeze();

	// Create virtual root if it doesn't exist
	if (!m_rootItem) {
		m_rootItem = std::make_shared<FlatTreeItem>("VirtualRoot", FlatTreeItem::ItemType::ROOT);
		m_rootItem->SetExpanded(true);
		m_treeView->SetRoot(m_rootItem);
	}

	// Create geometry item
	auto geometryItem = std::make_shared<FlatTreeItem>(geometry->getName(), FlatTreeItem::ItemType::FILE);

	// Set SVG icon for the tree item (12x12 size)
	m_treeView->SetItemSvgIcon(geometryItem, "file", wxSize(12, 12));

	// Set SVG icons for columns (12x12 size)
	// Column order: Vis | Color | Edit | Del
	m_treeView->SetItemColumnSvgIcon(geometryItem, 1, geometry->isVisible() ? "eyeopen" : "eyeclosed", wxSize(12, 12));
	m_treeView->SetItemColumnSvgIcon(geometryItem, 2, "palette", wxSize(12, 12));
	m_treeView->SetItemColumnSvgIcon(geometryItem, 3, "edit", wxSize(12, 12));
	m_treeView->SetItemColumnSvgIcon(geometryItem, 4, "delete", wxSize(12, 12));

	// Fallback to bitmap icons if SVG not available
	geometryItem->SetIcon(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(12, 12)));
	geometryItem->SetColumnIcon(1, geometry->isVisible() ? m_bmpEyeOpen : m_bmpEyeClosed);
	geometryItem->SetColumnIcon(2, m_bmpColor);
	geometryItem->SetColumnIcon(3, m_bmpEdit);
	geometryItem->SetColumnIcon(4, m_bmpDelete);

	// Determine parent item
	std::shared_ptr<FlatTreeItem> parentItem;
	if (parentGeometry) {
		auto it = m_occGeometryMap.find(parentGeometry);
		if (it != m_occGeometryMap.end()) {
			parentItem = it->second;
		}
	}
	if (!parentItem) {
		parentItem = m_rootItem; // Default to virtual root
	}

	// Add as child of parent
	parentItem->AddChild(geometryItem);
	m_occGeometryMap[geometry] = geometryItem;
	m_treeItemToOCCGeometry[geometryItem] = geometry;

	m_treeView->Thaw();

	// Force layout update to ensure proper expansion and scrolling
	m_treeView->InvalidateVisibleItemsList();
	m_treeView->UpdateScrollbars();
	m_treeView->Refresh();
}

// Add geometry from a specific file (creates file node as 1st level, geometry as child)
void ObjectTreePanel::addOCCGeometryFromFile(const wxString& fileName, std::shared_ptr<OCCGeometry> geometry)
{
	// Default behavior: immediate refresh for backward compatibility
	addOCCGeometryFromFile(fileName, geometry, true);
}

// Add geometry from a specific file (creates file node as 1st level, geometry as child)
void ObjectTreePanel::addOCCGeometryFromFile(const wxString& fileName, std::shared_ptr<OCCGeometry> geometry, bool immediateRefresh)
{
	if (!geometry) {
		LOG_ERR_S("Attempted to add null OCCGeometry to tree");
		return;
	}

	// Check if filename is empty
	if (fileName.IsEmpty()) {
		LOG_WRN_S("Filename is empty for geometry '" + geometry->getName() + "'");
		return;
	}

	// Update data structure
	m_treeData.addGeometry(geometry);

	// Refresh display only if requested (for batch operations)
	if (immediateRefresh) {
		refreshTreeDisplay();
	}
}


// Initialize tree data structure
void ObjectTreePanel::initializeTreeDataStructure()
{
	LOG_INF_S("Initializing tree data structure");
	m_treeData.clear();
	
	// Create virtual root if it doesn't exist
	if (!m_rootItem) {
		m_rootItem = std::make_shared<FlatTreeItem>("VirtualRoot", FlatTreeItem::ItemType::ROOT);
		m_rootItem->SetExpanded(true);
		m_treeView->SetRoot(m_rootItem);
	}
	
	LOG_INF_S("Tree data structure initialized");
}

// Update tree data structure
void ObjectTreePanel::updateTreeDataStructure()
{
	if (!m_treeData.needsUpdate) {
		return;
	}
	
	LOG_INF_S("Updating tree data structure");
	
	// Clear existing tree items
	m_treeView->Clear();
	
	// Clear selection state before clearing mappings
	if (m_lastSelectedItem) {
		m_lastSelectedItem->SetSelected(false);
		m_lastSelectedItem.reset();
	}
	
	m_occGeometryMap.clear();
	m_treeItemToOCCGeometry.clear();
	m_fileNodeMap.clear();
	
	// Recreate virtual root
	m_rootItem = std::make_shared<FlatTreeItem>("VirtualRoot", FlatTreeItem::ItemType::ROOT);
	m_rootItem->SetExpanded(true);
	m_treeView->SetRoot(m_rootItem);
	
	// Add file groups
	for (const auto& fileGroup : m_treeData.fileGroups) {
		const wxString& fileName = fileGroup.first;
		const auto& geometries = fileGroup.second;
		
		if (!geometries.empty()) {
			// Create file node
			auto fileNode = getOrCreateFileNode(fileName);
			
			// Add geometries to file node
			for (const auto& geometry : geometries) {
				addOCCGeometryToNode(fileNode, geometry);
			}
		}
	}
	
	// Add ungrouped geometries
	for (const auto& geometry : m_treeData.ungroupedGeometries) {
		addOCCGeometryToNode(m_rootItem, geometry);
	}
	
	m_treeData.needsUpdate = false;
	LOG_INF_S("Tree data structure updated");
}

// Refresh tree display
void ObjectTreePanel::refreshTreeDisplay()
{
	LOG_INF_S("Refreshing tree display");
	
	// Update data structure first
	updateTreeDataStructure();
	
	// Force layout update
	m_treeView->InvalidateVisibleItemsList();
	m_treeView->UpdateScrollbars();
	m_treeView->Refresh();
	
	LOG_INF_S("Tree display refreshed");
}

// Helper method to add geometry to a specific node
void ObjectTreePanel::addOCCGeometryToNode(std::shared_ptr<FlatTreeItem> parentNode, std::shared_ptr<OCCGeometry> geometry)
{
	if (!geometry || !parentNode) {
		return;
	}
	
	// Check if geometry already exists (silent check - this is expected during refresh)
	if (m_occGeometryMap.find(geometry) != m_occGeometryMap.end()) {
		return;
	}
	
	// Create geometry item
	auto geometryItem = std::make_shared<FlatTreeItem>(geometry->getName(), FlatTreeItem::ItemType::FILE);
	
	// Set SVG icon for the tree item
	m_treeView->SetItemSvgIcon(geometryItem, "file", wxSize(12, 12));
	
	// Set SVG icons for columns
	// Column order: Vis | Color | Edit | Del
	m_treeView->SetItemColumnSvgIcon(geometryItem, 1, geometry->isVisible() ? "eyeopen" : "eyeclosed", wxSize(12, 12));
	m_treeView->SetItemColumnSvgIcon(geometryItem, 2, "palette", wxSize(12, 12));
	m_treeView->SetItemColumnSvgIcon(geometryItem, 3, "edit", wxSize(12, 12));
	m_treeView->SetItemColumnSvgIcon(geometryItem, 4, "delete", wxSize(12, 12));
	
	// Fallback to bitmap icons if SVG not available
	geometryItem->SetIcon(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(12, 12)));
	geometryItem->SetColumnIcon(1, geometry->isVisible() ? m_bmpEyeOpen : m_bmpEyeClosed);
	geometryItem->SetColumnIcon(2, m_bmpColor);
	geometryItem->SetColumnIcon(3, m_bmpEdit);
	geometryItem->SetColumnIcon(4, m_bmpDelete);
	
	// Add to parent node
	parentNode->AddChild(geometryItem);
	
	// Update mappings
	m_occGeometryMap[geometry] = geometryItem;
	m_treeItemToOCCGeometry[geometryItem] = geometry;
}

// Backward compatibility - add geometry as root level
void ObjectTreePanel::addOCCGeometry(std::shared_ptr<OCCGeometry> geometry)
{
	if (!geometry) {
		LOG_ERR_S("Attempted to add null OCCGeometry to tree");
		return;
	}
	
	LOG_INF_S("Adding geometry '" + geometry->getName() + "' (no filename)");
	
	// Update data structure
	m_treeData.addGeometry(geometry);
	
	// Refresh display
	refreshTreeDisplay();
}

void ObjectTreePanel::removeOCCGeometry(std::shared_ptr<OCCGeometry> geometry)
{
	if (!geometry) {
		LOG_ERR_S("Attempted to remove null OCCGeometry from tree");
		return;
	}

	LOG_INF_S("Removing geometry '" + geometry->getName() + "' from tree");

	// Clear selection if the removed geometry is currently selected
	auto selectedGeometry = getSelectedOCCGeometry();
	if (selectedGeometry == geometry) {
		LOG_INF_S("Removing currently selected geometry, clearing selection");
		if (m_lastSelectedItem) {
			m_lastSelectedItem->SetSelected(false);
			m_lastSelectedItem.reset();
		}
		if (m_occViewer) {
			m_occViewer->deselectAll();
		}
		if (m_propertyPanel) {
			m_propertyPanel->clearProperties();
		}
	}

	// Remove from mappings before updating data structure
	auto it = m_occGeometryMap.find(geometry);
	if (it != m_occGeometryMap.end()) {
		auto treeItem = it->second;
		// Remove from reverse mapping
		m_treeItemToOCCGeometry.erase(treeItem);
		// Remove from forward mapping (will be done in refreshTreeDisplay)
	}
	
	// Update data structure
	m_treeData.removeGeometry(geometry);
	
	// Refresh display
	refreshTreeDisplay();
}

void ObjectTreePanel::removeOCCGeometryRecursive(std::shared_ptr<OCCGeometry> geometry)
{
	auto it = m_occGeometryMap.find(geometry);
	if (it == m_occGeometryMap.end()) {
		return; // Already removed
	}

	auto featureItem = it->second;

	// Recursively remove all children first
	std::vector<std::shared_ptr<FlatTreeItem>> children = featureItem->GetChildren();
	for (auto& child : children) {
		auto childIt = m_treeItemToOCCGeometry.find(child);
		if (childIt != m_treeItemToOCCGeometry.end()) {
			auto childGeometry = childIt->second;
			removeOCCGeometryRecursive(childGeometry); // Recursive call
		}
	}

	// Remove from parent
	if (featureItem && featureItem->GetParent()) {
		featureItem->GetParent()->RemoveChild(featureItem);
	}
	m_occGeometryMap.erase(it);
	m_treeItemToOCCGeometry.erase(featureItem);
}

std::shared_ptr<FlatTreeItem> ObjectTreePanel::getOrCreateFileNode(const wxString& fileName)
{
	auto it = m_fileNodeMap.find(fileName);
	if (it != m_fileNodeMap.end()) {
		return it->second;
	}

	// Create new file node
	auto fileNode = std::make_shared<FlatTreeItem>(fileName, FlatTreeItem::ItemType::FOLDER);
	m_treeView->SetItemSvgIcon(fileNode, "folder", wxSize(12, 12));

	// Set SVG icons for columns (12x12 size) - file nodes can also be controlled
	// Column order: Vis | Color | Edit | Del
	m_treeView->SetItemColumnSvgIcon(fileNode, 1, "eyeopen", wxSize(12, 12)); // Default visible
	m_treeView->SetItemColumnSvgIcon(fileNode, 2, "palette", wxSize(12, 12));
	m_treeView->SetItemColumnSvgIcon(fileNode, 3, "edit", wxSize(12, 12));
	m_treeView->SetItemColumnSvgIcon(fileNode, 4, "delete", wxSize(12, 12));

	// Fallback to bitmap icons if SVG not available
	fileNode->SetIcon(wxArtProvider::GetBitmap(wxART_FOLDER, wxART_OTHER, wxSize(12, 12)));
	fileNode->SetColumnIcon(1, m_bmpEyeOpen);
	fileNode->SetColumnIcon(2, m_bmpColor);
	fileNode->SetColumnIcon(3, m_bmpEdit);
	fileNode->SetColumnIcon(4, m_bmpDelete);

	// Add to virtual root
	m_rootItem->AddChild(fileNode);
	fileNode->SetExpanded(true); // Expand file nodes by default

	// Store in map
	m_fileNodeMap[fileName] = fileNode;

	return fileNode;
}

void ObjectTreePanel::updateOCCGeometryName(std::shared_ptr<OCCGeometry> geometry)
{
	if (!geometry) {
		LOG_ERR_S("Attempted to update name of null OCCGeometry");
		return;
	}

	auto it = m_occGeometryMap.find(geometry);
	if (it == m_occGeometryMap.end()) {
		LOG_WRN_S("OCCGeometry not found in tree for name update: " + geometry->getName());
		return;
	}

	m_treeView->Freeze();
	// Update visibility icon based on current state
	updateTreeItemIcon(it->second, geometry->isVisible());
	m_treeView->Thaw();
}

void ObjectTreePanel::selectOCCGeometry(std::shared_ptr<OCCGeometry> geometry)
{
	if (!geometry) return;

	auto it = m_occGeometryMap.find(geometry);
	if (it == m_occGeometryMap.end()) return;

	m_isUpdatingSelection = true;
	m_lastSelectedItem = it->second;
	m_isUpdatingSelection = false;
}

// Object management functions
void ObjectTreePanel::deleteSelectedObject()
{
	LOG_INF_S("ObjectTreePanel::deleteSelectedObject - Starting deletion process");
	auto geometry = getSelectedOCCGeometry();
	if (!geometry) {
		LOG_WRN_S("ObjectTreePanel::deleteSelectedObject - No object selected for deletion");
		return;
	}

	LOG_INF_S("ObjectTreePanel::deleteSelectedObject - Selected geometry for deletion: " + geometry->getName() +
		" (visible: " + std::string(geometry->isVisible() ? "true" : "false") + ")");

	wxString message = wxString::Format("Are you sure you want to delete '%s'?", geometry->getName());
	int result = wxMessageBox(message, "Confirm Delete", wxYES_NO | wxICON_QUESTION);

	if (result == wxYES) {
		LOG_INF_S("ObjectTreePanel::deleteSelectedObject - User confirmed deletion of geometry: " + geometry->getName());

		// Clear selection before deletion to avoid stale references
		if (m_lastSelectedItem) {
			m_lastSelectedItem->SetSelected(false);
			m_lastSelectedItem.reset();
		}
		if (m_occViewer) {
			m_occViewer->deselectAll();
		}
		if (m_propertyPanel) {
			m_propertyPanel->clearProperties();
		}

		// Remove from viewer (this will also remove from tree via ObjectTreeSync)
		if (m_occViewer) {
			LOG_INF_S("ObjectTreePanel::deleteSelectedObject - Calling OCCViewer::removeGeometry for: " + geometry->getName());
			m_occViewer->removeGeometry(geometry->getName());
			LOG_INF_S("ObjectTreePanel::deleteSelectedObject - OCCViewer::removeGeometry completed successfully");

			// Force Canvas refresh after geometry removal
			if (m_occViewer->getSceneManager()) {
				Canvas* canvas = m_occViewer->getSceneManager()->getCanvas();
				if (canvas) {
					if (canvas->getRefreshManager()) {
						canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::GEOMETRY_CHANGED, true);
						LOG_INF_S("ObjectTreePanel::deleteSelectedObject - Requested geometry change refresh after deletion");
					}
					canvas->Refresh(true);
					canvas->Update();
					LOG_INF_S("ObjectTreePanel::deleteSelectedObject - Forced Canvas refresh and update after deletion");
				}
			}
		} else {
			LOG_ERR_S("ObjectTreePanel::deleteSelectedObject - OCCViewer is null during deletion!");
		}
		// Note: Don't call removeOCCGeometry here - OCCViewer::removeGeometry already does this via ObjectTreeSync
	} else {
		LOG_INF_S("ObjectTreePanel::deleteSelectedObject - User cancelled deletion of geometry: " + geometry->getName());
	}
}

void ObjectTreePanel::editSelectedObjectNotes()
{
	LOG_INF_S("ObjectTreePanel::editSelectedObjectNotes - Starting notes/name editing process");
	auto geometry = getSelectedOCCGeometry();
	if (!geometry) {
		LOG_WRN_S("ObjectTreePanel::editSelectedObjectNotes - No object selected for notes editing");
		return;
	}

	LOG_INF_S("ObjectTreePanel::editSelectedObjectNotes - Selected geometry for editing: " + geometry->getName());

	// Create a text entry dialog for editing the object name/notes
	wxTextEntryDialog dialog(this, "Enter new name for the object:", "Edit Object Name",
		geometry->getName(), wxOK | wxCANCEL);

	LOG_INF_S("ObjectTreePanel::editSelectedObjectNotes - Opened name editing dialog for geometry: " + geometry->getName());

	if (dialog.ShowModal() == wxID_OK) {
		wxString newName = dialog.GetValue();
		LOG_INF_S("ObjectTreePanel::editSelectedObjectNotes - Dialog result: OK, new name: '" + std::string(newName.mb_str()) + "'");

		if (!newName.IsEmpty() && newName != geometry->getName()) {
			LOG_INF_S("ObjectTreePanel::editSelectedObjectNotes - Renaming object from '" + geometry->getName() + "' to '" + std::string(newName.mb_str()) + "'");

			// Update the geometry name
			geometry->setName(std::string(newName.mb_str()));
			LOG_INF_S("ObjectTreePanel::editSelectedObjectNotes - Geometry name updated in memory");

			// Update tree display
			updateOCCGeometryName(geometry);
			LOG_INF_S("ObjectTreePanel::editSelectedObjectNotes - Tree display updated with new name");

			// Update property panel to reflect the name change
			if (m_propertyPanel) {
				m_propertyPanel->updateProperties(geometry);
				LOG_INF_S("ObjectTreePanel::editSelectedObjectNotes - Property panel updated with new name");
			}

			// Force refresh to show name change
			m_treeView->Refresh();
			LOG_INF_S("ObjectTreePanel::editSelectedObjectNotes - Tree view refreshed to show name change");
		} else if (newName.IsEmpty()) {
			LOG_WRN_S("ObjectTreePanel::editSelectedObjectNotes - Empty name entered, ignoring rename for geometry: " + geometry->getName());
		} else {
			LOG_INF_S("ObjectTreePanel::editSelectedObjectNotes - Name unchanged for geometry: " + geometry->getName());
		}
	} else {
		LOG_INF_S("ObjectTreePanel::editSelectedObjectNotes - User cancelled name editing dialog for geometry: " + geometry->getName());
	}
}

void ObjectTreePanel::hideSelectedObject()
{
	auto geometry = getSelectedOCCGeometry();
	if (!geometry) {
		LOG_WRN_S("No object selected for hiding");
		return;
	}

	LOG_INF_S("Hiding object: " + geometry->getName());
	if (m_occViewer) m_occViewer->setGeometryVisible(geometry->getName(), false);

	// Update tree item icon
	auto it = m_occGeometryMap.find(geometry);
	if (it != m_occGeometryMap.end()) {
		updateTreeItemIcon(it->second, false);
	}
}

void ObjectTreePanel::showSelectedObject()
{
	auto geometry = getSelectedOCCGeometry();
	if (!geometry) {
		LOG_WRN_S("No object selected for showing");
		return;
	}

	LOG_INF_S("Showing object: " + geometry->getName());
	if (m_occViewer) m_occViewer->setGeometryVisible(geometry->getName(), true);

	// Update tree item icon
	auto it = m_occGeometryMap.find(geometry);
	if (it != m_occGeometryMap.end()) {
		updateTreeItemIcon(it->second, true);
	}
}

void ObjectTreePanel::toggleObjectVisibility()
{
	auto geometry = getSelectedOCCGeometry();
	if (!geometry) {
		LOG_WRN_S("No object selected for visibility toggle");
		return;
	}

	bool currentVisibility = geometry->isVisible();
	if (currentVisibility) {
		hideSelectedObject();
	}
	else {
		showSelectedObject();
	}
}

void ObjectTreePanel::showAllObjects()
{
	LOG_DBG("Showing all objects", "ObjectTreePanel");
	if (!m_occViewer) {
		LOG_WRN_S("OCCViewer is null");
		return;
	}

	// Use OCCViewer's showAll method for better consistency
	m_occViewer->showAll();

	// Update tree item icons
	m_treeView->Freeze();
	auto allGeometries = m_occViewer->getAllGeometry();
	for (const auto& geometry : allGeometries) {
		auto it = m_occGeometryMap.find(geometry);
		if (it != m_occGeometryMap.end()) {
			updateTreeItemIcon(it->second, true);
		}
	}
	m_treeView->Thaw();
}

void ObjectTreePanel::hideAllObjects()
{
	LOG_DBG("Hiding all objects", "ObjectTreePanel");
	if (!m_occViewer) {
		LOG_WRN_S("OCCViewer is null");
		return;
	}

	// Use OCCViewer's hideAll method for better consistency
	m_occViewer->hideAll();

	// Update tree item icons
	m_treeView->Freeze();
	auto allGeometries = m_occViewer->getAllGeometry();
	for (const auto& geometry : allGeometries) {
		auto it = m_occGeometryMap.find(geometry);
		if (it != m_occGeometryMap.end()) {
			updateTreeItemIcon(it->second, false);
		}
	}
	m_treeView->Thaw();
}

// Event handlers
// No direct right-click from FlatTreeView yet; context menu can be bound to panel if needed.

void ObjectTreePanel::onKeyDown(wxKeyEvent& event)
{
	int keyCode = event.GetKeyCode();

	switch (keyCode) {
	case WXK_DELETE:
	case WXK_BACK:
		deleteSelectedObject();
		break;
	case 'H':
	case 'h':
		if (event.ControlDown()) {
			hideSelectedObject();
		}
		break;
	case 'S':
	case 's':
		if (event.ControlDown()) {
			showSelectedObject();
		}
		break;
	case WXK_F5:
		toggleObjectVisibility();
		break;
	default:
		event.Skip();
		break;
	}
}

void ObjectTreePanel::onDeleteObject(wxCommandEvent& event)
{
	deleteSelectedObject();
}

void ObjectTreePanel::onHideObject(wxCommandEvent& event)
{
	hideSelectedObject();
}

void ObjectTreePanel::onShowObject(wxCommandEvent& event)
{
	showSelectedObject();
}

void ObjectTreePanel::onToggleVisibility(wxCommandEvent& event)
{
	toggleObjectVisibility();
}

void ObjectTreePanel::onShowAllObjects(wxCommandEvent& event)
{
	showAllObjects();
}

void ObjectTreePanel::onHideAllObjects(wxCommandEvent& event)
{
	hideAllObjects();
}

// Helper functions
void ObjectTreePanel::createContextMenu()
{
	m_contextMenu = new wxMenu();

	// Add menu items with keyboard shortcuts
	wxMenuItem* miDelete = m_contextMenu->Append(wxID_ANY, "Delete\tDel", "Delete selected object");
	m_contextMenu->AppendSeparator();
	wxMenuItem* miHide = m_contextMenu->Append(wxID_ANY, "Hide\tCtrl+H", "Hide selected object");
	wxMenuItem* miShow = m_contextMenu->Append(wxID_ANY, "Show\tCtrl+S", "Show selected object");
	wxMenuItem* miToggle = m_contextMenu->Append(wxID_ANY, "Toggle Visibility\tF5", "Toggle object visibility");
	m_contextMenu->AppendSeparator();
	wxMenuItem* miShowAll = m_contextMenu->Append(wxID_ANY, "Show All", "Show all objects");
	wxMenuItem* miHideAll = m_contextMenu->Append(wxID_ANY, "Hide All", "Hide all objects");

	// Bind menu events
	Bind(wxEVT_MENU, &ObjectTreePanel::onDeleteObject, this, miDelete->GetId());
	Bind(wxEVT_MENU, &ObjectTreePanel::onHideObject, this, miHide->GetId());
	Bind(wxEVT_MENU, &ObjectTreePanel::onShowObject, this, miShow->GetId());
	Bind(wxEVT_MENU, &ObjectTreePanel::onToggleVisibility, this, miToggle->GetId());
	Bind(wxEVT_MENU, &ObjectTreePanel::onShowAllObjects, this, miShowAll->GetId());
	Bind(wxEVT_MENU, &ObjectTreePanel::onHideAllObjects, this, miHideAll->GetId());
}

void ObjectTreePanel::updateTreeItemIcon(std::shared_ptr<FlatTreeItem> item, bool visible)
{
	if (!item) return;

	// Update SVG icon for visibility column (12x12 size)
	m_treeView->SetItemColumnSvgIcon(item, 1, visible ? "eyeopen" : "eyeclosed", wxSize(12, 12));

	// Fallback to bitmap icon if SVG not available
	item->SetColumnIcon(1, visible ? m_bmpEyeOpen : m_bmpEyeClosed);

	m_treeView->RefreshItem(item);
}

void ObjectTreePanel::ensurePartRoot()
{
	if (m_partRootItem) return;
	m_partRootItem = std::make_shared<FlatTreeItem>("Part", FlatTreeItem::ItemType::FOLDER);
	m_rootItem->AddChild(m_partRootItem);
	m_treeView->Refresh();
}

std::shared_ptr<OCCGeometry> ObjectTreePanel::getSelectedOCCGeometry()
{
	auto selectedItem = m_lastSelectedItem;
	if (!selectedItem) {
		return nullptr;
	}
	auto it = m_treeItemToOCCGeometry.find(selectedItem);
	if (it != m_treeItemToOCCGeometry.end()) {
		auto geometry = it->second;
		// Verify geometry still exists in viewer
		if (geometry && m_occViewer) {
			auto allGeometries = m_occViewer->getAllGeometry();
			for (const auto& g : allGeometries) {
				if (g == geometry) {
					return geometry;
				}
			}
			// Geometry no longer exists, clear stale selection
			LOG_WRN_S("ObjectTreePanel::getSelectedOCCGeometry - Selected geometry '" + geometry->getName() + "' no longer exists in viewer, clearing selection");
			m_lastSelectedItem.reset();
			selectedItem->SetSelected(false);
			return nullptr;
		}
		return geometry;
	}
	return nullptr;
}

void ObjectTreePanel::deselectOCCGeometry(std::shared_ptr<OCCGeometry> geometry)
{
	if (!geometry) return;

	auto it = m_occGeometryMap.find(geometry);
	if (it == m_occGeometryMap.end()) return;

	auto currentSelection = m_lastSelectedItem;
	if (currentSelection == it->second) {
		m_lastSelectedItem.reset();
	}
}

void ObjectTreePanel::setPropertyPanel(PropertyPanel* panel)
{
	m_propertyPanel = panel;
	LOG_INF_S("PropertyPanel set for ObjectTreePanel");
}

void ObjectTreePanel::setOCCViewer(OCCViewer* viewer)
{
	m_occViewer = viewer;
	LOG_INF_S("OCCViewer set for ObjectTreePanel");

	// Update tree selection from viewer if viewer has selected geometries
	if (m_occViewer) {
		updateTreeSelectionFromViewer();
	}
}

void ObjectTreePanel::onTreeItemClicked(std::shared_ptr<FlatTreeItem> item, int column)
{
	if (!item) {
		LOG_DBG_S("ObjectTreePanel::onTreeItemClicked - Clicked item is null, ignoring");
		return;
	}
	if (m_isUpdatingSelection) {
		LOG_DBG_S("ObjectTreePanel::onTreeItemClicked - Currently updating selection, ignoring click");
		return;
	}

	LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Processing click on item: " + item->GetText() + ", column: " + std::to_string(column));

	// Virtual root -> clear selection (should not happen since root is not visible)
	if (item == m_rootItem) {
		LOG_WRN_S("ObjectTreePanel::onTreeItemClicked - Clicked on virtual root, clearing all selections");
		if (m_propertyPanel) {
			m_propertyPanel->clearProperties();
			LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Cleared property panel");
		}
		if (m_occViewer) {
			m_occViewer->deselectAll();
			LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Cleared all geometry selections in viewer");
		}
		// Clear tree selection
		if (m_lastSelectedItem) {
			m_lastSelectedItem->SetSelected(false);
			m_lastSelectedItem.reset();
			LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Cleared previous tree selection");
		}
		m_treeView->Refresh();
		return;
	}

	auto itOcc = m_treeItemToOCCGeometry.find(item);
	if (itOcc != m_treeItemToOCCGeometry.end()) {
		auto geometry = itOcc->second;
		if (!geometry) {
			LOG_ERR_S("ObjectTreePanel::onTreeItemClicked - Geometry pointer is null for item: " + item->GetText());
			return;
		}

		// Verify geometry still exists in viewer (only check, don't refresh here)
		if (m_occViewer) {
			auto allGeometries = m_occViewer->getAllGeometry();
			bool geometryExists = false;
			for (const auto& g : allGeometries) {
				if (g == geometry) {
					geometryExists = true;
					break;
				}
			}
			if (!geometryExists) {
				LOG_WRN_S("ObjectTreePanel::onTreeItemClicked - Geometry '" + geometry->getName() + "' no longer exists in viewer, ignoring click");
				// Clear stale selection
				if (m_lastSelectedItem == item) {
					m_lastSelectedItem.reset();
					item->SetSelected(false);
				}
				// Note: Don't refresh tree here - it will be refreshed by the deletion process
				return;
			}
		}

		LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Found geometry: " + geometry->getName() + " for item: " + item->GetText());

		// Check if clicking on already selected item - allow deselection
		bool isCurrentlySelected = (m_lastSelectedItem == item && item->IsSelected());

		if (isCurrentlySelected && column == 0) {
			// Deselect the item when clicking on object name again
			LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Deselecting currently selected geometry: " + geometry->getName());
			item->SetSelected(false);
			m_lastSelectedItem.reset();
			if (m_occViewer) {
				m_isUpdatingSelection = true;
				m_occViewer->deselectAll();
				m_isUpdatingSelection = false;

				// Force rebuild Coin3D representation to restore original color
				LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Rebuilding Coin3D representation to restore original color for deselected geometry");
				if (geometry) {
					// Force rebuild to restore original color (preserve forceCustomColor if set)
					geometry->setMeshRegenerationNeeded(true);
					geometry->buildCoinRepresentation(m_occViewer->getMeshParameters());

					// Touch Coin3D node to force update
					if (geometry->getCoinNode()) {
						geometry->getCoinNode()->touch();
						LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Touched Coin3D node to force color restoration update");
					}
				}

				// Force Canvas refresh to show color restoration
				if (m_occViewer->getSceneManager()) {
					Canvas* canvas = m_occViewer->getSceneManager()->getCanvas();
					if (canvas) {
						if (canvas->getRefreshManager()) {
							canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::SELECTION_CHANGED, true);
							LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Requested view refresh for selection change");
						}
						canvas->Refresh(true);
						canvas->Update();
						LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Forced Canvas refresh and update");
					}
				}
			}
			if (m_propertyPanel) {
				m_propertyPanel->clearProperties();
				LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Cleared property panel for deselection");
			}
			m_treeView->Refresh();
			LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Deselection completed for geometry: " + geometry->getName());
			return;
		}

		// Clear previous selection if selecting a different item
		if (m_lastSelectedItem && m_lastSelectedItem != item) {
			LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Switching selection from: " + m_lastSelectedItem->GetText() + " to: " + item->GetText());
			m_lastSelectedItem->SetSelected(false);

			// Restore color for previously selected geometry
			auto prevItOcc = m_treeItemToOCCGeometry.find(m_lastSelectedItem);
			if (prevItOcc != m_treeItemToOCCGeometry.end() && prevItOcc->second) {
				auto prevGeometry = prevItOcc->second;
				if (m_occViewer) {
					// Force rebuild to restore original color (preserve forceCustomColor if set)
					LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Rebuilding Coin3D representation to restore original color for previous geometry: " + prevGeometry->getName());
					prevGeometry->setMeshRegenerationNeeded(true);
					prevGeometry->buildCoinRepresentation(m_occViewer->getMeshParameters());
					if (prevGeometry->getCoinNode()) {
						prevGeometry->getCoinNode()->touch();
						LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Touched Coin3D node for previous geometry color restoration");
					}
				}
			}
		}

		// Set new selection
		m_lastSelectedItem = item;
		item->SetSelected(true);
		LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Set tree selection to geometry: " + geometry->getName());

		// Update selection in OCCViewer
		if (m_occViewer) {
			m_isUpdatingSelection = true;
			m_occViewer->deselectAll();
			m_occViewer->setGeometrySelected(geometry->getName(), true);
			m_isUpdatingSelection = false;
			LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Updated OCCViewer selection to geometry: " + geometry->getName());
		}

		if (column == 1) {
			// Column 1: Toggle visibility (eye icon)
			bool nv = !geometry->isVisible();
			LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Toggling visibility for geometry: " + geometry->getName() +
				" from " + (geometry->isVisible() ? "visible" : "hidden") + " to " + (nv ? "visible" : "hidden"));
			if (m_occViewer) {
				m_occViewer->setGeometryVisible(geometry->getName(), nv);
				LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Called OCCViewer::setGeometryVisible for visibility change");
				// Note: Refresh is now handled by SelectionManager::setGeometryVisible to avoid duplicate refreshes
			}
			updateTreeItemIcon(item, nv);
			LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Updated tree icon for visibility change");
			return;
		}
		if (column == 2) {
			// Column 2: Change color (palette icon)
			LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Opening color dialog for geometry: " + geometry->getName());
			wxColourData cd; cd.SetChooseFull(true);
			wxColourDialog dlg(this, &cd);
			if (dlg.ShowModal() == wxID_OK && m_occViewer) {
				wxColour c = dlg.GetColourData().GetColour();
				LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Color selected: RGB(" +
					std::to_string(c.Red()) + "," + std::to_string(c.Green()) + "," + std::to_string(c.Blue()) + ") for geometry: " + geometry->getName());

				m_occViewer->setGeometryColor(geometry->getName(), Quantity_Color(c.Red() / 255.0, c.Green() / 255.0, c.Blue() / 255.0, Quantity_TOC_RGB));
				LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Called OCCViewer::setGeometryColor for color change");

				// Trigger mesh regeneration to apply color change with custom colors
				if (geometry) {
					geometry->setForceCustomColor(true);  // Force use of custom colors even when selected
					// Force rebuild by setting regeneration flag and calling build directly
					geometry->setMeshRegenerationNeeded(true);
					geometry->buildCoinRepresentation(m_occViewer->getMeshParameters());
					LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Rebuilt Coin3D representation for color change with custom colors forced");

					// Touch Coin3D node to force update
					if (geometry->getCoinNode()) {
						geometry->getCoinNode()->touch();
						LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Touched Coin3D node to force color update rendering");
					}
				}

				// Request view refresh to show the color change
				m_occViewer->requestViewRefresh();
				LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Requested view refresh from OCCViewer");

				// Force Canvas refresh through SceneManager
				if (m_occViewer->getSceneManager()) {
					Canvas* canvas = m_occViewer->getSceneManager()->getCanvas();
					if (canvas) {
						if (canvas->getRefreshManager()) {
							canvas->getRefreshManager()->requestRefresh(ViewRefreshManager::RefreshReason::RENDERING_CHANGED, true);
							LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Requested rendering change refresh from ViewRefreshManager");
						}
						canvas->Refresh(true);
						canvas->Update();
						LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Forced Canvas refresh and update for color change");
					}
				}
			} else {
				LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Color dialog cancelled for geometry: " + geometry->getName());
			}
			return;
		}
		if (column == 3) {
			// Column 3: Edit notes/name (edit icon)
			LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Starting notes/name editing for geometry: " + geometry->getName());
			editSelectedObjectNotes();
			LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Completed notes/name editing for geometry: " + geometry->getName());
			return;
		}
		if (column == 4) {
			// Column 4: Delete object (delete icon)
			LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Starting deletion process for geometry: " + geometry->getName());
			deleteSelectedObject();
			LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Completed deletion process for geometry: " + geometry->getName());
			return;
		}

		// Column 0 (tree item text): Select geometry and show properties
		// Note: Selection is already handled above, just show properties
		if (m_propertyPanel) {
			m_propertyPanel->updateProperties(geometry);
			LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Updated property panel with geometry: " + geometry->getName());
		}
		LOG_INF_S("ObjectTreePanel::onTreeItemClicked - Selection and property display completed for geometry: " + geometry->getName());
		return;
	}
	
	// Refresh tree view to show selection changes
	m_treeView->Refresh();
}

// Legacy dataview activation handler removed after integration with FlatTreeView

void ObjectTreePanel::updateTreeSelectionFromViewer()
{
	LOG_INF_S("ObjectTreePanel::updateTreeSelectionFromViewer - Starting viewer-to-tree selection sync");
	if (!m_occViewer) {
		LOG_WRN_S("ObjectTreePanel::updateTreeSelectionFromViewer - OCCViewer is null, cannot sync selection");
		return;
	}

	m_isUpdatingSelection = true;
	LOG_INF_S("ObjectTreePanel::updateTreeSelectionFromViewer - Set updating selection flag to prevent recursive updates");

	// Clear previous selection
	if (m_lastSelectedItem) {
		m_lastSelectedItem->SetSelected(false);
		LOG_INF_S("ObjectTreePanel::updateTreeSelectionFromViewer - Cleared previous tree selection");
	}
	m_lastSelectedItem.reset();

	// Select geometries that are selected in viewer
	auto selectedGeometries = m_occViewer->getSelectedGeometries();
	LOG_INF_S("ObjectTreePanel::updateTreeSelectionFromViewer - Viewer has " + std::to_string(selectedGeometries.size()) + " selected geometries");

	if (!selectedGeometries.empty()) {
		// Select the first one (single selection mode)
		auto geometry = selectedGeometries[0];
		LOG_INF_S("ObjectTreePanel::updateTreeSelectionFromViewer - Processing first selected geometry: " + geometry->getName());

		auto it = m_occGeometryMap.find(geometry);
		if (it != m_occGeometryMap.end()) {
			m_lastSelectedItem = it->second;
			m_lastSelectedItem->SetSelected(true);
			LOG_INF_S("ObjectTreePanel::updateTreeSelectionFromViewer - Successfully selected tree item for geometry: " + geometry->getName());
		}
		else {
			LOG_WRN_S("ObjectTreePanel::updateTreeSelectionFromViewer - Geometry not found in tree map: " + geometry->getName() + ", cannot update tree selection");
		}

		if (selectedGeometries.size() > 1) {
			LOG_INF_S("ObjectTreePanel::updateTreeSelectionFromViewer - Warning: Multiple geometries selected in viewer (" +
				std::to_string(selectedGeometries.size()) + "), only first one synced to tree");
		}
	}
	else {
		LOG_INF_S("ObjectTreePanel::updateTreeSelectionFromViewer - No geometries selected in viewer, tree selection cleared");
	}

	m_isUpdatingSelection = false;
	m_treeView->Refresh();
	LOG_INF_S("ObjectTreePanel::updateTreeSelectionFromViewer - Tree view refreshed, viewer-to-tree selection sync completed");
}