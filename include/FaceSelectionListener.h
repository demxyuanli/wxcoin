#pragma once

#include "BaseSelectionListener.h"
#include "rendering/GeometryProcessor.h"
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <memory>

// Forward declaration
namespace mod {
	struct SelectionChange;
}

/**
 * @brief Face selection input state for handling face picking, highlighting and selection
 */
class FaceSelectionListener : public BaseSelectionListener
{
public:
	FaceSelectionListener(class Canvas* canvas, PickingService* pickingService, class OCCViewer* occViewer);
	~FaceSelectionListener();

	virtual void onMouseButton(wxMouseEvent& event) override;
	virtual void onMouseMotion(wxMouseEvent& event) override;

private:
	void highlightFace(std::shared_ptr<class OCCGeometry> geometry, int faceId);
	void clearHighlight() override;
	void selectFace(std::shared_ptr<class OCCGeometry> geometry, int faceId);
	void clearSelection() override;
	void showContextMenu(const wxPoint& screenPos, std::shared_ptr<class OCCGeometry> geometry, int faceId);
	void onSelectionChanged(const class mod::SelectionChange& change) override;

	// Face highlighting implementation
	SoSwitch* getOrCreateHighlightNode(std::shared_ptr<class OCCGeometry> geometry, int faceId, bool isSelection = false);
	SoSeparator* createHighlightGeometry(std::shared_ptr<class OCCGeometry> geometry, int faceId, bool isSelection = false);
	bool extractFaceMesh(std::shared_ptr<class OCCGeometry> geometry, int faceId, TriangleMesh& faceMesh);
	bool extractMeshFromCoinNode(SoSeparator* rootNode, TriangleMesh& mesh);
	std::string getCacheKey(std::shared_ptr<class OCCGeometry> geometry, int faceId, bool isSelection) const;

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
};

