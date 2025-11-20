#include "mod/Selection.h"
#include "mod/SoHighlightElementAction.h"
#include "mod/SoSelectionElementAction.h"
#include "logger/Logger.h"
#include <algorithm>
#include <wx/msgdlg.h>
#include <wx/window.h>

namespace mod {

// Selection actions are now initialized in MainApplication::OnInit()
// This function is kept for backward compatibility but doesn't do anything
static void initializeSelectionActions() {
    // Actions are initialized globally in MainApplication
}

Selection& Selection::getInstance() {
	static Selection instance;
	initializeSelectionActions(); // Ensure actions are initialized
	return instance;
}

bool Selection::addSelection(const std::string& geometryName, const std::string& subElementName,
	const std::string& elementType, float x, float y, float z) {
	
	// Check if already selected
	for (const auto& sel : m_selection) {
		if (sel.geometryName == geometryName && sel.subElementName == subElementName) {
			return false; // Already selected
		}
	}
	
	SelectionChange change(SelectionChangeType::AddSelection, geometryName, subElementName, elementType, x, y, z);
	m_selection.push_back(change);
	
	LOG_INF_S("Selection::addSelection - Added: " + geometryName + 
		(subElementName.empty() ? "" : ("." + subElementName)));
	
	notifyObservers(change);
	return true;
}

bool Selection::removeSelection(const std::string& geometryName, const std::string& subElementName) {
	auto it = std::remove_if(m_selection.begin(), m_selection.end(),
		[&](const SelectionChange& sel) {
			return sel.geometryName == geometryName && 
				(subElementName.empty() || sel.subElementName == subElementName);
		});
	
	if (it != m_selection.end()) {
		SelectionChange change(SelectionChangeType::RemoveSelection, geometryName, subElementName);
		m_selection.erase(it, m_selection.end());
		
		LOG_INF_S("Selection::removeSelection - Removed: " + geometryName + 
			(subElementName.empty() ? "" : ("." + subElementName)));
		
		notifyObservers(change);
		return true;
	}
	return false;
}

void Selection::setSelection(const std::string& geometryName, const std::string& subElementName,
	const std::string& elementType, float x, float y, float z) {
	
	clearSelection();
	addSelection(geometryName, subElementName, elementType, x, y, z);
}

void Selection::clearSelection() {
	if (m_selection.empty()) return;
	
	m_selection.clear();
	SelectionChange change(SelectionChangeType::ClearSelection);
	
	LOG_INF_S("Selection::clearSelection - Cleared all");
	
	notifyObservers(change);
}

int Selection::setPreselect(const std::string& geometryName, const std::string& subElementName,
	const std::string& elementType, float x, float y, float z) {
	
	// Check if same as current preselection
	if (m_preselection.geometryName == geometryName && 
		m_preselection.subElementName == subElementName &&
		m_preselection.elementType == elementType) {
		// Update coordinates only
		m_preselection.x = x;
		m_preselection.y = y;
		m_preselection.z = z;
		return 0; // No change
	}
	
	m_preselection = SelectionChange(SelectionChangeType::SetPreselect, geometryName, subElementName, elementType, x, y, z);
	
	LOG_INF_S("Selection::setPreselect - Set: " + geometryName + 
		(subElementName.empty() ? "" : ("." + subElementName)));
	
	notifyObservers(m_preselection);
	return 1; // Changed
}

void Selection::removePreselect() {
	if (m_preselection.geometryName.empty()) return;
	
	SelectionChange change = m_preselection;
	change.type = SelectionChangeType::RemovePreselect;
	m_preselection = SelectionChange();
	
	LOG_INF_S("Selection::removePreselect - Removed");
	
	notifyObservers(change);
}

bool Selection::isSelected(const std::string& geometryName, const std::string& subElementName) const {
	for (const auto& sel : m_selection) {
		if (sel.geometryName == geometryName) {
			if (subElementName.empty() || sel.subElementName == subElementName) {
				return true;
			}
		}
	}
	return false;
}

void Selection::addObserver(SelectionObserverCallback callback) {
	m_observers.push_back(callback);
}

void Selection::removeObserver(SelectionObserverCallback callback) {
	// Note: std::function doesn't have operator==, so we can't easily remove
	// In practice, observers are usually added once and kept for lifetime
	// For now, we'll keep all observers
}

void Selection::notifyObservers(const SelectionChange& change) {
	for (auto& observer : m_observers) {
		try {
			observer(change);
		} catch (const std::exception& e) {
			LOG_ERR_S("Selection::notifyObservers - Exception in observer: " + std::string(e.what()));
		}
	}
}

} // namespace mod

