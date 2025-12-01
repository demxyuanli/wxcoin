#pragma once

#include "InputState.h"
#include "viewer/PickingService.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <memory>
#include <unordered_map>

// Forward declarations
class Canvas;
class OCCGeometry;
class OCCViewer;
namespace mod {
	struct SelectionChange;
}

/**
 * @brief Base class for selection listeners with common functionality
 */
class BaseSelectionListener : public InputState
{
public:
	BaseSelectionListener(Canvas* canvas, PickingService* pickingService, OCCViewer* occViewer);
	virtual ~BaseSelectionListener();

	virtual void onMouseWheel(wxMouseEvent& event) override;
	
	// Cleanup when tool is deactivated
	virtual void deactivate() override;

	// Clear highlight cache (call when configuration changes)
	void clearHighlightCache();

protected:
	virtual void clearHighlight() = 0;
	virtual void clearSelection() = 0;
	virtual void onSelectionChanged(const mod::SelectionChange& change) = 0;

	Canvas* m_canvas;
	PickingService* m_pickingService;
	OCCViewer* m_occViewer;

	// Cache for highlight nodes
	std::unordered_map<std::string, SoSwitch*> m_highlightCache;

	// Lifecycle flag to prevent accessing destroyed object
	std::shared_ptr<bool> m_isAlive;
};

