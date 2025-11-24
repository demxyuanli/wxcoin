#pragma once

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/SoPath.h>
#include <Inventor/details/SoDetail.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "mod/Selection.h"

class OCCGeometry;

/**
 * @brief ViewProvider base class (similar to FreeCAD ViewProvider)
 * 
 * Each geometry object has a ViewProvider that manages its visual representation
 * and handles selection/preselection highlighting.
 */
class ViewProvider {
public:
	ViewProvider(std::shared_ptr<OCCGeometry> geometry);
	virtual ~ViewProvider();
	
	// Get the root Coin3D node for this view provider
	SoSeparator* getRoot() const { return m_root; }
	
	// Get the geometry this view provider represents
	std::shared_ptr<OCCGeometry> getGeometry() const { return m_geometry; }
	
	// Selection and preselection highlighting
	virtual void highlightPreselection(const std::string& subElementName);
	virtual void clearPreselection();
	virtual void highlightSelection(const std::string& subElementName);
	virtual void clearSelection();
	
	// Get element name from detail (for picking)
	virtual std::string getElement(const std::string& subElementName) const;
	
	// Check if this view provider can handle the given sub-element
	virtual bool canSelectElement(const std::string& subElementName) const;

	// Convert sub-element name to Coin3D path and detail (FreeCAD-style getDetailPath)
	// append: if true, append to existing path; if false, truncate path first
	virtual bool getDetailPath(const std::string& subElementName, SoPath* path, bool append, SoDetail*& detail) const;

	// Handle selection change notifications from Selection system
	void onSelectionChange(const mod::SelectionChange& change);
	
protected:
	// Build the Coin3D scene graph
	virtual void buildSceneGraph();
	
	// Get or create highlight node for a sub-element
	virtual SoSwitch* getHighlightNode(const std::string& subElementName, bool isSelection);
	
	std::shared_ptr<OCCGeometry> m_geometry;
	SoSeparator* m_root;
	SoSwitch* m_modeSwitch;
	
	// Highlight nodes: subElementName -> SoSwitch node
	std::unordered_map<std::string, SoSwitch*> m_preselectionNodes;
	std::unordered_map<std::string, SoSwitch*> m_selectionNodes;
	
	std::string m_currentPreselection;
	std::vector<std::string> m_currentSelection;
	
	// Lifecycle flag to prevent accessing destroyed object
	std::shared_ptr<bool> m_isAlive;
};

