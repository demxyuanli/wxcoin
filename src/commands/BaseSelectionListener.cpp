#include "BaseSelectionListener.h"
#include "mod/Selection.h"
#include "logger/Logger.h"
#include "Canvas.h"
#include "OCCViewer.h"
#include <Inventor/nodes/SoSeparator.h>
#include <wx/event.h>

BaseSelectionListener::BaseSelectionListener(Canvas* canvas, PickingService* pickingService, OCCViewer* occViewer)
	: m_canvas(canvas), m_pickingService(pickingService), m_occViewer(occViewer)
	, m_isAlive(std::make_shared<bool>(true))
{
	LOG_INF_S("BaseSelectionListener created");

	// Register selection observer to handle preselection and selection changes
	auto& selection = mod::Selection::getInstance();
	auto isAlive = m_isAlive;
	selection.addObserver([this, isAlive](const mod::SelectionChange& change) {
		// Check if object is still alive before accessing
		if (!*isAlive) {
			return; // Object has been destroyed, ignore callback
		}
		this->onSelectionChanged(change);
	});
}

BaseSelectionListener::~BaseSelectionListener()
{
	// Mark object as destroyed first to prevent callbacks from accessing it
	if (m_isAlive) {
		*m_isAlive = false;
	}

	// Note: Cannot call pure virtual functions clearHighlight() and clearSelection() here
	// Derived classes should handle cleanup in their destructors

	// Clean up cache
	for (auto& pair : m_highlightCache) {
		if (pair.second) {
			pair.second->unref();
		}
	}
	m_highlightCache.clear();
}

void BaseSelectionListener::onMouseWheel(wxMouseEvent& event) {
	event.Skip();
}

void BaseSelectionListener::clearHighlightCache() {
	// Clear all cached highlight nodes
	for (auto& pair : m_highlightCache) {
		if (pair.second) {
			// Hide the node first
			pair.second->whichChild.setValue(SO_SWITCH_NONE);
			// Unreference the node (it will be removed from scene graph automatically if no other references)
			pair.second->unref();
		}
	}
	m_highlightCache.clear();
	LOG_INF_S("BaseSelectionListener::clearHighlightCache - Cache cleared");
}

void BaseSelectionListener::deactivate() {
	LOG_INF_S("BaseSelectionListener::deactivate - Cleaning up selection tool");
	
	// Clear highlights and selections when tool is deactivated
	clearHighlight();
	clearSelection();
	
	// Clear highlight cache to free resources
	clearHighlightCache();
	
	LOG_INF_S("BaseSelectionListener::deactivate - Cleanup completed");
}

