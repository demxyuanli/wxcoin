#pragma once

#include "InputState.h"
#include "viewer/PickingService.h"
#include "rendering/GeometryProcessor.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <memory>
#include <unordered_map>

// Forward declaration
namespace mod {
	struct SelectionChange;
}

/**
 * @brief Face selection input state for handling face picking, highlighting and selection
 */
class FaceSelectionListener : public InputState
{
public:
	FaceSelectionListener(class Canvas* canvas, PickingService* pickingService, class OCCViewer* occViewer);
	~FaceSelectionListener();

	virtual void onMouseButton(wxMouseEvent& event) override;
	virtual void onMouseMotion(wxMouseEvent& event) override;
	virtual void onMouseWheel(wxMouseEvent& event) override;

private:
	void highlightFace(std::shared_ptr<class OCCGeometry> geometry, int faceId);
	void clearHighlight();
	void selectFace(std::shared_ptr<class OCCGeometry> geometry, int faceId);
	void clearSelection();
	void showContextMenu(const wxPoint& screenPos, std::shared_ptr<class OCCGeometry> geometry, int faceId);
	void onSelectionChanged(const class mod::SelectionChange& change);

	// Face highlighting implementation
	SoSwitch* getOrCreateHighlightNode(std::shared_ptr<class OCCGeometry> geometry, int faceId, bool isSelection = false);
	SoSeparator* createHighlightGeometry(std::shared_ptr<class OCCGeometry> geometry, int faceId, bool isSelection = false);
	bool extractFaceMesh(std::shared_ptr<class OCCGeometry> geometry, int faceId, TriangleMesh& faceMesh);
	bool extractMeshFromCoinNode(SoSeparator* rootNode, TriangleMesh& mesh);
	std::string getCacheKey(std::shared_ptr<class OCCGeometry> geometry, int faceId, bool isSelection) const;

	class Canvas* m_canvas;
	PickingService* m_pickingService;
	class OCCViewer* m_occViewer;

	// Face highlighting state
	SoSwitch* m_highlightNode;
	SoSeparator* m_highlightGeometryRoot;
	std::shared_ptr<class OCCGeometry> m_highlightedGeometry;
	int m_highlightedFaceId;

	// Face selection state
	SoSwitch* m_selectedNode;
	SoSeparator* m_selectedGeometryRoot;
	std::shared_ptr<class OCCGeometry> m_selectedGeometry;
	int m_selectedFaceId;

	// Cache for highlight nodes
	std::unordered_map<std::string, SoSwitch*> m_highlightCache;

	// Lifecycle flag to prevent accessing destroyed object
	std::shared_ptr<bool> m_isAlive;
};

