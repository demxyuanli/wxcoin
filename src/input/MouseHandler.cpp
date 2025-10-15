#include "MouseHandler.h"
#include "Canvas.h"
#include "SceneManager.h"
#include "PickingAidManager.h"
#include "NavigationController.h"
#include "NavigationModeManager.h"
#include "GeometryFactory.h"
#include "PositionBasicDialog.h"
#include "OCCViewer.h"
#include "OCCGeometry.h"
#include "ObjectTreePanel.h"
#include "logger/Logger.h"
#include "OCCViewer.h"
#include <Inventor/nodes/SoSelection.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/SoPickedPoint.h>
#include <wx/frame.h>
#include <wx/statusbr.h>
#include <wx/utils.h>

namespace {
	inline void updateStatusBar(wxWindow* anyChild, const wxString& text) {
		if (!anyChild) return;
		wxWindow* top = wxGetTopLevelParent(anyChild);
		if (auto* frame = dynamic_cast<wxFrame*>(top)) {
			frame->SetStatusText(text, 0);
		}
	}
}

MouseHandler::MouseHandler(Canvas* canvas, ObjectTreePanel* objectTree, PropertyPanel* propertyPanel, CommandManager* commandManager)
	: m_canvas(canvas)
	, m_objectTree(objectTree)
	, m_propertyPanel(propertyPanel)
	, m_commandManager(commandManager)
	, m_navigationController(nullptr)
	, m_navigationModeManager(nullptr)
	, m_operationMode(OperationMode::VIEW)
	, m_isDragging(false)
	, m_currentPositionBasicDialog(nullptr)
{
	LOG_INF_S("MouseHandler initializing");
	if (!m_canvas) LOG_ERR_S("MouseHandler: Canvas is null");
	if (!m_objectTree) LOG_ERR_S("MouseHandler: ObjectTree is null");
	if (!m_propertyPanel) LOG_ERR_S("MouseHandler: PropertyPanel is null");
	if (!m_commandManager) LOG_ERR_S("MouseHandler: CommandManager is null");
	// Show initial mode on status bar
	updateStatusBar(m_canvas, wxString("Mode: VIEW"));
}

MouseHandler::~MouseHandler() {
	LOG_INF_S("MouseHandler destroying");
	if (m_currentPositionBasicDialog) {
		m_currentPositionBasicDialog->Destroy();
		m_currentPositionBasicDialog = nullptr;
	}
}

void MouseHandler::setOperationMode(OperationMode mode) {
	m_operationMode = mode;
	LOG_INF_S("Operation mode set to: " + std::to_string(static_cast<int>(mode)));
	// Reflect mode change on status bar
	if (m_canvas) {
		const wxString msg = (mode == OperationMode::VIEW) ? wxString("Mode: VIEW") : wxString("Mode: CREATE");
		updateStatusBar(m_canvas, msg);
	}
}

void MouseHandler::setCreationGeometryType(const std::string& type) {
	m_creationGeometryType = type;
	LOG_INF_S("Creation geometry type set to: " + type);

	// Update status to reflect current creation type
	if (!type.empty() && m_canvas) {
		updateStatusBar(m_canvas, wxString("Create: ") + wxString(type));
	}

	if (!type.empty()) {
		// Close existing position dialog if any
		if (m_currentPositionBasicDialog) {
			m_currentPositionBasicDialog->Destroy();
			m_currentPositionBasicDialog = nullptr;
		}

		// Create new position basic dialog
		PickingAidManager* pickingAidManager = m_canvas->getSceneManager()->getPickingAidManager();
		m_currentPositionBasicDialog = new PositionBasicDialog(m_canvas->GetParent(), "Set " + wxString(type) + " Position", pickingAidManager, type);
		m_currentPositionBasicDialog->SetPosition(SbVec3f(0.0f, 0.0f, 0.0f));

		// Set up picking callback - this will be called when picking is complete
		m_currentPositionBasicDialog->SetPickingCallback([this](const SbVec3f& position) {
			// Just log the completion, don't call OnPositionPicked again
			LOG_INF_S("Position picking completed via callback: X=" + std::to_string(position[0]) +
				", Y=" + std::to_string(position[1]) +
				", Z=" + std::to_string(position[2]));
			});

		m_currentPositionBasicDialog->Show(true);
	}
}

void MouseHandler::setNavigationController(NavigationController* controller) {
	m_navigationController = controller;
	LOG_INF_S("NavigationController set for MouseHandler");
}

