#pragma once

#include <wx/panel.h>
#include <wx/treectrl.h>
#include <map>
#include <vector>
#include <memory>
#include "GeometryObject.h"
#include "PropertyPanel.h"
#include "OCCGeometry.h"

// Forward declarations
class OCCViewer;

class ObjectTreePanel : public wxPanel
{
public:
    ObjectTreePanel(wxWindow* parent);
    ~ObjectTreePanel();

    // Legacy GeometryObject support
    void addObject(GeometryObject* object);
    void removeObject(GeometryObject* object);
    void updateObjectName(GeometryObject* object);
    
    // OCCGeometry support
    void addOCCGeometry(std::shared_ptr<OCCGeometry> geometry);
    void removeOCCGeometry(std::shared_ptr<OCCGeometry> geometry);
    void updateOCCGeometryName(std::shared_ptr<OCCGeometry> geometry);
    void selectOCCGeometry(std::shared_ptr<OCCGeometry> geometry);
    void deselectOCCGeometry(std::shared_ptr<OCCGeometry> geometry);
    
    // Setup methods
    void setPropertyPanel(PropertyPanel* panel);
    void setOCCViewer(OCCViewer* viewer);
    
    PropertyPanel* getPropertyPanel() const { return m_propertyPanel; }
    
    // Public method for OCCViewer to update tree selection
    void updateTreeSelectionFromViewer();
    
private:
    void onSelectionChanged(wxTreeEvent& event);
    void onTreeItemActivated(wxTreeEvent& event);


    wxTreeCtrl* m_treeCtrl;
    wxTreeItemId m_rootId;
    
    // Legacy GeometryObject support
    std::map<GeometryObject*, wxTreeItemId> m_objectMap;
    
    // OCCGeometry support
    std::map<std::shared_ptr<OCCGeometry>, wxTreeItemId> m_occGeometryMap;
    std::map<wxTreeItemId, std::shared_ptr<OCCGeometry>> m_treeItemToOCCGeometry;
    
    PropertyPanel* m_propertyPanel;
    OCCViewer* m_occViewer;
    
    bool m_isUpdatingSelection; // Prevent recursive updates
};