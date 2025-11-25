#pragma once

#include "BaseSelectionListener.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <memory>
#include <vector>

// Forward declarations
class OCCGeometry;
struct gp_Pnt;

/**
 * @brief Edge selection input state for handling edge picking, highlighting and selection
 */
class EdgeSelectionListener : public BaseSelectionListener
{
public:
	EdgeSelectionListener(class Canvas* canvas, PickingService* pickingService, class OCCViewer* occViewer);
	~EdgeSelectionListener();

	virtual void onMouseButton(wxMouseEvent& event) override;
	virtual void onMouseMotion(wxMouseEvent& event) override;

private:
	void highlightEdge(std::shared_ptr<OCCGeometry> geometry, int edgeId);
	void clearHighlight() override;
	void selectEdge(std::shared_ptr<OCCGeometry> geometry, int edgeId);
	void clearSelection() override;
	void onSelectionChanged(const class mod::SelectionChange& change) override;

	// Edge highlighting implementation
	SoSwitch* getOrCreateHighlightNode(std::shared_ptr<OCCGeometry> geometry, int edgeId, bool isSelection = false);
	SoSeparator* createHighlightGeometry(std::shared_ptr<OCCGeometry> geometry, int edgeId, bool isSelection = false);
	bool extractEdgeData(std::shared_ptr<OCCGeometry> geometry, int edgeId, std::vector<gp_Pnt>& edgePoints);
	std::string getCacheKey(std::shared_ptr<OCCGeometry> geometry, int edgeId, bool isSelection) const;

	// Edge highlighting state
	SoSwitch* m_highlightNode;
	SoSeparator* m_highlightGeometryRoot;
	std::shared_ptr<OCCGeometry> m_highlightedGeometry;
	int m_highlightedEdgeId;

	// Edge selection state
	SoSwitch* m_selectedNode;
	SoSeparator* m_selectedGeometryRoot;
	std::shared_ptr<OCCGeometry> m_selectedGeometry;
	int m_selectedEdgeId;
};


