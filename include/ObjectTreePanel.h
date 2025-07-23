#pragma once

#include "flatui/FlatUITitledPanel.h"
#include <wx/treectrl.h>
#include <map>
#include <vector>
#include <memory>
#include "GeometryObject.h"
#include "PropertyPanel.h"
#include "OCCGeometry.h"

// Forward declarations
class OCCViewer;

class ObjectTreePanel : public FlatUITitledPanel
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
    
    // Object management functions
    void deleteSelectedObject();
    void hideSelectedObject();
    void showSelectedObject();
    void toggleObjectVisibility();
    void showAllObjects();
    void hideAllObjects();
    
    // Setup methods
    void setPropertyPanel(PropertyPanel* panel);
    void setOCCViewer(OCCViewer* viewer);
    
    PropertyPanel* getPropertyPanel() const { return m_propertyPanel; }
    
    // Public method for OCCViewer to update tree selection
    void updateTreeSelectionFromViewer();
    
private:
    void onSelectionChanged(wxTreeEvent& event);
    void onTreeItemActivated(wxTreeEvent& event);
    void onTreeItemRightClick(wxTreeEvent& event);
    void onKeyDown(wxKeyEvent& event);
    void onDeleteObject(wxCommandEvent& event);
    void onHideObject(wxCommandEvent& event);
    void onShowObject(wxCommandEvent& event);
    void onToggleVisibility(wxCommandEvent& event);
    void onShowAllObjects(wxCommandEvent& event);
    void onHideAllObjects(wxCommandEvent& event);
    
    void createContextMenu();
    void updateTreeItemIcon(wxTreeItemId itemId, bool visible);
    std::shared_ptr<OCCGeometry> getSelectedOCCGeometry();

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
    
    // Context menu
    wxMenu* m_contextMenu;
    wxTreeItemId m_rightClickedItem;
};