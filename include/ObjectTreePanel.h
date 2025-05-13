#pragma once

#include <wx/panel.h>
#include <wx/treectrl.h>
#include <map>
#include "GeometryObject.h"
#include "PropertyPanel.h"


class ObjectTreePanel : public wxPanel
{
public:
    ObjectTreePanel(wxWindow* parent);
    ~ObjectTreePanel();

    void addObject(GeometryObject* object);
    void removeObject(GeometryObject* object);
    void updateObjectName(GeometryObject* object);
    void setPropertyPanel(PropertyPanel* panel);

private:
    void onSelectionChanged(wxTreeEvent& event);

    wxTreeCtrl* m_treeCtrl;
    wxTreeItemId m_rootId;
    std::map<GeometryObject*, wxTreeItemId> m_objectMap;
    PropertyPanel* m_propertyPanel;
};