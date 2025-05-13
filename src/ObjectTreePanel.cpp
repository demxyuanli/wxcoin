#include "ObjectTreePanel.h"
#include "GeometryObject.h"
#include "Logger.h"
#include "PropertyPanel.h"
#include <wx/treectrl.h>
#include <wx/sizer.h>

ObjectTreePanel::ObjectTreePanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
    , m_propertyPanel(nullptr)
{
    LOG_INF("ObjectTreePanel initializing");
    m_treeCtrl = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE | wxTR_SINGLE);

    wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_treeCtrl, 1, wxEXPAND | wxALL, 5);
    SetSizer(sizer);

    m_rootId = m_treeCtrl->AddRoot("Scene");

    m_treeCtrl->Bind(wxEVT_TREE_SEL_CHANGED, &ObjectTreePanel::onSelectionChanged, this);
}

ObjectTreePanel::~ObjectTreePanel()
{
    LOG_INF("ObjectTreePanel destroying");
}

void ObjectTreePanel::addObject(GeometryObject* object)
{
    if (!object) {
        LOG_ERR("Attempted to add null object to tree");
        return;
    }
    if (m_objectMap.find(object) != m_objectMap.end()) {
        LOG_WAR("Object already exists in tree: " + object->getName());
        return;
    }

    LOG_INF("Adding object to tree: " + object->getName());
    wxTreeItemId itemId = m_treeCtrl->AppendItem(m_rootId, object->getName());
    m_objectMap[object] = itemId;
    m_treeCtrl->Expand(m_rootId);
}

void ObjectTreePanel::removeObject(GeometryObject* object)
{
    if (!object) {
        LOG_ERR("Attempted to remove null object from tree");
        return;
    }

    auto it = m_objectMap.find(object);
    if (it == m_objectMap.end()) {
        LOG_WAR("Object not found in tree: " + object->getName());
        return;
    }

    LOG_INF("Removing object from tree: " + object->getName());
    m_treeCtrl->Delete(it->second);
    m_objectMap.erase(it);
}

void ObjectTreePanel::updateObjectName(GeometryObject* object)
{
    if (!object) {
        LOG_ERR("Attempted to update name of null object");
        return;
    }

    auto it = m_objectMap.find(object);
    if (it == m_objectMap.end()) {
        LOG_WAR("Object not found in tree for name update: " + object->getName());
        return;
    }

    LOG_INF("Updating object name in tree: " + object->getName());
    m_treeCtrl->SetItemText(it->second, object->getName());
}

void ObjectTreePanel::setPropertyPanel(PropertyPanel* panel)
{
    m_propertyPanel = panel;
    LOG_INF("PropertyPanel set for ObjectTreePanel");
}

void ObjectTreePanel::onSelectionChanged(wxTreeEvent& event)
{
    wxTreeItemId itemId = event.GetItem();
    if (!itemId.IsOk()) {
        LOG_WAR("Invalid tree item selected");
        return;
    }

    if (itemId == m_rootId) {
        LOG_INF("Root item selected");
        if (m_propertyPanel) {
            m_propertyPanel->clearProperties();
        }
        return;
    }

    GeometryObject* selectedObject = nullptr;
    for (const auto& pair : m_objectMap) {
        if (pair.second == itemId) {
            selectedObject = pair.first;
            break;
        }
    }

    if (selectedObject) {
        LOG_INF("Selected object in tree: " + selectedObject->getName());
        selectedObject->setSelected(true);
        if (m_propertyPanel) {
            m_propertyPanel->updateProperties(selectedObject);
        }
    }
}