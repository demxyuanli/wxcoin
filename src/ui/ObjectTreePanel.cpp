#include "ObjectTreePanel.h"
#include "GeometryObject.h"
#include "OCCGeometry.h"
#include "OCCViewer.h"
#include "Canvas.h"
#include "logger/Logger.h"
#include "PropertyPanel.h"
#include <wx/treectrl.h>
#include <wx/sizer.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <vector>
#include <algorithm>

ObjectTreePanel::ObjectTreePanel(wxWindow* parent)
    : FlatUITitledPanel(parent, "CAD Object Tree")
    , m_propertyPanel(nullptr)
    , m_occViewer(nullptr)
    , m_isUpdatingSelection(false)
    , m_contextMenu(nullptr)
{
    LOG_INF_S("ObjectTreePanel initializing");
    m_treeCtrl = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE | wxTR_SINGLE);
    m_mainSizer->Add(m_treeCtrl, 1, wxEXPAND | wxALL, 2);

    m_rootId = m_treeCtrl->AddRoot("Scene");

    m_treeCtrl->Bind(wxEVT_TREE_SEL_CHANGED, &ObjectTreePanel::onSelectionChanged, this);
    m_treeCtrl->Bind(wxEVT_TREE_ITEM_ACTIVATED, &ObjectTreePanel::onTreeItemActivated, this);
    m_treeCtrl->Bind(wxEVT_TREE_ITEM_RIGHT_CLICK, &ObjectTreePanel::onTreeItemRightClick, this);
    
    // Bind keyboard events for shortcuts
    m_treeCtrl->Bind(wxEVT_KEY_DOWN, &ObjectTreePanel::onKeyDown, this);
    
    // Create context menu
    createContextMenu();
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
    wxTreeItemId itemId = m_treeCtrl->AppendItem(m_rootId, object->getName());
    m_objectMap[object] = itemId;
    m_treeCtrl->Expand(m_rootId);
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
    m_treeCtrl->Delete(it->second);
    m_objectMap.erase(it);
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
    m_treeCtrl->SetItemText(it->second, object->getName());
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

    LOG_INF_S("Adding OCCGeometry to tree: " + geometry->getName() + " (total items: " + std::to_string(m_occGeometryMap.size()) + ")");
    
    // Create item text with visibility indicator
    wxString itemText = geometry->getName();
    if (!geometry->isVisible()) {
        itemText = "[H] " + itemText;
    }
    
    wxTreeItemId itemId = m_treeCtrl->AppendItem(m_rootId, itemText);
    if (!itemId.IsOk()) {
        LOG_ERR_S("Failed to create tree item for geometry: " + geometry->getName());
        return;
    }
    
    m_occGeometryMap[geometry] = itemId;
    m_treeItemToOCCGeometry[itemId] = geometry;
    m_treeCtrl->Expand(m_rootId);
    
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

    LOG_INF_S("Removing OCCGeometry from tree: " + geometry->getName());
    wxTreeItemId itemId = it->second;
    m_treeCtrl->Delete(itemId);
    m_occGeometryMap.erase(it);
    m_treeItemToOCCGeometry.erase(itemId);
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

    LOG_INF_S("Updating OCCGeometry name in tree: " + geometry->getName());
    
    // Update item text with visibility indicator
    wxString itemText = geometry->getName();
    if (!geometry->isVisible()) {
        itemText = "[H] " + itemText;
    }
    
    m_treeCtrl->SetItemText(it->second, itemText);
}

void ObjectTreePanel::selectOCCGeometry(std::shared_ptr<OCCGeometry> geometry)
{
    if (!geometry) return;
    
    auto it = m_occGeometryMap.find(geometry);
    if (it == m_occGeometryMap.end()) return;
    
    m_isUpdatingSelection = true;
    m_treeCtrl->SelectItem(it->second);
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
    } else {
        showSelectedObject();
    }
}

void ObjectTreePanel::showAllObjects()
{
    LOG_INF_S("Showing all objects");
    if (!m_occViewer) {
        LOG_WRN_S("OCCViewer is null");
        return;
    }
    
    // Use OCCViewer's showAll method for better consistency
    m_occViewer->showAll();
    
    // Update tree item icons
    auto allGeometries = m_occViewer->getAllGeometry();
    for (const auto& geometry : allGeometries) {
        auto it = m_occGeometryMap.find(geometry);
        if (it != m_occGeometryMap.end()) {
            updateTreeItemIcon(it->second, true);
        }
    }
}

void ObjectTreePanel::hideAllObjects()
{
    LOG_INF_S("Hiding all objects");
    if (!m_occViewer) {
        LOG_WRN_S("OCCViewer is null");
        return;
    }
    
    // Use OCCViewer's hideAll method for better consistency
    m_occViewer->hideAll();
    
    // Update tree item icons
    auto allGeometries = m_occViewer->getAllGeometry();
    for (const auto& geometry : allGeometries) {
        auto it = m_occGeometryMap.find(geometry);
        if (it != m_occGeometryMap.end()) {
            updateTreeItemIcon(it->second, false);
        }
    }
}

// Event handlers
void ObjectTreePanel::onTreeItemRightClick(wxTreeEvent& event)
{
    m_rightClickedItem = event.GetItem();
    
    if (!m_rightClickedItem.IsOk() || m_rightClickedItem == m_rootId) {
        return; // Don't show context menu for root or invalid items
    }
    // Ensure right-click selects the item so actions operate on it
    m_isUpdatingSelection = true;
    m_treeCtrl->SelectItem(m_rightClickedItem);
    m_isUpdatingSelection = false;
    
    // Show context menu
    wxPoint ptTree = event.GetPoint();
    // Convert tree-local point to this panel's client coords
    wxPoint screenPt = m_treeCtrl->ClientToScreen(ptTree);
    wxPoint panelPt = ScreenToClient(screenPt);
    PopupMenu(m_contextMenu, panelPt);
}

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

