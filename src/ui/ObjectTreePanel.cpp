#include "ObjectTreePanel.h"
#include "GeometryObject.h"
#include "OCCGeometry.h"
#include "OCCViewer.h"
#include "Canvas.h"
#include "UnifiedRefreshSystem.h"
#include "logger/Logger.h"
#include "PropertyPanel.h"
#include <wx/treectrl.h>
#include <wx/sizer.h>
#include <vector>
#include <algorithm>
#include "GlobalServices.h"

ObjectTreePanel::ObjectTreePanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
    , m_propertyPanel(nullptr)
    , m_occViewer(nullptr)
    , m_isUpdatingSelection(false)
{
    LOG_INF_S("ObjectTreePanel initializing");
    m_treeCtrl = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE | wxTR_SINGLE);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_treeCtrl, 1, wxEXPAND | wxALL, 5);
    SetSizer(sizer);

    m_rootId = m_treeCtrl->AddRoot("Scene");

    m_treeCtrl->Bind(wxEVT_TREE_SEL_CHANGED, &ObjectTreePanel::onSelectionChanged, this);
    m_treeCtrl->Bind(wxEVT_TREE_ITEM_ACTIVATED, &ObjectTreePanel::onTreeItemActivated, this);
}

ObjectTreePanel::~ObjectTreePanel()
{
    LOG_INF_S("ObjectTreePanel destroying");
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

    LOG_INF_S("Adding OCCGeometry to tree: " + geometry->getName());
    wxTreeItemId itemId = m_treeCtrl->AppendItem(m_rootId, geometry->getName());
    m_occGeometryMap[geometry] = itemId;
    m_treeItemToOCCGeometry[itemId] = geometry;
    m_treeCtrl->Expand(m_rootId);
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
    m_treeCtrl->SetItemText(it->second, geometry->getName());
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

    Canvas* canvas = nullptr;
    wxWindow* parent = GetParent();
    while (parent && !canvas) {
        canvas = dynamic_cast<Canvas*>(parent);
        if (!canvas) {
            parent = parent->GetParent();
        }
    }

    if (itemId == m_rootId) {
        LOG_INF_S("Root item selected");
        if (m_propertyPanel) {
            m_propertyPanel->clearProperties();
        }
        // Deselect all geometries
        if (m_occViewer) {
            m_occViewer->deselectAll();
            
            // Use unified refresh system for selection changes
            UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
            if (refreshSystem) {
                refreshSystem->refreshView("", false);  // Selection cleared
            }
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
            
            // Use unified refresh system for selection changes
            UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
            if (refreshSystem) {
                refreshSystem->refreshView(geometry->getName(), false);  // Selection changed
            }
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
        
        // Use unified refresh system for legacy object selection
        UnifiedRefreshSystem* refreshSystem = GlobalServices::GetRefreshSystem();
        if (refreshSystem) {
            refreshSystem->refreshView(selectedObject->getName(), false);
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
