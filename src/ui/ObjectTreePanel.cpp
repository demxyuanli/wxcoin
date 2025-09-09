#include "ObjectTreePanel.h"
#include "GeometryObject.h"
#include "OCCGeometry.h"
#include "OCCViewer.h"
#include "widgets/FlatTreeView.h"
#include "Canvas.h"
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
		sizer->Add(m_treeView, 1, wxEXPAND | wxALL, 0);
	}

	// Columns: Tree | Vis | Del | Color | Edit
	m_treeView->AddColumn("Vis", FlatTreeColumn::ColumnType::ICON, 28);
	m_treeView->AddColumn("Del", FlatTreeColumn::ColumnType::ICON, 28);
	m_treeView->AddColumn("Color", FlatTreeColumn::ColumnType::ICON, 28);
	m_treeView->AddColumn("Edit", FlatTreeColumn::ColumnType::ICON, 28);

	// Set SVG icons for columns (12x12 size, no text)
	m_treeView->SetColumnSvgIcon(1, "eyeopen", wxSize(12, 12));      // Visibility column
	m_treeView->SetColumnSvgIcon(2, "delete", wxSize(12, 12));       // Delete column
	m_treeView->SetColumnSvgIcon(3, "palette", wxSize(12, 12));      // Color column
	m_treeView->SetColumnSvgIcon(4, "edit", wxSize(12, 12));         // Edit column

	// Icons (fallback to bitmap if SVG not available)
	m_bmpEyeOpen = wxArtProvider::GetBitmap(wxART_TICK_MARK, wxART_BUTTON, wxSize(12, 12));
	m_bmpEyeClosed = wxArtProvider::GetBitmap(wxART_CROSS_MARK, wxART_BUTTON, wxSize(12, 12));
	m_bmpDelete = wxArtProvider::GetBitmap(wxART_DELETE, wxART_BUTTON, wxSize(12, 12));
	m_bmpColor = wxArtProvider::GetBitmap(wxART_TIP, wxART_BUTTON, wxSize(12, 12));
	m_bmpEdit = wxArtProvider::GetBitmap(wxART_EDIT, wxART_BUTTON, wxSize(12, 12));

	// Build root
	m_rootItem = std::make_shared<FlatTreeItem>("Root", FlatTreeItem::ItemType::ROOT);
	m_treeView->SetItemSvgIcon(m_rootItem, "folder", wxSize(12, 12));
	m_rootItem->SetExpanded(true);
	m_treeView->SetRoot(m_rootItem);
	m_partRootItem.reset();

	// Click handling
	m_treeView->OnItemClicked([this](std::shared_ptr<FlatTreeItem> item, int col) { onTreeItemClicked(item, col); });

	// Keyboard shortcuts from panel
	Bind(wxEVT_KEY_DOWN, &ObjectTreePanel::onKeyDown, this);

	// Context menu
	createContextMenu();

	// Tab 2: History (Undo/Redo trees)
	{
		wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
		m_tabHistory->SetSizer(sizer);
		m_historyView = new FlatTreeView(m_tabHistory, wxID_ANY, wxDefaultPosition, wxDefaultSize);
		m_historyView->SetItemHeight(18);
		m_historyView->SetIndentWidth(14);
		m_historyView->SetDoubleBuffered(true);
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

void ObjectTreePanel::addOCCGeometry(std::shared_ptr<OCCGeometry> geometry)
{
	if (!geometry) {
		LOG_ERR_S("Attempted to add null OCCGeometry to tree");
		return;
	}
	if (m_occGeometryMap.find(geometry) != m_occGeometryMap.end()) {
		LOG_WRN_S("OCCGeometry already exists in tree: " + geometry->getName());
		return;
	}

	LOG_DBG("Adding OCCGeometry to tree", "ObjectTreePanel");

	// Flat hierarchy: add geometry item directly under root with a type-distinguishing icon
	m_treeView->Freeze();
	auto featureItem = std::make_shared<FlatTreeItem>(geometry->getName(), FlatTreeItem::ItemType::FILE);

	// Set SVG icon for the tree item (12x12 size)
	m_treeView->SetItemSvgIcon(featureItem, "file", wxSize(12, 12));

	// Set SVG icons for columns (12x12 size)
	m_treeView->SetItemColumnSvgIcon(featureItem, 1, geometry->isVisible() ? "eyeopen" : "eyeclosed", wxSize(12, 12));
	m_treeView->SetItemColumnSvgIcon(featureItem, 2, "delete", wxSize(12, 12));
	m_treeView->SetItemColumnSvgIcon(featureItem, 3, "palette", wxSize(12, 12));
	m_treeView->SetItemColumnSvgIcon(featureItem, 4, "edit", wxSize(12, 12));

	// Fallback to bitmap icons if SVG not available
	featureItem->SetIcon(wxArtProvider::GetBitmap(wxART_NORMAL_FILE, wxART_OTHER, wxSize(12, 12)));
	featureItem->SetColumnIcon(1, geometry->isVisible() ? m_bmpEyeOpen : m_bmpEyeClosed);
	featureItem->SetColumnIcon(2, m_bmpDelete);
	featureItem->SetColumnIcon(3, m_bmpColor);
	featureItem->SetColumnIcon(4, m_bmpEdit);

	m_rootItem->AddChild(featureItem);
	m_treeView->Thaw();
	m_treeView->Refresh();

	m_occGeometryMap[geometry] = featureItem;
	m_treeItemToOCCGeometry[featureItem] = geometry;

	LOG_INF_S("Successfully added OCCGeometry to tree: " + geometry->getName() + " (new total: " + std::to_string(m_occGeometryMap.size()) + ")");
}

void ObjectTreePanel::removeOCCGeometry(std::shared_ptr<OCCGeometry> geometry)
{
	if (!geometry) {
		LOG_ERR_S("Attempted to remove null OCCGeometry from tree");
		return;
	}

	auto it = m_occGeometryMap.find(geometry);
	if (it == m_occGeometryMap.end()) {
		LOG_WRN_S("OCCGeometry not found in tree: " + geometry->getName());
		return;
	}

	LOG_DBG("Removing OCCGeometry from tree", "ObjectTreePanel");
	m_treeView->Freeze();
	auto featureItem = it->second;
	// Remove from parent
	if (featureItem && featureItem->GetParent()) {
		featureItem->GetParent()->RemoveChild(featureItem);
	}
	m_occGeometryMap.erase(it);
	m_treeItemToOCCGeometry.erase(featureItem);
	m_treeView->Thaw();
	m_treeView->Refresh();
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

	LOG_DBG("Updating OCCGeometry name in tree", "ObjectTreePanel");
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
	auto geometry = getSelectedOCCGeometry();
	if (!geometry) {
		LOG_WRN_S("No object selected for deletion");
		return;
	}

	wxString message = wxString::Format("Are you sure you want to delete '%s'?", geometry->getName());
	int result = wxMessageBox(message, "Confirm Delete", wxYES_NO | wxICON_QUESTION);

	if (result == wxYES) {
		LOG_INF_S("Deleting object: " + geometry->getName());
		if (m_occViewer) {
			m_occViewer->removeGeometry(geometry->getName());
		}
		removeOCCGeometry(geometry);
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
	if (it != m_treeItemToOCCGeometry.end()) return it->second;
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
	if (!item) return;
	if (m_isUpdatingSelection) return;
	
	// Clear previous selection
	if (m_lastSelectedItem) {
		m_lastSelectedItem->SetSelected(false);
	}
	
	// Set new selection
	m_lastSelectedItem = item;
	item->SetSelected(true);

	// Root -> clear selection
	if (item == m_rootItem) {
		if (m_propertyPanel) m_propertyPanel->clearProperties();
		if (m_occViewer) m_occViewer->deselectAll();
		// Clear tree selection
		if (m_lastSelectedItem) {
			m_lastSelectedItem->SetSelected(false);
			m_lastSelectedItem.reset();
		}
		m_treeView->Refresh();
		return;
	}

	auto itOcc = m_treeItemToOCCGeometry.find(item);
	if (itOcc != m_treeItemToOCCGeometry.end()) {
		auto geometry = itOcc->second;
		if (!geometry) return;

		if (column == 1) {
			bool nv = !geometry->isVisible();
			if (m_occViewer) m_occViewer->setGeometryVisible(geometry->getName(), nv);
			updateTreeItemIcon(item, nv);
			return;
		}
		if (column == 2) { deleteSelectedObject(); return; }
		if (column == 3) {
			wxColourData cd; cd.SetChooseFull(true);
			wxColourDialog dlg(this, &cd);
			if (dlg.ShowModal() == wxID_OK && m_occViewer) {
				wxColour c = dlg.GetColourData().GetColour();
				m_occViewer->setGeometryColor(geometry->getName(), Quantity_Color(c.Red() / 255.0, c.Green() / 255.0, c.Blue() / 255.0, Quantity_TOC_RGB));
			}
			return;
		}
		if (column == 4) {
			if (m_propertyPanel) m_propertyPanel->updateProperties(geometry);
			return;
		}

		if (m_occViewer) {
			m_isUpdatingSelection = true;
			m_occViewer->deselectAll();
			m_occViewer->setGeometrySelected(geometry->getName(), true);
			m_isUpdatingSelection = false;
		}
		if (m_propertyPanel) m_propertyPanel->updateProperties(geometry);
		return;
	}
	
	// Refresh tree view to show selection changes
	m_treeView->Refresh();
}

// Legacy dataview activation handler removed after integration with FlatTreeView

void ObjectTreePanel::updateTreeSelectionFromViewer()
{
	if (!m_occViewer) {
		LOG_WRN_S("OCCViewer is null in updateTreeSelectionFromViewer");
		return;
	}

	m_isUpdatingSelection = true;

	// Clear previous selection
	if (m_lastSelectedItem) {
		m_lastSelectedItem->SetSelected(false);
	}
	m_lastSelectedItem.reset();

	// Select geometries that are selected in viewer
	auto selectedGeometries = m_occViewer->getSelectedGeometries();
	LOG_INF_S("Updating tree selection from viewer - selected geometries: " + std::to_string(selectedGeometries.size()));

	if (!selectedGeometries.empty()) {
		// Select the first one (single selection mode)
		auto geometry = selectedGeometries[0];
		auto it = m_occGeometryMap.find(geometry);
		if (it != m_occGeometryMap.end()) {
			m_lastSelectedItem = it->second;
			m_lastSelectedItem->SetSelected(true);
			LOG_INF_S("Selected tree item for geometry: " + geometry->getName());
		}
		else {
			LOG_WRN_S("Geometry not found in tree map: " + geometry->getName());
		}
	}
	else {
		LOG_INF_S("No geometries selected in viewer");
	}

	m_isUpdatingSelection = false;
	m_treeView->Refresh();
}