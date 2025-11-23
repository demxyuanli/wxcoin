#include "mod/ViewProvider.h"
#include "OCCGeometry.h"
#include "mod/Selection.h"
#include "mod/SoHighlightElementAction.h"
#include "mod/SoSelectionElementAction.h"
#include "logger/Logger.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <algorithm>
#include <wx/msgdlg.h>

ViewProvider::ViewProvider(std::shared_ptr<OCCGeometry> geometry)
	: m_geometry(geometry), m_root(nullptr), m_modeSwitch(nullptr) {
	
	if (!geometry) {
		LOG_ERR_S("ViewProvider::ViewProvider - Null geometry");
		return;
	}
	
	// Create root node
	m_root = new SoSeparator();
	m_root->ref();
	
	// Create mode switch (for different display modes)
	m_modeSwitch = new SoSwitch();
	m_modeSwitch->ref();
	m_modeSwitch->whichChild.setValue(SO_SWITCH_ALL);
	
	// Add geometry's Coin3D node to mode switch
	SoSeparator* geometryNode = geometry->getCoinNode();
	if (geometryNode) {
		m_modeSwitch->addChild(geometryNode);
	}
	
	m_root->addChild(m_modeSwitch);
	
	// Register with Selection system to receive selection change notifications
	auto& selection = mod::Selection::getInstance();
	selection.addObserver([this](const mod::SelectionChange& change) {
		this->onSelectionChange(change);
	});
	
}

ViewProvider::~ViewProvider() {
	clearPreselection();
	clearSelection();
	
	// Clean up highlight nodes
	for (auto& pair : m_preselectionNodes) {
		if (pair.second) {
			pair.second->unref();
		}
	}
	for (auto& pair : m_selectionNodes) {
		if (pair.second) {
			pair.second->unref();
		}
	}
	
	if (m_modeSwitch) {
		m_modeSwitch->unref();
	}
	if (m_root) {
		m_root->unref();
	}
}

void ViewProvider::highlightPreselection(const std::string& subElementName) {
	if (subElementName == m_currentPreselection) {
		return; // Already preselected
	}

	clearPreselection();

	if (subElementName.empty()) {
		// Preselect whole object (not implemented yet)
		return;
	}

	// Use action-based highlighting
	SoFullPath* path = static_cast<SoFullPath*>(new SoPath());
	path->ref();
	SoDetail* detail = nullptr;

	if (getDetailPath(subElementName, path, false, detail)) {
		// Apply highlight action
		SoHighlightElementAction highlightAction;
		highlightAction.setHighlighted(true);
		highlightAction.setColor(SbColor(1.0f, 1.0f, 0.0f)); // Yellow highlight
		highlightAction.setElement(detail);
		highlightAction.apply(path);

		m_currentPreselection = subElementName;

		delete detail;
		path->unref();
	} else {
	}
}

void ViewProvider::clearPreselection() {
	if (m_currentPreselection.empty()) return;

	// Use action-based clearing
	SoFullPath* path = static_cast<SoFullPath*>(new SoPath());
	path->ref();
	SoDetail* detail = nullptr;

	if (getDetailPath(m_currentPreselection, path, false, detail)) {
		// Apply clear highlight action
		SoHighlightElementAction highlightAction;
		highlightAction.setHighlighted(false); // Clear highlight
		highlightAction.setElement(detail);
		highlightAction.apply(path);

		delete detail;
		path->unref();
	}

	m_currentPreselection.clear();
}

void ViewProvider::highlightSelection(const std::string& subElementName) {
	// Check if already selected
	for (const auto& sel : m_currentSelection) {
		if (sel == subElementName) {
			return; // Already selected
		}
	}

	// Use action-based selection
	SoFullPath* path = static_cast<SoFullPath*>(new SoPath());
	path->ref();
	SoDetail* detail = nullptr;

	if (getDetailPath(subElementName, path, false, detail)) {
		// Apply selection action
		SoSelectionElementAction selectionAction(SoSelectionElementAction::Append);
		selectionAction.setColor(SbColor(0.0f, 0.5f, 1.0f)); // Blue selection
		selectionAction.setElement(detail);
		selectionAction.apply(path);

		m_currentSelection.push_back(subElementName);

		delete detail;
		path->unref();
	} else {
	}
}

void ViewProvider::clearSelection() {
	if (m_currentSelection.empty()) return;

	// Clear all selected elements using action-based clearing
	for (const auto& subElementName : m_currentSelection) {
		SoFullPath* path = static_cast<SoFullPath*>(new SoPath());
		path->ref();
		SoDetail* detail = nullptr;

		if (getDetailPath(subElementName, path, false, detail)) {
			// Apply clear selection action
			SoSelectionElementAction selectionAction(SoSelectionElementAction::Remove);
			selectionAction.setElement(detail);
			selectionAction.apply(path);

			delete detail;
			path->unref();
		}
	}

	m_currentSelection.clear();
}

std::string ViewProvider::getElement(const std::string& subElementName) const {
	return subElementName; // Default implementation just returns the name
}

bool ViewProvider::canSelectElement(const std::string& subElementName) const {
	if (subElementName.empty()) {
		return true; // Can select whole object
	}

	// Check if it's a valid sub-element name
	if ((subElementName.length() >= 4 && subElementName.substr(0, 4) == "Face") ||
		(subElementName.length() >= 4 && subElementName.substr(0, 4) == "Edge") ||
		(subElementName.length() >= 6 && subElementName.substr(0, 6) == "Vertex")) {
		return true;
	}

	return false;
}