void ObjectTreePanel::updateTreeItemIcon(wxTreeItemId itemId, bool visible)
{
    if (!itemId.IsOk()) return;
    
    // Update the item text to show visibility status
    wxString currentText = m_treeCtrl->GetItemText(itemId);
    wxString newText = currentText;
    
    if (visible) {
        // Remove hidden indicator if present
        if (newText.StartsWith("[H] ")) {
            newText = newText.Mid(4);
        }
    } else {
        // Add hidden indicator if not present
        if (!newText.StartsWith("[H] ")) {
            newText = "[H] " + newText;
        }
    }
    
    if (newText != currentText) {
        m_treeCtrl->SetItemText(itemId, newText);
    }
}

std::shared_ptr<OCCGeometry> ObjectTreePanel::getSelectedOCCGeometry()
{
    wxTreeItemId selectedItem = m_treeCtrl->GetSelection();
    if (!selectedItem.IsOk() || selectedItem == m_rootId) {
        return nullptr;
    }
    
    auto it = m_treeItemToOCCGeometry.find(selectedItem);
    if (it != m_treeItemToOCCGeometry.end()) {
        return it->second;
    }
    
    return nullptr;
}

void ObjectTreePanel::deselectOCCGeometry(std::shared_ptr<OCCGeometry> geometry)
{
    if (!geometry) return;
    
    auto it = m_occGeometryMap.find(geometry);
    if (it == m_occGeometryMap.end()) return;
    
    wxTreeItemId currentSelection = m_treeCtrl->GetSelection();
    if (currentSelection == it->second) {
        m_isUpdatingSelection = true;
        m_treeCtrl->Unselect();
        m_isUpdatingSelection = false;
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

void ObjectTreePanel::onSelectionChanged(wxTreeEvent& event)
{
    if (m_isUpdatingSelection) return;
    
    wxTreeItemId itemId = event.GetItem();
    if (!itemId.IsOk()) {
        LOG_WRN_S("Invalid tree item selected");
        return;
    }

    if (itemId == m_rootId) {
        LOG_INF_S("Root item selected");
        if (m_propertyPanel) {
            m_propertyPanel->clearProperties();
        }
        // Deselect all geometries
        if (m_occViewer) {
            m_occViewer->deselectAll();
        }
        return;
    }

    // Check if it's an OCCGeometry
    auto occIt = m_treeItemToOCCGeometry.find(itemId);
    if (occIt != m_treeItemToOCCGeometry.end()) {
        std::shared_ptr<OCCGeometry> geometry = occIt->second;
        LOG_INF_S("Selected OCCGeometry in tree: " + geometry->getName());
        
        // Update viewer selection
        if (m_occViewer) {
            m_occViewer->deselectAll();
            m_occViewer->setGeometrySelected(geometry->getName(), true);
            LOG_INF_S("Updated OCCViewer selection for: " + geometry->getName());
        } else {
            LOG_WRN_S("OCCViewer is null in ObjectTreePanel");
        }
        
        // Update property panel
        if (m_propertyPanel) {
            m_propertyPanel->updateProperties(geometry);
            LOG_INF_S("Updated PropertyPanel for OCCGeometry: " + geometry->getName());
        } else {
            LOG_WRN_S("PropertyPanel is null in ObjectTreePanel");
        }
        return;
    }

    // Legacy GeometryObject handling
    GeometryObject* selectedObject = nullptr;
    for (const auto& pair : m_objectMap) {
        if (pair.second == itemId) {
            selectedObject = pair.first;
            break;
        }
    }

    if (selectedObject) {
        LOG_INF_S("Selected object in tree: " + selectedObject->getName());
        selectedObject->setSelected(true);
        if (m_propertyPanel) {
            m_propertyPanel->updateProperties(selectedObject);
        }
    }
}

void ObjectTreePanel::onTreeItemActivated(wxTreeEvent& event)
{
    // Handle double-click or activation
    onSelectionChanged(event);
}

void ObjectTreePanel::updateTreeSelectionFromViewer()
{
    if (!m_occViewer) {
        LOG_WRN_S("OCCViewer is null in updateTreeSelectionFromViewer");
        return;
    }
    
    m_isUpdatingSelection = true;
    
    // Clear current selection
    m_treeCtrl->Unselect();
    
    // Select geometries that are selected in viewer
    auto selectedGeometries = m_occViewer->getSelectedGeometries();
    LOG_INF_S("Updating tree selection from viewer - selected geometries: " + std::to_string(selectedGeometries.size()));
    
    if (!selectedGeometries.empty()) {
        // Select the first one (single selection mode)
        auto geometry = selectedGeometries[0];
        auto it = m_occGeometryMap.find(geometry);
        if (it != m_occGeometryMap.end()) {
            m_treeCtrl->SelectItem(it->second);
            LOG_INF_S("Selected tree item for geometry: " + geometry->getName());
        } else {
            LOG_WRN_S("Geometry not found in tree map: " + geometry->getName());
        }
    } else {
        LOG_INF_S("No geometries selected in viewer");
    }
    
    m_isUpdatingSelection = false;
}
