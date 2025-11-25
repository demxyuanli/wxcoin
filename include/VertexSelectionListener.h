#pragma once

#include "BaseSelectionListener.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <memory>

// Forward declarations
class OCCGeometry;
struct gp_Pnt;

/**
 * @brief Vertex selection input state for handling vertex picking, highlighting and selection
 */
class VertexSelectionListener : public BaseSelectionListener
{
public:
	VertexSelectionListener(class Canvas* canvas, PickingService* pickingService, class OCCViewer* occViewer);
	~VertexSelectionListener();

	virtual void onMouseButton(wxMouseEvent& event) override;
	virtual void onMouseMotion(wxMouseEvent& event) override;

private:
	void highlightVertex(std::shared_ptr<OCCGeometry> geometry, int vertexId);
	void clearHighlight() override;
	void selectVertex(std::shared_ptr<OCCGeometry> geometry, int vertexId);
	void clearSelection() override;
	void onSelectionChanged(const class mod::SelectionChange& change) override;

	// Vertex highlighting implementation
	SoSwitch* getOrCreateHighlightNode(std::shared_ptr<OCCGeometry> geometry, int vertexId, bool isSelection = false);
	SoSeparator* createHighlightGeometry(std::shared_ptr<OCCGeometry> geometry, int vertexId, bool isSelection = false);
	bool extractVertexData(std::shared_ptr<OCCGeometry> geometry, int vertexId, gp_Pnt& vertexPoint);
	std::string getCacheKey(std::shared_ptr<OCCGeometry> geometry, int vertexId, bool isSelection) const;

	// Vertex highlighting state
	SoSwitch* m_highlightNode;
	SoSeparator* m_highlightGeometryRoot;
	std::shared_ptr<OCCGeometry> m_highlightedGeometry;
	int m_highlightedVertexId;

	// Vertex selection state
	SoSwitch* m_selectedNode;
	SoSeparator* m_selectedGeometryRoot;
	std::shared_ptr<OCCGeometry> m_selectedGeometry;
	int m_selectedVertexId;
};


