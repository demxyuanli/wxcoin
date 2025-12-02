#pragma once

#include "flatui/FlatUITitledPanel.h"
#include "widgets/FlatTreeView.h"
#include <wx/imaglist.h>
#include <wx/artprov.h>
#include "ui/FlatBarNotebook.h"
#include <map>
#include <vector>
#include <memory>
#include <algorithm>
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
	void addOCCGeometry(std::shared_ptr<OCCGeometry> geometry, std::shared_ptr<OCCGeometry> parentGeometry);
	void addOCCGeometryFromFile(const wxString& fileName, std::shared_ptr<OCCGeometry> geometry);
	void addOCCGeometryFromFile(const wxString& fileName, std::shared_ptr<OCCGeometry> geometry, bool immediateRefresh);
	void removeOCCGeometry(std::shared_ptr<OCCGeometry> geometry);
	void updateOCCGeometryName(std::shared_ptr<OCCGeometry> geometry);
	void selectOCCGeometry(std::shared_ptr<OCCGeometry> geometry);
	void deselectOCCGeometry(std::shared_ptr<OCCGeometry> geometry);

	// Object management functions
	void deleteSelectedObject();
	void editSelectedObjectNotes();
	void hideSelectedObject();
	void showSelectedObject();
	void toggleObjectVisibility();
	void showAllObjects();
	void hideAllObjects();

	// Setup methods
	void setPropertyPanel(PropertyPanel* panel);
	void setOCCViewer(OCCViewer* viewer);

	PropertyPanel* getPropertyPanel() const { return m_propertyPanel; }
	bool isUpdatingSelection() const { return m_isUpdatingSelection; }

	
	// Tree data structure management
	void initializeTreeDataStructure();
	void updateTreeDataStructure();
	void refreshTreeDisplay();
	void addOCCGeometryToNode(std::shared_ptr<FlatTreeItem> parentNode, std::shared_ptr<OCCGeometry> geometry);

	// Public method for OCCViewer to update tree selection
	void updateTreeSelectionFromViewer();

private:
	// UI helpers
	void ensurePartRoot();
	void onTreeItemClicked(std::shared_ptr<FlatTreeItem> item, int column);
	void onKeyDown(wxKeyEvent& event);
	void onDeleteObject(wxCommandEvent& event);
	void onHideObject(wxCommandEvent& event);
	void onShowObject(wxCommandEvent& event);
	void onToggleVisibility(wxCommandEvent& event);
	void onShowAllObjects(wxCommandEvent& event);
	void onHideAllObjects(wxCommandEvent& event);

	void createContextMenu();
	void updateTreeItemIcon(std::shared_ptr<FlatTreeItem> item, bool visible);
	std::shared_ptr<OCCGeometry> getSelectedOCCGeometry();
	void refreshActionIconsFor(std::shared_ptr<OCCGeometry> geometry);

	// Tabs
	FlatBarNotebook* m_notebook;
	wxPanel* m_tabPanel;
	wxPanel* m_tabHistory;
	wxPanel* m_tabVersion;

	// Main object tree (Tab 1)
	FlatTreeView* m_treeView;
	std::shared_ptr<FlatTreeItem> m_rootItem;
	std::shared_ptr<FlatTreeItem> m_partRootItem; // "Part" root like FreeCAD-style hierarchy

	// Legacy GeometryObject support
	std::map<GeometryObject*, std::shared_ptr<FlatTreeItem>> m_objectMap;

	// OCCGeometry support
	// Map geometry -> feature item (leaf; used for selection)
	std::map<std::shared_ptr<OCCGeometry>, std::shared_ptr<FlatTreeItem>> m_occGeometryMap; // feature leaf
	std::map<std::shared_ptr<OCCGeometry>, std::shared_ptr<FlatTreeItem>> m_occGeometryBodyMap; // body container
	std::map<std::shared_ptr<FlatTreeItem>, std::shared_ptr<OCCGeometry>> m_treeItemToOCCGeometry; // reverse

	// File-based organization
	std::map<wxString, std::shared_ptr<FlatTreeItem>> m_fileNodeMap; // filename -> file node
	
	// Tree data structure for efficient updates
	struct TreeDataStructure {
		std::map<wxString, std::vector<std::shared_ptr<OCCGeometry>>> fileGroups;
		std::vector<std::shared_ptr<OCCGeometry>> ungroupedGeometries;
		bool needsUpdate;
		
		TreeDataStructure() : needsUpdate(false) {}
		
		void clear() {
			fileGroups.clear();
			ungroupedGeometries.clear();
			needsUpdate = true;
		}
		
		void addGeometry(std::shared_ptr<OCCGeometry> geometry) {
			if (!geometry) return;
			
			// Check for duplicates in file groups
			std::string fileName = geometry->getFileName();
			if (!fileName.empty()) {
				auto& group = fileGroups[fileName];
				// Check if geometry already exists in this file group
				if (std::find(group.begin(), group.end(), geometry) == group.end()) {
					group.push_back(geometry);
					needsUpdate = true;
				}
			} else {
				// Check if geometry already exists in ungrouped geometries
				if (std::find(ungroupedGeometries.begin(), ungroupedGeometries.end(), geometry) == ungroupedGeometries.end()) {
					ungroupedGeometries.push_back(geometry);
					needsUpdate = true;
				}
			}
		}
		
		void removeGeometry(std::shared_ptr<OCCGeometry> geometry) {
			if (!geometry) return;
			
			std::string fileName = geometry->getFileName();
			if (!fileName.empty()) {
				auto it = fileGroups.find(fileName);
				if (it != fileGroups.end()) {
					auto& geometries = it->second;
					geometries.erase(std::remove(geometries.begin(), geometries.end(), geometry), geometries.end());
					if (geometries.empty()) {
						fileGroups.erase(it);
					}
				}
			} else {
				ungroupedGeometries.erase(std::remove(ungroupedGeometries.begin(), ungroupedGeometries.end(), geometry), ungroupedGeometries.end());
			}
			needsUpdate = true;
		}
		
		void updateGeometry(std::shared_ptr<OCCGeometry> geometry) {
			// For now, just mark as needing update
			// In the future, we could implement more sophisticated change tracking
			needsUpdate = true;
		}
	};
	
	TreeDataStructure m_treeData;

	// Column indices for treelist actions
	enum Columns { COL_VIS = 1, COL_DEL = 2, COL_COLOR = 3, COL_EDIT = 4 };

	// Images for action columns
	wxBitmap m_bmpEyeOpen;
	wxBitmap m_bmpEyeClosed;
	wxBitmap m_bmpDelete;
	wxBitmap m_bmpColor;
	wxBitmap m_bmpEdit;

	PropertyPanel* m_propertyPanel;
	OCCViewer* m_occViewer;

	bool m_isUpdatingSelection; // Prevent recursive updates

	// Context menu
	wxMenu* m_contextMenu;
	std::shared_ptr<FlatTreeItem> m_rightClickedItem;
	std::shared_ptr<FlatTreeItem> m_lastSelectedItem;

	// History tree (Tab 2)
	FlatTreeView* m_historyView;
	std::shared_ptr<FlatTreeItem> m_historyRoot;
	std::shared_ptr<FlatTreeItem> m_undoRoot;
	std::shared_ptr<FlatTreeItem> m_redoRoot;

private:
	// Helper methods for OCCGeometry hierarchy management
	void removeOCCGeometryRecursive(std::shared_ptr<OCCGeometry> geometry);
	std::shared_ptr<FlatTreeItem> getOrCreateFileNode(const wxString& fileName);
};