bool ViewProvider::getDetailPath(const std::string& subElementName, SoPath* path, bool append, SoDetail*& detail) const {
	if (!path || !m_root || !m_geometry) {
		return false;
	}

	// Path should be properly cast to SoFullPath by caller
	if (!append) {
		path->truncate(0);
	}

	// Handle whole object selection (no sub-element)
	if (subElementName.empty()) {
		// For whole object, we just need the path to the geometry node
		// and no detail (nullptr)
		detail = nullptr;
		if (path->getLength() == 0) {
			path->append(m_root);
		}
		if (m_modeSwitch) {
			path->append(m_modeSwitch);
		}
		SoSeparator* geometryNode = m_geometry->getCoinNode();
		if (geometryNode) {
			path->append(geometryNode);
			return true;
		}
		return false;
	}

	// Parse sub-element name (e.g., "Face5" -> type="Face", id=5)
	std::string elementType;
	int elementId = -1;

	if (subElementName.length() >= 4 && subElementName.substr(0, 4) == "Face") {
		elementType = "Face";
		try {
			elementId = std::stoi(subElementName.substr(4));
		} catch (...) {
			elementId = -1;
		}
	} else if (subElementName.length() >= 4 && subElementName.substr(0, 4) == "Edge") {
		elementType = "Edge";
		try {
			elementId = std::stoi(subElementName.substr(4));
		} catch (...) {
			elementId = -1;
		}
	} else if (subElementName.length() >= 6 && subElementName.substr(0, 6) == "Vertex") {
		elementType = "Vertex";
		try {
			elementId = std::stoi(subElementName.substr(6));
		} catch (...) {
			elementId = -1;
		}
	} else {
		return false;
	}

	if (elementId < 0) {
		return false;
	}

	// Build path to geometry node
	if (!m_root || !m_modeSwitch) {
		return false;
	}

	if (path->getLength() == 0) {
		path->append(m_root);
	}
	if (path->getTail() != m_modeSwitch) {
		path->append(m_modeSwitch);
	}

	SoSeparator* geometryNode = m_geometry->getCoinNode();
	if (!geometryNode) {
		return false;
	}

	if (path->getTail() != geometryNode) {
		path->append(geometryNode);
	}

	// Create appropriate detail based on element type
	if (elementType == "Face") {
		// For faces, we need to find the first triangle index that belongs to this face
		std::vector<int> triangleIndices = m_geometry->getTrianglesForGeometryFace(elementId);
		if (triangleIndices.empty()) {
			return false;
		}

		// Create SoFaceDetail with the first triangle index
		detail = new SoFaceDetail();
		static_cast<SoFaceDetail*>(detail)->setFaceIndex(triangleIndices[0]);
		static_cast<SoFaceDetail*>(detail)->setPartIndex(0); // Not used in our implementation

		return true;

	} else if (elementType == "Edge") {
		// For edges, create SoLineDetail
		// Get the first line index that belongs to this edge
		std::vector<int> lineIndices = m_geometry->getLinesForGeometryEdge(elementId);
		if (lineIndices.empty()) {
			return false;
		}

		detail = new SoLineDetail();
		static_cast<SoLineDetail*>(detail)->setLineIndex(lineIndices[0]);
		static_cast<SoLineDetail*>(detail)->setPartIndex(0);

		return true;

	} else if (elementType == "Vertex") {
		// For vertices, create SoPointDetail
		// Get the coordinate index for this vertex
		int coordinateIndex = m_geometry->getCoordinateForGeometryVertex(elementId);
		if (coordinateIndex < 0) {
			return false;
		}

		detail = new SoPointDetail();
		static_cast<SoPointDetail*>(detail)->setCoordinateIndex(coordinateIndex);
		static_cast<SoPointDetail*>(detail)->setMaterialIndex(0);
		static_cast<SoPointDetail*>(detail)->setNormalIndex(0);
		static_cast<SoPointDetail*>(detail)->setTextureCoordIndex(0);

		return true;
	}

	return false;
}

SoSwitch* ViewProvider::getHighlightNode(const std::string& subElementName, bool isSelection) {
	// This is a placeholder - actual implementation would create/manage highlight nodes
	return nullptr;
}

void ViewProvider::onSelectionChange(const mod::SelectionChange& change) {
	// Only handle changes for this geometry
	if (change.geometryName != m_geometry->getName()) {
		return;
	}
	
	switch (change.type) {
	case mod::SelectionChangeType::SetPreselect:
	case mod::SelectionChangeType::MovePreselect:
		if (!change.subElementName.empty()) {
			highlightPreselection(change.subElementName);
		}
		break;
		
	case mod::SelectionChangeType::RemovePreselect:
		clearPreselection();
		break;
		
	case mod::SelectionChangeType::AddSelection:
	case mod::SelectionChangeType::SetSelection:
		if (!change.subElementName.empty()) {
			highlightSelection(change.subElementName);
		}
		break;
		
	case mod::SelectionChangeType::RemoveSelection:
		if (!change.subElementName.empty()) {
			// Remove specific sub-element from selection
			auto it = std::find(m_currentSelection.begin(), m_currentSelection.end(), change.subElementName);
			if (it != m_currentSelection.end()) {
				m_currentSelection.erase(it);
				// TODO: Clear specific highlight node
			}
		}
		break;
		
	case mod::SelectionChangeType::ClearSelection:
		clearSelection();
		break;
		
	default:
		break;
	}
}

