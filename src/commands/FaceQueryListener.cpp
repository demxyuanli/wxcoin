#include "FaceQueryListener.h"
#include "viewer/PickingService.h"
#include "logger/Logger.h"
#include "Canvas.h"
#include "mod/FaceQueryDialog.h"
#include "OCCGeometry.h"
#include "OCCViewer.h"

FaceQueryListener::FaceQueryListener(Canvas* canvas, PickingService* pickingService)
	: m_canvas(canvas), m_pickingService(pickingService)
{
	LOG_INF_S("FaceQueryListener created");
}

void FaceQueryListener::onMouseButton(wxMouseEvent& event) {
	LOG_INF_S("FaceQueryListener::onMouseButton - Event received");

	// Log mouse event details
	wxPoint mousePos = event.GetPosition();
	bool isLeftDown = event.LeftDown();
	bool isLeftUp = event.LeftUp();
	bool isMiddleDown = event.MiddleDown();
	bool isMiddleUp = event.MiddleUp();
	bool isRightDown = event.RightDown();
	bool isRightUp = event.RightUp();

	LOG_INF_S("FaceQueryListener::onMouseButton - Mouse position: (" +
		std::to_string(mousePos.x) + ", " + std::to_string(mousePos.y) +
		"), LeftDown: " + (isLeftDown ? "true" : "false") +
		", LeftUp: " + (isLeftUp ? "true" : "false") +
		", MiddleDown: " + (isMiddleDown ? "true" : "false") +
		", MiddleUp: " + (isMiddleUp ? "true" : "false") +
		", RightDown: " + (isRightDown ? "true" : "false") +
		", RightUp: " + (isRightUp ? "true" : "false"));

	// Check if this is a mouse button up event (left or middle for face query)
	if (!isLeftUp && !isMiddleUp) {
		LOG_INF_S("FaceQueryListener::onMouseButton - Ignoring non-button-up event, left-click or middle-click on faces to query");
		event.Skip();
		return;
	}

	// Check if picking service is available
	if (!m_pickingService) {
		LOG_WRN_S("FaceQueryListener::onMouseButton - PickingService not available");
		event.Skip();
		return;
	}

	// For left-click, we consume the event to prevent view rotation
	// For middle-click, we also consume it for consistency
	event.Skip(false); // Don't skip - consume the event

	// Determine which button was used
	std::string buttonType = isMiddleUp ? "middle-click" : "left-click";
	LOG_INF_S("FaceQueryListener::onMouseButton - Starting detailed picking with " + buttonType +
		" at position (" + std::to_string(mousePos.x) + ", " + std::to_string(mousePos.y) + ")");

	// Log scene state before picking
	if (m_canvas && m_canvas->getOCCViewer()) {
		std::vector<std::shared_ptr<OCCGeometry>> geometries = m_canvas->getOCCViewer()->getAllGeometry();
		LOG_INF_S("FaceQueryListener::onMouseButton - Scene contains " + std::to_string(geometries.size()) + " geometries");
		for (size_t i = 0; i < geometries.size(); ++i) {
			LOG_INF_S("  Geometry " + std::to_string(i) + ": " + geometries[i]->getName() +
				" (file: " + geometries[i]->getFileName() + ")");
		}
	} else {
		LOG_INF_S("FaceQueryListener::onMouseButton - No OCCViewer available");
	}

	// Perform detailed picking
	PickingResult result = m_pickingService->pickDetailedAtScreen(mousePos);

	// Additional debugging for picking failures
	if (!result.geometry) {
		// Try to find what might be at this screen position by checking if there are any geometries at all
		if (m_canvas && m_canvas->getOCCViewer()) {
			auto geometries = m_canvas->getOCCViewer()->getAllGeometry();
			if (!geometries.empty()) {
				LOG_INF_S("FaceQueryListener::onMouseButton - No geometry found at click position, but scene contains " +
					std::to_string(geometries.size()) + " geometries. Try:");
				LOG_INF_S("  - Clicking directly on visible geometry surfaces");
				LOG_INF_S("  - Adjusting camera view to ensure geometry is visible");
				LOG_INF_S("  - Zooming in closer to the geometry");
				LOG_INF_S("  - Checking if geometry is obscured by other objects");
			}
		}
	}

	// Log picking result details
	if (result.geometry) {
		std::string geomName = result.geometry->getName();
		std::string fileName = result.geometry->getFileName();
		int triangleIdx = result.triangleIndex;
		int faceId = result.geometryFaceId;
		bool hasMapping = result.geometry->hasFaceDomainMapping();

		LOG_INF_S("FaceQueryListener::onMouseButton - Picking successful:");
		LOG_INF_S("  Geometry: " + geomName + " (file: " + fileName + ")");
		LOG_INF_S("  Triangle Index: " + std::to_string(triangleIdx));
		LOG_INF_S("  Geometry Face ID: " + std::to_string(faceId));
		LOG_INF_S("  Has Face Mapping: " + std::string(hasMapping ? "true" : "false"));

		if (hasMapping) {
			auto triangles = result.geometry->getTrianglesForGeometryFace(faceId);
			LOG_INF_S("  Triangles in Face: " + std::to_string(triangles.size()));
		}

		// Show result in overlay instead of modal dialog
		LOG_INF_S("FaceQueryListener::onMouseButton - Showing face info in overlay");
		if (m_canvas && m_canvas->getFaceInfoOverlay()) {
			m_canvas->getFaceInfoOverlay()->setPickingResult(result);
			m_canvas->Refresh(false); // Refresh canvas to draw overlay
		}

	} else {
		LOG_WRN_S("FaceQueryListener::onMouseButton - No geometry found at position (" +
			std::to_string(mousePos.x) + ", " + std::to_string(mousePos.y) + ")");

		// Show "no result" in overlay
		if (m_canvas && m_canvas->getFaceInfoOverlay()) {
			m_canvas->getFaceInfoOverlay()->setPickingResult(result);
			m_canvas->Refresh(false); // Refresh canvas to draw overlay
		}
	}

	LOG_INF_S("FaceQueryListener::onMouseButton - Event processing completed");
	// Event already consumed with event.Skip(false) above
}

void FaceQueryListener::onMouseMotion(wxMouseEvent& event) {
	// For now, just pass through mouse move events
	event.Skip();
}

void FaceQueryListener::onMouseWheel(wxMouseEvent& event) {
	// For now, just pass through mouse wheel events
	event.Skip();
}