void MouseHandler::setNavigationModeManager(NavigationModeManager* manager) {
	m_navigationModeManager = manager;
	LOG_INF_S("NavigationModeManager set for MouseHandler");
}

void MouseHandler::handleMouseButton(wxMouseEvent& event) {
	LOG_INF_S("Mouse button event - Mode: " + std::to_string(static_cast<int>(m_operationMode)) +
		", LeftDown: " + std::to_string(event.LeftDown()));

	if (m_operationMode == OperationMode::VIEW) {
		// Start/stop slice dragging with left button when slice is enabled AND drag mode is active
		if (auto* viewer = m_canvas ? m_canvas->getOCCViewer() : nullptr) {
			if (viewer->isSliceEnabled() && viewer->isSliceDragEnabled()) {
				if (event.LeftDown()) {
					enableSliceDragging(true);
					m_lastMousePos = event.GetPosition();
					// Do not forward to navigation when starting slice drag
					return;
				}
				else if (event.LeftUp()) {
					enableSliceDragging(false);
					// Consume release as well
					return;
				}
			}
		}
		
		// Use NavigationModeManager if available, otherwise fall back to NavigationController
		if (m_navigationModeManager) {
			m_navigationModeManager->handleMouseButton(event);
		}
		else if (m_navigationController) {
			m_navigationController->handleMouseButton(event);
		}
	}
	else {
		event.Skip();
	}
}

void MouseHandler::handleMouseMotion(wxMouseEvent& event) {
	if (m_operationMode == OperationMode::VIEW && m_navigationController) {
		// If slice dragging is active, convert mouse delta to world movement along slice normal
		if (m_sliceDragState == SliceDragState::Dragging && m_canvas && m_canvas->getSceneManager()) {
			// Convert pixel delta to a movement along the slice normal using scene scale
			wxPoint cur = event.GetPosition();
			wxPoint dpx = cur - m_lastMousePos;
			m_lastMousePos = cur;
			if (auto* viewer = m_canvas->getOCCViewer()) {
				float sceneSize = m_canvas->getSceneManager()->getSceneBoundingBoxSize();
				if (sceneSize <= 0.0f) sceneSize = 1.0f;
				// Scale by viewport height to be resolution independent; invert Y for intuitive drag
				int h = std::max(1, m_canvas->GetClientSize().GetHeight());
				// Use camera orientation to align vertical drag with slice normal projection
				SbVec3f n = viewer->getSliceNormal();
				n.normalize();
				// Map pixel delta to world along camera up vector, then project to normal
				SbVec3f up(0, 1, 0);
				if (auto* cam = m_canvas->getSceneManager()->getCamera()) {
					SbRotation r = cam->orientation.getValue();
					r.multVec(up, up);
				}
				up.normalize();
				float worldPerPixel = sceneSize / static_cast<float>(h);
				float alongUp = -static_cast<float>(dpx.y) * worldPerPixel;
				float delta = alongUp * up.dot(n);
				viewer->moveSliceAlongNormal(delta);
			}
		}
		else {
			// Use NavigationModeManager if available, otherwise fall back to NavigationController
			if (m_navigationModeManager) {
				m_navigationModeManager->handleMouseMotion(event);
			}
			else if (m_navigationController) {
				m_navigationController->handleMouseMotion(event);
			}
		}
	}
	else {
		event.Skip();
	}
}
void MouseHandler::enableSliceDragging(bool enable) {
	if (!m_canvas || !m_canvas->getSceneManager()) return;
	if (enable) {
		m_sliceDragState = SliceDragState::Dragging;
		SbVec3f world;
		if (m_canvas->getSceneManager()->screenToWorld(wxGetMousePosition() - m_canvas->GetScreenPosition(), world)) {
			m_sliceDragLastWorld = world;
		}
		else {
			m_sliceDragLastWorld = SbVec3f(0, 0, 0);
		}
	}
	else {
		m_sliceDragState = SliceDragState::None;
	}
}

void MouseHandler::OnPositionPicked(const SbVec3f& position)
{
	if (m_currentPositionBasicDialog) {
		// Update the dialog with the picked position
		m_currentPositionBasicDialog->OnPickingComplete(position);

		// Stop picking mode
		if (m_canvas->getSceneManager()->getPickingAidManager()) {
			m_canvas->getSceneManager()->getPickingAidManager()->stopPicking();
		}
	}
}